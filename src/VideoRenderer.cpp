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

void VideoRenderer::scaleInQt(bool q)
{
    d_func().scale_in_qt = q;
}

bool VideoRenderer::scaleInQt() const
{
    return d_func().scale_in_qt;
}

void VideoRenderer::setOutAspectRatioMode(FrameAspectRatioMode mode)
{
    DPTR_D(VideoRenderer);
    if (mode == d.aspect_ratio_mode)
        return;
    d.aspect_ratio_mode = mode;
    if (mode == RendererAspectRatio) {
        //compute out_rect
        d.out_rect = QRect(0, 0, d.width, d.height);
        d.out_aspect_ratio = qreal(d.width)/qreal(d.height);
        //is that thread safe?
        resizeFrame(d.out_rect.width(), d.out_rect.height());
    } else if (mode == VideoAspectRatio) {
        /* do nothing here. the out_rect should be calculated when the next frame is ready
         * and setSource() will pass the original video size.
         * Alternatively we can store the original aspect ratio, which can be got from AVCodecContext
         */
    }
}

VideoRenderer::FrameAspectRatioMode VideoRenderer::outAspectRatioMode() const
{
    return d_func().aspect_ratio_mode;
}

void VideoRenderer::setOutAspectRatio(qreal ratio)
{
    DPTR_D(VideoRenderer);
    bool ratio_changed = d.out_aspect_ratio != ratio;
    d.out_aspect_ratio = ratio;
    d.aspect_ratio_mode = CustomAspectRation;
    //compute the out out_rect
    qreal r = qreal(d.width)/qreal(d.height); //renderer aspect ratio
    d.computeOutParameters(r, ratio);
    if (ratio_changed) {
        resizeFrame(d.out_rect.width(), d.out_rect.height());
    }
}

qreal VideoRenderer::outAspectRatio() const
{
    return d_func().out_aspect_ratio;
}

void VideoRenderer::setSourceSize(const QSize& s)
{
    setSourceSize(s.width(), s.height());
}

void VideoRenderer::setSourceSize(int width, int height)
{
    DPTR_D(VideoRenderer);
    //check
    if (d.src_width == width && d.src_height == height)
        return;
    d.src_width = width;
    d.src_height = height;
    d.source_aspect_ratio = qreal(d.src_width)/qreal(d.src_height);
    //see setOutAspectRatioMode
    if (d.aspect_ratio_mode == VideoAspectRatio) {
        qreal r = qreal(d.width)/qreal(d.height); //renderer aspect ratio
        //source_aspect_ratio equals to original video aspect ratio here, also equals to out ratio
        d.computeOutParameters(r, d.source_aspect_ratio);
        resizeFrame(d.out_rect.width(), d.out_rect.height());
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

void VideoRenderer::resizeVideo(const QSize &size)
{
    resizeVideo(size.width(), size.height());
}

void VideoRenderer::resizeVideo(int width, int height)
{
    DPTR_D(VideoRenderer);
    if (width == 0 || height == 0)
        return;

    d.width = width;
    d.height = height;
    qreal r = qreal(d.width)/qreal(d.height);
    d.computeOutParameters(r, d.out_aspect_ratio);
    resizeFrame(d.out_rect.width(), d.out_rect.height());
}

QSize VideoRenderer::videoSize() const
{
    return QSize(d_func().width, d_func().height);
}

int VideoRenderer::videoWidth() const
{
    return d_func().width;
}

int VideoRenderer::videoHeight() const
{
    return d_func().height;
}

QRect VideoRenderer::videoRect() const
{
    return d_func().out_rect;
}

void VideoRenderer::resizeFrame(int width, int height)
{
}

} //namespace QtAV
