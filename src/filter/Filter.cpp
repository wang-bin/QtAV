/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
******************************************************************************/

#include "QtAV/Filter.h"
#include "QtAV/private/Filter_p.h"
#include "QtAV/Statistics.h"
#include "QtAV/AVOutput.h"
#include "QtAV/AVPlayer.h"
#include "filter/FilterManager.h"
#include "utils/Logger.h"

/*
 * 1. parent == target (QObject):
 * in ~target(), remove the filter but not delete it (parent not null now).
 * in ~QObject, filter is deleted as a child
 *-----------------------------------------------
 * 2. parent != target.parent:
 * if delete filter first, filter must notify FilterManager (uninstall in dtor here) to uninstall to avoid target to access it (in ~target())
 * if delete target first, target remove the filter but not delete it (parent not null now).
 */
namespace QtAV {

void safeReleaseFilter(Filter **ppFilter)
{
    if (!ppFilter || !*ppFilter) {
        qWarning("filter to release is null!");
        return;
    }
    FilterManager::instance().releaseFilter(*ppFilter);
    *ppFilter = 0;
}

Filter::Filter(QObject *parent)
    : QObject(parent)
{
    if (parent)
        setOwnedByTarget(false);
}

Filter::Filter(FilterPrivate &d, QObject *parent)
    : QObject(parent)
    , DPTR_INIT(&d)
{
    if (parent)
        setOwnedByTarget(false);
}

Filter::~Filter()
{
    uninstall();
}

void Filter::setEnabled(bool enabled)
{
    DPTR_D(Filter);
    if (d.enabled == enabled)
        return;
    d.enabled = enabled;
    emit enableChanged(enabled);
}

bool Filter::isEnabled() const
{
    DPTR_D(const Filter);
    return d.enabled;
}

void Filter::setOwnedByTarget(bool value)
{
    d_func().owned_by_target = value;
}

bool Filter::isOwnedByTarget() const
{
    return d_func().owned_by_target;
}

bool Filter::uninstall()
{
    return FilterManager::instance().uninstallFilter(this);
}

AudioFilter::AudioFilter(QObject *parent)
    : Filter(*new AudioFilterPrivate(), parent)
{}

AudioFilter::AudioFilter(AudioFilterPrivate& d, QObject *parent)
    : Filter(d, parent)
{}

/*TODO: move to AVPlayer.cpp to reduce dependency?*/
bool AudioFilter::installTo(AVPlayer *player)
{
    return player->installAudioFilter(this);
}

void AudioFilter::apply(Statistics *statistics, const QByteArray &data)
{
    Q_UNUSED(statistics);
    Q_UNUSED(data);
}

VideoFilter::VideoFilter(QObject *parent)
    : Filter(*new VideoFilterPrivate(), parent)
{}

VideoFilter::VideoFilter(VideoFilterPrivate &d, QObject *parent)
    : Filter(d, parent)
{}

VideoFilterContext *VideoFilter::context()
{
    DPTR_D(VideoFilter);
    if (!d.context) {
        d.context = VideoFilterContext::create(contextType());
    }
    return d.context;
}

VideoFilterContext::Type VideoFilter::contextType() const
{
    return VideoFilterContext::None;
}

bool VideoFilter::installTo(AVPlayer *player)
{
    return player->installVideoFilter(this);
}

/*TODO: move to AVOutput.cpp to reduce dependency?*/
bool VideoFilter::installTo(AVOutput *output)
{
    return output->installFilter(this);
}

//copy qpainter if context nut null
void VideoFilter::prepareContext(VideoFilterContext *&context, Statistics *statistics, VideoFrame* frame)
{
    if (contextType() == VideoFilterContext::None)
        return;
    DPTR_D(VideoFilter);
    if (!d.context) {
        d.context = VideoFilterContext::create(contextType());
        d.context->video_width = statistics->video_only.width;
        d.context->video_height = statistics->video_only.height;
    }
    // TODO: reduce mem allocation
    if (!context || context->type() != contextType()) {
        if (context) {
            delete context;
        }
        context = VideoFilterContext::create(contextType());
        context->video_width = statistics->video_only.width;
        context->video_height = statistics->video_only.height;
    }
    // share common data
    d.context->shareFrom(context);
    d.context->initializeOnFrame(frame);
    context->shareFrom(d.context);
}

void VideoFilter::apply(Statistics *statistics, VideoFrame *frame)
{
    process(statistics, frame);
}

} //namespace QtAV
