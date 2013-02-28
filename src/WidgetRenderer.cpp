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

namespace QtAV {
WidgetRenderer::WidgetRenderer(QWidget *parent, Qt::WindowFlags f) :
    QWidget(parent, f),ImageRenderer(*new WidgetRendererPrivate())
{
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    setAutoFillBackground(false);
}

WidgetRenderer::WidgetRenderer(WidgetRendererPrivate &d, QWidget *parent, Qt::WindowFlags f)
    :QWidget(parent, f),ImageRenderer(d)
{
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    setAutoFillBackground(false);
}

WidgetRenderer::~WidgetRenderer()
{
}

bool WidgetRenderer::write()
{
    update();
	return true;
}

void WidgetRenderer::resizeEvent(QResizeEvent *e)
{
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
    DPTR_D(WidgetRenderer);
    if (!d.scale_in_qt) {
        d.img_mutex.lock();
    }
    QPainter p(this);
    p.fillRect(rect(), QColor(0, 0, 0));
    if (d.image.isNull()) {
        //TODO: when setInSize()?
        d.image = QImage(rendererSize(), QImage::Format_RGB32);
        d.image.fill(Qt::black); //maemo 4.7.0: QImage.fill(uint)
    }
    if (d.image.size() == d.out_rect.size()) {
        //d.preview = d.image;
        p.drawImage(d.out_rect.topLeft(), d.image);
    } else {
        //qDebug("size not fit. may slow. %dx%d ==> %dx%d"
        //       , d.image.size().width(), image.size().height(), d.renderer_width, d.renderer_height);
        p.drawImage(d.out_rect, d.image);
        //what's the difference?
        //p.drawImage(QPoint(), image.scaled(d.renderer_width, d.renderer_height));
    }
    if (!d.scale_in_qt) {
        d.img_mutex.unlock();
    }
}

} //namespace QtAV
