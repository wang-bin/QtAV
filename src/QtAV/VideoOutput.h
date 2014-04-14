/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2014 Wang Bin <wbsecg1@gmail.com>

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
    Q_PROPERTY(qreal outAspectRatio READ outAspectRatio WRITE setOutAspectRatio NOTIFY outAspectRatioChanged)
    //fillMode
    // TODO: how to use enums in base class as property or Q_ENUM
    Q_PROPERTY(OutAspectRatioMode outAspectRatioMode READ outAspectRatioMode WRITE setOutAspectRatioMode NOTIFY outAspectRatioModeChanged)
    Q_ENUMS(OutAspectRatioMode)
    Q_ENUMS(Quality)
public:
    VideoOutput(VideoRendererId rendererId, QObject *parent = 0);
    ~VideoOutput();
    VideoRendererId id() const;

    virtual bool receive(const VideoFrame& frame); //has default
    //void setVideoFormat(const VideoFormat& format); //has default

    VideoFormat::PixelFormat preferredPixelFormat() const;
    bool isSupported(VideoFormat::PixelFormat pixfmt) const;
    bool open();
    bool close();
    QWidget* widget();
    QGraphicsItem* graphicsItem();

signals:
    void regionOfInterestChanged(const QRectF&);
    void outAspectRatioChanged(qreal);
    void outAspectRatioModeChanged(OutAspectRatioMode);
    void brightnessChanged(qreal value);
    void contrastChanged(qreal value);
    void hueChanged(qreal value);
    void saturationChanged(qreal value);

protected:
    bool receiveFrame(const VideoFrame& frame);
    bool needUpdateBackground() const;
    void drawBackground();
    bool needDrawFrame() const; //not important.
    void drawFrame();
    void resizeFrame(int width, int height);
    void handlePaintEvent();
    bool onChangingBrightness(qreal b);
    bool onChangingContrast(qreal c);
    bool onChangingHue(qreal h);
    bool onChangingSaturation(qreal s);


private: //for proxy
    virtual bool onSetPreferredPixelFormat(VideoFormat::PixelFormat pixfmt);
    virtual bool onForcePreferredPixelFormat(bool force = true);
    virtual bool onScaleInRenderer(bool q);
    virtual void onSetOutAspectRatioMode(OutAspectRatioMode mode);
    virtual void onSetOutAspectRatio(qreal ratio);
    virtual bool onSetQuality(Quality q);
    virtual void onResizeRenderer(int width, int height);
    virtual bool onSetRegionOfInterest(const QRectF& roi);
    virtual QPointF onMapToFrame(const QPointF& p) const;
    virtual QPointF onMapFromFrame(const QPointF& p) const;
    virtual void onSetOSDFilter(OSDFilter *filter);
    virtual void onSetSubtitleFilter(Filter *filter);

    virtual bool onSetBrightness(qreal brightness);
    virtual bool onSetContrast(qreal contrast);
    virtual bool onSetHue(qreal hue);
    virtual bool onSetSaturation(qreal saturation);

    void onSetInSize(int width, int height); //private? for internal use only, called by VideoThread.

    // from AVOutput
    virtual void setStatistics(Statistics* statistics); //called by friend AVPlayer
    virtual bool onInstallFilter(Filter *filter);
    virtual bool onUninstallFilter(Filter *filter);
    virtual void onAddOutputSet(OutputSet *set);
    virtual void onRemoveOutputSet(OutputSet *set);
    virtual void onAttach(OutputSet *set); //add this to set
    virtual void onDetach(OutputSet *set = 0); //detatch from (all, if 0) output set(s)
    virtual bool onHanlePendingTasks();

};

} //namespace QtAV

#endif // QTAV_VIDEOOUTPUT_H
