/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2017 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2016)

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

#include "QmlAV/QuickFilter.h"
#include "QtAV/private/Filter_p.h"
#include "QtAV/LibAVFilter.h"
#include "QtAV/GLSLFilter.h"
#include "QtAV/VideoShaderObject.h"
#include "QtAV/OpenGLVideo.h"

//namespace QtAV {

class QuickVideoFilterPrivate : public VideoFilterPrivate
{
public:
    QuickVideoFilterPrivate() : type(QuickVideoFilter::AVFilter)
      , user_filter(NULL)
      , avfilter(new LibAVFilterVideo())
      , glslfilter(new GLSLFilter())
    {
        filter = avfilter.data();
    }

    QuickVideoFilter::FilterType type;
    VideoFilter *filter;
    VideoFilter *user_filter; // life time is managed by qml
    QScopedPointer<LibAVFilterVideo> avfilter;
    QScopedPointer<GLSLFilter> glslfilter;
};

QuickVideoFilter::QuickVideoFilter(QObject *parent)
    : VideoFilter(*new QuickVideoFilterPrivate(), parent)
{
    DPTR_D(QuickVideoFilter);
    connect(d.avfilter.data(), SIGNAL(optionsChanged()), this, SIGNAL(avfilterChanged()));
}

bool QuickVideoFilter::isSupported(VideoFilterContext::Type ct) const
{
    DPTR_D(const QuickVideoFilter);
    if (d.filter)
        return d.filter->isSupported(ct);
    return false;
}

QuickVideoFilter::FilterType QuickVideoFilter::type() const
{
    return d_func().type;
}

void QuickVideoFilter::setType(FilterType value)
{
    DPTR_D(QuickVideoFilter);
    if (d.type == value)
        return;
    d.type = value;
    if (value == GLSLFilter)
        d.filter = d.glslfilter.data();
    else if (value == AVFilter)
        d.filter = d.avfilter.data();
    else
        d.filter = d.user_filter;
    Q_EMIT typeChanged();
}

QStringList QuickVideoFilter::supportedAVFilters() const
{
    return d_func().avfilter->filters();
}

QString QuickVideoFilter::avfilter() const
{
    return d_func().avfilter->options();
}

void QuickVideoFilter::setAVFilter(const QString &options)
{
    d_func().avfilter->setOptions(options);
}

VideoFilter* QuickVideoFilter::userFilter() const
{
    return d_func().user_filter;
}

void QuickVideoFilter::setUserFilter(VideoFilter *f)
{
    DPTR_D(QuickVideoFilter);
    if (d.user_filter == f)
        return;
    d.user_filter = f;
    if (d.type == UserFilter)
        d.filter = d.user_filter;
    Q_EMIT userFilterChanged();
}

DynamicShaderObject* QuickVideoFilter::shader() const
{
    return static_cast<DynamicShaderObject*>(d_func().glslfilter->opengl()->userShader());
}

void QuickVideoFilter::setShader(DynamicShaderObject *value)
{
    DPTR_D(QuickVideoFilter);
    if (shader() == value)
        return;
    d.glslfilter->opengl()->setUserShader(value);
    Q_EMIT shaderChanged();
}

void QuickVideoFilter::process(Statistics *statistics, VideoFrame *frame)
{
    DPTR_D(QuickVideoFilter);
    if (!d.filter)
        return;
    d.filter->apply(statistics, frame);
}

class QuickAudioFilterPrivate : public AudioFilterPrivate
{
public:
    QuickAudioFilterPrivate() : AudioFilterPrivate()
      , type(QuickAudioFilter::AVFilter)
      , user_filter(NULL)
      , avfilter(new LibAVFilterAudio())
    {
        filter = avfilter.data();
    }

    QuickAudioFilter::FilterType type;
    AudioFilter *filter;
    AudioFilter* user_filter; // life time is managed by qml
    QScopedPointer<LibAVFilterAudio> avfilter;
};

QuickAudioFilter::QuickAudioFilter(QObject *parent)
    : AudioFilter(*new QuickAudioFilterPrivate(), parent)
{
    DPTR_D(QuickAudioFilter);
    connect(d.avfilter.data(), SIGNAL(optionsChanged()), this, SIGNAL(avfilterChanged()));
}

QuickAudioFilter::FilterType QuickAudioFilter::type() const
{
    return d_func().type;
}

void QuickAudioFilter::setType(FilterType value)
{
    DPTR_D(QuickAudioFilter);
    if (d.type == value)
        return;
    d.type = value;
    if (value == AVFilter)
        d.filter = d.avfilter.data();
    else
        d.filter = d.user_filter;
    Q_EMIT typeChanged();
}

QStringList QuickAudioFilter::supportedAVFilters() const
{
    return d_func().avfilter->filters();
}

QString QuickAudioFilter::avfilter() const
{
    return d_func().avfilter->options();
}

void QuickAudioFilter::setAVFilter(const QString &options)
{
    d_func().avfilter->setOptions(options);
}

AudioFilter *QuickAudioFilter::userFilter() const
{
    return d_func().user_filter;
}

void QuickAudioFilter::setUserFilter(AudioFilter *f)
{
    DPTR_D(QuickAudioFilter);
    if (d.user_filter == f)
        return;
    d.user_filter = f;
    if (d.type == UserFilter)
        d.filter = d.user_filter;
    Q_EMIT userFilterChanged();
}

void QuickAudioFilter::process(Statistics *statistics, AudioFrame *frame)
{
    DPTR_D(QuickAudioFilter);
    if (!d.filter)
        return;
    d.filter->apply(statistics, frame);
}
//} //namespace QtAV
