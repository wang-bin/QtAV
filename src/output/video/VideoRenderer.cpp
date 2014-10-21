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

#include <QtAV/VideoRenderer.h>
#include <QtAV/private/VideoRenderer_p.h>
#include <QtAV/Filter.h>
#include <QtCore/QCoreApplication>
#if QTAV_HAVE(WIDGETS)
#include <QWidget>
#include <QGraphicsItem>
#endif //QTAV_HAVE(WIDGETS)
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QtGui/QWindow>
#endif
#include "utils/Logger.h"

namespace QtAV {

VideoRenderer::VideoRenderer()
    :AVOutput(*new VideoRendererPrivate)
{
}

VideoRenderer::VideoRenderer(VideoRendererPrivate &d)
    :AVOutput(d)
{
}

VideoRenderer::~VideoRenderer()
{
}

bool VideoRenderer::receive(const VideoFrame &frame)
{
    DPTR_D(VideoRenderer);
    d.source_aspect_ratio = frame.displayAspectRatio();
    setInSize(frame.width(), frame.height());
    return receiveFrame(frame);
}

bool VideoRenderer::setPreferredPixelFormat(VideoFormat::PixelFormat pixfmt)
{
    DPTR_D(VideoRenderer);
    if (d.preferred_format == pixfmt)
        return false;
    if (!isSupported(pixfmt)) {
        qWarning("pixel format '%s' is not supported", VideoFormat(pixfmt).name().toUtf8().constData());
        return false;
    }
    VideoFormat::PixelFormat old = d.preferred_format;
    d.preferred_format = pixfmt;
    if (!onSetPreferredPixelFormat(pixfmt)) {
        qWarning("onSetPreferredPixelFormat failed");
        d.preferred_format = old;
        return false;
    }
    return true;
}

bool VideoRenderer::onSetPreferredPixelFormat(VideoFormat::PixelFormat pixfmt)
{
    Q_UNUSED(pixfmt);
    return true;
}

VideoFormat::PixelFormat VideoRenderer::preferredPixelFormat() const
{
    return d_func().preferred_format;
}

void VideoRenderer::forcePreferredPixelFormat(bool force)
{
    DPTR_D(VideoRenderer);
    if (d.force_preferred == force)
        return;
    bool old = d.force_preferred;
    d.force_preferred = force;
    if (!onForcePreferredPixelFormat(force)) {
        qWarning("onForcePreferredPixelFormat failed");
        d.force_preferred = old;
    }
}

bool VideoRenderer::onForcePreferredPixelFormat(bool force)
{
    Q_UNUSED(force);
    return true;
}

bool VideoRenderer::isPreferredPixelFormatForced() const
{
    return d_func().force_preferred;
}

void VideoRenderer::setOutAspectRatioMode(OutAspectRatioMode mode)
{
    DPTR_D(VideoRenderer);
    if (mode == d.out_aspect_ratio_mode)
        return;
    d.aspect_ratio_changed = true;
    d.out_aspect_ratio_mode = mode;
    if (mode == RendererAspectRatio) {
        //compute out_rect
        d.out_rect = QRect(1, 0, d.renderer_width, d.renderer_height); //remove? already in computeOutParameters()
        setOutAspectRatio(qreal(d.renderer_width)/qreal(d.renderer_height));
        //is that thread safe?
    } else if (mode == VideoAspectRatio) {
        setOutAspectRatio(d.source_aspect_ratio);
    }
    onSetOutAspectRatioMode(mode);
}

void VideoRenderer::onSetOutAspectRatioMode(OutAspectRatioMode mode)
{
    Q_UNUSED(mode);
}

VideoRenderer::OutAspectRatioMode VideoRenderer::outAspectRatioMode() const
{
    return d_func().out_aspect_ratio_mode;
}

void VideoRenderer::setOutAspectRatio(qreal ratio)
{
    DPTR_D(VideoRenderer);
    bool ratio_changed = d.out_aspect_ratio != ratio;
    d.out_aspect_ratio = ratio;
    //indicate that this function is called by user. otherwise, called in VideoRenderer
    if (!d.aspect_ratio_changed) {
        d.out_aspect_ratio_mode = CustomAspectRation;
    }
    d.aspect_ratio_changed = false; //TODO: when is false?
    if (d.out_aspect_ratio_mode != RendererAspectRatio) {
        d.update_background = true; //can not fill the whole renderer with video
    }
    //compute the out out_rect
    d.computeOutParameters(ratio);
    if (ratio_changed) {
        resizeFrame(d.out_rect.width(), d.out_rect.height());
    }
    onSetOutAspectRatio(ratio);
}

void VideoRenderer::onSetOutAspectRatio(qreal ratio)
{
    Q_UNUSED(ratio);
}

qreal VideoRenderer::outAspectRatio() const
{
    return d_func().out_aspect_ratio;
}

void VideoRenderer::setQuality(Quality q)
{
    DPTR_D(VideoRenderer);
    if (d.quality == q)
        return;
    Quality old = quality();
    d.quality = q;
    if (!onSetQuality(q)) {
        d.quality = old;
    }
}

bool VideoRenderer::onSetQuality(Quality q)
{
    Q_UNUSED(q);
    return true;
}

VideoRenderer::Quality VideoRenderer::quality() const
{
    return d_func().quality;
}

void VideoRenderer::setInSize(const QSize& s)
{
    setInSize(s.width(), s.height());
}

void VideoRenderer::setInSize(int width, int height)
{
    DPTR_D(VideoRenderer);
    if (d.src_width != width || d.src_height != height) {
        d.aspect_ratio_changed = true; //?? for VideoAspectRatio mode
    }
    if (!d.aspect_ratio_changed)// && (d.src_width == width && d.src_height == height))
        return;
    d.src_width = width;
    d.src_height = height;
    //d.source_aspect_ratio = qreal(d.src_width)/qreal(d.src_height);
    qDebug("%s => calculating aspect ratio from converted input data(%f)", __FUNCTION__, d.source_aspect_ratio);
    //see setOutAspectRatioMode
    if (d.out_aspect_ratio_mode == VideoAspectRatio) {
        //source_aspect_ratio equals to original video aspect ratio here, also equals to out ratio
        setOutAspectRatio(d.source_aspect_ratio);
    }
    d.aspect_ratio_changed = false; //TODO: why graphicsitemrenderer need this? otherwise aspect_ratio_changed is always true?
}

void VideoRenderer::resizeRenderer(const QSize &size)
{
    resizeRenderer(size.width(), size.height());
}

void VideoRenderer::resizeRenderer(int width, int height)
{
    DPTR_D(VideoRenderer);
    if (width == 0 || height == 0)
        return;

    d.renderer_width = width;
    d.renderer_height = height;
    d.computeOutParameters(d.out_aspect_ratio);
    resizeFrame(d.out_rect.width(), d.out_rect.height());
    onResizeRenderer(width, height);
}

void VideoRenderer::onResizeRenderer(int width, int height)
{
    Q_UNUSED(width);
    Q_UNUSED(height);
}

QSize VideoRenderer::rendererSize() const
{
    DPTR_D(const VideoRenderer);
    return QSize(d.renderer_width, d.renderer_height);
}

int VideoRenderer::rendererWidth() const
{
    return d_func().renderer_width;
}

int VideoRenderer::rendererHeight() const
{
    return d_func().renderer_height;
}

void VideoRenderer::setOrientation(int value)
{
    // currently only supports a multiple of 90
    value = (value + 360) % 360;
    if (value % 90)
        return;
    DPTR_D(VideoRenderer);
    if (d.orientation == value)
        return;
    int old = orientation();
    d.orientation = value;
    if (!onSetOrientation(value)) {
        d.orientation = old;
    } else {
        d.computeOutParameters(d.out_aspect_ratio);
        onSetOutAspectRatio(outAspectRatio());
        resizeFrame(d.out_rect.width(), d.out_rect.height());
    }
}

int VideoRenderer::orientation() const
{
    return d_func().orientation;
}

// only qpainter and opengl based renderers support orientation.
bool VideoRenderer::onSetOrientation(int value)
{
    Q_UNUSED(value);
    return false;
}

QSize VideoRenderer::frameSize() const
{
    DPTR_D(const VideoRenderer);
    return QSize(d.src_width, d.src_height);
}

QRect VideoRenderer::videoRect() const
{
    return d_func().out_rect;
}

QRectF VideoRenderer::regionOfInterest() const
{
    return d_func().roi;
}

void VideoRenderer::setRegionOfInterest(qreal x, qreal y, qreal width, qreal height)
{
    setRegionOfInterest(QRectF(x, y, width, height));
}

void VideoRenderer::setRegionOfInterest(const QRectF &roi)
{
    DPTR_D(VideoRenderer);
    if (d.roi == roi)
        return;
    QRectF old = regionOfInterest();
    d.roi = roi;
    if (!onSetRegionOfInterest(roi)) {
        d.roi = old;
    }
}

bool VideoRenderer::onSetRegionOfInterest(const QRectF &roi)
{
    Q_UNUSED(roi);
    return true;
}

QRect VideoRenderer::realROI() const
{
    DPTR_D(const VideoRenderer);
    if (!d.roi.isValid()) {
        return QRect(QPoint(), d.video_frame.size());
    }
    QRect r = d.roi.toRect();
    // nomalized x, y < 1
    bool normalized = false;
    if (qAbs(d.roi.x()) < 1) {
        normalized = true;
        r.setX(d.roi.x()*qreal(d.src_width)); //TODO: why not video_frame.size()? roi not correct
    }
    if (qAbs(d.roi.y()) < 1) {
        normalized = true;
        r.setY(d.roi.y()*qreal(d.src_height));
    }
    // whole size use width or height = 0, i.e. null size
    // nomalized width, height <= 1. If 1 is normalized value iff |x|<1 || |y| < 1
    if (qAbs(d.roi.width()) < 1)
        r.setWidth(d.roi.width()*qreal(d.src_width));
    if (qAbs(d.roi.height() < 1))
        r.setHeight(d.roi.height()*qreal(d.src_height));
    if (d.roi.width() == 1.0 && normalized) {
        r.setWidth(d.src_width);
    }
    if (d.roi.height() == 1.0 && normalized) {
        r.setHeight(d.src_height);
    }
    //TODO: insect with source rect?
    return r;
}

QRectF VideoRenderer::normalizedROI() const
{
    DPTR_D(const VideoRenderer);
    if (!d.roi.isValid()) {
        return QRectF(0, 0, 1, 1);
    }
    QRectF r = d.roi;
    bool normalized = false;
    if (qAbs(r.x()) >= 1)
        r.setX(r.x()/qreal(d.src_width));
    else
        normalized = true;
    if (qAbs(r.y() >= 1))
        r.setY(r.y()/qreal(d.src_height));
    else
        normalized = true;
    if (r.width() > 1 || (!normalized && r.width() == 1))
        r.setWidth(r.width()/qreal(d.src_width));
    if (r.height() > 1 || (!normalized && r.width() == 1)) {
        r.setHeight(r.height()/qreal(d.src_height));
    }
    return r;
}

QPointF VideoRenderer::mapToFrame(const QPointF &p) const
{
    return onMapToFrame(p);
}

// TODO: orientation
QPointF VideoRenderer::onMapToFrame(const QPointF &p) const
{
    QRectF roi = realROI();
    // zoom=roi.w/roi.h>vo.w/vo.h?roi.w/vo.w:roi.h/vo.h
    qreal zoom = qMax(roi.width()/rendererWidth(), roi.height()/rendererHeight());
    QPointF delta = p - QPointF(rendererWidth()/2, rendererHeight()/2);
    return roi.center() + delta * zoom;
}

QPointF VideoRenderer::mapFromFrame(const QPointF &p) const
{
    return onMapFromFrame(p);
}

QPointF VideoRenderer::onMapFromFrame(const QPointF &p) const
{
    QRectF roi = realROI();
    // zoom=roi.w/roi.h>vo.w/vo.h?roi.w/vo.w:roi.h/vo.h
    qreal zoom = qMax(roi.width()/rendererWidth(), roi.height()/rendererHeight());
    // (p-roi.c)/zoom + c
    QPointF delta = p - roi.center();
    return QPointF(rendererWidth()/2, rendererHeight()/2) + delta / zoom;
}

bool VideoRenderer::needUpdateBackground() const
{
    return d_func().update_background;
}

void VideoRenderer::drawBackground()
{
}

bool VideoRenderer::needDrawFrame() const
{
    return d_func().video_frame.isValid();
}

void VideoRenderer::resizeFrame(int width, int height)
{
    Q_UNUSED(width);
    Q_UNUSED(height);
}

void VideoRenderer::handlePaintEvent()
{
    DPTR_D(VideoRenderer);
    d.setupQuality();
    //begin paint. how about QPainter::beginNativePainting()?
    {
        //lock is required only when drawing the frame
        QMutexLocker locker(&d.img_mutex);
        Q_UNUSED(locker);
        /* begin paint. how about QPainter::beginNativePainting()?
         * fill background color when necessary, e.g. renderer is resized, image is null
         * if we access d.data which will be modified in AVThread, the following must be
         * protected by mutex. otherwise, e.g. QPainterRenderer, it's not required if drawing
         * on the shared data is safe
         */
        if (needUpdateBackground()) {
            /* xv: should always draw the background. so shall we only paint the border
             * rectangles, but not the whole widget
             */
            d.update_background = false;
            //fill background color. DO NOT return, you must continue drawing
            drawBackground();
        }
        /* DO NOT return if no data. we should draw other things
         * NOTE: if data is not copyed in receiveFrame(), you should always call drawFrame()
         */
        /*
         * why the background is white if return? the below code draw an empty bitmap?
         */
        //DO NOT return if no data. we should draw other things
        if (needDrawFrame()) {
            drawFrame();
        }
    }
    hanlePendingTasks();
    //TODO: move to AVOutput::applyFilters() //protected?
    if (!d.filters.isEmpty() && d.filter_context && d.statistics) {
        foreach(Filter* filter, d.filters) {
            VideoFilter *vf = static_cast<VideoFilter*>(filter);
            if (!vf) {
                qWarning("a null filter!");
                //d.filters.removeOne(filter);
                continue;
            }
            if (!vf->isEnabled())
                continue;
            vf->prepareContext(d.filter_context, d.statistics, 0);
            vf->apply(d.statistics, &d.video_frame); //painter and paint device are ready, pass video frame is ok.
        }
    } else {
        //warn once
    }
    //end paint. how about QPainter::endNativePainting()?
}

void VideoRenderer::enableDefaultEventFilter(bool e)
{
    d_func().default_event_filter = e;
}

bool VideoRenderer::isDefaultEventFilterEnabled() const
{
    return d_func().default_event_filter;
}

qreal VideoRenderer::brightness() const
{
    return d_func().brightness;
}

bool VideoRenderer::setBrightness(qreal brightness)
{
    DPTR_D(VideoRenderer);
    if (d.brightness == brightness)
        return false;
    // may emit signal in onSetXXX. ensure get the new value in slot
    qreal old = d.brightness;
    d.brightness = brightness;
    if (!onSetBrightness(brightness)) {
        d.brightness = old;
        return false;
    }
    updateUi();
    return true;
}

qreal VideoRenderer::contrast() const
{
    return d_func().contrast;
}

bool VideoRenderer::setContrast(qreal contrast)
{
    DPTR_D(VideoRenderer);
    if (d.contrast == contrast)
        return false;
    // may emit signal in onSetXXX. ensure get the new value in slot
    qreal old = d.contrast;
    d.contrast = contrast;
    if (!onSetContrast(contrast)) {
        d.contrast = old;
        return false;
    }
    updateUi();
    return true;
}

qreal VideoRenderer::hue() const
{
    return d_func().hue;
}

bool VideoRenderer::setHue(qreal hue)
{
    DPTR_D(VideoRenderer);
    if (d.hue == hue)
        return false;
    // may emit signal in onSetXXX. ensure get the new value in slot
    qreal old = d.hue;
    d.hue = hue;
    if (!onSetHue(hue)) {
        d.hue = old;
        return false;
    }
    updateUi();
    return true;
}

qreal VideoRenderer::saturation() const
{
    return d_func().saturation;
}

bool VideoRenderer::setSaturation(qreal saturation)
{
    DPTR_D(VideoRenderer);
    if (d.saturation == saturation)
        return false;
    // may emit signal in onSetXXX. ensure get the new value in slot
    qreal old = d.saturation;
    d.saturation = saturation;
    if (!onSetSaturation(saturation)) {
        d.saturation = old;
        return false;
    }
    updateUi();
    return true;
}

bool VideoRenderer::onSetBrightness(qreal b)
{
    Q_UNUSED(b);
    return false;
}

bool VideoRenderer::onSetContrast(qreal c)
{
    Q_UNUSED(c);
    return false;
}

bool VideoRenderer::onSetHue(qreal h)
{
    Q_UNUSED(h);
    return false;
}

bool VideoRenderer::onSetSaturation(qreal s)
{
    Q_UNUSED(s);
    return false;
}

void VideoRenderer::updateUi()
{
    // TODO: qwindow() and widget() can both use event?
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    if (qwindow()) {
        // DO NOT use qApp macro inside QtAV because QtAV may not depend QtWidgets module
        QCoreApplication::instance()->postEvent(qwindow(), new QEvent(QEvent::UpdateRequest));
    }
#endif
#if QTAV_HAVE(WIDGETS)
    if (widget()) {
        widget()->update();
    } else if (graphicsItem()) {
        graphicsItem()->update();
    }
#endif //QTAV_HAVE(WIDGETS)
}

} //namespace QtAV
