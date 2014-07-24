/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2014 Wang Bin <wbsecg1@gmail.com>

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

#include "QtAV/GLWidgetRenderer2.h"
#include "private/VideoRenderer_p.h"
#include "QtAV/OpenGLVideo.h"
#include "QtAV/FilterContext.h"
#include "QtAV/OSDFilter.h"
#include <QResizeEvent>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QtGui/QOpenGLShaderProgram>
#else
#include <QtOpenGL/QGLShaderProgram>
#define QOpenGLShaderProgram QGLShaderProgram
#endif

namespace QtAV {

class GLWidgetRenderer2Private : public VideoRendererPrivate
{
public:
    GLWidgetRenderer2Private()
        : painter(0)
    {}
    virtual ~GLWidgetRenderer2Private() {}
    void setupAspectRatio() {
        matrix(0, 0) = (GLfloat)out_rect.width()/(GLfloat)renderer_width;
        matrix(1, 1) = (GLfloat)out_rect.height()/(GLfloat)renderer_height;
    }

    QPainter *painter;
    OpenGLVideo glv;
    QMatrix4x4 matrix;
};


GLWidgetRenderer2::GLWidgetRenderer2(QWidget *parent, const QGLWidget* shareWidget, Qt::WindowFlags f):
    QGLWidget(parent, shareWidget, f),VideoRenderer(*new GLWidgetRenderer2Private())
{
    DPTR_INIT_PRIVATE(GLWidgetRenderer2);
    DPTR_D(GLWidgetRenderer2);
    setPreferredPixelFormat(VideoFormat::Format_YUV420P);
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


bool GLWidgetRenderer2::isSupported(VideoFormat::PixelFormat pixfmt) const
{
    Q_UNUSED(pixfmt);
    return true;
}

bool GLWidgetRenderer2::receiveFrame(const VideoFrame& frame)
{
    DPTR_D(GLWidgetRenderer2);
    QMutexLocker locker(&d.img_mutex);
    Q_UNUSED(locker);
    d.video_frame = frame;

    d.glv.setCurrentFrame(frame);

    update(); //can not call updateGL() directly because no event and paintGL() will in video thread
    return true;
}

bool GLWidgetRenderer2::needUpdateBackground() const
{
    return true;
}

void GLWidgetRenderer2::drawBackground()
{
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GLWidgetRenderer2::drawFrame()
{
    DPTR_D(GLWidgetRenderer2);
    QRect roi = realROI();
    //d.glv.render(QRectF(-1, 1, 2, -2), roi, d.matrix);
    // QRectF() means the whole viewport
    d.glv.render(QRectF(), roi, d.matrix);
}

void GLWidgetRenderer2::initializeGL()
{
    DPTR_D(GLWidgetRenderer2);
    makeCurrent();
    QOpenGLContext *ctx = const_cast<QOpenGLContext*>(QOpenGLContext::currentContext()); //qt4 returns const
    d.glv.setOpenGLContext(ctx);
    //const QByteArray extensions(reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS)));
    bool hasGLSL = QOpenGLShaderProgram::hasOpenGLShaderPrograms();
    qDebug("OpenGL version: %d.%d  hasGLSL: %d", format().majorVersion(), format().minorVersion(), hasGLSL);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.0, 0.0, 0.0, 0.0);
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
    DPTR_D(GLWidgetRenderer2);
    glViewport(0, 0, w, h);
    d.glv.setViewport(QRect(0, 0, w, h));
    //qDebug("%s @%d %dx%d", __FUNCTION__, __LINE__, d.out_rect.width(), d.out_rect.height());
    d.setupAspectRatio();
}

void GLWidgetRenderer2::resizeEvent(QResizeEvent *e)
{
    DPTR_D(GLWidgetRenderer2);
    d.update_background = true;
    resizeRenderer(e->size());
    d.setupAspectRatio();
    QGLWidget::resizeEvent(e); //will call resizeGL(). TODO:will call paintEvent()?
}

//TODO: out_rect not correct when top level changed
void GLWidgetRenderer2::showEvent(QShowEvent *)
{
    DPTR_D(GLWidgetRenderer2);
    d.update_background = true;
    /*
     * Do something that depends on widget below! e.g. recreate render target for direct2d.
     * When Qt::WindowStaysOnTopHint changed, window will hide first then show. If you
     * don't do anything here, the widget content will never be updated.
     */
}

void GLWidgetRenderer2::onSetOutAspectRatio(qreal ratio)
{
    Q_UNUSED(ratio);
    DPTR_D(GLWidgetRenderer2);
    d.setupAspectRatio();
}

void GLWidgetRenderer2::onSetOutAspectRatioMode(OutAspectRatioMode mode)
{
    Q_UNUSED(mode);
    DPTR_D(GLWidgetRenderer2);
    d.setupAspectRatio();
}

bool GLWidgetRenderer2::onSetBrightness(qreal b)
{
    d_func().glv.setBrightness(b);
    return true;
}

bool GLWidgetRenderer2::onSetContrast(qreal c)
{
    d_func().glv.setContrast(c);
    return true;
}

bool GLWidgetRenderer2::onSetHue(qreal h)
{
    d_func().glv.setHue(h);
    return true;
}

bool GLWidgetRenderer2::onSetSaturation(qreal s)
{
    d_func().glv.setSaturation(s);
    return true;
}

} //namespace QtAV
