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

#include <QtAV/WidgetRenderer.h>
#include <private/WidgetRenderer_p.h>
#include <qfont.h>
#include <qevent.h>
#include <qpainter.h>
#include <QtAV/Filter.h>

namespace QtAV {
WidgetRenderer::WidgetRenderer(QWidget *parent, Qt::WindowFlags f) :
    QWidget(parent, f),QPainterRenderer(*new WidgetRendererPrivate())
{
    DPTR_D(WidgetRenderer);
    d.widget_holder = this;
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
}

WidgetRenderer::WidgetRenderer(WidgetRendererPrivate &d, QWidget *parent, Qt::WindowFlags f)
    :QWidget(parent, f),QPainterRenderer(d)
{
    d.widget_holder = this;
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
}

WidgetRenderer::~WidgetRenderer()
{
    DPTR_D(WidgetRenderer);
    if (d.painter) {
        delete d.painter;
        d.painter = 0; //move to private class? what about graphicsitem
    }
}

bool WidgetRenderer::write()
{
    update();
    return true;
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
    //assume that the image data is already scaled to out_size(NOT renderer size!)
    if (!d.scale_in_renderer || d.image.size() == d.out_rect.size()) {
        //d.preview = d.image;
        d.painter->drawImage(d.out_rect.topLeft(), d.image);
    } else {
        //qDebug("size not fit. may slow. %dx%d ==> %dx%d"
        //       , d.image.size().width(), image.size().height(), d.renderer_width, d.renderer_height);
        d.painter->drawImage(d.out_rect, d.image);
        //what's the difference?
        //d.painter->drawImage(QPoint(), image.scaled(d.renderer_width, d.renderer_height));
    }
}

void WidgetRenderer::drawSubtitle()
{
}

void WidgetRenderer::drawOSD()
{
}

void WidgetRenderer::drawCustom()
{
}

void WidgetRenderer::resizeEvent(QResizeEvent *e)
{
    DPTR_D(WidgetRenderer);
    d.update_background = true;
    resizeRenderer(e->size());
    update();
}

void WidgetRenderer::mousePressEvent(QMouseEvent *e)
{
    DPTR_D(WidgetRenderer);
    d.gMousePos = e->globalPos();
    d.iMousePos = e->pos();
}

void WidgetRenderer::mouseMoveEvent(QMouseEvent *e)
{
    if (parentWidget())
        return;
    DPTR_D(WidgetRenderer);
    int x = pos().x();
    int y = pos().y();
    int dx = e->globalPos().x() - d.gMousePos.x();
    int dy = e->globalPos().y() - d.gMousePos.y();
    d.gMousePos = e->globalPos();
    int w = width();
    int h = height();
    switch (d.action) {
    case GestureMove:
        x += dx;
        y += dy;
        move(x, y);
        break;
    case GestureResize:
        if(d.iMousePos.x() < w/2) {
            x += dx;
            w -= dx;
        }
        if(d.iMousePos.x() > w/2) {
            w += dx;
        }
        if(d.iMousePos.y() < h/2) {
            y += dy;
            h -= dy;
        }
        if(d.iMousePos.y() > h/2) {
            h += dy;
        }
        //setGeometry(x, y, w, h);
        move(x, y);
        resize(w, h);
        break;
    }
    repaint();
}

void WidgetRenderer::mouseDoubleClickEvent(QMouseEvent *)
{
    DPTR_D(WidgetRenderer);
    if (d.action == GestureMove)
        d.action = GestureResize;
    else
        d.action = GestureMove;
}

void WidgetRenderer::paintEvent(QPaintEvent *)
{
    //TODO: set QPainter to context;
    DPTR_D(WidgetRenderer);
    d.painter->begin(this); //Widget painting can only begin as a result of a paintEvent
    //begin paint. how about QPainter::beginNativePainting()?
    //fill background color when necessary, e.g. renderer is resized, image is null
    //if we access d.data which will be modified in AVThread, the following must be protected
    if (d.out_rect != rect()) {
        d.update_background = false;
        //fill background color. DO NOT return, you must continue drawing
        drawBackground();
    }
    {
        //lock is required only when drawing the frame
        QMutexLocker locker(&d.img_mutex);
        Q_UNUSED(locker);
        //DO NOT return if no data. we should draw other things
        if (!d.data.isEmpty()) {
            drawFrame();
        }
    }
    //drawXXX only implement the painting, no other logic
    if (d.draw_osd)
        drawOSD();
    if (d.draw_subtitle)
        drawSubtitle();
    if (d.draw_custom)
        drawCustom();
    //end paint. how about QPainter::endNativePainting()?

    //TODO: move to applyFilters() //private?
    if (d.filter_context && d.statistics) {
        foreach(Filter* filter, d.filters) {
            if (!filter) {
                qWarning("a null filter!");
                //d.filters.removeOne(filter);
                continue;
            }
            filter->process(d.filter_context, d.statistics);
        }
    } else {
        //warn once
    }
    d.painter->end();
}

} //namespace QtAV
