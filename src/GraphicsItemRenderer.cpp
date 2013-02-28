/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2013 Wang Bin <wbsecg1@gmail.com>

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

#include <QtAV/GraphicsItemRenderer.h>
#include <private/GraphicsItemRenderer_p.h>
#include <QGraphicsScene>
#include <QtGui/QPainter>
#include <QEvent>
#include <QKeyEvent>
#include <QGraphicsSceneEvent>

namespace QtAV {

GraphicsItemRenderer::GraphicsItemRenderer(QGraphicsItem * parent)
    :GraphicsWidget(parent),ImageRenderer(*new GraphicsItemRendererPrivate())
{
    setFlag(ItemIsFocusable); //receive key events
    //setAcceptHoverEvents(true);
#if CONFIG_GRAPHICSWIDGET
    setFocusPolicy(Qt::ClickFocus); //for widget
#endif //CONFIG_GRAPHICSWIDGET
}

GraphicsItemRenderer::GraphicsItemRenderer(GraphicsItemRendererPrivate &d, QGraphicsItem *parent)
    :GraphicsWidget(parent),ImageRenderer(d)
{
    setFlag(ItemIsFocusable); //receive key events
    //setAcceptHoverEvents(true);
#if CONFIG_GRAPHICSWIDGET
    setFocusPolicy(Qt::ClickFocus); //for widget
#endif //CONFIG_GRAPHICSWIDGET
}

GraphicsItemRenderer::~GraphicsItemRenderer()
{
}

bool GraphicsItemRenderer::write()
{
    scene()->update(sceneBoundingRect());
    //update(); //does not cause an immediate paint. my not redraw.
	return true;
}

QRectF GraphicsItemRenderer::boundingRect() const
{
    return QRectF(0, 0, rendererWidth(), rendererHeight());
}

void GraphicsItemRenderer::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);
	DPTR_D(GraphicsItemRenderer);
    if (!d.scale_in_qt) {
        d.img_mutex.lock();
    }
    painter->fillRect(boundingRect(), QColor(0, 0, 0));
    if (d.image.isNull()) {
        //TODO: when setInSize()?
        d.image = QImage(rendererSize(), QImage::Format_RGB32);
        d.image.fill(Qt::black); //maemo 4.7.0: QImage.fill(uint)
    }
    if (d.image.size() == QSize(d.renderer_width, d.renderer_height))
        painter->drawImage(d.out_rect.topLeft(), d.image);
    else
        painter->drawImage(d.out_rect, d.image);
    if (!d.scale_in_qt) {
        d.img_mutex.unlock();
    }
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
