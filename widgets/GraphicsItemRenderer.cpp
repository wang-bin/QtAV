/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2017 Wang Bin <wbsecg1@gmail.com>

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

#include "QtAVWidgets/GraphicsItemRenderer.h"
#include "QtAV/private/QPainterRenderer_p.h"
#include "QtAV/FilterContext.h"
#if !defined QT_NO_OPENGL && (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0) || defined(QT_OPENGL_LIB))
#define QTAV_HAVE_OPENGL 1
#else
#define QTAV_HAVE_OPENGL 0
#endif
#if QTAV_HAVE(OPENGL)
#include "QtAV/OpenGLVideo.h"
#else
typedef float GLfloat;
#endif
#include <QMatrix4x4>
#include <QGraphicsScene>
#include <QtGui/QPainter>
#include <QEvent>
#include <QKeyEvent>
#include <QGraphicsSceneEvent>
#include <QtCore/QCoreApplication>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QtGui/QSurface>
#endif

namespace QtAV {

class GraphicsItemRendererPrivate : public QPainterRendererPrivate
{
public:
    GraphicsItemRendererPrivate()
        : frame_changed(false)
        , opengl(false)
    {}
    virtual ~GraphicsItemRendererPrivate(){}
    void setupAspectRatio() {
        matrix.setToIdentity();
        matrix.scale((GLfloat)out_rect.width()/(GLfloat)renderer_width, (GLfloat)out_rect.height()/(GLfloat)renderer_height, 1);
        if (rotation())
            matrix.rotate(rotation(), 0, 0, 1); // Z axis
    }
    // return true if opengl is enabled and context is ready. may called by non-rendering thread
    bool checkGL() {
#if QTAV_HAVE(OPENGL)
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
#endif
        return false;
    }

    bool frame_changed;
    bool opengl;
#if QTAV_HAVE(OPENGL)
    OpenGLVideo glv;
#endif
    QMatrix4x4 matrix;
};

VideoRendererId GraphicsItemRenderer::id() const
{
    return VideoRendererId_GraphicsItem;
}

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
#if QTAV_HAVE(OPENGL)
    DPTR_D(GraphicsItemRenderer);
    if (isOpenGL()) {
        d.video_frame = frame;
        d.frame_changed = true;
    } else
#endif
    {
        preparePixmap(frame);
    }
	// moved to event
    // scene()->update(sceneBoundingRect()); //TODO: thread?
    QCoreApplication::postEvent(this, new QEvent(QEvent::User));
    //update(); //does not cause an immediate paint. my not redraw.
    return true;
}

QRectF GraphicsItemRenderer::boundingRect() const
{
    return QRectF(0, 0, rendererWidth(), rendererHeight());
}

bool GraphicsItemRenderer::isOpenGL() const
{
#if QTAV_HAVE(OPENGL)
    return d_func().opengl;
#endif
    return false;
}

void GraphicsItemRenderer::setOpenGL(bool o)
{
    DPTR_D(GraphicsItemRenderer);
    if (d.opengl == o)
        return;
    d.opengl = o;
    Q_EMIT openGLChanged();
}

OpenGLVideo* GraphicsItemRenderer::opengl() const
{
#if QTAV_HAVE(OPENGL)
    return const_cast<OpenGLVideo*>(&d_func().glv);
#endif
    return NULL;
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
    // save painter state, switch to native opengl painting
	painter->save();
    painter->beginNativePainting();
	
    handlePaintEvent();
	
	// end native painting, restore state
    painter->endNativePainting();
    painter->restore();
	
    d.painter = 0; //painter may be not available outside this function
    if (ctx)
        ctx->painter = 0;
}

void GraphicsItemRenderer::drawBackground()
{
    DPTR_D(GraphicsItemRenderer);
#if QTAV_HAVE(OPENGL)
    if (d.checkGL()) {
       // d.glv.fill(QColor(0, 0, 0)); //FIXME: fill boundingRect
        return;
    } else
#endif
    {
        QPainterRenderer::drawBackground();
    }
}

void GraphicsItemRenderer::drawFrame()
{
    DPTR_D(GraphicsItemRenderer);
    if (!d.painter)
        return;
#if QTAV_HAVE(OPENGL)
    if (d.checkGL()) {
        if (d.frame_changed) {
            d.glv.setCurrentFrame(d.video_frame);
            d.frame_changed = false;
        }
        d.glv.render(boundingRect(), realROI(), d.matrix*sceneTransform());
        return;
    }
#endif
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
    update(); //TODO: thread?
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
    Q_UNUSED(b);
#if QTAV_HAVE(OPENGL)
    d_func().glv.setBrightness(b);
    update();
    return true;
#endif
    return false;
}

bool GraphicsItemRenderer::onSetContrast(qreal c)
{
    if (!isOpenGL())
        return false;
    Q_UNUSED(c);
#if QTAV_HAVE(OPENGL)
    d_func().glv.setContrast(c);
    update();
    return true;
#endif
    return false;
}

bool GraphicsItemRenderer::onSetHue(qreal h)
{
    if (!isOpenGL())
        return false;
    Q_UNUSED(h);
#if QTAV_HAVE(OPENGL)
    d_func().glv.setHue(h);
    update();
    return true;
#endif
    return false;
}

bool GraphicsItemRenderer::onSetSaturation(qreal s)
{
    if (!isOpenGL())
        return false;
    Q_UNUSED(s);
#if QTAV_HAVE(OPENGL)
    d_func().glv.setSaturation(s);
    update();
    return true;
#endif
    return false;
}
//GraphicsWidget will lose focus forever if focus out. Why?

#if CONFIG_GRAPHICSWIDGET
bool GraphicsItemRenderer::event(QEvent *event)
{
    if (e->type() == QEvent::User) {
		scene()->update(sceneBoundingRect());
    }
	else {
        setFocus(); //WHY: Force focus
        QEvent::Type type = event->type();
        qDebug("GraphicsItemRenderer event type = %d", type);
        if (type == QEvent::KeyPress) {
            qDebug("KeyPress Event. key=%d", static_cast<QKeyEvent*>(event)->key());
        }
	}	
    return true;
}
#else
bool GraphicsItemRenderer::event(QEvent *event)
{
    if (event->type() != QEvent::User)
        return GraphicsWidget::event(event);
	scene()->update(sceneBoundingRect());
    return true;
}
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
