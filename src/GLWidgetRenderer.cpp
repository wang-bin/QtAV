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

#include <QtAV/FilterContext.h>
#include <QtAV/OSDFilter.h>

//TODO: QGLfunctions?
namespace QtAV {

const GLfloat kTexCoords[] = {
    0, 0,
    1, 0,
    1, 1,
    0, 1,
};

const GLfloat kVertices[] = {
    -1, 1,
    1, 1,
    1, -1,
    -1, -1,
};

#ifdef QT_OPENGL_ES_2
static inline void checkGlError(const char* op = 0) {
    GLenum error = glGetError();
    if (error == GL_NO_ERROR)
        return;
    qWarning("GL error %s (%#x): %s", op, error, glGetString(error));
}

static const char kVertexShader[] =
    "attribute vec4 a_Position;\n"
    "attribute vec2 a_TexCoords; \n"
    "uniform highp mat4 u_MVP_matrix;\n"
    "varying vec2 v_TexCoords; \n"
    "void main() {\n"
    "  gl_Position = u_MVP_matrix * a_Position;\n"
    "  v_TexCoords = a_TexCoords; \n"
    "}\n";

static const char kFragmentShader[] =
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

GLWidgetRenderer::GLWidgetRenderer(QWidget *parent, const QGLWidget* shareWidget, Qt::WindowFlags f):
    QGLWidget(parent, shareWidget, f),VideoRenderer(*new GLWidgetRendererPrivate())
{
    DPTR_INIT_PRIVATE(GLWidgetRenderer);
    DPTR_D(GLWidgetRenderer);
    d_func().widget_holder = this;
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    /* To rapidly update custom widgets that constantly paint over their entire areas with
     * opaque content, e.g., video streaming widgets, it is better to set the widget's
     * Qt::WA_OpaquePaintEvent, avoiding any unnecessary overhead associated with repainting the
     * widget's background
     */
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
    //default: swap in qpainter dtor. we should swap before QPainter.endNativePainting()
    setAutoBufferSwap(false);
    setAutoFillBackground(false);
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
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GLWidgetRenderer::drawFrame()
{
    DPTR_D(GLWidgetRenderer);
#ifdef QT_OPENGL_ES_2
#define FMT_INTERNAL GL_BGRA //why BGRA?
#define FMT GL_BGRA
    glUseProgram(d.program); //qpainter need
    glActiveTexture(GL_TEXTURE0); //TODO: can remove??
    glUniform1i(d.tex_location, 0);
#else //QT_OPENGL_ES_2
#define FMT_INTERNAL GL_RGBA //why? why 3 works?
#define FMT GL_BGRA
#endif //QT_OPENGL_ES_2
    glBindTexture(GL_TEXTURE_2D, d.texture);
    d.setupQuality();
    glTexImage2D(GL_TEXTURE_2D
                 , 0                //level
                 , FMT_INTERNAL               //internal format. 4? why GL_RGBA? GL_RGB?
                 , d.src_width, d.src_height
                 , 0                //border, ES not support
                 , FMT          //format, must the same as internal format?
                 , GL_UNSIGNED_BYTE
                 , d.data.constData());
#ifndef QT_OPENGL_ES_2
    //qpainter will reset gl state, so need glMatrixMode and clear color(in drawBackground())
    //TODO: study what state will be reset
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glPushMatrix();
    d.setupAspectRatio(); //TODO: can we avoid calling this every time but only in resize event?
    glVertexPointer(2, GL_FLOAT, 0, kVertices);
    glEnableClientState(GL_VERTEX_ARRAY);
    glTexCoordPointer(2, GL_FLOAT, 0, kTexCoords);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glPopMatrix();
#else //QT_OPENGL_ES_2
    d.setupAspectRatio(); //TODO: can we avoid calling this every time but only in resize event?
    //qpainter need. TODO: VBO?
    glVertexAttribPointer(d.position_location, 2, GL_FLOAT, GL_FALSE, 0, kVertices);
    glEnableVertexAttribArray(d.position_location);
    glVertexAttribPointer(d.tex_coords_location, 2, GL_FLOAT, GL_FALSE, 0, kTexCoords);
    glEnableVertexAttribArray(d.tex_coords_location);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glDisableVertexAttribArray(d.tex_coords_location);
    glDisableVertexAttribArray(d.position_location);
#endif //QT_OPENGL_ES_2
}

bool GLWidgetRenderer::write()
{
    update(); //can not call updateGL() directly because no event and paintGL() will in video thread
    return true;
}

void GLWidgetRenderer::initializeGL()
{
    DPTR_D(GLWidgetRenderer);
    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &d.texture);
    qDebug("initializeGL~~~~~~~");
#ifdef QT_OPENGL_ES_2
    if (d.program)
        return;
    d.program = createProgram(kVertexShader, kFragmentShader);
    if (!d.program) {
        qWarning("Could not create program.");
        return;
    }
    d.position_location = glGetAttribLocation(d.program, "a_Position");
    checkGlError("glGetAttribLocation");
    qDebug("glGetAttribLocation(\"a_Position\") = %d\n", d.position_location);
    d.tex_coords_location = glGetAttribLocation(d.program, "a_TexCoords");
    checkGlError("glGetAttribLocation");
    qDebug("glGetAttribLocation(\"a_TexCoords\") = %d\n", d.tex_coords_location);
    d.tex_location = glGetUniformLocation(d.program, "u_Texture");
    checkGlError("glGetUniformLocation");
    qDebug("glGetUniformLocation(\"u_Texture\") = %d\n", d.tex_location);
    d.u_matrix = glGetUniformLocation(d.program, "u_MVP_matrix");
    checkGlError("glGetUniformLocation");
    qDebug("glGetUniformLocation(\"u_MVP_matrix\") = %d\n", d.u_matrix);

    glUseProgram(d.program);
    checkGlError("glUseProgram");
    glVertexAttribPointer(d.position_location, 2, GL_FLOAT, GL_FALSE, 0, kVertices);
    checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(d.position_location);
    checkGlError("glEnableVertexAttribArray");
    glVertexAttribPointer(d.tex_coords_location, 2, GL_FLOAT, GL_FALSE, 0, kTexCoords);
    checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(d.tex_coords_location);
    checkGlError("glEnableVertexAttribArray");
#else
    glShadeModel(GL_SMOOTH); //setupQuality?
    glClearDepth(1.0f);
#endif //QT_OPENGL_ES_2
    glClearColor(0.0, 0.0, 0.0, 0.0);
    d.setupQuality();
}

void GLWidgetRenderer::paintGL()
{
    /* we can mix gl and qpainter.
     * QPainter painter(this);
     * painter.beginNativePainting();
     * gl functions...
     * swapBuffers();
     * painter.endNativePainting();
     */
    handlePaintEvent();
    swapBuffers();
}

void GLWidgetRenderer::resizeGL(int w, int h)
{
    DPTR_D(GLWidgetRenderer);
    qDebug("%s @%d %dx%d", __FUNCTION__, __LINE__, d.out_rect.width(), d.out_rect.height());
    glViewport(0, 0, w, h);
    d.setupAspectRatio();
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
