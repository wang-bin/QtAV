/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013-2015 Wang Bin <wbsecg1@gmail.com>

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
    Q_EMIT enabledChanged(enabled);
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
    return FilterManager::instance().uninstallFilter(this); // TODO: target
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
    return player->installFilter(this);
}

void AudioFilter::apply(Statistics *statistics, AudioFrame *frame)
{
    process(statistics, frame);
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
        //fake. only to store some parameters at the beginnig. it will be destroyed and set to a new instance if context type mismatch in prepareContext, with old parameters
        d.context = VideoFilterContext::create(VideoFilterContext::QtPainter);
    }
    return d.context;
}

VideoFilterContext::Type VideoFilter::contextType() const
{
    return VideoFilterContext::None;
}

bool VideoFilter::isSupported(VideoFilterContext::Type ct) const
{
    // TODO: return false
    return contextType() == ct;
}

bool VideoFilter::installTo(AVPlayer *player)
{
    return player->installFilter(this);
}

/*TODO: move to AVOutput.cpp to reduce dependency?*/
bool VideoFilter::installTo(AVOutput *output)
{
    return output->installFilter(this);
}

bool VideoFilter::prepareContext(VideoFilterContext *&ctx, Statistics *statistics, VideoFrame *frame)
{
    DPTR_D(VideoFilter);
    if (!ctx || !isSupported(ctx->type())) {
        //qDebug("no context: %p, or context type %d is not supported", ctx, ctx? ctx->type() : 0);
        return false;
    }
    if (!d.context || d.context->type() != ctx->type()) {
        VideoFilterContext* c = VideoFilterContext::create(ctx->type());//each filter has it's own context instance, but share the common parameters
        if (d.context) {
            c->pen = d.context->pen;
            c->brush = d.context->brush;
            c->clip_path = d.context->clip_path;
            c->rect = d.context->rect;
            c->transform = d.context->transform;
            c->font = d.context->font;
            c->opacity = d.context->opacity;
            c->paint_device = d.context->paint_device;
        }
        if (d.context) {
            delete d.context;
        }
        d.context = c;
    }
    d.context->video_width = statistics->video_only.width;
    d.context->video_height = statistics->video_only.height;
    ctx->video_width = statistics->video_only.width;
    ctx->video_height = statistics->video_only.height;

    // share common data
    d.context->shareFrom(ctx);
    d.context->initializeOnFrame(frame);
    ctx->shareFrom(d.context);
    return true;
}

void VideoFilter::apply(Statistics *statistics, VideoFrame *frame)
{
    process(statistics, frame);
}

} //namespace QtAV
