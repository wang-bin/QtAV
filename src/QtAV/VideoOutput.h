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

    virtual bool receive(const VideoFrame& frame); //has default
    //void setVideoFormat(const VideoFormat& format); //has default

    VideoFormat::PixelFormat preferredPixelFormat() const;
    bool isSupported(VideoFormat::PixelFormat pixfmt) const;
    bool open();
    bool close();
    QWidget* widget();
    QGraphicsItem* graphicsItem();

private:
    /*!
     * \brief setPreferredPixelFormat
     * \param pixfmt
     *  pixfmt will be used if decoded format is not supported by this renderer. otherwise, use decoded format.
     *  return false if \a pixfmt is not supported and not changed.
     */
    virtual bool onSetPreferredPixelFormat(VideoFormat::PixelFormat pixfmt);
    virtual void onForcePreferredPixelFormat(bool force = true);
    virtual void onScaleInRenderer(bool q);
    virtual void onSetOutAspectRatioMode(OutAspectRatioMode mode);
    virtual void onSetOutAspectRatio(qreal ratio);
    virtual void onSetQuality(Quality q);
    virtual void onResizeRenderer(int width, int height);
    virtual void onSetRegionOfInterest(const QRectF& roi);
    virtual QPointF onMapToFrame(const QPointF& p) const;
    virtual QPointF onMapFromFrame(const QPointF& p) const;
    virtual OSDFilter* onSetOSDFilter(OSDFilter *filter);
    virtual Filter* onSetSubtitleFilter(Filter *filter);

    virtual bool onSetBrightness(qreal brightness);
    virtual bool onSetContrast(qreal contrast);
    virtual bool onSetHue(qreal hue);
    virtual bool onSetSaturation(qreal saturation);

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
