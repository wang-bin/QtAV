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


/*!
 * setter(for private data): call d.impl->setXYZ, d.xyz = d.impl->xyz()
 * other virtual: simply call d.impl's.
 * check isAvailable may be needed because the backend may be null (and now call it's member directly, even nu;;)
 */
namespace QtAV {

class VideoOutputPrivate;
class Q_AV_EXPORT VideoOutput : public QObject, public VideoRenderer
{
    DPTR_DECLARE_PRIVATE(VideoOutput)
public:
    VideoOutput(VideoRendererId rendererId, QObject *parent = 0);
    ~VideoOutput();
    VideoRendererId id() const;

    bool receive(const VideoFrame& frame); //has default
    //void setVideoFormat(const VideoFormat& format); //has default
    /*!
     * \brief setPreferredPixelFormat
     * \param pixfmt
     *  pixfmt will be used if decoded format is not supported by this renderer. otherwise, use decoded format.
     *  return false if \a pixfmt is not supported and not changed.
     */
    bool setPreferredPixelFormat(VideoFormat::PixelFormat pixfmt);  //has default
    VideoFormat::PixelFormat preferredPixelFormat() const;
    void forcePreferredPixelFormat(bool force = true); //has default
    bool isSupported(VideoFormat::PixelFormat pixfmt) const;
    void scaleInRenderer(bool q); //has default
    void setOutAspectRatioMode(OutAspectRatioMode mode); //has default
    void setOutAspectRatio(qreal ratio); //has default
    void setQuality(Quality q); //has default
    bool open();
    bool close();
    void resizeRenderer(int width, int height); //has default
    void setRegionOfInterest(const QRectF& roi); //has default
    QPointF mapToFrame(const QPointF& p) const; //has default
    QPointF mapFromFrame(const QPointF& p) const; //has default
    QWidget* widget();
    QGraphicsItem* graphicsItem();

    //TODO: enable/disable = new a default for this vo engine or push back/remove from list
    //filter: null means disable
    //return the old filter. you may release the ptr manually
    OSDFilter* setOSDFilter(OSDFilter *filter); //has default
    Filter* setSubtitleFilter(Filter *filter); //has default
    void enableDefaultEventFilter(bool e); //has default
    bool setBrightness(qreal brightness); //has default
    bool setContrast(qreal contrast); //has default
    bool setHue(qreal hue); //has default
    bool setSaturation(qreal saturation); //has default

signals:
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

private:
    void setInSize(int width, int height); //private? for internal use only, called by VideoThread.
};

} //namespace QtAV

#endif // QTAV_VIDEOOUTPUT_H
