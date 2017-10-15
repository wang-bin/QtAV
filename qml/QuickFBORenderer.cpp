/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2017 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2015)

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
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QOpenGLFramebufferObject>
#include <QtQuick/QQuickWindow>
// for dynamicgl. qglfunctions before qt5.3 does not have portable gl functions
// use qt gl func if possible to avoid linking to opengl directly
#if QT_VERSION >= QT_VERSION_CHECK(5, 3, 0)
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
    QOpenGLFramebufferObject* createFramebufferObject(const QSize &size) Q_DECL_OVERRIDE {
        m_item->fboSizeChanged(size);
        return QQuickFramebufferObject::Renderer::createFramebufferObject(size);
    }
    void render() Q_DECL_OVERRIDE {
        Q_ASSERT(m_item);
        m_item->renderToFbo(framebufferObject());
    }
    void synchronize(QQuickFramebufferObject *item) Q_DECL_OVERRIDE {
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
      , frame_changed(false)
      , opengl(true)
      , fill_mode(QuickFBORenderer::PreserveAspectFit)
      , node(0)
      , source(0)
      , glctx(0)
    {}
    void setupAspectRatio() { //TODO: call when out_rect, renderer_size, orientation changed
        matrix.setToIdentity();
        matrix.scale((GLfloat)out_rect.width()/(GLfloat)renderer_width, (GLfloat)out_rect.height()/(GLfloat)renderer_height, 1);
        if (rotation())
            matrix.rotate(rotation(), 0, 0, 1); // Z axis
        // FIXME: why x/y is mirrored?
        if (rotation()%180)
            matrix.scale(-1, 1);
        else
            matrix.scale(1, -1);
    }
    bool frame_changed;
    bool opengl;
    QuickFBORenderer::FillMode fill_mode;
    QSGNode *node;
    QObject *source;
    QOpenGLContext *glctx;
    QMatrix4x4 matrix;
    OpenGLVideo glv;

    QOpenGLFramebufferObject *fbo;
    QList<QuickVideoFilter*> filters;
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
    if (pixfmt == VideoFormat::Format_RGB48BE
            || pixfmt == VideoFormat::Format_Invalid)
        return false;
    if (!isOpenGL())
        return VideoFormat::isRGB(pixfmt);
    return OpenGLVideo::isSupported(pixfmt);
}

OpenGLVideo* QuickFBORenderer::opengl() const
{
    return const_cast<OpenGLVideo*>(&d_func().glv);
}

bool QuickFBORenderer::receiveFrame(const VideoFrame &frame)
{
    DPTR_D(QuickFBORenderer);
    d.video_frame = frame;
    d.frame_changed = true;
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

    AVPlayer* p0 = 0;
    p0 = qobject_cast<AVPlayer*>(d.source);
    if (!p0) {
        QmlAVPlayer* qp0 = qobject_cast<QmlAVPlayer*>(d.source);
        if (qp0)
            p0 = qp0->player();
    }
    if(p0)
        p0->removeVideoRenderer(this);

    d.source = source;
    Q_EMIT sourceChanged();
    if (!source)
        return;
    AVPlayer* p = qobject_cast<AVPlayer*>(source);
    if (!p) {
        QmlAVPlayer* qp = qobject_cast<QmlAVPlayer*>(source);
        if (!qp) {
            qWarning("source MUST be of type AVPlayer or QmlAVPlayer");
            return;
        }
        p = qp->player();
    }
    p->addVideoRenderer(this);
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

QPointF QuickFBORenderer::mapPointToItem(const QPointF &point) const
{
    if (videoFrameSize().isEmpty())
        return QPointF();

    // Just normalize and use that function
    // m_nativeSize is transposed in some orientations
    if (d_func().rotation()%180 == 0)
        return mapNormalizedPointToItem(QPointF(point.x() / videoFrameSize().width(), point.y() / videoFrameSize().height()));
    else
        return mapNormalizedPointToItem(QPointF(point.x() / videoFrameSize().height(), point.y() / videoFrameSize().width()));
}

QRectF QuickFBORenderer::mapRectToItem(const QRectF &rectangle) const
{
    return QRectF(mapPointToItem(rectangle.topLeft()),
                  mapPointToItem(rectangle.bottomRight())).normalized();
}

QPointF QuickFBORenderer::mapNormalizedPointToItem(const QPointF &point) const
{
    qreal dx = point.x();
    qreal dy = point.y();
    if (d_func().rotation()%180 == 0) {
        dx *= contentRect().width();
        dy *= contentRect().height();
    } else {
        dx *= contentRect().height();
        dy *= contentRect().width();
    }

    switch (d_func().rotation()) {
        case 0:
        default:
            return contentRect().topLeft() + QPointF(dx, dy);
        case 90:
            return contentRect().bottomLeft() + QPointF(dy, -dx);
        case 180:
            return contentRect().bottomRight() + QPointF(-dx, -dy);
        case 270:
            return contentRect().topRight() + QPointF(-dy, dx);
    }
}

QRectF QuickFBORenderer::mapNormalizedRectToItem(const QRectF &rectangle) const
{
    return QRectF(mapNormalizedPointToItem(rectangle.topLeft()),
                  mapNormalizedPointToItem(rectangle.bottomRight())).normalized();
}

QPointF QuickFBORenderer::mapPointToSource(const QPointF &point) const
{
    QPointF norm = mapPointToSourceNormalized(point);
    if (d_func().rotation()%180 == 0)
        return QPointF(norm.x() * videoFrameSize().width(), norm.y() * videoFrameSize().height());
    else
        return QPointF(norm.x() * videoFrameSize().height(), norm.y() * videoFrameSize().width());
}

QRectF QuickFBORenderer::mapRectToSource(const QRectF &rectangle) const
{
    return QRectF(mapPointToSource(rectangle.topLeft()),
                  mapPointToSource(rectangle.bottomRight())).normalized();
}

QPointF QuickFBORenderer::mapPointToSourceNormalized(const QPointF &point) const
{
    if (contentRect().isEmpty())
        return QPointF();

    // Normalize the item source point
    qreal nx = (point.x() - contentRect().x()) / contentRect().width();
    qreal ny = (point.y() - contentRect().y()) / contentRect().height();
    switch (d_func().rotation()) {
        case 0:
        default:
            return QPointF(nx, ny);
        case 90:
            return QPointF(1.0 - ny, nx);
        case 180:
            return QPointF(1.0 - nx, 1.0 - ny);
        case 270:
            return QPointF(ny, 1.0 - nx);
    }
}

QRectF QuickFBORenderer::mapRectToSourceNormalized(const QRectF &rectangle) const
{
    return QRectF(mapPointToSourceNormalized(rectangle.topLeft()),
                  mapPointToSourceNormalized(rectangle.bottomRight())).normalized();
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
    if (d.glctx != QOpenGLContext::currentContext()) {
        d.glctx = QOpenGLContext::currentContext();
        d.glv.setOpenGLContext(d.glctx); // will set viewport. but maybe wrong value for hi dpi
    }
    // ensure viewport is correct set
    d.glv.setProjectionMatrixToRect(QRectF(0, 0, size.width(), size.height()));
    d.setupAspectRatio();
}

void QuickFBORenderer::renderToFbo(QOpenGLFramebufferObject *fbo)
{
    d_func().fbo = fbo;
    handlePaintEvent();
}

QQmlListProperty<QuickVideoFilter> QuickFBORenderer::filters()
{
    return QQmlListProperty<QuickVideoFilter>(this, NULL, vf_append, vf_count, vf_at, vf_clear);
}

void QuickFBORenderer::vf_append(QQmlListProperty<QuickVideoFilter> *property, QuickVideoFilter *value)
{
    QuickFBORenderer* self = static_cast<QuickFBORenderer*>(property->object);
    self->d_func().filters.append(value);
    self->installFilter(value);
}

int QuickFBORenderer::vf_count(QQmlListProperty<QuickVideoFilter> *property)
{
    QuickFBORenderer* self = static_cast<QuickFBORenderer*>(property->object);
    return self->d_func().filters.size();
}

QuickVideoFilter* QuickFBORenderer::vf_at(QQmlListProperty<QuickVideoFilter> *property, int index)
{
    QuickFBORenderer* self = static_cast<QuickFBORenderer*>(property->object);
    return self->d_func().filters.at(index);
}

void QuickFBORenderer::vf_clear(QQmlListProperty<QuickVideoFilter> *property)
{
    QuickFBORenderer* self = static_cast<QuickFBORenderer*>(property->object);
    foreach (QuickVideoFilter *f, self->d_func().filters) {
        self->uninstallFilter(f);
    }
    self->d_func().filters.clear();
}

void QuickFBORenderer::drawBackground()
{
    if (backgroundRegion().isEmpty())
        return;
    DPTR_D(QuickFBORenderer);
    d.fbo->bind();
    DYGL(glViewport(0, 0, d.fbo->width(), d.fbo->height()));
    d.glv.fill(backgroundColor());
}

void QuickFBORenderer::drawFrame()
{
    DPTR_D(QuickFBORenderer);
    d.fbo->bind();
    DYGL(glViewport(0, 0, d.fbo->width(), d.fbo->height()));
    if (!d.video_frame.isValid()) {
        d.glv.fill(QColor(0, 0, 0, 0));
        return;
    }
    if (d.frame_changed) {
        d.glv.setCurrentFrame(d.video_frame);
        d.frame_changed = false;
    }
    d.glv.render(QRectF(), realROI(), d.matrix);
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

bool QuickFBORenderer::onSetBrightness(qreal b)
{
    d_func().glv.setBrightness(b);
    return true;
}

bool QuickFBORenderer::onSetContrast(qreal c)
{
    d_func().glv.setContrast(c);
    return true;
}

bool QuickFBORenderer::onSetHue(qreal h)
{
    d_func().glv.setHue(h);
    return true;
}

bool QuickFBORenderer::onSetSaturation(qreal s)
{
    d_func().glv.setSaturation(s);
    return true;
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
