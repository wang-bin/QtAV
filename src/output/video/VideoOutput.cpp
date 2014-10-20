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

/*!
 * onSetXXX(...): impl->onSetXXX(...); set value as impl; return ;
 */

namespace QtAV {

class VideoOutputPrivate : public VideoRendererPrivate
{
public:
    VideoOutputPrivate(VideoRendererId rendererId) {
        impl = VideoRendererFactory::create(rendererId);
        // assert not null?
        available = !!impl;
        if (!available)
            return;
        // set members as impl's that may be already set in ctor
        filters = impl->filters();
        renderer_width = impl->rendererWidth();
        renderer_height = impl->rendererHeight();
        src_width = impl->frameSize().width();
        src_height = impl->frameSize().height();
        source_aspect_ratio = qreal(src_width)/qreal(src_height);
        out_aspect_ratio_mode = impl->outAspectRatioMode();
        out_aspect_ratio = impl->outAspectRatio();
        quality = impl->quality();
        out_rect = impl->videoRect();
        roi = impl->regionOfInterest();
        default_event_filter = impl->isDefaultEventFilterEnabled();
        preferred_format = impl->preferredPixelFormat();
        force_preferred = impl->isPreferredPixelFormatForced();
        brightness = impl->brightness();
        contrast = impl->contrast();
        hue = impl->hue();
        saturation = impl->saturation();
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
    d.source_aspect_ratio = frame.displayAspectRatio();
    d.impl->d_func().source_aspect_ratio = d.source_aspect_ratio;
    setInSize(frame.width(), frame.height());
    return d.impl->receiveFrame(frame);
}
/*
void VideoOutput::setVideoFormat(const VideoFormat& format)
{
    d_func().impl->setVideoFormat(format);
}
*/
bool VideoOutput::onSetPreferredPixelFormat(VideoFormat::PixelFormat pixfmt)
{
    DPTR_D(VideoOutput);
    d.impl->setPreferredPixelFormat(pixfmt);
    return pixfmt == d.impl->preferredPixelFormat();

}

VideoFormat::PixelFormat VideoOutput::preferredPixelFormat() const
{
    return d_func().impl->preferredPixelFormat();
}

bool VideoOutput::isSupported(VideoFormat::PixelFormat pixfmt) const
{
    return d_func().impl->isSupported(pixfmt);
}

QWindow* VideoOutput::qwindow()
{
    return d_func().impl->qwindow();
}

QWidget* VideoOutput::widget()
{
    return d_func().impl->widget();
}

QGraphicsItem* VideoOutput::graphicsItem()
{
    return d_func().impl->graphicsItem();
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


bool VideoOutput::onForcePreferredPixelFormat(bool force)
{
    DPTR_D(VideoOutput);
    d.impl->forcePreferredPixelFormat(force);
    return d.impl->isPreferredPixelFormatForced() == force;
}

void VideoOutput::onSetOutAspectRatioMode(OutAspectRatioMode mode)
{
    DPTR_D(VideoOutput);
    qreal a = d.impl->outAspectRatio();
    OutAspectRatioMode am = d.impl->outAspectRatioMode();
    d.impl->setOutAspectRatioMode(mode);
    if (a != outAspectRatio())
        emit outAspectRatioChanged(outAspectRatio());
    if (am != outAspectRatioMode())
        emit outAspectRatioModeChanged(mode);
}

void VideoOutput::onSetOutAspectRatio(qreal ratio)
{
    DPTR_D(VideoOutput);
    qreal a = d.impl->outAspectRatio();
    OutAspectRatioMode am = d.impl->outAspectRatioMode();
    d.impl->setOutAspectRatio(ratio);
    if (a != outAspectRatio())
        emit outAspectRatioChanged(ratio);
    if (am != outAspectRatioMode())
        emit outAspectRatioModeChanged(outAspectRatioMode());
}

bool VideoOutput::onSetQuality(Quality q)
{
    DPTR_D(VideoOutput);
    d.impl->setQuality(q);
    return d.impl->quality() == q;
}

bool VideoOutput::onSetOrientation(int value)
{
    value = (value + 360) % 360;
    DPTR_D(VideoOutput);
    d.impl->setOrientation(value);
    if (d.impl->orientation() != value) {
        return false;
    }
    emit orientationChanged(value);
    return true;
}

void VideoOutput::onResizeRenderer(int width, int height)
{
    DPTR_D(VideoOutput);
    d.impl->resizeRenderer(width, height);
}

bool VideoOutput::onSetRegionOfInterest(const QRectF& roi)
{
    DPTR_D(VideoOutput);
    d.impl->setRegionOfInterest(roi);
    emit regionOfInterestChanged(roi);
    return true;
}

QPointF VideoOutput::onMapToFrame(const QPointF& p) const
{
    DPTR_D(const VideoOutput);
    return d.impl->onMapToFrame(p);
}

QPointF VideoOutput::onMapFromFrame(const QPointF& p) const
{
    DPTR_D(const VideoOutput);
    return d.impl->onMapFromFrame(p);
}

bool VideoOutput::onSetBrightness(qreal brightness)
{
    DPTR_D(VideoOutput);
    // not call onSetXXX here, otherwise states in impl will not change
    d.impl->setBrightness(brightness);
    if (brightness != d.impl->brightness()) {
        return false;
    }
    emit brightnessChanged(brightness);
    return true;
}

bool VideoOutput::onSetContrast(qreal contrast)
{
    DPTR_D(VideoOutput);
    // not call onSetXXX here, otherwise states in impl will not change
    d.impl->setContrast(contrast);
    if (contrast != d.impl->contrast()) {
        return false;
    }
    emit contrastChanged(contrast);
    return true;
}

bool VideoOutput::onSetHue(qreal hue)
{
    DPTR_D(VideoOutput);
    // not call onSetXXX here, otherwise states in impl will not change
    d.impl->setHue(hue);
    if (hue != d.impl->hue()) {
        return false;
    }
    emit hueChanged(hue);
    return true;
}

bool VideoOutput::onSetSaturation(qreal saturation)
{
    DPTR_D(VideoOutput);
    // not call onSetXXX here, otherwise states in impl will not change
    d.impl->setSaturation(saturation);
    if (saturation != d.impl->saturation()) {
        return false;
    }
    emit saturationChanged(saturation);
    return true;
}

void VideoOutput::setStatistics(Statistics* statistics)
{
    DPTR_D(VideoOutput);
    d.impl->setStatistics(statistics);
    // only used internally for AVOutput
    //d.statistics =
}

bool VideoOutput::onInstallFilter(Filter *filter)
{
    DPTR_D(VideoOutput);
    bool ret = d.impl->onInstallFilter(filter);
    d.filters = d.impl->filters();
    return ret;
}

bool VideoOutput::onUninstallFilter(Filter *filter)
{
    DPTR_D(VideoOutput);
    bool ret = d.impl->onUninstallFilter(filter);
    // only used internally for AVOutput
    //d.pending_uninstall_filters =
    return ret;
}

void VideoOutput::onAddOutputSet(OutputSet *set)
{
    DPTR_D(VideoOutput);
    d.impl->onAddOutputSet(set);
}

void VideoOutput::onRemoveOutputSet(OutputSet *set)
{
    DPTR_D(VideoOutput);
    d.impl->onRemoveOutputSet(set);
}

void VideoOutput::onAttach(OutputSet *set)
{
    DPTR_D(VideoOutput);
    d.impl->onAttach(set);
}

void VideoOutput::onDetach(OutputSet *set)
{
    DPTR_D(VideoOutput);
    d.impl->onDetach(set);
    //d.output_sets = d.impl->
}

bool VideoOutput::onHanlePendingTasks()
{
    DPTR_D(VideoOutput);
    if (!d.impl->onHanlePendingTasks())
        return false;
    d.filters = d.impl->filters();
    return true;
}

} //namespace QtAV
