/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2015-2016 Wang Bin <wbsecg1@gmail.com>

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

#include "QmlAV/QuickFBORenderer.h"
#include "QmlAV/QmlAVPlayer.h"
#include "QtAV/AVPlayer.h"
#include "QtAV/OpenGLVideo.h"
#include "QtAV/private/VideoRenderer_p.h"
#include "QtAV/private/mkid.h"
#include "QtAV/private/factory.h"
#include <QtCore/QCoreApplication>
#include <QtGui/QOpenGLFramebufferObject>
#include <QtQuick/QQuickWindow>
// for dynamicgl. qglfunctions before qt5.3 does not have portable gl functions
#ifdef QT_OPENGL_DYNAMIC
#include <QtGui/QOpenGLFunctions>
#define DYGL(glFunc) QOpenGLContext::currentContext()->functions()->glFunc
#else
#define DYGL(glFunc) glFunc
#endif

namespace QtAV {
static const VideoRendererId VideoRendererId_QuickFBO = mkid::id32base36_4<'Q','F','B','O'>::value;
FACTORY_REGISTER(VideoRenderer, QuickFBO, "QuickFBO")

class FBORenderer : public QQuickFramebufferObject::Renderer
{
public:
    FBORenderer(QuickFBORenderer* item) : m_item(item) {}
    QOpenGLFramebufferObject* createFramebufferObject(const QSize &size) {
        static bool sInfo = true;
        if (sInfo) {
            sInfo = false;
            qDebug("GL_VERSION: %s", DYGL(glGetString(GL_VERSION)));
            qDebug("GL_VENDOR: %s", DYGL(glGetString(GL_VENDOR)));
            qDebug("GL_RENDERER: %s", DYGL(glGetString(GL_RENDERER)));
            qDebug("GL_SHADING_LANGUAGE_VERSION: %s", DYGL(glGetString(GL_SHADING_LANGUAGE_VERSION)));
        }

        m_item->fboSizeChanged(size);
        return QQuickFramebufferObject::Renderer::createFramebufferObject(size);
    }
    void render() {
        Q_ASSERT(m_item);
        m_item->renderToFbo();
    }
    void synchronize(QQuickFramebufferObject *item) {
        m_item = static_cast<QuickFBORenderer*>(item);
    }
private:
    QuickFBORenderer *m_item;
};

class QuickFBORendererPrivate : public VideoRendererPrivate
{
public:
    QuickFBORendererPrivate():
        VideoRendererPrivate()
      , opengl(true)
      , fill_mode(QuickFBORenderer::PreserveAspectFit)
      , node(0)
      , source(0)
      , glctx(0)
    {}
    void setupAspectRatio() { //TODO: call when out_rect, renderer_size, orientation changed
        matrix.setToIdentity();
        matrix.scale((GLfloat)out_rect.width()/(GLfloat)renderer_width, (GLfloat)out_rect.height()/(GLfloat)renderer_height, 1);
        if (orientation)
            matrix.rotate(orientation, 0, 0, 1); // Z axis
        // FIXME: why x/y is mirrored?
        if (orientation%180)
            matrix.scale(-1, 1);
        else
            matrix.scale(1, -1);
    }
    bool opengl;
    QuickFBORenderer::FillMode fill_mode;
    QSGNode *node;
    QObject *source;
    QOpenGLContext *glctx;
    QMatrix4x4 matrix;
    OpenGLVideo glv;
};

QuickFBORenderer::QuickFBORenderer(QQuickItem *parent)
    : QQuickFramebufferObject(parent)
    , VideoRenderer(*new QuickFBORendererPrivate())
{
    setPreferredPixelFormat(VideoFormat::Format_YUV420P);
}

VideoRendererId QuickFBORenderer::id() const
{
    return VideoRendererId_QuickFBO;
}

QQuickFramebufferObject::Renderer* QuickFBORenderer::createRenderer() const
{
    return new FBORenderer((QuickFBORenderer*)this);
}

bool QuickFBORenderer::isSupported(VideoFormat::PixelFormat pixfmt) const
{
    if (pixfmt == VideoFormat::Format_RGB48BE)
        return false;
    if (!isOpenGL())
        return VideoFormat::isRGB(pixfmt);
    return OpenGLVideo::isSupported(pixfmt);
}

bool QuickFBORenderer::receiveFrame(const VideoFrame &frame)
{
    DPTR_D(QuickFBORenderer);
    d.video_frame = frame;
    d.glv.setCurrentFrame(frame);
//    update();  // why update slow? because of calling in a different thread?
    //QMetaObject::invokeMethod(this, "update"); // slower than directly postEvent
    QCoreApplication::postEvent(this, new QEvent(QEvent::User));
    return true;
}

QObject* QuickFBORenderer::source() const
{
    return d_func().source;
}

void QuickFBORenderer::setSource(QObject *source)
{
    DPTR_D(QuickFBORenderer);
    if (d.source == source)
        return;
    d.source = source;
    Q_EMIT sourceChanged();
    if (!source)
        return;
    ((QmlAVPlayer*)source)->player()->addVideoRenderer(this);
}

QuickFBORenderer::FillMode QuickFBORenderer::fillMode() const
{
    return d_func().fill_mode;
}

void QuickFBORenderer::setFillMode(FillMode mode)
{
    DPTR_D(QuickFBORenderer);
    if (d.fill_mode == mode)
        return;
    d_func().fill_mode = mode;
    updateRenderRect();
    Q_EMIT fillModeChanged(mode);
}

QRectF QuickFBORenderer::contentRect() const
{
    return videoRect();
}

QRectF QuickFBORenderer::sourceRect() const
{
    return QRectF(QPointF(), videoFrameSize());
}

bool QuickFBORenderer::isOpenGL() const
{
    return d_func().opengl;
}

void QuickFBORenderer::setOpenGL(bool o)
{
    DPTR_D(QuickFBORenderer);
    if (d.opengl == o)
        return;
    d.opengl = o;
    Q_EMIT openGLChanged();
    if (o)
        setPreferredPixelFormat(VideoFormat::Format_YUV420P);
    else
        setPreferredPixelFormat(VideoFormat::Format_RGB32);
}

void QuickFBORenderer::fboSizeChanged(const QSize &size)
{
    DPTR_D(QuickFBORenderer);
    d.update_background = true;
    resizeRenderer(size);
    d.glv.setProjectionMatrixToRect(QRectF(0, 0, size.width(), size.height()));
    d.setupAspectRatio();
}

void QuickFBORenderer::renderToFbo()
{
    handlePaintEvent();
}

void QuickFBORenderer::drawBackground()
{
    d_func().glv.fill(QColor(Qt::black));
}

void QuickFBORenderer::drawFrame()
{
    DPTR_D(QuickFBORenderer);
    if (d.glctx != QOpenGLContext::currentContext()) {
        d.glctx = QOpenGLContext::currentContext();
        d.glv.setOpenGLContext(d.glctx);
    }
    if (!d.video_frame.isValid()) {
        d.glv.fill(QColor(0, 0, 0, 0));
        return;
    }
    //d.glv.setCurrentFrame(d.video_frame);
    d.glv.render(d.out_rect, normalizedROI(), d.matrix);
}

bool QuickFBORenderer::event(QEvent *e)
{
    if (e->type() != QEvent::User)
        return QQuickFramebufferObject::event(e);
    update();
    return true;
}

bool QuickFBORenderer::onSetOrientation(int value)
{
    Q_UNUSED(value);
    d_func().setupAspectRatio();
    return true;
}

void QuickFBORenderer::onSetOutAspectRatio(qreal ratio)
{
    Q_UNUSED(ratio);
    DPTR_D(QuickFBORenderer);
    d.setupAspectRatio();
}

void QuickFBORenderer::onSetOutAspectRatioMode(OutAspectRatioMode mode)
{
    Q_UNUSED(mode);
    DPTR_D(QuickFBORenderer);
    d.setupAspectRatio();
}

void QuickFBORenderer::updateRenderRect()
{
    DPTR_D(QuickFBORenderer);
    if (d.fill_mode == Stretch) {
        setOutAspectRatioMode(RendererAspectRatio);
    } else {//compute out_rect fits video aspect ratio then compute again if crop
        setOutAspectRatioMode(VideoAspectRatio);
    }
    //update();
    d.setupAspectRatio();
}

} //namespace QtAV
