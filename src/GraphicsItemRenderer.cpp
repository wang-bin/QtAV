/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012 Wang Bin <wbsecg1@gmail.com>

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

#include <QtAV/GraphicsItemRenderer.h>
#include <private/GraphicsItemRenderer_p.h>
#include <QGraphicsScene>
#include <QtGui/QPainter>

namespace QtAV {

GraphicsItemRenderer::GraphicsItemRenderer(QGraphicsItem * parent)
    :QGraphicsItem(parent),ImageRenderer(*new GraphicsItemRendererPrivate())
{
}

GraphicsItemRenderer::GraphicsItemRenderer(GraphicsItemRendererPrivate &d, QGraphicsItem *parent)
    :QGraphicsItem(parent),ImageRenderer(d)
{
}

GraphicsItemRenderer::~GraphicsItemRenderer()
{
}

int GraphicsItemRenderer::write(const QByteArray &data)
{
    int s = ImageRenderer::write(data);
    scene()->update(sceneBoundingRect());
    //update(); //does not cause an immediate paint. my not redraw.
    return s;
}

QRectF GraphicsItemRenderer::boundingRect() const
{
    return QRectF(0, 0, videoWidth(), videoHeight());
}

void GraphicsItemRenderer::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    DPTR_D(GraphicsItemRenderer);
    if (d.image.isNull())
        painter->drawImage(QPointF(), d.preview);
    else
        painter->drawImage(QPointF(), d.image);
}

} //namespace QtAV
