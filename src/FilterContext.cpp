/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013 Wang Bin <wbsecg1@gmail.com>

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

#include "QtAV/FilterContext.h"
#include <QtGui/QImage>
#include <QtGui/QPainter>

namespace QtAV {

FilterContext* FilterContext::create(Type t)
{
    FilterContext *ctx = 0;
    switch (t) {
    case QtPainter:
        ctx = new QPainterFilterContext();
        break;
    case OpenGL:
        ctx = new GLFilterContext();
        break;
    default:
        break;
    }
    return ctx;
}

FilterContext::FilterContext():
    video_width(0)
  , video_height(0)
{
}

FilterContext::~FilterContext()
{
}

void FilterContext::initializeOnData(QByteArray *data)
{
    Q_UNUSED(data);
}

void FilterContext::shareFrom(FilterContext *ctx)
{
    Q_UNUSED(ctx);
    video_width = ctx->video_width;
    video_height = ctx->video_height;
}

VideoFilterContext::VideoFilterContext():
    FilterContext()
  , painter(0)
  , paint_device(0)
  , own_painter(false)
  , own_paint_device(false)
  , opacity(1)
{
    font.setBold(true);
    font.setPixelSize(26);
    pen.setColor(Qt::white);
    rect = QRect(32, 32, 0, 0); //TODO: why painting will above the visible area if the draw at (0, 0)?
}

VideoFilterContext::~VideoFilterContext()
{
    if (painter) {
        // painter is shared, so may be end() multiple times.
        // TODO: use shared ptr
        //if (painter->isActive())
        //    painter->end();
        if (own_painter) {
            qDebug("VideoFilterContext %p delete painter %p", this, painter);
            delete painter;
            painter = 0;
        }
    }
    if (paint_device) {
        qDebug("VideoFilterContext %p delete paint device in %p", this, paint_device);
        if (own_paint_device)
            delete paint_device; //delete recursively for widget
        paint_device = 0;
    }
}

void VideoFilterContext::drawImage(const QPointF &pos, const QImage &image, const QRectF& source, Qt::ImageConversionFlags flags)
{
    Q_UNUSED(pos);
    Q_UNUSED(image);
    Q_UNUSED(source);
    Q_UNUSED(flags);
}

void VideoFilterContext::drawImage(const QRectF &target, const QImage &image, const QRectF &source, Qt::ImageConversionFlags flags)
{
    Q_UNUSED(target);
    Q_UNUSED(image);
    Q_UNUSED(source);
    Q_UNUSED(flags);
}

void VideoFilterContext::drawPlainText(const QPointF &pos, const QString &text)
{
    Q_UNUSED(pos);
    Q_UNUSED(text);
}

void VideoFilterContext::drawPlainText(const QRectF &rect, int flags, const QString &text)
{
    Q_UNUSED(rect);
    Q_UNUSED(flags);
    Q_UNUSED(text);
}

void VideoFilterContext::drawRichText(const QRectF &rect, const QString &text)
{
    Q_UNUSED(rect);
    Q_UNUSED(text);
}

void VideoFilterContext::shareFrom(FilterContext *ctx)
{
    if (!ctx) {
        qWarning("shared filter context is null!");
        return;
    }
    VideoFilterContext *vctx = static_cast<VideoFilterContext*>(ctx);
    painter = vctx->painter;
    paint_device = vctx->paint_device;
    own_painter = false;
    own_paint_device = false;
    video_width = vctx->video_width;
    video_height = vctx->video_height;
}


FilterContext::Type QPainterFilterContext::type() const
{
    return FilterContext::QtPainter;
}

void QPainterFilterContext::drawImage(const QPointF &pos, const QImage &image, const QRectF& source, Qt::ImageConversionFlags flags)
{
    if (!prepare())
        return;
    painter->drawImage(pos, image, source, flags);
    painter->restore();
}

void QPainterFilterContext::drawImage(const QRectF &target, const QImage &image, const QRectF &source, Qt::ImageConversionFlags flags)
{
    if (!prepare())
        return;
    painter->drawImage(target, image, source, flags);
    painter->restore();
}

void QPainterFilterContext::drawPlainText(const QPointF &pos, const QString &text)
{
    if (!prepare())
        return;
    painter->drawText(pos, text);
    painter->restore();
}

void QPainterFilterContext::drawPlainText(const QRectF &rect, int flags, const QString &text)
{
    if (!prepare())
        return;
    if (rect.isNull())
        painter->drawText(rect.topLeft(), text);
    else
        painter->drawText(rect, flags, text);
    painter->restore();
}

void QPainterFilterContext::drawRichText(const QRectF &rect, const QString &text)
{
    Q_UNUSED(rect);
    Q_UNUSED(text);
    if (!prepare())
        return;
    //QTextDocument
    painter->restore();
}

bool QPainterFilterContext::isReady() const
{
    return !!painter && painter->isActive();
}

bool QPainterFilterContext::prepare()
{
    if (!isReady())
        return false;
    painter->save(); //is it necessary?
    painter->setBrush(brush);
    painter->setPen(pen);
    painter->setFont(font);
    painter->setOpacity(opacity);
    if (!clip_path.isEmpty()) {
        painter->setClipPath(clip_path);
    }
    //transform at last! because clip_path is relative to paint device coord
    painter->setTransform(transform);

    return true;
}

void QPainterFilterContext::initializeOnData(QByteArray *data)
{
    if (!data) {
        if (!painter) {
            painter = new QPainter(); //warning: more than 1 painter on 1 device
        }
        if (!paint_device) {
            paint_device = painter->device();
        }
        if (!paint_device && !painter->isActive()) {
            qWarning("No paint device and painter is not active. No painting!");
            return;
        }
        if (!painter->isActive())
            painter->begin(paint_device);
        return;
    }
    if (data->isEmpty())
        return;
    if (paint_device) {
        if (painter && painter->isActive()) {
            painter->end(); //destroy a paint device that is being painted is not allowed!
        }
        delete paint_device;
        paint_device = 0;
    }
    Q_ASSERT(video_width > 0 && video_height > 0);
    paint_device = new QImage((uchar*)data->data(), video_width, video_height, QImage::Format_RGB32);
    if (!painter)
        painter = new QPainter();
    own_painter = true;
    own_paint_device = true; //TODO: what about renderer is not a widget?
    painter->begin((QImage*)paint_device);
}

FilterContext::Type GLFilterContext::type() const
{
    return FilterContext::OpenGL;
}

bool GLFilterContext::isReady() const
{
    return false;
}

bool GLFilterContext::prepare()
{
    if (!isReady())
        return false;
    return true;
}

} //namespace QtAV
