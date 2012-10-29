#include <QtAV/GraphicsItemRenderer.h>
#include <QtGui/QPainter>

namespace QtAV {
GraphicsItemRenderer::GraphicsItemRenderer(QGraphicsItem * parent)
    :QGraphicsItem(parent),ImageRenderer()
{
}

GraphicsItemRenderer::~GraphicsItemRenderer()
{

}

int GraphicsItemRenderer::write(const QByteArray &data)
{
    int s = ImageRenderer::write(data);
    update();
    return s;
}

QRectF GraphicsItemRenderer::boundingRect() const
{
    return QRectF(0, 0, videoWidth(), videoHeight());
}

void GraphicsItemRenderer::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->drawImage(QPointF(), image);
}

}
