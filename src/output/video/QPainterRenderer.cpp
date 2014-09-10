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

#include <QtAV/QPainterRenderer.h>
#include <QtAV/private/QPainterRenderer_p.h>
#include <QtAV/FilterContext.h>

namespace QtAV {

QPainterRenderer::QPainterRenderer()
    :VideoRenderer(*new QPainterRendererPrivate())
{
    DPTR_D(QPainterRenderer);
    d.filter_context = VideoFilterContext::create(VideoFilterContext::QtPainter);
}

QPainterRenderer::QPainterRenderer(QPainterRendererPrivate &d)
    :VideoRenderer(d)
{
    d.filter_context = VideoFilterContext::create(VideoFilterContext::QtPainter);
}

bool QPainterRenderer::isSupported(VideoFormat::PixelFormat pixfmt) const
{
    return VideoFormat::isRGB(pixfmt);
}

bool QPainterRenderer::prepareFrame(const VideoFrame &frame)
{
    DPTR_D(QPainterRenderer);
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
    QMutexLocker locker(&d.img_mutex);
    Q_UNUSED(locker);
    //If use d.data.data() it will eat more cpu, deep copy?
    d.video_frame = frame;
    // DO NOT use frameData().data() because it's temp ptr while QImage does not deep copy the data
    d.image = QImage((uchar*)frame.bits(), frame.width(), frame.height(), frame.bytesPerLine(), frame.imageFormat());
    //Format_RGB32 is fast. see document
    return true;
}

} //namespace QtAV
