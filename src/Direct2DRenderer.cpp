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

#include "QtAV/Direct2DRenderer.h"
#include "private/ImageRenderer_p.h"
#include <QtGui/QPainter>
#include <QtGui/QPaintEngine>
#include <QResizeEvent>

namespace QtAV {

class Direct2DRendererPrivate : public ImageRendererPrivate
{
public:
    DPTR_DECLARE_PUBLIC(Direct2DRenderer)

    Direct2DRendererPrivate():
		use_qpainter(true)
    {
    }
    ~Direct2DRendererPrivate() {
    }
    bool use_qpainter; //TODO: move to base class
};

Direct2DRenderer::Direct2DRenderer(QWidget *parent, Qt::WindowFlags f):
    QWidget(parent, f),ImageRenderer(*new Direct2DRendererPrivate())
{
    DPTR_INIT_PRIVATE(Direct2DRenderer);
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    //setAttribute(Qt::WA_OpaquePaintEvent);
    //setAttribute(Qt::WA_NoSystemBackground);
    setAutoFillBackground(false);
}

Direct2DRenderer::~Direct2DRenderer()
{
}

bool Direct2DRenderer::write()
{
    update();
    return true;
}

QPaintEngine* Direct2DRenderer::paintEngine() const
{
    if (d_func().use_qpainter) {
        return QWidget::paintEngine();
    } else {
        return 0; //use native engine
    }
}

void Direct2DRenderer::useQPainter(bool qp)
{
    DPTR_D(Direct2DRenderer);
    d.use_qpainter = qp;
    setAttribute(Qt::WA_PaintOnScreen, !d.use_qpainter);
}

bool Direct2DRenderer::useQPainter() const
{
    DPTR_D(const Direct2DRenderer);
	return d.use_qpainter;
}

void Direct2DRenderer::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::ActivationChange) { //auto called when show
        useQPainter(d_func().use_qpainter);
        event->accept();
    }
}

void Direct2DRenderer::resizeEvent(QResizeEvent *e)
{
    resizeVideo(e->size());
    update();
}

void Direct2DRenderer::paintEvent(QPaintEvent *)
{
    DPTR_D(Direct2DRenderer);
    if (!d.scale_in_qt) {
        d.img_mutex.lock();
    }
    QImage image = d.image; //TODO: other renderer use this style
    if (image.isNull()) {
        if (d.preview.isNull()) {
            d.preview = QImage(videoSize(), QImage::Format_RGB32);
            d.preview.fill(Qt::black); //maemo 4.7.0: QImage.fill(uint)
        }
        image = d.preview;
    }
    //begin paint
    
    //end paint
    if (!d.scale_in_qt) {
        d.img_mutex.unlock();
    }
}

} //namespace QtAV
