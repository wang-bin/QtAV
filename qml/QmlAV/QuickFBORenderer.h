/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2017 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2015)

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

#ifndef QTAV_QUICKFBORENDERER_H
#define QTAV_QUICKFBORENDERER_H

#include <QtAV/VideoRenderer.h>
#include <QtQml/QQmlListProperty>
#include <QtQuick/QQuickFramebufferObject>
#include <QmlAV/QuickFilter.h>

namespace QtAV {
class QuickFBORendererPrivate;
class QuickFBORenderer : public QQuickFramebufferObject, public VideoRenderer
{
    Q_OBJECT
    Q_DISABLE_COPY(QuickFBORenderer)
    DPTR_DECLARE_PRIVATE(QuickFBORenderer)
    Q_PROPERTY(bool opengl READ isOpenGL WRITE setOpenGL NOTIFY openGLChanged)
    Q_PROPERTY(QObject* source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(FillMode fillMode READ fillMode WRITE setFillMode NOTIFY fillModeChanged)
    Q_PROPERTY(int orientation READ orientation WRITE setOrientation NOTIFY orientationChanged)
    Q_PROPERTY(QRectF contentRect READ contentRect NOTIFY contentRectChanged)
    Q_PROPERTY(QRectF sourceRect READ sourceRect NOTIFY videoFrameSizeChanged)
    Q_PROPERTY(QRectF regionOfInterest READ regionOfInterest WRITE setRegionOfInterest NOTIFY regionOfInterestChanged)
    Q_PROPERTY(qreal sourceAspectRatio READ sourceAspectRatio NOTIFY sourceAspectRatioChanged)
    Q_PROPERTY(QSize videoFrameSize READ videoFrameSize NOTIFY videoFrameSizeChanged)
    Q_PROPERTY(QSize frameSize READ videoFrameSize NOTIFY videoFrameSizeChanged)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY backgroundColorChanged)
    Q_PROPERTY(QQmlListProperty<QuickVideoFilter> filters READ filters)
    Q_PROPERTY(qreal brightness READ brightness WRITE setBrightness NOTIFY brightnessChanged)
    Q_PROPERTY(qreal contrast READ contrast WRITE setContrast NOTIFY contrastChanged)
    Q_PROPERTY(qreal hue READ hue WRITE setHue NOTIFY hueChanged)
    Q_PROPERTY(qreal saturation READ saturation WRITE setSaturation NOTIFY saturationChanged)
    Q_ENUMS(FillMode)
public:
    enum FillMode {
        Stretch            = Qt::IgnoreAspectRatio,
        PreserveAspectFit  = Qt::KeepAspectRatio,
        PreserveAspectCrop = Qt::KeepAspectRatioByExpanding
    };

    Renderer *createRenderer() const Q_DECL_OVERRIDE;

    explicit QuickFBORenderer(QQuickItem *parent = 0);
    VideoRendererId id() const Q_DECL_OVERRIDE;
    bool isSupported(VideoFormat::PixelFormat pixfmt) const Q_DECL_OVERRIDE;
    OpenGLVideo* opengl() const Q_DECL_OVERRIDE;

    QObject *source() const;
    /*!
     * \brief setSource
     * \param source nullptr, an object of type AVPlayer or QmlAVPlayer
     */
    void setSource(QObject *source);

    FillMode fillMode() const;
    void setFillMode(FillMode mode);

    QRectF contentRect() const;
    QRectF sourceRect() const;

    Q_INVOKABLE QPointF mapPointToItem(const QPointF &point) const;
    Q_INVOKABLE QRectF mapRectToItem(const QRectF &rectangle) const;
    Q_INVOKABLE QPointF mapNormalizedPointToItem(const QPointF &point) const;
    Q_INVOKABLE QRectF mapNormalizedRectToItem(const QRectF &rectangle) const;
    Q_INVOKABLE QPointF mapPointToSource(const QPointF &point) const;
    Q_INVOKABLE QRectF mapRectToSource(const QRectF &rectangle) const;
    Q_INVOKABLE QPointF mapPointToSourceNormalized(const QPointF &point) const;
    Q_INVOKABLE QRectF mapRectToSourceNormalized(const QRectF &rectangle) const;

    bool isOpenGL() const;
    void setOpenGL(bool o);
    void fboSizeChanged(const QSize& size);
    void renderToFbo(QOpenGLFramebufferObject *fbo);

    QQmlListProperty<QuickVideoFilter> filters();
Q_SIGNALS:
    void sourceChanged();
    void fillModeChanged(QuickFBORenderer::FillMode);
    void orientationChanged() Q_DECL_OVERRIDE;
    void contentRectChanged() Q_DECL_OVERRIDE;
    void regionOfInterestChanged() Q_DECL_OVERRIDE;
    void openGLChanged();    
    void sourceAspectRatioChanged(qreal value) Q_DECL_OVERRIDE;
    void videoFrameSizeChanged() Q_DECL_OVERRIDE;
    void backgroundColorChanged() Q_DECL_OVERRIDE;
    void brightnessChanged(qreal value) Q_DECL_OVERRIDE;
    void contrastChanged(qreal) Q_DECL_OVERRIDE;
    void hueChanged(qreal) Q_DECL_OVERRIDE;
    void saturationChanged(qreal) Q_DECL_OVERRIDE;
protected:
    bool event(QEvent *e) Q_DECL_OVERRIDE;
    bool receiveFrame(const VideoFrame &frame) Q_DECL_OVERRIDE;
    void drawBackground() Q_DECL_OVERRIDE;
    void drawFrame() Q_DECL_OVERRIDE;
private:
    bool onSetOrientation(int value) Q_DECL_OVERRIDE;
    void onSetOutAspectRatio(qreal ratio) Q_DECL_OVERRIDE;
    void onSetOutAspectRatioMode(OutAspectRatioMode mode) Q_DECL_OVERRIDE;
    bool onSetBrightness(qreal b) Q_DECL_OVERRIDE;
    bool onSetContrast(qreal c) Q_DECL_OVERRIDE;
    bool onSetHue(qreal h) Q_DECL_OVERRIDE;
    bool onSetSaturation(qreal s) Q_DECL_OVERRIDE;
    void updateRenderRect();

    static void vf_append(QQmlListProperty<QuickVideoFilter> *property, QuickVideoFilter *value);
    static int vf_count(QQmlListProperty<QuickVideoFilter> *property);
    static QuickVideoFilter *vf_at(QQmlListProperty<QuickVideoFilter> *property, int index);
    static void vf_clear(QQmlListProperty<QuickVideoFilter> *property);
};
typedef QuickFBORenderer VideoRendererQuickFBO;
} //namespace QtAV
QML_DECLARE_TYPE(QtAV::QuickFBORenderer)
#endif // QTAV_QUICKFBORENDERER_H
