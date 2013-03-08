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
#include "private/VideoRenderer_p.h"
#include <QResizeEvent>

#include <GL/glext.h> //GL_BGRA_EXT for OpenGL<=1.1

//TODO: vsync http://stackoverflow.com/questions/589064/how-to-enable-vertical-sync-in-opengl
//TODO: check gl errors
//GL_BGRA is available in OpenGL >= 1.2
#ifndef GL_BGRA
#define GL_BGRA GL_BGRA_EXT
#endif //GL_BGRA
#ifndef GL_BGR
#define GL_BGR GL_BGR_EXT
#endif //GL_BGR

namespace QtAV {

class GLWidgetRendererPrivate : public VideoRendererPrivate
{
public:
    GLWidgetRendererPrivate():
        texture(0)
    {
        if (QGLFormat::openGLVersionFlags() == QGLFormat::OpenGL_Version_None) {
            available = false;
            return;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
    }
    ~GLWidgetRendererPrivate() {
        glDeleteTextures(1, &texture);
    }

    QRect out_rect_old;
    GLuint texture;
};

GLWidgetRenderer::GLWidgetRenderer(QWidget *parent, const QGLWidget* shareWidget, Qt::WindowFlags f):
    QGLWidget(parent, shareWidget, f),VideoRenderer(*new GLWidgetRendererPrivate())
{
    DPTR_INIT_PRIVATE(GLWidgetRenderer);
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    //setAttribute(Qt::WA_OpaquePaintEvent);
    //setAttribute(Qt::WA_NoSystemBackground);
    setAutoFillBackground(false);
//    makeCurrent();
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

bool GLWidgetRenderer::write()
{
    update();
    return true;
}

void GLWidgetRenderer::initializeGL()
{
    glEnable(GL_TEXTURE_2D);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//
    glShadeModel(GL_SMOOTH);
    glClearColor(0.0, 0.0, 0.0, 0.5);
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
}

void GLWidgetRenderer::paintGL()
{
    DPTR_D(GLWidgetRenderer);
    if (d.aspect_ratio_changed || d.update_background || d.out_rect_old != d.out_rect) {
        d.out_rect_old = d.out_rect;
        resizeGL(width(), height());
        d.update_background = false;
    }
    //begin paint
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (d.data.isEmpty()) {
        return;
    }
    QMutexLocker locker(&d.img_mutex);
    Q_UNUSED(locker);
    glTexImage2D(GL_TEXTURE_2D, 0, 3/*internalFormat? 4?*/, d.src_width, d.src_height, 0/*border*/, GL_BGRA, GL_UNSIGNED_BYTE, d.data.constData());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glPushMatrix();
    glLoadIdentity();
    //glRotatef(180.0f, 0.0f, 0.0f, 0.0f); //flip the image

    glBegin(GL_QUADS);
    glTexCoord2d(0.0, 0.0); glVertex2d(-1.0, +1.0);
    glTexCoord2d(1.0, 0.0); glVertex2d(+1.0, +1.0);
    glTexCoord2d(1.0, 1.0); glVertex2d(+1.0, -1.0);
    glTexCoord2d(0.0, 1.0); glVertex2d(-1.0, -1.0);
    glEnd();
    //glFlush();
    glPopMatrix();
    //swapBuffers(); //why flickers?
}

void GLWidgetRenderer::resizeGL(int w, int h)
{
    Q_UNUSED(w);
    Q_UNUSED(h);
    DPTR_D(GLWidgetRenderer);
    qDebug("%s @%d %dx%d", __FUNCTION__, __LINE__, d.out_rect.width(), d.out_rect.height());
    //TODO: if whole widget as viewport, we can set rect by glVertex, thus paint logic is the same as others
    glViewport(d.out_rect.x(), d.out_rect.y(), d.out_rect.width(), d.out_rect.height());
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

} //namespace QtAV
