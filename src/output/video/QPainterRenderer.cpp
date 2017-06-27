/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2017 Wang Bin <wbsecg1@gmail.com>

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
    return VideoFormat::imageFormatFromPixelFormat(pixfmt) != QImage::Format_Invalid;
}

bool QPainterRenderer::preparePixmap(const VideoFrame &frame)
{
    DPTR_D(QPainterRenderer);
    // already locked in a larger scope of receive()
    QImage::Format imgfmt = frame.imageFormat();
    if (frame.constBits(0)) {
        d.video_frame = frame;
    } else {
        if (imgfmt == QImage::Format_Invalid) {
            d.video_frame = frame.to(VideoFormat::Format_RGB32);
            imgfmt = d.video_frame.imageFormat();
        } else {
            d.video_frame = frame.to(frame.pixelFormat());
        }
    }
    const bool swapRGB = (int)imgfmt < 0;
    if (swapRGB) {
        imgfmt = (QImage::Format)(-imgfmt);
    }
    // DO NOT use frameData().data() because it's temp ptr while d.image does not deep copy the data
    QImage image = QImage((uchar*)d.video_frame.constBits(), d.video_frame.width(), d.video_frame.height(), d.video_frame.bytesPerLine(), imgfmt);
    if (swapRGB)
        image = image.rgbSwapped();
    d.pixmap = QPixmap::fromImage(image);
    //Format_RGB32 is fast. see document
    return true;
}

void QPainterRenderer::drawBackground()
{
    DPTR_D(QPainterRenderer);
    if (!d.painter)
        return;
    const QRegion bgRegion(backgroundRegion());
    if (bgRegion.isEmpty())
        return;
#if 0
    d.painter->save();
    d.painter->setClipRegion(bgRegion);
    d.painter->fillRect(QRect(QPoint(), rendererSize()), backgroundColor());
    d.painter->restore();
#else
    const QVector<QRect> bg(bgRegion.rects());
    foreach (const QRect& r, bg) {
        d.painter->fillRect(r, backgroundColor());
    }
#endif
}

void QPainterRenderer::drawFrame()
{
    DPTR_D(QPainterRenderer);
    if (!d.painter)
        return;
    if (d.pixmap.isNull())
        return;
    QRect roi = realROI();
    if (d.rotation() == 0) {
        //assume that the image data is already scaled to out_size(NOT renderer size!)
        if (roi.size() == d.out_rect.size()) {
            d.painter->drawPixmap(d.out_rect.topLeft(), d.pixmap, roi);
        } else {
            d.painter->drawPixmap(d.out_rect, d.pixmap, roi);
            //what's the difference?
            //d.painter->drawPixmap(QPoint(), d.pixmap.scaled(d.renderer_width, d.renderer_height));
        }
        return;
    }
    // render to whole renderer rect in painter's transformed coordinate
    // scale ratio is different from gl based renderers. gl always fill the whole rect
    d.painter->save();
    d.painter->translate(rendererWidth()/2, rendererHeight()/2);
    // TODO: why rotate then scale gives wrong result?
    if (d.rotation() % 180)
        d.painter->scale((qreal)d.out_rect.width()/(qreal)rendererHeight(), (qreal)d.out_rect.height()/(qreal)rendererWidth());
    else
        d.painter->scale((qreal)d.out_rect.width()/(qreal)rendererWidth(), (qreal)d.out_rect.height()/(qreal)rendererHeight());
    d.painter->rotate(d.rotation());
    d.painter->translate(-rendererWidth()/2, -rendererHeight()/2);
    d.painter->drawPixmap(QRect(0, 0, rendererWidth(), rendererHeight()), d.pixmap, roi);
    d.painter->restore();
}

} //namespace QtAV
