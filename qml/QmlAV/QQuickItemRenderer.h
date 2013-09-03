#ifndef QQUICKRENDERER_H
#define QQUICKRENDERER_H

#include <QmlAV/Export.h>
#include <QtAV/VideoRenderer.h>
#include <QtQuick/QQuickItem>

namespace QtAV
{
extern QMLAV_EXPORT VideoRendererId VideoRendererId_QQuickItem;

class QQuickItemRendererPrivate;
class QMLAV_EXPORT QQuickItemRenderer : public QQuickItem, public VideoRenderer
{
    DPTR_DECLARE_PRIVATE(QQuickItemRenderer)
public:
    explicit QQuickItemRenderer(QQuickItem *parent = 0);
    ~QQuickItemRenderer() {}
    virtual VideoRendererId id() const;
    
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
