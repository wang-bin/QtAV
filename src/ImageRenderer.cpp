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
/*
QImage ImageRenderer::currentFrameImage() const
{
    return d_func().image;
}
*/
//FIXME: why crash if QImage use widget size?
void ImageRenderer::convertData(const QByteArray &data)
{
    DPTR_D(ImageRenderer);
    //int ss = 4*d.src_width*d.src_height*sizeof(char);
    //if (ss != data.size())
    //    qDebug("src size=%d, data size=%d", ss, data.size());
    //if (d.src_width != d.width || d.src_height != d.height)
    //    return;
    /*
     * QImage constructed from memory do not deep copy the data, data should be available throughout
     * image's lifetime and not be modified. painting image happens in main thread, we must ensure the
     * image data is not changed if not use the original frame size, so we need the lock.
     * In addition, the data passed by ref, so even if we lock, the original data may changed in decoder
     * (ImageConverter), so we need a copy of this data(not deep copy) to ensure the image's data not changes before it
     * is painted.
     * But if we use the fixed original frame size, the data address and size always the same, so we can
     * avoid the lock and use the ref data directly and safely
     */
    if (!d.scale_in_qt) {
        /*if lock is required, do not use locker in if() scope, it will unlock outside the scope*/
        d.img_mutex.lock();
        d.data = data;
        //qDebug("data address = %p, %p", data.data(), d.data.data());
    #if QT_VERSION >= QT_VERSION_CHECK(4, 0, 0)
        d.image = QImage((uchar*)d.data.data(), d.src_width, d.src_height, QImage::Format_RGB32);
    #else
        d.image = QImage((uchar*)d.data.data(), d.src_width, d.src_height, 16, NULL, 0, QImage::IgnoreEndian);
    #endif
        d.img_mutex.unlock();
    } else {
        //qDebug("data address = %p", data.data());
        //Format_RGB32 is fast. see document
#if QT_VERSION >= QT_VERSION_CHECK(4, 0, 0)
        d.image = QImage((uchar*)data.data(), d.src_width, d.src_height, QImage::Format_RGB32);
#else
    d.image = QImage((uchar*)data.data(), d.src_width, d.src_height, 16, NULL, 0, QImage::IgnoreEndian);
#endif
    }
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
