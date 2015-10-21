/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2014-2015 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_VIDEOOUTPUT_H
#define QTAV_VIDEOOUTPUT_H

#include <QtCore/QObject>
#include <QtAV/VideoRenderer.h>

namespace QtAV {

class VideoOutputPrivate;
class Q_AV_EXPORT VideoOutput : public QObject, public VideoRenderer
{
    DPTR_DECLARE_PRIVATE(VideoOutput)
    Q_OBJECT
    Q_PROPERTY(qreal brightness READ brightness WRITE setBrightness NOTIFY brightnessChanged)
    Q_PROPERTY(qreal contrast READ contrast WRITE setContrast NOTIFY contrastChanged)
    Q_PROPERTY(qreal hue READ hue WRITE setHue NOTIFY hueChanged)
    Q_PROPERTY(qreal saturation READ saturation WRITE setSaturation NOTIFY saturationChanged)
    Q_PROPERTY(QRectF regionOfInterest READ regionOfInterest WRITE setRegionOfInterest NOTIFY regionOfInterestChanged)
    Q_PROPERTY(qreal sourceAspectRatio READ sourceAspectRatio NOTIFY sourceAspectRatioChanged)
    Q_PROPERTY(qreal outAspectRatio READ outAspectRatio WRITE setOutAspectRatio NOTIFY outAspectRatioChanged)
    //fillMode
    // TODO: how to use enums in base class as property or Q_ENUM
    Q_PROPERTY(OutAspectRatioMode outAspectRatioMode READ outAspectRatioMode WRITE setOutAspectRatioMode NOTIFY outAspectRatioModeChanged)
    Q_ENUMS(OutAspectRatioMode)
    Q_PROPERTY(int orientation READ orientation WRITE setOrientation NOTIFY orientationChanged)
    Q_PROPERTY(QSize frameSize READ frameSize NOTIFY frameSizeChanged)
    Q_ENUMS(Quality)
public:
    /*!
     * \brief VideoOutput
     * Create a QWidget based renderer. Result can be a QOpenGLWidget or QGLWidget based renderer if possible. Otherwise fallback to a software renderer
     */
    VideoOutput(QObject *parent = 0);
    /*!
     * \brief VideoOutput
     * Create a renderer with given rendererId. MUST check VideoOutput::isAvailable() later!
     */
    VideoOutput(VideoRendererId rendererId, QObject *parent = 0);
    ~VideoOutput();
    VideoRendererId id() const Q_DECL_OVERRIDE;

    VideoFormat::PixelFormat preferredPixelFormat() const Q_DECL_OVERRIDE;
    bool isSupported(VideoFormat::PixelFormat pixfmt) const Q_DECL_OVERRIDE;
    QWindow* qwindow() Q_DECL_OVERRIDE Q_DECL_FINAL;
    QWidget* widget() Q_DECL_OVERRIDE Q_DECL_FINAL;
    QGraphicsItem* graphicsItem() Q_DECL_OVERRIDE Q_DECL_FINAL;

Q_SIGNALS:
    void sourceAspectRatioChanged(qreal value) Q_DECL_OVERRIDE Q_DECL_FINAL;
    void regionOfInterestChanged(const QRectF&);
    void outAspectRatioChanged(qreal);
    void outAspectRatioModeChanged(OutAspectRatioMode);
    void brightnessChanged(qreal value);
    void contrastChanged(qreal value);
    void hueChanged(qreal value);
    void saturationChanged(qreal value);
    void orientationChanged(int value);
    void frameSizeChanged(const QSize& size);
protected:
    bool receiveFrame(const VideoFrame& frame) Q_DECL_OVERRIDE;
    bool needUpdateBackground() const Q_DECL_OVERRIDE;
    void drawBackground() Q_DECL_OVERRIDE;
    bool needDrawFrame() const Q_DECL_OVERRIDE; //not important.
    void drawFrame() Q_DECL_OVERRIDE;
    void handlePaintEvent() Q_DECL_OVERRIDE;

private:
    virtual bool onSetPreferredPixelFormat(VideoFormat::PixelFormat pixfmt) Q_DECL_OVERRIDE;
    virtual bool onForcePreferredPixelFormat(bool force = true) Q_DECL_OVERRIDE;
    virtual void onSetOutAspectRatioMode(OutAspectRatioMode mode) Q_DECL_OVERRIDE;
    virtual void onSetOutAspectRatio(qreal ratio) Q_DECL_OVERRIDE;
    virtual bool onSetQuality(Quality q) Q_DECL_OVERRIDE;
    virtual bool onSetOrientation(int value) Q_DECL_OVERRIDE;
    virtual void onResizeRenderer(int width, int height) Q_DECL_OVERRIDE;
    virtual bool onSetRegionOfInterest(const QRectF& roi) Q_DECL_OVERRIDE;
    virtual QPointF onMapToFrame(const QPointF& p) const Q_DECL_OVERRIDE;
    virtual QPointF onMapFromFrame(const QPointF& p) const Q_DECL_OVERRIDE;

    virtual bool onSetBrightness(qreal brightness) Q_DECL_OVERRIDE;
    virtual bool onSetContrast(qreal contrast) Q_DECL_OVERRIDE;
    virtual bool onSetHue(qreal hue) Q_DECL_OVERRIDE;
    virtual bool onSetSaturation(qreal saturation) Q_DECL_OVERRIDE;
    virtual void onFrameSizeChanged(const QSize& size) Q_DECL_OVERRIDE;
    // from AVOutput
    virtual void setStatistics(Statistics* statistics) Q_DECL_OVERRIDE; //called by friend AVPlayer
    virtual bool onInstallFilter(Filter *filter, int index) Q_DECL_OVERRIDE;
    virtual bool onUninstallFilter(Filter *filter) Q_DECL_OVERRIDE;
    virtual bool onHanlePendingTasks() Q_DECL_OVERRIDE;
};

} //namespace QtAV

#endif // QTAV_VIDEOOUTPUT_H
