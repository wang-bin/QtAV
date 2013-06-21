/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2013 Wang Bin <wbsecg1@gmail.com>

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

#include <private/AVOutput_p.h>
//#include <QtAV/ImageConverter.h>
#include <QtAV/VideoRenderer.h>
#include <QtAV/QtAV_Compat.h>
#include <QtCore/QMutex>
#include <QtCore/QRect>

/*TODO:
 * Region of Interest(ROI)
 */
class QObject;
class QWidget;
namespace QtAV {
class Filter;
class Q_EXPORT VideoRendererPrivate : public AVOutputPrivate
{
public:
    VideoRendererPrivate():
        update_background(true)
      , scale_in_renderer(true)
      , draw_osd(true)
      , draw_subtitle(true)
      , draw_custom(true)
      , renderer_width(480)
      , renderer_height(320)
      , source_aspect_ratio(0)
      , src_width(0)
      , src_height(0)
      , aspect_ratio_changed(true) //to set the initial parameters
      , out_aspect_ratio_mode(VideoRenderer::VideoAspectRatio)
      , out_aspect_ratio(0)
      , quality(VideoRenderer::QualityBest)
      , widget_holder(0)
      , item_holder(0)
      , osd_filter(0)
      , subtitle_filter(0)
    {
        //conv.setInFormat(PIX_FMT_YUV420P);
        //conv.setOutFormat(PIX_FMT_BGR32); //TODO: why not RGB32?
    }
    virtual ~VideoRendererPrivate(){
        widget_holder = 0;
    }
    void computeOutParameters(qreal outAspectRatio) {
        qreal rendererAspectRatio = qreal(renderer_width)/qreal(renderer_height);
        if (out_aspect_ratio_mode == VideoRenderer::RendererAspectRatio) {
            out_aspect_ratio = rendererAspectRatio;
            out_rect = QRect(0, 0, renderer_width, renderer_height);
            return;
        }
        qDebug("out rect: %f %dx%d ==>", out_aspect_ratio, out_rect.width(), out_rect.height());
        if (rendererAspectRatio > outAspectRatio) { //equals to original video aspect ratio here, also equals to out ratio
            //renderer is too wide, use renderer's height, horizonal align center
            int h = renderer_height;
            int w = outAspectRatio * qreal(h);
            out_rect = QRect((renderer_width - w)/2, 0, w, h);
        } else if (rendererAspectRatio < outAspectRatio) {
            //renderer is too high, use renderer's width
            int w = renderer_width;
            int h = qreal(w)/outAspectRatio;
            out_rect = QRect(0, (renderer_height - h)/2, w, h);
        }
        out_aspect_ratio = outAspectRatio;
        qDebug("%f %dx%d <<<<<<<<", out_aspect_ratio, out_rect.width(), out_rect.height());
    }
    virtual void setupQuality() {}

    //draw background when necessary, for example, renderer is resized. Then set to false
    bool update_background;
    bool scale_in_renderer;
    bool draw_osd, draw_subtitle, draw_custom;
    // width, height: the renderer's size. i.e. size of video frame with the value with borders
    //TODO: rename to renderer_width/height
    int renderer_width, renderer_height;
    qreal source_aspect_ratio;
    int src_width, src_height; //TODO: in_xxx
    //ImageConverter conv;
    QMutex img_mutex;
    //for both source, out aspect ratio. because source change may result in out change if mode is VideoAspectRatio
    bool aspect_ratio_changed;
    VideoRenderer::OutAspectRatioMode out_aspect_ratio_mode;
    qreal out_aspect_ratio;
    VideoRenderer::Quality quality;
    //out_rect: the displayed video frame out_rect in the renderer
    QRect out_rect; //TODO: out_out_rect

    /* Stores but not own the ptr if renderer is a subclass of QWidget.
     * Some operations are based on QWidget
     */
    QWidget *widget_holder;
    QGraphicsItem *item_holder;

    Filter *osd_filter, *subtitle_filter; //should be at the end of list and draw top level
};

} //namespace QtAV
#endif // QAV_VIDEORENDERER_P_H
