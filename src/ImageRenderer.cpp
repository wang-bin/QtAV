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
#include <private/VideoRenderer_p.h>

namespace QtAV {

ImageRenderer::~ImageRenderer()
{

}

int ImageRenderer::write(const QByteArray &data)
{
    //picture.data[0]
#if QT_VERSION >= QT_VERSION_CHECK(4, 0, 0)
    image = QImage((uchar*)data.data(), d_func()->width, d_func()->height, QImage::Format_RGB32);
#else
    image = QImage((uchar*)data.data(), d_func()->width, d_func()->height, 16, NULL, 0, QImage::IgnoreEndian);
#endif
    return data.size();
}


void ImageRenderer::setPreview(const QImage &preivew)
{
    this->preview = preivew;
    image = preivew;
}

QImage ImageRenderer::previewImage() const
{
    return preview;
}

QImage ImageRenderer::currentImage() const
{
    return image;
}

} //namespace QtAV
