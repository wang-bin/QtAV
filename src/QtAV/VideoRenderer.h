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

#ifndef QAV_VIDEORENDERER_H
#define QAV_VIDEORENDERER_H

#include <QtCore/QByteArray>
#include <QtCore/QSize>
#include <QtCore/QRectF>
#include <QtAV/AVOutput.h>
#include <QtAV/VideoFrame.h>
/*TODO:
 *  broadcast to network
 *  background color for original aspect ratio
 *api:
 *  inSize: the converted image size
 *  outSize: the displaying frame size with out borders in renderer
 *  rendererSize: the original video size
 *  outAspectRatio:
 *  videoAspectRatio:
 *  rendererAspectRatio:
 *
 *  or videoXXX is out(display) XXX, the original XXX is videoOriginalXXX
 */
struct AVCodecContext;
struct AVFrame;
class QImage;
class QObject;
class QPaintEvent;
class QRect;
class QWidget;
class QGraphicsItem;

namespace QtAV {

typedef int VideoRendererId;

class Filter;
class OSDFilter;
class VideoFormat;
class VideoRendererPrivate;
class Q_AV_EXPORT VideoRenderer : public AVOutput
{
    DPTR_DECLARE_PRIVATE(VideoRenderer)
public:
    //TODO: original video size mode
    // fillmode: keepsize
    enum OutAspectRatioMode {
        RendererAspectRatio //Use renderer's aspect ratio, i.e. stretch to fit the renderer rect
      , VideoAspectRatio    //Use video's aspect ratio and align center in renderer.
      , CustomAspectRation  //Use the ratio set by setOutAspectRatio(qreal). Mode will be set to this if that function is called
      //, AspectRatio4_3, AspectRatio16_9
    };
    enum Quality {
        QualityDefault, //good
        QualityBest,
        QualityFastest
    };

    VideoRenderer();
    virtual ~VideoRenderer();
    virtual VideoRendererId id() const = 0;

    bool receive(const VideoFrame& frame);
    void setVideoFormat(const VideoFormat& format);
    //VideoFormat& videoFormat();
    //const VideoFormat& videoFormat() const;
    /*!
     * \brief setPreferredPixelFormat
     * \param pixfmt
     *  pixfmt will be used if decoded format is not supported by this renderer. otherwise, use decoded format.
     *  return false if \a pixfmt is not supported and not changed.
     */
    bool setPreferredPixelFormat(VideoFormat::PixelFormat pixfmt);
    /*!
     * \brief preferredPixelFormat
     * \return preferred pixel format. e.g. WidgetRenderer is rgb formats.
     */
    VideoFormat::PixelFormat preferredPixelFormat() const;
    /*!
     * \brief forcePreferredPixelFormat
     *  force to use preferredPixelFormat() even if incoming format is supported
     * \param force
     */
    void forcePreferredPixelFormat(bool force = true);
    bool isPreferredPixelFormatForced() const;
    virtual bool isSupported(VideoFormat::PixelFormat pixfmt) const = 0;

    //for testing performance
    void scaleInRenderer(bool q);
    bool scaleInRenderer() const;

    void setOutAspectRatioMode(OutAspectRatioMode mode);
    OutAspectRatioMode outAspectRatioMode() const;
    //If setOutAspectRatio(qreal) is used, then OutAspectRatioMode is CustomAspectRation
    void setOutAspectRatio(qreal ratio);
    qreal outAspectRatio() const;//

    void setQuality(Quality q);
    Quality quality() const;

    //TODO: unregister
    virtual bool open();
    virtual bool close();
    //virtual QImage currentFrameImage() const = 0; //const QImage& const?
    //TODO: resizeRenderer
    void resizeRenderer(const QSize& size);
    void resizeRenderer(int width, int height);
    QSize rendererSize() const;
    int rendererWidth() const;
    int rendererHeight() const;
    //geometry size of current video frame
    QSize frameSize() const;
    //The video frame rect in renderer you shoud paint to. e.g. in RendererAspectRatio mode, the rect equals to renderer's
    QRect videoRect() const;
    /*
     * region of interest, ROI
     * invalid rect means the whole source rect
     * null rect is the whole available source rect. e.g. (0, 0, 0, 0) equals whole source rect
     * (20, 30, 0, 0) equals (20, 30, sourceWidth - 20, sourceHeight - 30)
     * if |x|<=1, |y|<=1, |width|<1, |height|<1 means the ratio of source rect
     * call realROI() to get the frame rect actually to be render
     * TODO: nagtive width or height means invert direction. is nagtive necessary?
     */
    QRectF regionOfInterest() const;
    // TODO: reset aspect ratio to roi.width/roi/heghit
    void setRegionOfInterest(qreal x, qreal y, qreal width, qreal height);
    void setRegionOfInterest(const QRectF& roi);
    // compute the real ROI
    QRect realROI() const;

    // TODO: map normalized
    /*!
     * \brief mapToFrame
     *  map point in VideoRenderer coordinate to VideoFrame, with current ROI
     */
    QPointF mapToFrame(const QPointF& p) const;
    /*!
     * \brief mapFromFrame
     *  map point in VideoFrame coordinate to VideoRenderer, with current ROI
     */
    QPointF mapFromFrame(const QPointF& p) const;

    /*!
     * \brief widget
     * \return default is 0. A QWidget subclass can return \a this
     */
    virtual QWidget* widget() { return 0; }
    /*!
     * \brief graphicsItem
     * \return default is 0. A QGraphicsItem subclass can return \a this
     */
    virtual QGraphicsItem* graphicsItem() { return 0; }

    //TODO: enable/disable = new a default for this vo engine or push back/remove from list
    //filter: null means disable
    //return the old filter. you may release the ptr manually
    OSDFilter* setOSDFilter(OSDFilter *filter);
    OSDFilter *osdFilter();
    Filter* setSubtitleFilter(Filter *filter);
    Filter* subtitleFilter();
    void enableDefaultEventFilter(bool e);
    bool isDefaultEventFilterEnabled() const;
protected:
    VideoRenderer(VideoRendererPrivate &d);
    virtual bool receiveFrame(const VideoFrame& frame) = 0;
    virtual bool needUpdateBackground() const;
    //TODO: drawXXX() is pure virtual
    //called in paintEvent before drawFrame() when required
    virtual void drawBackground();
    virtual bool needDrawFrame() const; //TODO: no virtual func. it's a solution for temporary
    //draw the current frame using the current paint engine. called by paintEvent()
    virtual void drawFrame() = 0; //You MUST reimplement this to display a frame. Other draw functions are not essential
    /*!
     * This function is called whenever resizeRenderer() is called or aspect ratio is changed?
     * You can reimplement it to recreate the offscreen surface.
     * The default does nothing.
     * NOTE: usually it is thread safe, because it is called in main thread resizeEvent,
     * and the surface is only used by painting, which is usually in main thread too.
     * If you are doing offscreen painting in other threads, pay attention to thread safe
     */
    virtual void resizeFrame(int width, int height);
    //TODO: parameter QRect?
    void handlePaintEvent();

private:
    friend class VideoThread;

    //the size of image (QByteArray) that decoded
    void setInSize(const QSize& s); //private? for internal use only, called by VideoThread.
    void setInSize(int width, int height); //private? for internal use only, called by VideoThread.
    //qreal sourceAspectRatio() const;//TODO: from AVCodecContext
    //we don't need api like QSize sourceSize() const. you should get them from player or avinfo(not implemented)
};
typedef VideoRenderer VideoOutput;

} //namespace QtAV
#endif // QAV_VIDEORENDERER_H
