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

#include <QtAV/ImageRenderer.h>
#include <private/ImageRenderer_p.h>

namespace QtAV {

ImageRenderer::ImageRenderer()
    :VideoRenderer(*new ImageRendererPrivate())
{
}

ImageRenderer::ImageRenderer(ImageRendererPrivate &d)
    :VideoRenderer(d)
{
}

ImageRenderer::~ImageRenderer()
{
}

QImage ImageRenderer::currentFrameImage() const
{
    return d_func().image;
}

void ImageRenderer::convertData(const QByteArray &data)
{
    //qDebug("%s", __PRETTY_FUNCTION__);
    DPTR_D(ImageRenderer);
    //picture.data[0]
#if QT_VERSION >= QT_VERSION_CHECK(4, 0, 0)
    d.image = QImage((uchar*)data.data(), d.src_width, d.src_height, QImage::Format_RGB32);
#else
    d.image = QImage((uchar*)data.data(), d.src_width, d.src_height, 16, NULL, 0, QImage::IgnoreEndian);
#endif
}


void ImageRenderer::setPreview(const QImage &preivew)
{
    DPTR_D(ImageRenderer);
    d.preview = preivew;
    d.image = preivew;
}

QImage ImageRenderer::previewImage() const
{
    return d_func().preview;
}

QImage ImageRenderer::currentImage() const
{
    return d_func().image;
}

} //namespace QtAV
