/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2014 Wang Bin <wbsecg1@gmail.com>

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

#include "QtAV/VideoOutput.h"
#include "QtAV/private/VideoRenderer_p.h"
#include "QtAV/VideoRendererTypes.h"

namespace QtAV {

class VideoOutputPrivate : public VideoRendererPrivate
{
public:
    VideoOutputPrivate(VideoRendererId rendererId) {
        impl = VideoRendererFactory::create(rendererId);
        // assert not null?
        available = !!impl;
    }
    ~VideoOutputPrivate() {
        if (impl) {
            delete impl;
            impl = 0;
        }
    }

    VideoRenderer *impl;
};

VideoOutput::VideoOutput(VideoRendererId rendererId, QObject *parent)
    : QObject(parent)
    , VideoRenderer(*new VideoOutputPrivate(rendererId))
{
}

VideoOutput::~VideoOutput()
{
}

VideoRendererId VideoOutput::id() const
{
    return d_func().impl->id();
}

bool VideoOutput::receive(const VideoFrame& frame)
{
    DPTR_D(VideoOutput);
    setInSize(frame.width(), frame.height());
    return d.impl->receiveFrame(frame);
}
/*
void VideoOutput::setVideoFormat(const VideoFormat& format)
{
    d_func().impl->setVideoFormat(format);
}
*/
bool VideoOutput::setPreferredPixelFormat(VideoFormat::PixelFormat pixfmt)
{
    DPTR_D(VideoOutput);
    bool ret = d.impl->setPreferredPixelFormat(pixfmt);
    d.preferred_format = d.impl->preferredPixelFormat();
    return ret;
}

VideoFormat::PixelFormat VideoOutput::preferredPixelFormat() const
{
    return d_func().impl->preferredPixelFormat();
}

void VideoOutput::forcePreferredPixelFormat(bool force)
{
    DPTR_D(VideoOutput);
    d.impl->forcePreferredPixelFormat(force);
    d.force_preferred = d.impl->isPreferredPixelFormatForced();
}

bool VideoOutput::isSupported(VideoFormat::PixelFormat pixfmt) const
{
    return d_func().impl->isSupported(pixfmt);
}

void VideoOutput::scaleInRenderer(bool q)
{
    DPTR_D(VideoOutput);

}

void VideoOutput::setOutAspectRatioMode(OutAspectRatioMode mode)
{
    DPTR_D(VideoOutput);
    d.impl->setOutAspectRatioMode(mode);
    d.out_rect = d.impl->videoRect();
    d.out_aspect_ratio = d.impl->outAspectRatio();
    d.out_aspect_ratio_mode = d.impl->outAspectRatioMode();
}

void VideoOutput::setOutAspectRatio(qreal ratio)
{
    DPTR_D(VideoOutput);
    d.impl->setOutAspectRatio(ratio);
    d.out_rect = d.impl->videoRect();
    d.out_aspect_ratio = d.impl->outAspectRatio();
    d.out_aspect_ratio_mode = d.impl->outAspectRatioMode();
}

void VideoOutput::setQuality(Quality q)
{
    DPTR_D(VideoOutput);
    d.impl->setQuality(q);
    d.quality = d.impl->quality();
}

bool VideoOutput::open()
{
    DPTR_D(VideoOutput);
    return d.impl->open();
}

bool VideoOutput::close()
{
    DPTR_D(VideoOutput);
    return d.impl->close();
}

void VideoOutput::resizeRenderer(int width, int height)
{
    DPTR_D(VideoOutput);
    if (width == 0 || height == 0)
        return;
    d.impl->resizeRenderer(width, height);
    d.renderer_width = width;
    d.renderer_height = height;
    d.out_aspect_ratio = d.impl->outAspectRatio();
    d.out_rect = d.impl->videoRect();
}

void VideoOutput::setRegionOfInterest(const QRectF& roi)
{
    DPTR_D(VideoOutput);
    d.impl->setRegionOfInterest(roi);
    d.roi = d.impl->regionOfInterest();
}

QPointF VideoOutput::mapToFrame(const QPointF& p) const
{
    DPTR_D(const VideoOutput);
    return d.impl->mapToFrame(p);
}

QPointF VideoOutput::mapFromFrame(const QPointF& p) const
{
    DPTR_D(const VideoOutput);
    return d.impl->mapFromFrame(p);
}

QWidget* VideoOutput::widget()
{
    return d_func().impl->widget();
}

QGraphicsItem* VideoOutput::graphicsItem()
{
    return d_func().impl->graphicsItem();
}

OSDFilter* VideoOutput::setOSDFilter(OSDFilter *filter)
{
    DPTR_D(VideoOutput);
    return d.impl->setOSDFilter(filter);
}

Filter* VideoOutput::setSubtitleFilter(Filter *filter)
{
    DPTR_D(VideoOutput);
    return d.impl->setSubtitleFilter(filter);
}

void VideoOutput::enableDefaultEventFilter(bool e)
{
    DPTR_D(VideoOutput);
    d.impl->enableDefaultEventFilter(e);
    d.default_event_filter = d.impl->isDefaultEventFilterEnabled();
}

bool VideoOutput::setBrightness(qreal brightness)
{
    DPTR_D(VideoOutput);
    if (!d.impl->setBrightness(brightness))
        return false;
    d.brightness = brightness;
    return false;
}

bool VideoOutput::setContrast(qreal contrast)
{
    DPTR_D(VideoOutput);
    if (!d.impl->setContrast(contrast))
        return false;
    d.contrast = d.impl->contrast();
    return true;
}

bool VideoOutput::setHue(qreal hue)
{
    DPTR_D(VideoOutput);
    if (!d.impl->setHue(hue))
        return false;
    d.hue = d.impl->hue();
    return true;
}

bool VideoOutput::setSaturation(qreal saturation)
{
    DPTR_D(VideoOutput);
    if (!d.impl->setSaturation(saturation))
        return false;
    d.saturation = d.impl->saturation();
    return true;
}

bool VideoOutput::receiveFrame(const VideoFrame& frame)
{
    DPTR_D(VideoOutput);
    return d.impl->receiveFrame(frame);
}

bool VideoOutput::needUpdateBackground() const
{
    DPTR_D(const VideoOutput);
    return d.impl->needUpdateBackground();
}

void VideoOutput::drawBackground()
{
    DPTR_D(VideoOutput);
    d.impl->drawBackground();
}

bool VideoOutput::needDrawFrame() const
{
    DPTR_D(const VideoOutput);
    return d.impl->needDrawFrame();
}

void VideoOutput::drawFrame()
{
    DPTR_D(VideoOutput);
    d.impl->drawFrame();
}

void VideoOutput::resizeFrame(int width, int height)
{
    DPTR_D(VideoOutput);
    d.impl->resizeFrame(width, height);
}

void VideoOutput::handlePaintEvent()
{
    DPTR_D(VideoOutput);
    d.impl->handlePaintEvent();
}

bool VideoOutput::onChangingBrightness(qreal b)
{
    DPTR_D(VideoOutput);
    if (!d.impl->onChangingBrightness(b))
        return false;
    d.brightness = d.impl->brightness();
    return true;
}

bool VideoOutput::onChangingContrast(qreal c)
{
    DPTR_D(VideoOutput);
    if (!d.impl->onChangingContrast(c))
        return false;
    d.contrast = d.impl->contrast();
    return true;
}

bool VideoOutput::onChangingHue(qreal h)
{
    DPTR_D(VideoOutput);
    if (!d.impl->onChangingHue(h))
        return false;
    d.hue = d.impl->hue();
    return true;
}

bool VideoOutput::onChangingSaturation(qreal s)
{
    DPTR_D(VideoOutput);
    if (!d.impl->onChangingSaturation(s))
        return false;
    d.saturation = d.impl->saturation();
    return true;
}

void VideoOutput::setInSize(int width, int height)
{
    DPTR_D(VideoOutput);
    d.impl->setInSize(width, height);
    d.src_width = d.impl->frameSize().width();
    d.src_height = d.impl->frameSize().height();
    //d.source_aspect_ratio = ;
    d.out_rect = d.impl->videoRect();
    d.out_aspect_ratio = d.impl->outAspectRatio();
    d.out_aspect_ratio_mode = d.impl->outAspectRatioMode();
    //d.update_background = d.impl->
}

} //namespace QtAV
