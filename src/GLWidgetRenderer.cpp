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

#include "QtAV/GLWidgetRenderer.h"
#include "private/GLWidgetRenderer_p.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QResizeEvent>
#include <QtOpenGL/QGLShaderProgram>
#include <QtOpenGL/QGLShader>
//TODO: vsync http://stackoverflow.com/questions/589064/how-to-enable-vertical-sync-in-opengl
//TODO: check gl errors
/*
//GL_BGRA is available in OpenGL >= 1.2
#ifndef GL_BGRA
#ifndef GL_BGRA_EXT
#if defined QT_OPENGL_ES_2
#include <GLES2/gl2ext.h>
#elif defined QT_OPENGL_ES
#include <GLES/glext.h>
#else
#include <GL/glext.h> //GL_BGRA_EXT for OpenGL<=1.1 //TODO Apple include <OpenGL/xxx>
#endif
#endif //GL_BGRA_EXT
//TODO: glPixelStorei(GL_PACK_SWAP_BYTES, ) to swap rgba?
#ifndef GL_BGRA //it may be defined in glext.h
#define GL_BGRA GL_BGRA_EXT
#define GL_BGR GL_BGR_EXT
#endif //GL_BGRA
#endif //GL_BGRA
*/
#ifdef QT_OPENGL_ES_2
#define FMT_INTERNAL GL_BGRA //why BGRA?
#define FMT GL_BGRA
#else //QT_OPENGL_ES_2
#define FMT_INTERNAL GL_RGBA //why? why 3 works?
#define FMT GL_BGRA
#endif //QT_OPENGL_ES_2

#include <QtAV/FilterContext.h>
#include <QtAV/OSDFilter.h>

//TODO: QGLfunctions?
namespace QtAV {


const GLfloat kVertices[] = {
    -1, 1,
    1, 1,
    1, -1,
    -1, -1,
};

static inline void checkGlError(const char* op = 0) {
    GLenum error = glGetError();
    if (error == GL_NO_ERROR)
        return;
    qWarning("GL error %s (%#x): %s", op, error, glGetString(error));
}


static const char kVertexShader[] =
    "attribute vec4 a_Position;\n"
    "attribute vec2 a_TexCoords;\n"
    "uniform mat4 u_MVP_matrix;\n"
    "varying vec2 v_TexCoords;\n"
    "void main() {\n"
    "  gl_Position = u_MVP_matrix * a_Position;\n"
    "  v_TexCoords = a_TexCoords; \n"
    "}\n";

static const char kFragmentShader[] =
#ifdef QT_OPENGL_ES_2
    "precision mediump float;\n"
#endif
    "uniform sampler2D u_Texture0;\n"
    "varying vec2 v_TexCoords;\n"
    "void main() {\n"
    "  gl_FragColor = texture2D(u_Texture0, v_TexCoords);\n"
    "}\n";

// TODO: use QGLShaderProgram for better compatiblity
GLuint GLWidgetRendererPrivate::loadShader(GLenum shaderType, const char* pSource) {
    if (!hasGLSL)
        return 0;
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

GLuint GLWidgetRendererPrivate::createProgram(const char* pVertexSource, const char* pFragmentSource) {
    if (!hasGLSL)
        return 0;
    program = glCreateProgram(); //TODO: var name conflict. temp var is better
    if (!program)
        return 0;
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    if (!vertexShader) {
        return 0;
    }
    GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (!pixelShader) {
        glDeleteShader(vertexShader);
        return 0;
    }
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
        glDetachShader(program, vertexShader);
        glDeleteShader(vertexShader);
        glDetachShader(program, pixelShader);
        glDeleteShader(pixelShader);
        glDeleteProgram(program);
        program = 0;
        return 0;
    }
    vert = vertexShader;
    frag = pixelShader;
    return program;
}

bool GLWidgetRendererPrivate::releaseShaderProgram()
{
    glDeleteTextures(texture.size(), texture.data());
    texture.clear();
    if (vert) {
        if (program)
            glDetachShader(program, vert);
        glDeleteShader(vert);
    }
    if (frag) {
        if (program)
            glDetachShader(program, frag);
        glDeleteShader(frag);
    }
    if (program) {
        glDeleteProgram(program);
        program = 0;
    }
}

bool GLWidgetRendererPrivate::prepareShaderProgram(const VideoFormat &fmt)
{
    if (!hasGLSL) {
        qWarning("Does not support GLSL!");
        return false;
    }
    // isSupported(pixfmt)
    if (!fmt.isValid())
        return false;
    releaseShaderProgram();
    // FIXME
    if (fmt.isRGB()) {
        program = createProgram(kVertexShader, kFragmentShader);
        if (!program) {
            qWarning("Could not create shader program.");
            return false;
        }
    } else if (fmt == VideoFormat::Format_YUV420P) {
        QString shader_file = "/shaders/yuv_rgb.f.glsl";
        QFile f(qApp->applicationDirPath() + shader_file);
        if (!f.exists()) {
            f.setFileName(":" + shader_file);
        }
        if (!f.open(QIODevice::ReadOnly)) {
            qWarning("Can not load shader: %s", f.errorString().toUtf8().constData());
            return false;
        }
        program = createProgram(kVertexShader, f.readAll().constData());
        if (!program) {
            qWarning("Could not create shader program.");
            return false;
        }
    } else {
        return false;
    }
    // vertex shader
    position_location = glGetAttribLocation(program, "a_Position");
    checkGlError("glGetAttribLocation");
    qDebug("glGetAttribLocation(\"a_Position\") = %d\n", position_location);
    tex_coords_location = glGetAttribLocation(program, "a_TexCoords");
    checkGlError("glGetAttribLocation");
    qDebug("glGetAttribLocation(\"a_TexCoords\") = %d\n", tex_coords_location);
    u_matrix = glGetUniformLocation(program, "u_MVP_matrix");
    checkGlError("glGetUniformLocation");
    qDebug("glGetUniformLocation(\"u_MVP_matrix\") = %d\n", u_matrix);

    // fragment shader
    texture.resize(fmt.planeCount());
    tex_location.resize(fmt.planeCount());
    glGenTextures(texture.size(), texture.data());
    for (int i = 0; i < texture.size(); ++i) {
        QString tex_var = QString("u_Texture%1").arg(i);
        tex_location[i] = glGetUniformLocation(program, tex_var.toUtf8().constData());
        checkGlError("glGetUniformLocation");
        qDebug("glGetUniformLocation(\"%s\") = %d\n", tex_var.toUtf8().constData(), tex_location[i]);
    }

    glUseProgram(program);
    checkGlError("glUseProgram");
    glVertexAttribPointer(position_location, 2, GL_FLOAT, GL_FALSE, 0, kVertices);
    checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(position_location);
    checkGlError("glEnableVertexAttribArray");

    return true;
}

void GLWidgetRendererPrivate::upload(const QRect &roi)
{
    GLint internalFormat = GL_LUMINANCE;
    GLenum format = GL_LUMINANCE;
    if (video_frame.format().isRGB()) {
        internalFormat = FMT_INTERNAL;
        format = FMT;
    }
    for (int i = 0; i < video_frame.planeCount(); ++i) {
        uploadPlane(i, internalFormat, format, roi);
    }
}

void GLWidgetRendererPrivate::uploadPlane(int p, GLint internalFormat, GLenum format, const QRect& roi)
{
    if (hasGLSL) {
        glActiveTexture(GL_TEXTURE0 + p); //TODO: can remove??
    }
    glBindTexture(GL_TEXTURE_2D, texture[p]);
    glUniform1i(tex_location[p], p);
    setupQuality();
    // This is necessary for non-power-of-two textures
    glTexParameteri(texture[p], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(texture[p], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    //uploading part of image eats less gpu memory, but may be more cpu(gles)
    //FIXME: more cpu usage then qpainter. FBO, VBO?
#define ROI_TEXCOORDS 1
    //roi for planes?
    if (ROI_TEXCOORDS || roi.size() == video_frame.size()) {
        glTexImage2D(GL_TEXTURE_2D
                     , 0                //level
                     , internalFormat               //internal format. 4? why GL_RGBA? GL_RGB?
                     , video_frame.planeWidth(p)
                     , video_frame.planeHeight(p)
                     , 0                //border, ES not support
                     , format          //format, must the same as internal format?
                     , GL_UNSIGNED_BYTE
                     , video_frame.bits(p));
    } else {
        VideoFormat fmt = video_frame.format();
#ifdef GL_UNPACK_ROW_LENGTH
// http://stackoverflow.com/questions/205522/opengl-subtexturing
        glPixelStorei(GL_UNPACK_ROW_LENGTH, video_frame.planeWidth(p));
        //glPixelStorei or compute pointer
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, fmt.chromaWidth(roi.x()));
        glPixelStorei(GL_UNPACK_SKIP_ROWS, fmt.chromaHeight(roi.y()));
        glTexImage2D(GL_TEXTURE_2D, 0, FMT_INTERNAL, fmt.chromaWidth(roi.width()), fmt.chromaHeight(roi.height()), 0, FMT, GL_UNSIGNED_BYTE, video_frame.bits(p));
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
        glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
#else // GL ES
//define it? or any efficient way?
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, fmt.chromaWidth(roi.width()), fmt.chromaHeight(roi.height()), 0, format, GL_UNSIGNED_BYTE, NULL);
        // how to use only 1 call?
        //glTexSubImage2D(GL_TEXTURE_2D, 0, roi.x(), roi.y(), roi.width(), roi.height(), FMT, GL_UNSIGNED_BYTE, d.data.constData());
        //qDebug("plane=%d %d", p, fmt.chromaHeight(roi.height()));
        for (int y = 0; y < fmt.chromaHeight(roi.height()); y++) {
            char *row = (char*)video_frame.bits(p) + ((y+fmt.chromaHeight(roi.y()))*video_frame.planeWidth(p) + fmt.chromaWidth(roi.x())) * fmt.bytesPerPixel();
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, y, fmt.chromaWidth(roi.width()), 1, FMT, GL_UNSIGNED_BYTE, row);
        }
#endif //GL_UNPACK_ROW_LENGTH
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}


GLWidgetRenderer::GLWidgetRenderer(QWidget *parent, const QGLWidget* shareWidget, Qt::WindowFlags f):
    QGLWidget(parent, shareWidget, f),VideoRenderer(*new GLWidgetRendererPrivate())
{
    DPTR_INIT_PRIVATE(GLWidgetRenderer);
    DPTR_D(GLWidgetRenderer);
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
    d.painter = new QPainter();
    d.filter_context = FilterContext::create(FilterContext::QtPainter);
    QPainterFilterContext *ctx = static_cast<QPainterFilterContext*>(d.filter_context);
    ctx->paint_device = this;
    ctx->painter = d.painter;
    setOSDFilter(new OSDFilterQPainter());
}

bool GLWidgetRenderer::receiveFrame(const VideoFrame& frame)
{
    DPTR_D(GLWidgetRenderer);
    QMutexLocker locker(&d.img_mutex);
    Q_UNUSED(locker);
    d.video_frame = frame;

    update(); //can not call updateGL() directly because no event and paintGL() will in video thread
    return true;
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
    if (d.hasGLSL) {
        glUseProgram(d.program); //qpainter need
    }
    QRect roi = realROI();
#if 1
    d.upload(roi);
#else
    if (d.hasGLSL) {
        glUseProgram(d.program); //qpainter need
        glActiveTexture(GL_TEXTURE0); //TODO: can remove??
    }
    glUniform1i(d.tex_location[0], 0);
    glBindTexture(GL_TEXTURE_2D, d.texture[0]);
    d.setupQuality();
    //uploading part of image eats less gpu memory, but may be more cpu(gles)
    //FIXME: more cpu usage then qpainter. FBO, VBO?
#define ROI_TEXCOORDS 1
    if (ROI_TEXCOORDS || roi.size() == d.video_frame.size()) {
        glTexImage2D(GL_TEXTURE_2D
                     , 0                //level
                     , FMT_INTERNAL               //internal format. 4? why GL_RGBA? GL_RGB?
                     , d.video_frame.width(), d.video_frame.height()
                     , 0                //border, ES not support
                     , FMT          //format, must the same as internal format?
                     , GL_UNSIGNED_BYTE
                     , d.video_frame.bits());
    } else {
#ifdef GL_UNPACK_ROW_LENGTH
// http://stackoverflow.com/questions/205522/opengl-subtexturing
        glPixelStorei(GL_UNPACK_ROW_LENGTH, d.video_frame.width());
        //glPixelStorei or compute pointer
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, roi.x());
        glPixelStorei(GL_UNPACK_SKIP_ROWS, roi.y());
        glTexImage2D(GL_TEXTURE_2D, 0, FMT_INTERNAL, roi.width(), roi.height(), 0, FMT, GL_UNSIGNED_BYTE, d.video_frame.bits());
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
        glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
#else // GL ES
//define it? or any efficient way?
        //TODO: copy to temp buff
        glTexImage2D(GL_TEXTURE_2D, 0, FMT_INTERNAL, roi.width(), roi.height(), 0, FMT, GL_UNSIGNED_BYTE, NULL);
        // how to use only 1 call?
        //glTexSubImage2D(GL_TEXTURE_2D, 0, roi.x(), roi.y(), roi.width(), roi.height(), FMT, GL_UNSIGNED_BYTE, d.data.constData());
        for (int y = 0; y < roi.height(); y++) {
            char *row = (char*)d.video_frame.bits() + ((y + roi.y())*d.video_frame.width() + roi.x()) * 4;
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, y, roi.width(), 1, FMT, GL_UNSIGNED_BYTE, row);
        }
#endif //GL_UNPACK_ROW_LENGTH
    }
#endif //1
    //TODO: compute kTexCoords only if roi changed
#if ROI_TEXCOORDS
        const GLfloat kTexCoords[] = {
            (GLfloat)roi.x()/(GLfloat)d.video_frame.width(), (GLfloat)roi.y()/(GLfloat)d.video_frame.height(),
            (GLfloat)(roi.x() + roi.width())/(GLfloat)d.video_frame.width(), (GLfloat)roi.y()/(GLfloat)d.video_frame.height(),
            (GLfloat)(roi.x() + roi.width())/(GLfloat)d.video_frame.width(), (GLfloat)(roi.y()+roi.height())/(GLfloat)d.video_frame.height(),
            (GLfloat)roi.x()/(GLfloat)d.video_frame.width(), (GLfloat)(roi.y()+roi.height())/(GLfloat)d.video_frame.height(),
        };
///        glVertexAttribPointer(d.tex_coords_location, 2, GL_FLOAT, GL_FALSE, 0, kTexCoords);
///        glEnableVertexAttribArray(d.tex_coords_location);
#else
        const GLfloat kTexCoords[] = {
            0, 0,
            1, 0,
            1, 1,
            0, 1,
        };
#endif //ROI_TEXCOORDS
#ifndef QT_OPENGL_ES_2
    //GL_XXX may not defined in ES2. so macro is required
    if (!d.hasGLSL) {
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
    }
#endif //QT_OPENGL_ES_2
    if (d.hasGLSL) {
        d.setupAspectRatio(); //TODO: can we avoid calling this every time but only in resize event?
        //qpainter need. TODO: VBO?
        glVertexAttribPointer(d.position_location, 2, GL_FLOAT, GL_FALSE, 0, kVertices);
        glEnableVertexAttribArray(d.position_location);
        glVertexAttribPointer(d.tex_coords_location, 2, GL_FLOAT, GL_FALSE, 0, kTexCoords);
        glEnableVertexAttribArray(d.tex_coords_location);

        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        glDisableVertexAttribArray(d.tex_coords_location);
        glDisableVertexAttribArray(d.position_location);
    }
}

void GLWidgetRenderer::initializeGL()
{
    DPTR_D(GLWidgetRenderer);
    makeCurrent();
    qDebug("OpenGL version: %d.%d", format().majorVersion(), format().minorVersion());
    //const QByteArray extensions(reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS)));
    d.hasGLSL = QGLShaderProgram::hasOpenGLShaderPrograms();
    initializeGLFunctions();
    d.initializeGLFunctions();
    checkGlError("initializeGLFunctions");

    glEnable(GL_TEXTURE_2D);
    checkGlError("glEnable");
    qDebug("initializeGL textures: %d~~~~~~~ has GLSL: %d", d.texture.size(), d.hasGLSL);
    if (d.hasGLSL) {
        if (!d.prepareShaderProgram(VideoFormat(VideoFormat::Format_RGB32))) {
            return;
        }
    }
#ifndef QT_OPENGL_ES_2
    if (!d.hasGLSL) {
        glShadeModel(GL_SMOOTH); //setupQuality?
        glClearDepth(1.0f);
        d.texture.resize(1);
        glGenTextures(d.texture.size(), d.texture.data());
        checkGlError("glGenTextures");
    }
#endif //QT_OPENGL_ES_2
    glClearColor(0.0, 0.0, 0.0, 0.0);
    d.setupQuality();
}

void GLWidgetRenderer::paintGL()
{
    DPTR_D(GLWidgetRenderer);
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

void GLWidgetRenderer::resizeGL(int w, int h)
{
    DPTR_D(GLWidgetRenderer);
    qDebug("%s @%d %dx%d", __FUNCTION__, __LINE__, d.out_rect.width(), d.out_rect.height());
    glViewport(0, 0, w, h);
    d.setupAspectRatio();
#ifndef QT_OPENGL_ES_2
    //??
    if (!d.hasGLSL) {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
    }
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
