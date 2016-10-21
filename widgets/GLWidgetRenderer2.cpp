/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2014)

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

#include "QtAVWidgets/GLWidgetRenderer2.h"
#include "QtAV/private/OpenGLRendererBase_p.h"
#include <QResizeEvent>

namespace QtAV {

class GLWidgetRenderer2Private : public OpenGLRendererBasePrivate
{
public:
    GLWidgetRenderer2Private(QPaintDevice *pd)
        : OpenGLRendererBasePrivate(pd)
    {}
};

VideoRendererId GLWidgetRenderer2::id() const
{
    return VideoRendererId_GLWidget2;
}

GLWidgetRenderer2::GLWidgetRenderer2(QWidget *parent, const QGLWidget* shareWidget, Qt::WindowFlags f):
    QGLWidget(parent, shareWidget, f)
  , OpenGLRendererBase(*new GLWidgetRenderer2Private(this))
{
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    /* To rapidly update custom widgets that constantly paint over their entire areas with
     * opaque content, e.g., video streaming widgets, it is better to set the widget's
     * Qt::WA_OpaquePaintEvent, avoiding any unnecessary overhead associated with repainting the
     * widget's background
     */
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NoSystemBackground);
    //default: swap in qpainter dtor. we should swap before QPainter.endNativePainting()
    setAutoBufferSwap(false);
    setAutoFillBackground(false);
}

void GLWidgetRenderer2::initializeGL()
{
    // already current
    onInitializeGL();
}

void GLWidgetRenderer2::paintGL()
{
    DPTR_D(GLWidgetRenderer2);
    /* we can mix gl and qpainter.
     * QPainter painter(this);
     * painter.beginNativePainting();
     * gl functions...
     * painter.endNativePainting();
     * swapBuffers();
     */
    handlePaintEvent();
    swapBuffers();
    if (d.painter && d.painter->isActive())
        d.painter->end();
}

void GLWidgetRenderer2::resizeGL(int w, int h)
{
    onResizeGL(w, h);
}

void GLWidgetRenderer2::resizeEvent(QResizeEvent *e)
{
    onResizeEvent(e->size().width(), e->size().height());
    QGLWidget::resizeEvent(e); //will call resizeGL(). TODO:will call paintEvent()?
}

void GLWidgetRenderer2::showEvent(QShowEvent *)
{
    onShowEvent();
    resizeGL(width(), height());
}
} //namespace QtAV
