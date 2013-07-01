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

#ifndef QAV_GRAPHICSITEMRENDERER_H
#define QAV_GRAPHICSITEMRENDERER_H

#include <QtAV/QPainterRenderer.h>
#include <QGraphicsWidget>

//QGraphicsWidget will lose focus forever if TextItem inserted text. Why?
#define CONFIG_GRAPHICSWIDGET 0
#if CONFIG_GRAPHICSWIDGET
#define GraphicsWidget QGraphicsWidget
#else
#define GraphicsWidget QGraphicsObject
#endif

namespace QtAV {

class GraphicsItemRendererPrivate;
class Q_EXPORT GraphicsItemRenderer : public GraphicsWidget, public QPainterRenderer
{
    DPTR_DECLARE_PRIVATE(GraphicsItemRenderer)
public:
    GraphicsItemRenderer(QGraphicsItem * parent = 0);
    virtual ~GraphicsItemRenderer();
    virtual VideoRendererId id() const;

    QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

protected:
    GraphicsItemRenderer(GraphicsItemRendererPrivate& d, QGraphicsItem *parent);

    virtual bool write();
    virtual bool needUpdateBackground() const;
    //called in paintEvent before drawFrame() when required
    virtual void drawBackground();
    //draw the current frame using the current paint engine. called by paintEvent()
    virtual void drawFrame();
#if CONFIG_GRAPHICSWIDGET
    virtual bool event(QEvent *event);
#else
    //virtual bool sceneEvent(QEvent *event);
#endif //CONFIG_GRAPHICSWIDGET
};
typedef GraphicsItemRenderer VideoRendererGraphicsItem;
}

#endif // QAV_GRAPHICSITEMRENDERER_H
