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
#include <QtAV/FactoryDefine.h>

/*!
 * A bridge for VideoOutput(QObject based) and video renderer backend classes
 * Every public setter call it's virtual onSetXXX(...) which has default behavior.
 * While VideoOutput.onSetXXX(...) simply calls backend's setXXX(...) and return whether the result is desired.
 */
struct AVCodecContext;
struct AVFrame;
class QImage;
class QObject;
class QPaintEvent;
class QRect;
class QWidget;
class QWindow;
class QGraphicsItem;

namespace QtAV {

typedef int VideoRendererId;
class VideoRenderer;
FACTORY_DECLARE(VideoRenderer)

class Filter;
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

    virtual bool receive(const VideoFrame& frame); //has default
    //virtual void setVideoFormat(const VideoFormat& format); //has default
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
    virtual VideoFormat::PixelFormat preferredPixelFormat() const; //virtual?
    /*!
     * \brief forcePreferredPixelFormat
     *  force to use preferredPixelFormat() even if incoming format is supported
     * \param force
     */
    void forcePreferredPixelFormat(bool force = true);
    bool isPreferredPixelFormatForced() const;
    virtual bool isSupported(VideoFormat::PixelFormat pixfmt) const = 0;

    void setOutAspectRatioMode(OutAspectRatioMode mode);
    OutAspectRatioMode outAspectRatioMode() const;
    //If setOutAspectRatio(qreal) is used, then OutAspectRatioMode is CustomAspectRation
    void setOutAspectRatio(qreal ratio);
    qreal outAspectRatio() const;//

    void setQuality(Quality q);
    Quality quality() const;

    void resizeRenderer(const QSize& size);
    void resizeRenderer(int width, int height);
    QSize rendererSize() const;
    int rendererWidth() const;
    int rendererHeight() const;
    //geometry size of current video frame
    QSize frameSize() const;

    /*!
     * \brief orientation
     * 0, 90, 180, 270. other values are ignored
     * outAspectRatio() corresponds with orientation == 0. displayed aspect ratio may change if orientation is not 0
     */
    int orientation() const;
    void setOrientation(int value);

    //The video frame rect in renderer you shoud paint to. e.g. in RendererAspectRatio mode, the rect equals to renderer's
    QRect videoRect() const;
    /*
     * region of interest, ROI
     * invalid rect means the whole source rect
     * null rect is the whole available source rect. e.g. (0, 0, 0, 0) equals whole source rect
     * (20, 30, 0, 0) equals (20, 30, sourceWidth - 20, sourceHeight - 30)
     * if |x|<1, |y|<1, |width|<1, |height|<1 means the ratio of source rect(normalized value)
     * |width| == 1 or |height| == 1 is a normalized value iff x or y is normalized
     * call realROI() to get the frame rect actually to be render
     * TODO: nagtive width or height means invert direction. is nagtive necessary?
     */
    QRectF regionOfInterest() const;
    // TODO: reset aspect ratio to roi.width/roi/heghit
    void setRegionOfInterest(qreal x, qreal y, qreal width, qreal height);
    void setRegionOfInterest(const QRectF& roi);
    // compute the real ROI
    QRect realROI() const;
    // |w| <= 1, |x| < 1
    QRectF normalizedROI() const;

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
    // to avoid conflicting width QWidget::window()
    virtual QWindow* qwindow() { return 0;}
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

    void enableDefaultEventFilter(bool e);
    bool isDefaultEventFilterEnabled() const;

    /*!
     * \brief brightness, contrast, hue, saturation
     *  values range between -1.0 and 1.0, the default is 0.
     *  value is not changed if does not implementd and onChangingXXX() returns false.
     *  video widget/item will update after if onChangingXXX/setXXX returns true
     * \return \a false if failed (may be onChangingXXX not implemented or return false)
     */
    qreal brightness() const;
    bool setBrightness(qreal brightness);
    qreal contrast() const;
    bool setContrast(qreal contrast);
    qreal hue() const;
    bool setHue(qreal hue);
    qreal saturation() const;
    bool setSaturation(qreal saturation);

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
    virtual void handlePaintEvent(); //has default. User don't have to implement it

private: //used by VideoOutput class
    /*!
     * return false if value not changed. default is true
     */
    virtual bool onSetPreferredPixelFormat(VideoFormat::PixelFormat pixfmt);
    virtual bool onForcePreferredPixelFormat(bool force = true);
    virtual void onSetOutAspectRatioMode(OutAspectRatioMode mode);
    virtual void onSetOutAspectRatio(qreal ratio);
    virtual bool onSetQuality(Quality q);
    virtual void onResizeRenderer(int width, int height);
    virtual bool onSetOrientation(int value);
    virtual bool onSetRegionOfInterest(const QRectF& roi);
    virtual QPointF onMapToFrame(const QPointF& p) const;
    virtual QPointF onMapFromFrame(const QPointF& p) const;
    /*!
     * \brief onSetXX
     *  It's called when user call setXXX() with a new value. You should implement how to actually change the value, e.g. change brightness with shader.
     * \return
     *  false: It's default. means not implemented. \a brightness() does not change.
     *  true: Implement this and return true. \a brightness() will change to new value
     */
    virtual bool onSetBrightness(qreal brightness);
    virtual bool onSetContrast(qreal contrast);
    virtual bool onSetHue(qreal hue);
    virtual bool onSetSaturation(qreal saturation);
    void updateUi();
private:
    friend class VideoOutput;
    //the size of decoded frame. get called in receiveFrame(). internal use only
    void setInSize(const QSize& s);
    void setInSize(int width, int height);
};

} //namespace QtAV
#endif // QAV_VIDEORENDERER_H
