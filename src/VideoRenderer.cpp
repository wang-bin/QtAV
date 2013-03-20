/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2013 Wang Bin <wbsecg1@gmail.com>

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
#include <QtAV/VideoDecoder.h>
#include <private/VideoRenderer_p.h>
#include <QtCore/QCoreApplication>
#include <QWidget>

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

void VideoRenderer::scaleInRenderer(bool q)
{
    d_func().scale_in_renderer = q;
}

bool VideoRenderer::scaleInRenderer() const
{
    return d_func().scale_in_renderer;
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
        /* do nothing here. the out_rect should be calculated when the next frame is ready
         * and setSource() will pass the original video size.
         * Alternatively we can store the original aspect ratio, which can be got from AVCodecContext
         */
    }
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
}

qreal VideoRenderer::outAspectRatio() const
{
    return d_func().out_aspect_ratio;
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
    d.source_aspect_ratio = qreal(d.src_width)/qreal(d.src_height);
    qDebug("%s => calculating aspect ratio from converted input data(%f)", __FUNCTION__, d.source_aspect_ratio);
    //see setOutAspectRatioMode
    if (d.out_aspect_ratio_mode == VideoAspectRatio) {
        //source_aspect_ratio equals to original video aspect ratio here, also equals to out ratio
        setOutAspectRatio(d.source_aspect_ratio);
    }
}

QSize VideoRenderer::lastSize() const
{
    DPTR_D(const VideoRenderer);
    return QSize(d.src_width, d.src_height);
}

int VideoRenderer::lastWidth() const
{
    DPTR_D(const VideoRenderer);
    return d.src_width;
}
int VideoRenderer::lastHeight() const
{
    DPTR_D(const VideoRenderer);
    return  d.src_height;
}

bool VideoRenderer::open()
{
    return true;
}

bool VideoRenderer::close()
{
    return true;
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

QRect VideoRenderer::videoRect() const
{
    return d_func().out_rect;
}

void VideoRenderer::drawBackground()
{
}

void VideoRenderer::drawFrame()
{
}

void VideoRenderer::drawSubtitle()
{
}

void VideoRenderer::drawOSD()
{
}

void VideoRenderer::drawCustom()
{
}

void VideoRenderer::resizeFrame(int width, int height)
{
    Q_UNUSED(width);
    Q_UNUSED(height);
}

} //namespace QtAV
