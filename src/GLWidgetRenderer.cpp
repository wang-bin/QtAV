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
/*
 * TODO:
 *   why image flip
 *   include gl header?
 *   GLES 2 support
 *   inherits QPainterRenderer? GL is wrapped as QPainter
 *   GLuint bindTexture(const QImage & image, GLenum target = GL_TEXTURE_2D, GLint format)
 */

#include "QtAV/GLWidgetRenderer.h"
#include "private/GLWidgetRenderer_p.h"
#include <QResizeEvent>
//TODO: vsync http://stackoverflow.com/questions/589064/how-to-enable-vertical-sync-in-opengl
//TODO: check gl errors
//GL_BGRA is available in OpenGL >= 1.2
#ifndef GL_BGRA
#ifndef GL_BGRA_EXT
#include <GL/glext.h> //GL_BGRA_EXT for OpenGL<=1.1 //TODO Apple include <OpenGL/xxx>
#endif //GL_BGRA_EXT
#ifndef GL_BGRA //it may be defined in glext.h
#define GL_BGRA GL_BGRA_EXT
#define GL_BGR GL_BGR_EXT
#endif //GL_BGRA
#endif //GL_BGRA

#include <QtAV/FilterContext.h>
#include <QtAV/OSDFilter.h>

namespace QtAV {

GLWidgetRenderer::GLWidgetRenderer(QWidget *parent, const QGLWidget* shareWidget, Qt::WindowFlags f):
    QGLWidget(parent, shareWidget, f),VideoRenderer(*new GLWidgetRendererPrivate())
{
    DPTR_INIT_PRIVATE(GLWidgetRenderer);
    DPTR_D(GLWidgetRenderer);
    d_func().widget_holder = this;
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    //setAttribute(Qt::WA_OpaquePaintEvent);
    //setAttribute(Qt::WA_NoSystemBackground);
    setAutoFillBackground(false);
//    makeCurrent();
    d.filter_context = FilterContext::create(FilterContext::OpenGL);
    ((GLFilterContext*)d.filter_context)->paint_device = this;
    setOSDFilter(new OSDFilterGL());
}

GLWidgetRenderer::~GLWidgetRenderer()
{
}

void GLWidgetRenderer::convertData(const QByteArray &data)
{
    DPTR_D(GLWidgetRenderer);
    QMutexLocker locker(&d.img_mutex);
    Q_UNUSED(locker);
    d.data = data;
}

bool GLWidgetRenderer::needUpdateBackground() const
{
    return true;
}

void GLWidgetRenderer::drawBackground()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GLWidgetRenderer::drawFrame()
{
    DPTR_D(GLWidgetRenderer);
    glTexImage2D(GL_TEXTURE_2D
                 , 0                //level
                 , 3                //internal format. 4? why GL_RGBA?
                 , d.src_width, d.src_height
                 , 0                //border, ES not support
                 , GL_BGRA          //format, must the same as internal format?
                 , GL_UNSIGNED_BYTE
                 , d.data.constData());
    glPushMatrix();
    glOrtho( 0, width(), height(), 0, -1, 1 );
    const int x = d.out_rect.x(), y = d.out_rect.y();
    const int w = d.out_rect.width(), h = d.out_rect.height();
#if 1
    //GLfloat?
    const GLint V[] = {
        x,     y + h,  //top left
        x,     y,      //bottom left
        x + w, y,      //bottom right
        x + w, y + h   //top right
    };
    const GLshort T[] = {
        0, 1,
        0, 0,
        1, 0,
        1, 1
    };
    glVertexPointer(2, GL_INT, 0, V);
    glEnableClientState(GL_VERTEX_ARRAY);
    glTexCoordPointer(2, GL_SHORT, 0, T);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#else
    glBegin(GL_QUADS); //TODO: what if no GL_QUADS? triangle?
    glTexCoord2i(-1, -1);
    glVertex2f(x, y);
    glTexCoord2i(0, -1);
    glVertex2f(x + w, y);
    glTexCoord2i(0, 0);
    glVertex2f(x + w, y + h);
    glTexCoord2i(-1, 0);
    glVertex2f(x, y + h );
    glEnd();
#endif
    glPopMatrix();
}

bool GLWidgetRenderer::write()
{
    update();
    return true;
}

void GLWidgetRenderer::initializeGL()
{
    glEnable(GL_TEXTURE_2D);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glShadeModel(GL_SMOOTH); //setupQuality?
    glClearColor(0.0, 0.0, 0.0, 0.5);
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    d_func().setupQuality();
}

void GLWidgetRenderer::paintGL()
{
    handlePaintEvent();
}

void GLWidgetRenderer::resizeGL(int w, int h)
{
    DPTR_D(GLWidgetRenderer);
    qDebug("%s @%d %dx%d", __FUNCTION__, __LINE__, d.out_rect.width(), d.out_rect.height());
    glViewport(0, 0, w, h);
    //??
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void GLWidgetRenderer::resizeEvent(QResizeEvent *e)
{
    DPTR_D(GLWidgetRenderer);
    d.update_background = true;
    resizeRenderer(e->size());
    QGLWidget::resizeEvent(e); //will call resizeGL(). TODO:will call paintEvent()?
}

//TODO: out_rect not correct when top level changed
void GLWidgetRenderer::showEvent(QShowEvent *)
{
    DPTR_D(GLWidgetRenderer);
    d.update_background = true;
    /*
     * Do something that depends on widget below! e.g. recreate render target for direct2d.
     * When Qt::WindowStaysOnTopHint changed, window will hide first then show. If you
     * don't do anything here, the widget content will never be updated.
     */
}
} //namespace QtAV
