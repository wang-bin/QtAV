/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

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

#include <QtAVWidgets/global.h>
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
class Q_AVWIDGETS_EXPORT GraphicsItemRenderer : public GraphicsWidget, public QPainterRenderer
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(GraphicsItemRenderer)
    Q_PROPERTY(bool opengl READ isOpenGL WRITE setOpenGL NOTIFY openGLChanged)
    Q_PROPERTY(qreal brightness READ brightness WRITE setBrightness NOTIFY brightnessChanged)
    Q_PROPERTY(qreal contrast READ contrast WRITE setContrast NOTIFY contrastChanged)
    Q_PROPERTY(qreal hue READ hue WRITE setHue NOTIFY hueChanged)
    Q_PROPERTY(qreal saturation READ saturation WRITE setSaturation NOTIFY saturationChanged)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY backgroundColorChanged)
    Q_PROPERTY(QRectF regionOfInterest READ regionOfInterest WRITE setRegionOfInterest NOTIFY regionOfInterestChanged)
    Q_PROPERTY(qreal sourceAspectRatio READ sourceAspectRatio NOTIFY sourceAspectRatioChanged)
    Q_PROPERTY(qreal outAspectRatio READ outAspectRatio WRITE setOutAspectRatio NOTIFY outAspectRatioChanged)
    //fillMode
    // TODO: how to use enums in base class as property or Q_ENUM
    Q_PROPERTY(OutAspectRatioMode outAspectRatioMode READ outAspectRatioMode WRITE setOutAspectRatioMode NOTIFY outAspectRatioModeChanged)
    Q_ENUMS(OutAspectRatioMode)
    Q_PROPERTY(int orientation READ orientation WRITE setOrientation NOTIFY orientationChanged)
    Q_PROPERTY(QRect videoRect READ videoRect NOTIFY videoRectChanged)
    Q_PROPERTY(QSize videoFrameSize READ videoFrameSize NOTIFY videoFrameSizeChanged)
    Q_ENUMS(Quality)
public:
    GraphicsItemRenderer(QGraphicsItem * parent = 0);
    VideoRendererId id() const Q_DECL_OVERRIDE;
    bool isSupported(VideoFormat::PixelFormat pixfmt) const Q_DECL_OVERRIDE;

    QRectF boundingRect() const Q_DECL_OVERRIDE;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) Q_DECL_OVERRIDE;
    QGraphicsItem* graphicsItem() Q_DECL_OVERRIDE { return this; }

    /*!
     * \brief isOpenGL
     * true: user set to enabling opengl renderering. if viewport is not GLWidget, nothing will be rendered
     * false: otherwise. opengl resources in QtAV (e.g. shader manager) will be released later
     */
    bool isOpenGL() const;
    void setOpenGL(bool o);

    OpenGLVideo* opengl() const Q_DECL_OVERRIDE;
Q_SIGNALS:
    void sourceAspectRatioChanged(qreal value) Q_DECL_OVERRIDE Q_DECL_FINAL;
    void regionOfInterestChanged() Q_DECL_OVERRIDE;
    void outAspectRatioChanged() Q_DECL_OVERRIDE;
    void outAspectRatioModeChanged() Q_DECL_OVERRIDE;
    void brightnessChanged(qreal value) Q_DECL_OVERRIDE;
    void contrastChanged(qreal) Q_DECL_OVERRIDE;
    void hueChanged(qreal) Q_DECL_OVERRIDE;
    void saturationChanged(qreal) Q_DECL_OVERRIDE;
    void backgroundColorChanged() Q_DECL_OVERRIDE;
    void orientationChanged() Q_DECL_OVERRIDE;
    void videoRectChanged() Q_DECL_OVERRIDE;
    void videoFrameSizeChanged() Q_DECL_OVERRIDE;
    void openGLChanged();
protected:
    GraphicsItemRenderer(GraphicsItemRendererPrivate& d, QGraphicsItem *parent);

    bool receiveFrame(const VideoFrame& frame) Q_DECL_OVERRIDE;
    void drawBackground() Q_DECL_OVERRIDE;
    //draw the current frame using the current paint engine. called by paintEvent()
    void drawFrame() Q_DECL_OVERRIDE;
#if CONFIG_GRAPHICSWIDGET
    bool event(QEvent *event) Q_DECL_OVERRIDE;
#else
    bool event(QEvent *event) Q_DECL_OVERRIDE;
    //bool sceneEvent(QEvent *event) Q_DECL_OVERRIDE;
#endif //CONFIG_GRAPHICSWIDGET

private:
    void onSetOutAspectRatioMode(OutAspectRatioMode mode) Q_DECL_OVERRIDE;
    void onSetOutAspectRatio(qreal ratio) Q_DECL_OVERRIDE;
    bool onSetOrientation(int value) Q_DECL_OVERRIDE;
    bool onSetBrightness(qreal b) Q_DECL_OVERRIDE;
    bool onSetContrast(qreal c) Q_DECL_OVERRIDE;
    bool onSetHue(qreal h) Q_DECL_OVERRIDE;
    bool onSetSaturation(qreal s) Q_DECL_OVERRIDE;
};
typedef GraphicsItemRenderer VideoRendererGraphicsItem;
}

#endif // QAV_GRAPHICSITEMRENDERER_H
