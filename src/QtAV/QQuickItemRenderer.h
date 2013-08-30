#ifndef QQUICKRENDERER_H
#define QQUICKRENDERER_H

#include "QtAV/VideoRenderer.h"
#include <QtQuick/QQuickItem>

#include <QtAV/private/QQuickItemRenderer_p.h>

namespace QtAV
{

class QQuickItemRendererPrivate;
class Q_EXPORT QQuickItemRenderer : public QQuickItem, public VideoRenderer
{
    DPTR_DECLARE_PRIVATE(QQuickItemRenderer)
public:
    explicit QQuickItemRenderer(QQuickItem *parent = 0);
    ~QQuickItemRenderer() {}
    virtual VideoRendererId id () const;
    
signals:
    
public slots:
    

protected:
    virtual void drawFrame();

    // QQuickItem interface
protected:
    virtual QSGNode *updatePaintNode(QSGNode *node, UpdatePaintNodeData *data);

    // AVOutput interface
protected:
    virtual void convertData(const QByteArray &data);
    virtual bool write();

private:

};
typedef QQuickItemRenderer VideoRendererQQuickItem;
}

#endif // QQUICKRENDERER_H
