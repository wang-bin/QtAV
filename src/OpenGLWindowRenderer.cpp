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

#include "QtAV/OpenGLWindowRenderer.h"
#include "QtAV/private/VideoRenderer_p.h"
#include "QtAV/OpenGLVideo.h"
#include "QtAV/FilterContext.h"
#include <QResizeEvent>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLFunctions>

namespace QtAV {

class OpenGLWindowRendererPrivate : public VideoRendererPrivate
{
public:
    OpenGLWindowRendererPrivate(QPaintDevice* pd)
        : painter(new QPainter())
    {
        filter_context = VideoFilterContext::create(VideoFilterContext::QtPainter);
        filter_context->paint_device = pd;
        filter_context->painter = painter;
    }
    virtual ~OpenGLWindowRendererPrivate() {
        if (painter) {
            delete painter;
            painter = 0;
        }
    }
    void setupAspectRatio() {
        matrix(0, 0) = (GLfloat)out_rect.width()/(GLfloat)renderer_width;
        matrix(1, 1) = (GLfloat)out_rect.height()/(GLfloat)renderer_height;
    }

    QPainter *painter;
    OpenGLVideo glv;
    QMatrix4x4 matrix;
};


OpenGLWindowRenderer::OpenGLWindowRenderer(UpdateBehavior updateBehavior, QWindow *parent):
    QOpenGLWindow(updateBehavior, parent)
  , VideoRenderer(*new OpenGLWindowRendererPrivate(this))
{
    setPreferredPixelFormat(VideoFormat::Format_YUV420P);
}

OpenGLWindowRenderer::~OpenGLWindowRenderer()
{
   d_func().glv.setOpenGLContext(0);
}

bool OpenGLWindowRenderer::isSupported(VideoFormat::PixelFormat pixfmt) const
{
    Q_UNUSED(pixfmt);
    return true;
}

bool OpenGLWindowRenderer::receiveFrame(const VideoFrame& frame)
{
    DPTR_D(OpenGLWindowRenderer);
    QMutexLocker locker(&d.img_mutex);
    Q_UNUSED(locker);
    d.video_frame = frame;

    d.glv.setCurrentFrame(frame);

    update(); //can not call updateGL() directly because no event and paintGL() will in video thread
    return true;
}

bool OpenGLWindowRenderer::needUpdateBackground() const
{
    return true;
}

void OpenGLWindowRenderer::drawBackground()
{
    QOpenGLFunctions *glf = context()->functions();
    glf->glClearColor(0, 0, 0, 0);
    glf->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLWindowRenderer::drawFrame()
{
    DPTR_D(OpenGLWindowRenderer);
    QRect roi = realROI();
    //d.glv.render(QRectF(-1, 1, 2, -2), roi, d.matrix);
    // QRectF() means the whole viewport
    d.glv.render(QRectF(), roi, d.matrix);
}

void OpenGLWindowRenderer::initializeGL()
{
    DPTR_D(OpenGLWindowRenderer);
    makeCurrent();
    d.glv.setOpenGLContext(context());
    //const QByteArray extensions(reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS)));
    bool hasGLSL = QOpenGLShaderProgram::hasOpenGLShaderPrograms();
    qDebug("OpenGL version: %d.%d  hasGLSL: %d", format().majorVersion(), format().minorVersion(), hasGLSL);
    QOpenGLFunctions *glf = context()->functions();
    glf->glEnable(GL_TEXTURE_2D);
    glf->glDisable(GL_DEPTH_TEST);
    glf->glClearColor(0.0, 0.0, 0.0, 0.0);
}

void OpenGLWindowRenderer::paintGL()
{
    DPTR_D(OpenGLWindowRenderer);
    /* we can mix gl and qpainter.
     * QPainter painter(this);
     * painter.beginNativePainting();
     * gl functions...
     * painter.endNativePainting();
     * swapBuffers();
     */
    handlePaintEvent();
    //context()->swapBuffers(this);
    if (d.painter && d.painter->isActive())
        d.painter->end();
}

void OpenGLWindowRenderer::resizeGL(int w, int h)
{
    if (!context())
        return;
    QOpenGLFunctions *glf = context()->functions();
    if (!glf)
        return;
    DPTR_D(OpenGLWindowRenderer);
    glf->glViewport(0, 0, w, h);
    d.glv.setProjectionMatrixToRect(QRectF(0, 0, w, h));
    d.setupAspectRatio();
}

void OpenGLWindowRenderer::resizeEvent(QResizeEvent *e)
{
    DPTR_D(OpenGLWindowRenderer);
    d.update_background = true;
    resizeRenderer(e->size());
    d.setupAspectRatio();
    QOpenGLWindow::resizeEvent(e); //will call resizeGL(). TODO:will call paintEvent()?
}

void OpenGLWindowRenderer::onSetOutAspectRatio(qreal ratio)
{
    Q_UNUSED(ratio);
    DPTR_D(OpenGLWindowRenderer);
    d.setupAspectRatio();
}

void OpenGLWindowRenderer::onSetOutAspectRatioMode(OutAspectRatioMode mode)
{
    Q_UNUSED(mode);
    DPTR_D(OpenGLWindowRenderer);
    d.setupAspectRatio();
}

bool OpenGLWindowRenderer::onSetBrightness(qreal b)
{
    d_func().glv.setBrightness(b);
    return true;
}

bool OpenGLWindowRenderer::onSetContrast(qreal c)
{
    d_func().glv.setContrast(c);
    return true;
}

bool OpenGLWindowRenderer::onSetHue(qreal h)
{
    d_func().glv.setHue(h);
    return true;
}

bool OpenGLWindowRenderer::onSetSaturation(qreal s)
{
    d_func().glv.setSaturation(s);
    return true;
}

} //namespace QtAV
