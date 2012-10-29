#ifndef QAVGRAPHICSITEMRENDERER_H
#define QAVGRAPHICSITEMRENDERER_H

#include <QtAV/QtAV_Global.h>
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

#endif // QAVGRAPHICSITEMRENDERER_H
