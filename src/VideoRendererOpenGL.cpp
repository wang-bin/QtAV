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

#include "QtAV/VideoRendererOpenGL.h"
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

class VideoRendererOpenGLPrivate : public VideoRendererPrivate
{
public:
    VideoRendererOpenGLPrivate()
        : painter(0)
    {}
    virtual ~VideoRendererOpenGLPrivate() {}

    QPainter *painter;
    OpenGLVideo glv;
};


VideoRendererOpenGL::VideoRendererOpenGL(QWidget *parent, const QGLWidget* shareWidget, Qt::WindowFlags f):
    QGLWidget(parent, shareWidget, f),VideoRenderer(*new VideoRendererOpenGLPrivate())
{
    DPTR_INIT_PRIVATE(VideoRendererOpenGL);
    DPTR_D(VideoRendererOpenGL);
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


bool VideoRendererOpenGL::isSupported(VideoFormat::PixelFormat pixfmt) const
{
    Q_UNUSED(pixfmt);
    return true;
}

bool VideoRendererOpenGL::receiveFrame(const VideoFrame& frame)
{
    DPTR_D(VideoRendererOpenGL);
    QMutexLocker locker(&d.img_mutex);
    Q_UNUSED(locker);
    d.video_frame = frame;

    d.glv.setCurrentFrame(frame);

    update(); //can not call updateGL() directly because no event and paintGL() will in video thread
    return true;
}

bool VideoRendererOpenGL::needUpdateBackground() const
{
    return true;
}

void VideoRendererOpenGL::drawBackground()
{
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void VideoRendererOpenGL::drawFrame()
{
    DPTR_D(VideoRendererOpenGL);
    QRect roi = realROI();
    d.glv.render(roi);
}

void VideoRendererOpenGL::initializeGL()
{
    DPTR_D(VideoRendererOpenGL);
    makeCurrent();
    d.glv.setOpenGLContext(QOpenGLContext::currentContext());
    //const QByteArray extensions(reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS)));
    bool hasGLSL = QOpenGLShaderProgram::hasOpenGLShaderPrograms();
    qDebug("OpenGL version: %d.%d  hasGLSL: %d", format().majorVersion(), format().minorVersion(), hasGLSL);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.0, 0.0, 0.0, 0.0);
}

void VideoRendererOpenGL::paintGL()
{
    DPTR_D(VideoRendererOpenGL);
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

void VideoRendererOpenGL::resizeGL(int w, int h)
{
    DPTR_D(VideoRendererOpenGL);
    //qDebug("%s @%d %dx%d", __FUNCTION__, __LINE__, d.out_rect.width(), d.out_rect.height());
    d.glv.setViewport(QRect(0, 0, w, h));
    d.glv.setVideoRect(d.out_rect);
}

void VideoRendererOpenGL::resizeEvent(QResizeEvent *e)
{
    DPTR_D(VideoRendererOpenGL);
    d.update_background = true;
    resizeRenderer(e->size());
    d.glv.setVideoRect(d.out_rect);
    QGLWidget::resizeEvent(e); //will call resizeGL(). TODO:will call paintEvent()?
}

//TODO: out_rect not correct when top level changed
void VideoRendererOpenGL::showEvent(QShowEvent *)
{
    DPTR_D(VideoRendererOpenGL);
    d.update_background = true;
    /*
     * Do something that depends on widget below! e.g. recreate render target for direct2d.
     * When Qt::WindowStaysOnTopHint changed, window will hide first then show. If you
     * don't do anything here, the widget content will never be updated.
     */
}

void VideoRendererOpenGL::onSetOutAspectRatio(qreal ratio)
{
    Q_UNUSED(ratio);
    DPTR_D(VideoRendererOpenGL);
    d.glv.setVideoRect(d.out_rect);
}

void VideoRendererOpenGL::onSetOutAspectRatioMode(OutAspectRatioMode mode)
{
    Q_UNUSED(mode);
    DPTR_D(VideoRendererOpenGL);
    d.glv.setVideoRect(d.out_rect);
}

bool VideoRendererOpenGL::onSetBrightness(qreal b)
{
    d_func().glv.setBrightness(b);
    return true;
}

bool VideoRendererOpenGL::onSetContrast(qreal c)
{
    d_func().glv.setContrast(c);
    return true;
}

bool VideoRendererOpenGL::onSetHue(qreal h)
{
    d_func().glv.setHue(h);
    return true;
}

bool VideoRendererOpenGL::onSetSaturation(qreal s)
{
    d_func().glv.setSaturation(s);
    return true;
}

} //namespace QtAV
