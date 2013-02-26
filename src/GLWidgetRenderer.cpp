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

#include "QtAV/GLWidgetRenderer.h"
#include "private/ImageRenderer_p.h"
#include <QtGui/QPainter>
#include <QtGui/QPaintEngine>
#include <QResizeEvent>

namespace QtAV {

class GLWidgetRendererPrivate : public ImageRendererPrivate
{
public:
};

GLWidgetRenderer::GLWidgetRenderer(QWidget *parent, const QGLWidget* shareWidget, Qt::WindowFlags f):
    QGLWidget(parent, shareWidget, f),ImageRenderer(*new GLWidgetRendererPrivate())
{
    DPTR_INIT_PRIVATE(GLWidgetRenderer);
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    //setAttribute(Qt::WA_OpaquePaintEvent);
    //setAttribute(Qt::WA_NoSystemBackground);
    setAutoFillBackground(false);
}

GLWidgetRenderer::~GLWidgetRenderer()
{
}


bool GLWidgetRenderer::write()
{
    update();
    return true;
}

void GLWidgetRenderer::paintEvent(QPaintEvent *)
{
    DPTR_D(GLWidgetRenderer);
    if (!d.scale_in_qt) {
        d.img_mutex.lock();
    }
    //begin paint. how about QPainter::beginNativePainting()?
    QPainter p(this); //QPaintEngine is OpenGL
    if (d.image.isNull()) {
        //TODO: when setSourceSize()?
        d.image = QImage(videoSize(), QImage::Format_RGB32);
        d.image.fill(Qt::black); //maemo 4.7.0: QImage.fill(uint)
    }
    if (d.image.size() == QSize(d.width, d.height)) {
        //d.preview = d.image;
        p.drawImage(QPoint(), d.image);
    } else {
        //qDebug("size not fit. may slow. %dx%d ==> %dx%d"
        //       , d.image.size().width(), image.size().height(), d.width, d.height);
        p.drawImage(rect(), d.image);
        //what's the difference?
        //p.drawImage(QPoint(), image.scaled(d.width, d.height));
    }
    //end paint. how about QPainter::endNativePainting()?
    if (!d.scale_in_qt) {
        d.img_mutex.unlock();
    }
}

void GLWidgetRenderer::resizeEvent(QResizeEvent *e)
{
    resizeVideo(e->size());
    update();
}

} //namespace QtAV
