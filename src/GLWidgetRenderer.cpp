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

#ifdef QT_OPENGL_ES_2
const static GLfloat PI = 3.1415f;
static void printGLString(const char *name, GLenum s) {
    const char *v = (const char *)glGetString(s);
    qDebug("GL %s = %s", name, v);
}
static void checkGlError(const char* op) {
    return;
    for (GLint error = glGetError(); error; error = glGetError()) {
        qCritical("after %s() glError (0%#x): %s", op, error, glGetString(error));
    }
}
static GLuint textureID = 0;
static GLuint gProgram = 0;
static GLuint gPositionHandle = 0;
static GLuint gTexCoordsHandle = 0;
static GLuint gTexHandle = 0;
const GLfloat gVertices[] = { -1, -1, 1, -1, -1, 1, 1, 1 };
const GLfloat gTexCoords[] = { 0, 1, 1, 1, 0, 0, 1, 0};
static const char gVertexShader[] =
    "attribute vec4 a_Position;\n"
    "attribute vec2 a_TexCoords; \n"
    "varying vec2 v_TexCoords; \n"
    "void main() {\n"
    "  gl_Position = a_Position;\n"
    "  v_TexCoords = a_TexCoords; \n"
    "}\n";

static const char gFragmentShader[] =
    "precision mediump float;\n"
    "uniform sampler2D u_Texture; \n"
    "varying vec2 v_TexCoords; \n"
    "void main() {\n"
    "  gl_FragColor = texture2D(u_Texture, v_TexCoords);\n"
    "}\n";

GLuint loadShader(GLenum shaderType, const char* pSource) {
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char* buf = (char*)malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    qWarning("Could not compile shader %d:\n%s\n", shaderType, buf);
                    free(buf);
                }
                glDeleteShader(shader);
                shader = 0;
            }
        }
    }
    return shader;
}

GLuint createProgram(const char* pVertexSource, const char* pFragmentSource) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    if (!vertexShader) {
        return 0;
    }

    GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (!pixelShader) {
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        checkGlError("glAttachShader v");
        glAttachShader(program, pixelShader);
        checkGlError("glAttachShader f");
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char* buf = (char*)malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
                    qWarning("Could not link program:\n%s\n", buf);
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    }
    return program;
}
#endif

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
    initializeGL(); //why here manually?
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
#ifdef QT_OPENGL_ES_2
#define FMT_INTERNAL GL_BGRA
#define FMT GL_BGRA
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(gTexHandle, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
#else
#define FMT_INTERNAL GL_RGBA //why 3 works?
#define FMT GL_BGRA
#endif //QT_OPENGL_ES_2
    glTexImage2D(GL_TEXTURE_2D
                 , 0                //level
                 , FMT_INTERNAL               //internal format. 4? why GL_RGBA? GL_RGB?
                 , d.src_width, d.src_height
                 , 0                //border, ES not support
                 , FMT          //format, must the same as internal format?
                 , GL_UNSIGNED_BYTE
                 , d.data.constData());
#ifndef QT_OPENGL_ES_2
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
#else
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
#endif //QT_OPENGL_ES_2
}

bool GLWidgetRenderer::write()
{
    update();
    return true;
}

void GLWidgetRenderer::initializeGL()
{
    glEnable(GL_TEXTURE_2D);
    qDebug("initializeGL~~~~~~~");
#ifdef QT_OPENGL_ES_2
    if (gProgram)
        return;
    gProgram = createProgram(gVertexShader, gFragmentShader);
    if (!gProgram) {
        qWarning("Could not create program.");
        return;
    }
    gPositionHandle = glGetAttribLocation(gProgram, "a_Position");
    checkGlError("glGetAttribLocation");
    qDebug("glGetAttribLocation(\"a_Position\") = %d\n", gPositionHandle);
    gTexCoordsHandle = glGetAttribLocation(gProgram, "a_TexCoords");
    checkGlError("glGetAttribLocation");
    qDebug("glGetAttribLocation(\"a_TexCoords\") = %d\n", gTexCoordsHandle);
    gTexHandle = glGetUniformLocation(gProgram, "u_Texture");
    checkGlError("glGetUniformLocation");
    qDebug("glGetUniformLocation(\"u_Texture\") = %d\n", gTexHandle);


    glUseProgram(gProgram);
    checkGlError("glUseProgram");
    glVertexAttribPointer(gPositionHandle, 2, GL_FLOAT, GL_FALSE, 0, gVertices);
    checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(gPositionHandle);
    checkGlError("glEnableVertexAttribArray");
    glVertexAttribPointer(gTexCoordsHandle, 2, GL_FLOAT, GL_FALSE, 0, gTexCoords);
    checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(gTexCoordsHandle);
    checkGlError("glEnableVertexAttribArray");
#else
    glShadeModel(GL_SMOOTH); //setupQuality?
    glClearDepth(1.0f);
#endif //QT_OPENGL_ES_2
    glClearColor(0.0, 0.0, 0.0, 0.0);
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
#ifndef QT_OPENGL_ES_2
    //??
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
#endif //QT_OPENGL_ES_2
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
