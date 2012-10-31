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

#ifndef QAV_GRAPHICSITEMRENDERER_H
#define QAV_GRAPHICSITEMRENDERER_H

#include <QtAV/ImageRenderer.h>
#include <QGraphicsItem>

namespace QtAV {
class Q_EXPORT GraphicsItemRenderer : public QGraphicsItem, public ImageRenderer
{
public:
    GraphicsItemRenderer(QGraphicsItem * parent = 0);
    virtual ~GraphicsItemRenderer();
    virtual int write(const QByteArray &data);

    QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
};
}

#endif // QAV_GRAPHICSITEMRENDERER_H
