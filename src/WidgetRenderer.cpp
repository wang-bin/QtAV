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

#include <QtAV/WidgetRenderer.h>
#include <private/WidgetRenderer_p.h>
#include <qfont.h>
#include <qevent.h>
#include <qpainter.h>
#include <QApplication>
#include <QtAV/Filter.h>

namespace QtAV {
WidgetRenderer::WidgetRenderer(QWidget *parent, Qt::WindowFlags f) :
    QWidget(parent, f),QPainterRenderer(*new WidgetRendererPrivate())
{
    DPTR_D(WidgetRenderer);
    d.painter = new QPainter();
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    /* To rapidly update custom widgets that constantly paint over their entire areas with
     * opaque content, e.g., video streaming widgets, it is better to set the widget's
     * Qt::WA_OpaquePaintEvent, avoiding any unnecessary overhead associated with repainting the
     * widget's background
     */
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAutoFillBackground(false);
    QPainterFilterContext *ctx = static_cast<QPainterFilterContext*>(d.filter_context);
    if (ctx) {
        ctx->painter = d.painter;
    } else {
        qWarning("FilterContext not available!");
    }
    connect(this, SIGNAL(imageReady()), SLOT(update()));
}

WidgetRenderer::WidgetRenderer(WidgetRendererPrivate &d, QWidget *parent, Qt::WindowFlags f)
    :QWidget(parent, f),QPainterRenderer(d)
{
    d.painter = new QPainter();
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    setAutoFillBackground(false);
    QPainterFilterContext *ctx = static_cast<QPainterFilterContext*>(d.filter_context);
    if (ctx) {
        ctx->painter = d.painter;
    } else {
        qWarning("FilterContext not available!");
    }
    connect(this, SIGNAL(imageReady()), SLOT(update()));
}

bool WidgetRenderer::receiveFrame(const VideoFrame &frame)
{
    prepareFrame(frame);
    //update();
    /*
     * workaround for the widget not updated if has parent. don't know why it works and why update() can't
     * Thanks to Vito Covito and Carlo Scarpato
     */
    emit imageReady();
    return true;
}

bool WidgetRenderer::needUpdateBackground() const
{
    DPTR_D(const WidgetRenderer);
    return d.out_rect != rect();
}

void WidgetRenderer::drawBackground()
{
    DPTR_D(WidgetRenderer);
    d.painter->fillRect(rect(), QColor(0, 0, 0));
}

void WidgetRenderer::drawFrame()
{
    DPTR_D(WidgetRenderer);
    if (d.image.isNull()) {
        //TODO: when setInSize()?
        d.image = QImage(rendererSize(), QImage::Format_RGB32);
        d.image.fill(Qt::black); //maemo 4.7.0: QImage.fill(uint)
    }
    QRect roi = realROI();
    //assume that the image data is already scaled to out_size(NOT renderer size!)
    if (!d.scale_in_renderer || roi.size() == d.out_rect.size()) {
        //d.preview = d.image;
        d.painter->drawImage(d.out_rect.topLeft(), d.image, roi);
    } else {
        //qDebug("size not fit. may slow. %dx%d ==> %dx%d"
        //       , d.image.size().width(), image.size().height(), d.renderer_width, d.renderer_height);
        d.painter->drawImage(d.out_rect, d.image, roi);
        //what's the difference?
        //d.painter->drawImage(QPoint(), image.scaled(d.renderer_width, d.renderer_height));
    }
}

void WidgetRenderer::resizeEvent(QResizeEvent *e)
{
    DPTR_D(WidgetRenderer);
    d.update_background = true;
    resizeRenderer(e->size());
    update();
}

void WidgetRenderer::paintEvent(QPaintEvent *)
{
    DPTR_D(WidgetRenderer);
    d.painter->begin(this); //Widget painting can only begin as a result of a paintEvent
    handlePaintEvent();
    if (d.painter->isActive())
        d.painter->end();
}

} //namespace QtAV
