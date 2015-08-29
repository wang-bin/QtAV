/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2015 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QAV_VIDEORENDERER_P_H
#define QAV_VIDEORENDERER_P_H

#include <QtAV/private/AVOutput_p.h>
#include <QtAV/VideoRenderer.h>
#include <QtCore/QMutex>
#include <QtCore/QRect>
#include <QtAV/VideoFrame.h>

/*TODO:
 * Region of Interest(ROI)
 * use matrix to compute out rect, mapped point etc
 */
class QObject;
class QWidget;
namespace QtAV {
class Filter;
class Q_AV_PRIVATE_EXPORT VideoRendererPrivate : public AVOutputPrivate
{
public:
    VideoRendererPrivate():
        AVOutputPrivate()
      , update_background(true)
      , renderer_width(480)
      , renderer_height(320)
      , source_aspect_ratio(0)
      , src_width(0)
      , src_height(0)
      , aspect_ratio_changed(true) //to set the initial parameters
      , out_aspect_ratio_mode(VideoRenderer::VideoAspectRatio)
      , out_aspect_ratio(0)
      , quality(VideoRenderer::QualityBest)
      , orientation(0)
      , preferred_format(VideoFormat::Format_RGB32)
      , force_preferred(false)
      , brightness(0)
      , contrast(0)
      , hue(0)
      , saturation(0)
    {
        //conv.setInFormat(PIX_FMT_YUV420P);
        //conv.setOutFormat(PIX_FMT_BGR32); //TODO: why not RGB32?
    }
    virtual ~VideoRendererPrivate(){
    }
    void computeOutParameters(qreal outAspectRatio) {
        qreal rendererAspectRatio = qreal(renderer_width)/qreal(renderer_height);
        if (out_aspect_ratio_mode == VideoRenderer::RendererAspectRatio) {
            out_aspect_ratio = rendererAspectRatio;
            out_rect = QRect(0, 0, renderer_width, renderer_height);
            return;
        }
        // dar: displayed aspect ratio in video renderer orientation
        const qreal dar = (orientation % 180) ? 1.0/outAspectRatio : outAspectRatio;
        //qDebug("out rect: %f %dx%d ==>", out_aspect_ratio, out_rect.width(), out_rect.height());
        if (rendererAspectRatio >= dar) { //equals to original video aspect ratio here, also equals to out ratio
            //renderer is too wide, use renderer's height, horizonal align center
            const int h = renderer_height;
            const int w = dar * qreal(h);
            out_rect = QRect((renderer_width - w)/2, 0, w, h);
        } else if (rendererAspectRatio < dar) {
            //renderer is too high, use renderer's width
            const int w = renderer_width;
            const int h = qreal(w)/dar;
            out_rect = QRect(0, (renderer_height - h)/2, w, h);
        }
        out_aspect_ratio = outAspectRatio;
        //qDebug("%f %dx%d <<<<<<<<", out_aspect_ratio, out_rect.width(), out_rect.height());
    }
    virtual void setupQuality() {}

    //draw background when necessary, for example, renderer is resized. Then set to false
    bool update_background;
    // width, height: the renderer's size. i.e. size of video frame with the value with borders
    //TODO: rename to renderer_width/height
    int renderer_width, renderer_height;
    qreal source_aspect_ratio;
    int src_width, src_height; //TODO: in_xxx
    QMutex img_mutex;
    //for both source, out aspect ratio. because source change may result in out change if mode is VideoAspectRatio
    bool aspect_ratio_changed;
    VideoRenderer::OutAspectRatioMode out_aspect_ratio_mode;
    qreal out_aspect_ratio;
    VideoRenderer::Quality quality;
    //out_rect: the displayed video frame out_rect in the renderer
    QRect out_rect; //TODO: out_out_rect
    QRectF roi;
    int orientation;

    VideoFrame video_frame;
    VideoFormat::PixelFormat preferred_format;
    bool force_preferred;

    qreal brightness, contrast, hue, saturation;
};

} //namespace QtAV
#endif // QAV_VIDEORENDERER_P_H
