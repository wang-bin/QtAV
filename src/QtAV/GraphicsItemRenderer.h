/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2013 Wang Bin <wbsecg1@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#ifndef QAV_GRAPHICSITEMRENDERER_H
#define QAV_GRAPHICSITEMRENDERER_H

#include <QtAV/ImageRenderer.h>
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
class Q_EXPORT GraphicsItemRenderer : public GraphicsWidget, public ImageRenderer
{
    DPTR_DECLARE_PRIVATE(GraphicsItemRenderer)
public:
    GraphicsItemRenderer(QGraphicsItem * parent = 0);
    virtual ~GraphicsItemRenderer();

	virtual bool write();
    QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

protected:
    GraphicsItemRenderer(GraphicsItemRendererPrivate& d, QGraphicsItem *parent);

#if CONFIG_GRAPHICSWIDGET
    virtual bool event(QEvent *event);
#else
    //virtual bool sceneEvent(QEvent *event);
#endif //CONFIG_GRAPHICSWIDGET
};
}

#endif // QAV_GRAPHICSITEMRENDERER_H
