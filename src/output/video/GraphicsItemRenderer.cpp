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

#include "QtAV/GraphicsItemRenderer.h"
#include "QtAV/private/QPainterRenderer_p.h"
#include "QtAV/FilterContext.h"
#include "QtAV/OpenGLVideo.h"
#include <QGraphicsScene>
#include <QtGui/QPainter>
#include <QEvent>
#include <QKeyEvent>
#include <QGraphicsSceneEvent>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QtGui/QSurface>
#endif
#include "utils/Logger.h"

namespace QtAV {

class GraphicsItemRendererPrivate : public QPainterRendererPrivate
{
public:
    GraphicsItemRendererPrivate()
        : opengl(false)
    {}
    virtual ~GraphicsItemRendererPrivate(){}
    void setupAspectRatio() {
        matrix.setToIdentity();
        matrix.scale((GLfloat)out_rect.width()/(GLfloat)renderer_width, (GLfloat)out_rect.height()/(GLfloat)renderer_height, 1);
        if (orientation)
            matrix.rotate(orientation, 0, 0, 1); // Z axis
    }
    // return true if opengl is enabled and context is ready. may called by non-rendering thread
    bool checkGL() {
        if (!opengl) {
            glv.setOpenGLContext(0); // it's for Qt4. may not in rendering thread
            return false;
        }
        if (!glv.openGLContext()) {
            //qWarning("no opengl context! set current");
            // null if not called from renderering thread;
            QOpenGLContext *ctx = const_cast<QOpenGLContext*>(QOpenGLContext::currentContext());
            if (!ctx)
                return false;
            glv.setOpenGLContext(ctx);
        }
        return true;
    }

    bool opengl;
    OpenGLVideo glv;
    QMatrix4x4 matrix;
};

GraphicsItemRenderer::GraphicsItemRenderer(QGraphicsItem * parent)
    :GraphicsWidget(parent),QPainterRenderer(*new GraphicsItemRendererPrivate())
{
    setFlag(ItemIsFocusable); //receive key events
    //setAcceptHoverEvents(true);
#if CONFIG_GRAPHICSWIDGET
    setFocusPolicy(Qt::ClickFocus); //for widget
#endif //CONFIG_GRAPHICSWIDGET
}

GraphicsItemRenderer::GraphicsItemRenderer(GraphicsItemRendererPrivate &d, QGraphicsItem *parent)
    :GraphicsWidget(parent),QPainterRenderer(d)
{
    setFlag(ItemIsFocusable); //receive key events
    //setAcceptHoverEvents(true);
#if CONFIG_GRAPHICSWIDGET
    setFocusPolicy(Qt::ClickFocus); //for widget
#endif //CONFIG_GRAPHICSWIDGET
}

bool GraphicsItemRenderer::isSupported(VideoFormat::PixelFormat pixfmt) const
{
    if (isOpenGL())
        return true;
    return QPainterRenderer::isSupported(pixfmt);
}

bool GraphicsItemRenderer::receiveFrame(const VideoFrame& frame)
{
    DPTR_D(GraphicsItemRenderer);
    if (isOpenGL()) {
        {
            QMutexLocker locker(&d.img_mutex);
            Q_UNUSED(locker);
            d.video_frame = frame;
        }
        if (d.checkGL())
            d.glv.setCurrentFrame(frame);
    } else {
        prepareFrame(frame);
    }
    scene()->update(sceneBoundingRect());
    //update(); //does not cause an immediate paint. my not redraw.
    return true;
}

QRectF GraphicsItemRenderer::boundingRect() const
{
    return QRectF(0, 0, rendererWidth(), rendererHeight());
}

bool GraphicsItemRenderer::isOpenGL() const
{
    return d_func().opengl;
}

void GraphicsItemRenderer::setOpenGL(bool o)
{
    DPTR_D(GraphicsItemRenderer);
    if (d.opengl == o)
        return;
    d.opengl = o;
    emit openGLChanged();
}

void GraphicsItemRenderer::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);
    DPTR_D(GraphicsItemRenderer);
    d.painter = painter;
    QPainterFilterContext *ctx = static_cast<QPainterFilterContext*>(d.filter_context);
    if (ctx) {
        ctx->painter = d.painter;
    } else {
        qWarning("FilterContext not available!");
    }
    handlePaintEvent();
    d.painter = 0; //painter may be not available outside this function
    if (ctx)
        ctx->painter = 0;
}

bool GraphicsItemRenderer::needUpdateBackground() const
{
    DPTR_D(const GraphicsItemRenderer);
    return d.out_rect != boundingRect() || !d.video_frame.isValid();
}

void GraphicsItemRenderer::drawBackground()
{
    DPTR_D(GraphicsItemRenderer);
    if (!d.painter)
        return;
    d.painter->fillRect(boundingRect(), QColor(0, 0, 0));
}

void GraphicsItemRenderer::drawFrame()
{
    DPTR_D(GraphicsItemRenderer);
    if (!d.painter)
        return;
    if (d.checkGL()) {
        d.glv.render(boundingRect(), realROI(), d.matrix*sceneTransform());
        return;
    }
    QPainterRenderer::drawFrame();
}


void GraphicsItemRenderer::onSetOutAspectRatio(qreal ratio)
{
    Q_UNUSED(ratio);
    DPTR_D(GraphicsItemRenderer);
    d.setupAspectRatio();
}

bool GraphicsItemRenderer::onSetOrientation(int value)
{
    Q_UNUSED(value);
    d_func().setupAspectRatio();
    update();
    return true;
}

void GraphicsItemRenderer::onSetOutAspectRatioMode(OutAspectRatioMode mode)
{
    Q_UNUSED(mode);
    DPTR_D(GraphicsItemRenderer);
    d.setupAspectRatio();
}

bool GraphicsItemRenderer::onSetBrightness(qreal b)
{
    if (!isOpenGL())
        return false;
    d_func().glv.setBrightness(b);
    return true;
}

bool GraphicsItemRenderer::onSetContrast(qreal c)
{
    if (!isOpenGL())
        return false;
    d_func().glv.setContrast(c);
    return true;
}

bool GraphicsItemRenderer::onSetHue(qreal h)
{
    if (!isOpenGL())
        return false;
    d_func().glv.setHue(h);
    return true;
}

bool GraphicsItemRenderer::onSetSaturation(qreal s)
{
    if (!isOpenGL())
        return false;
    d_func().glv.setSaturation(s);
    return true;
}
//GraphicsWidget will lose focus forever if focus out. Why?

#if CONFIG_GRAPHICSWIDGET
bool GraphicsItemRenderer::event(QEvent *event)
{
    setFocus(); //WHY: Force focus
    QEvent::Type type = event->type();
    qDebug("GraphicsItemRenderer event type = %d", type);
    if (type == QEvent::KeyPress) {
        qDebug("KeyPress Event. key=%d", static_cast<QKeyEvent*>(event)->key());
    }
    return true;
}
#else
/*simply passes event to QGraphicsWidget::event(). you should not have to
 *reimplement sceneEvent() in a subclass of QGraphicsWidget.
 */
/*
bool GraphicsItemRenderer::sceneEvent(QEvent *event)
{
    QEvent::Type type = event->type();
    qDebug("sceneEvent type = %d", type);
    if (type == QEvent::KeyPress) {
        qDebug("KeyPress Event. key=%d", static_cast<QKeyEvent*>(event)->key());
    }
    return true;
}
*/
#endif //!CONFIG_GRAPHICSWIDGET
} //namespace QtAV
