#ifndef QAVWIDGETRENDERER_H
#define QAVWIDGETRENDERER_H

#include <QtAV/ImageRenderer.h>
#include <qwidget.h>

namespace QtAV {
class Q_EXPORT WidgetRenderer : public QWidget, public ImageRenderer
{
    Q_OBJECT
public:
    enum GestureAction { GestureMove, GestureResize};
    explicit WidgetRenderer(QWidget *parent = 0);
    virtual ~WidgetRenderer();
    virtual int write(const QByteArray &data);
    void setPreview(const QImage& preivew);

signals:
    void sizeChanged(const QSize& size);

protected:
    virtual void resizeEvent(QResizeEvent *);
    virtual void mousePressEvent(QMouseEvent *);
    virtual void mouseMoveEvent(QMouseEvent *);
    virtual void mouseDoubleClickEvent(QMouseEvent *);
#if !CONFIG_EZX
    virtual void paintEvent(QPaintEvent *);
#endif
    
private:
    QPoint pos, gPos;
    GestureAction action;
};
}

#endif // QAVWIDGETRENDERER_H
