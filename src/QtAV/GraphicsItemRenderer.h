/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>

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
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QtWidgets/QGraphicsWidget>
#else
#include <QtGui/QGraphicsWidget>
#endif

//QGraphicsWidget will lose focus forever if TextItem inserted text. Why?
#define CONFIG_GRAPHICSWIDGET 0
#if CONFIG_GRAPHICSWIDGET
#define GraphicsWidget QGraphicsWidget
#else
#define GraphicsWidget QGraphicsObject
#endif

namespace QtAV {

class GraphicsItemRendererPrivate;
class Q_AV_EXPORT GraphicsItemRenderer : public GraphicsWidget, public QPainterRenderer
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(GraphicsItemRenderer)
    Q_PROPERTY(bool opengl READ isOpenGL WRITE setOpenGL NOTIFY openGLChanged)
public:
    GraphicsItemRenderer(QGraphicsItem * parent = 0);
    virtual VideoRendererId id() const;
    virtual bool isSupported(VideoFormat::PixelFormat pixfmt) const;

    QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    virtual QGraphicsItem* graphicsItem() { return this; }

    /*!
     * \brief isOpenGL
     * true: user set to enabling opengl renderering. if viewport is not GLWidget, nothing will be rendered
     * false: otherwise. opengl resources in QtAV (e.g. shader manager) will be released later
     */
    bool isOpenGL() const;
    void setOpenGL(bool o);
signals:
    void openGLChanged();
protected:
    GraphicsItemRenderer(GraphicsItemRendererPrivate& d, QGraphicsItem *parent);

    virtual bool receiveFrame(const VideoFrame& frame);
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

private:
    virtual void onSetOutAspectRatioMode(OutAspectRatioMode mode);
    virtual void onSetOutAspectRatio(qreal ratio);
    virtual bool onSetOrientation(int value);
    virtual bool onSetBrightness(qreal b);
    virtual bool onSetContrast(qreal c);
    virtual bool onSetHue(qreal h);
    virtual bool onSetSaturation(qreal s);
};
typedef GraphicsItemRenderer VideoRendererGraphicsItem;
}

#endif // QAV_GRAPHICSITEMRENDERER_H
