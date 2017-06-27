/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2017 Wang Bin <wbsecg1@gmail.com>

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

#include "QtAV/OpenGLRendererBase.h"
#include "QtAV/private/OpenGLRendererBase_p.h"
#include "QtAV/OpenGLVideo.h"
#include "QtAV/FilterContext.h"
#include <QResizeEvent>
#include "opengl/OpenGLHelper.h"
#include "utils/Logger.h"

namespace QtAV {

OpenGLRendererBasePrivate::OpenGLRendererBasePrivate(QPaintDevice* pd)
    : painter(new QPainter())
    , frame_changed(false)
{
    filter_context = VideoFilterContext::create(VideoFilterContext::QtPainter);
    filter_context->paint_device = pd;
    filter_context->painter = painter;
}

OpenGLRendererBasePrivate::~OpenGLRendererBasePrivate() {
    if (painter) {
        delete painter;
        painter = 0;
    }
}

void OpenGLRendererBasePrivate::setupAspectRatio()
{
    matrix.setToIdentity();
    matrix.scale((GLfloat)out_rect.width()/(GLfloat)renderer_width, (GLfloat)out_rect.height()/(GLfloat)renderer_height, 1);
    if (rotation())
        matrix.rotate(rotation(), 0, 0, 1); // Z axis
}

OpenGLRendererBase::OpenGLRendererBase(OpenGLRendererBasePrivate &d)
    : VideoRenderer(d)
{
    setPreferredPixelFormat(VideoFormat::Format_YUV420P);
}

OpenGLRendererBase::~OpenGLRendererBase()
{
   d_func().glv.setOpenGLContext(0);
}

bool OpenGLRendererBase::isSupported(VideoFormat::PixelFormat pixfmt) const
{
    return OpenGLVideo::isSupported(pixfmt);
}

OpenGLVideo* OpenGLRendererBase::opengl() const
{
    return const_cast<OpenGLVideo*>(&d_func().glv);
}

bool OpenGLRendererBase::receiveFrame(const VideoFrame& frame)
{
    DPTR_D(OpenGLRendererBase);
    d.video_frame = frame;
    d.frame_changed = true;
    updateUi(); //can not call updateGL() directly because no event and paintGL() will in video thread
    return true;
}

void OpenGLRendererBase::drawBackground()
{
    d_func().glv.fill(backgroundColor());
}

void OpenGLRendererBase::drawFrame()
{
    DPTR_D(OpenGLRendererBase);
    QRect roi = realROI();
    //d.glv.render(QRectF(-1, 1, 2, -2), roi, d.matrix);
    // QRectF() means the whole viewport
    if (d.frame_changed) {
        d.glv.setCurrentFrame(d.video_frame);
        d.frame_changed = false;
    }
    d.glv.render(QRectF(), roi, d.matrix);
}

void OpenGLRendererBase::onInitializeGL()
{
    DPTR_D(OpenGLRendererBase);
    //makeCurrent();
#if QT_VERSION >= QT_VERSION_CHECK(4, 8, 0)
    initializeOpenGLFunctions();
#endif
    QOpenGLContext *ctx = const_cast<QOpenGLContext*>(QOpenGLContext::currentContext()); //qt4 returns const
    d.glv.setOpenGLContext(ctx);
}

void OpenGLRendererBase::onPaintGL()
{
    DPTR_D(OpenGLRendererBase);
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

void OpenGLRendererBase::onResizeGL(int w, int h)
{
    if (!QOpenGLContext::currentContext())
        return;
    DPTR_D(OpenGLRendererBase);
    d.glv.setProjectionMatrixToRect(QRectF(0, 0, w, h));
    d.setupAspectRatio();
}

void OpenGLRendererBase::onResizeEvent(int w, int h)
{
    DPTR_D(OpenGLRendererBase);
    d.update_background = true;
    resizeRenderer(w, h);
    d.setupAspectRatio();
    //QOpenGLWindow::resizeEvent(e); //will call resizeGL(). TODO:will call paintEvent()?
}
//TODO: out_rect not correct when top level changed
void OpenGLRendererBase::onShowEvent()
{
    DPTR_D(OpenGLRendererBase);
    d.update_background = true;
    /*
     * Do something that depends on widget below! e.g. recreate render target for direct2d.
     * When Qt::WindowStaysOnTopHint changed, window will hide first then show. If you
     * don't do anything here, the widget content will never be updated.
     */
}

void OpenGLRendererBase::onSetOutAspectRatio(qreal ratio)
{
    Q_UNUSED(ratio);
    DPTR_D(OpenGLRendererBase);
    d.setupAspectRatio();
}

void OpenGLRendererBase::onSetOutAspectRatioMode(OutAspectRatioMode mode)
{
    Q_UNUSED(mode);
    DPTR_D(OpenGLRendererBase);
    d.setupAspectRatio();
}

bool OpenGLRendererBase::onSetOrientation(int value)
{
    Q_UNUSED(value)
    d_func().setupAspectRatio();
    return true;
}

bool OpenGLRendererBase::onSetBrightness(qreal b)
{
    d_func().glv.setBrightness(b);
    return true;
}

bool OpenGLRendererBase::onSetContrast(qreal c)
{
    d_func().glv.setContrast(c);
    return true;
}

bool OpenGLRendererBase::onSetHue(qreal h)
{
    d_func().glv.setHue(h);
    return true;
}

bool OpenGLRendererBase::onSetSaturation(qreal s)
{
    d_func().glv.setSaturation(s);
    return true;
}

} //namespace QtAV
