/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012 Wang Bin <wbsecg1@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include <QtAV/VideoRenderer.h>
#include <QtAV/VideoDecoder.h>
#include <private/VideoRenderer_p.h>

/*!
    EZX:
    PIX_FMT_BGR565, 16bpp,
    PIX_FMT_RGB18,  18bpp,
*/

namespace QtAV {

VideoRenderer::VideoRenderer()
    :AVOutput(*new VideoRendererPrivate)
{
}

VideoRenderer::~VideoRenderer()
{
    if (d_ptr) {
        delete d_ptr;
        d_ptr = 0;
    }
}

void VideoRenderer::resizeVideo(const QSize &size)
{
    resizeVideo(size.width(), size.height());
}

void VideoRenderer::resizeVideo(int width, int height)
{
    Q_D(VideoRenderer);
    if (width == 0 || height == 0)
        return;
    if (d->dec) {
        static_cast<VideoDecoder*>(d->dec)->resizeVideo(width, height);
    }
    d->width = width;
    d->height = height;
}



QSize VideoRenderer::videoSize() const
{
    Q_D(const VideoRenderer);
    return QSize(d->width, d->height);
}

int VideoRenderer::videoWidth() const
{
    Q_D(const VideoRenderer);
    return d->width;
}

int VideoRenderer::videoHeight() const
{
    Q_D(const VideoRenderer);
    return d->height;
}

} //namespace QtAV
