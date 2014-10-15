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

#include "QtAV/LibAVFilter.h"
#include <QtCore/QSharedPointer>
#include "QtAV/private/Filter_p.h"
#include "QtAV/Statistics.h"
#include "QtAV/VideoFrame.h"
#include "QtAV/private/AVCompat.h"
#include "utils/Logger.h"

/*
 * libav10.x, ffmpeg2.x: av_buffersink_read deprecated
 * libav9.x: only av_buffersink_read can be used
 * ffmpeg<2.0: av_buffersink_get_buffer_ref and av_buffersink_read
 */
#define QTAV_HAVE_av_buffersink_get_frame (LIBAV_MODULE_CHECK(LIBAVFILTER, 4, 2, 0) || FFMPEG_MODULE_CHECK(LIBAVFILTER, 3, 79, 100)) //3.79.101: ff2.0.4

namespace QtAV {

class LibAVFilterPrivate : public VideoFilterPrivate
{
public:
    LibAVFilterPrivate()
        : pixfmt(QTAV_PIX_FMT_C(NONE))
        , width(0)
        , height(0)
        , avframe(0)
        , status(LibAVFilter::NotConfigured)
    {
#if QTAV_HAVE(AVFILTER)
        filter_graph = 0;
        in_filter_ctx = 0;
        out_filter_ctx = 0;
        avfilter_register_all();
#endif //QTAV_HAVE(AVFILTER)
    }
    virtual ~LibAVFilterPrivate() {
#if QTAV_HAVE(AVFILTER)
        avfilter_graph_free(&filter_graph);
#endif //QTAV_HAVE(AVFILTER)
        if (avframe) {
            av_frame_free(&avframe);
            avframe = 0;
        }
    }

    bool setOptions(const QString& opt) {
        if (options == opt)
            return false;
        options = opt;
        status = LibAVFilter::NotConfigured;
        return true;
    }

    bool push(Frame *frame, qreal pts);
    bool pull(Frame *f);

    bool setup() {
        if (avframe) {
            av_frame_free(&avframe);
            avframe = 0;
        }
#if QTAV_HAVE(AVFILTER)
        avfilter_graph_free(&filter_graph);
        filter_graph = avfilter_graph_alloc();
        //QString sws_flags_str;
        // pixel_aspect==sar, pixel_aspect is more compatible
        QString buffersrc_args = QString("video_size=%1x%2:pix_fmt=%3:time_base=%4/%5:pixel_aspect=1")
                .arg(width).arg(height).arg(pixfmt).arg(1).arg(AV_TIME_BASE);
        qDebug("buffersrc_args=%s", buffersrc_args.toUtf8().constData());
        AVFilter *buffersrc  = avfilter_get_by_name("buffer");
        Q_ASSERT(buffersrc);
        int ret = avfilter_graph_create_filter(&in_filter_ctx,
                                               buffersrc,
                                               "in", buffersrc_args.toUtf8().constData(), NULL,
                                               filter_graph);
        if (ret < 0) {
            qWarning("Can not create buffer source: %s", av_err2str(ret));
            status = LibAVFilter::ConfigureFailed;
            return false;
        }
        /* buffer video sink: to terminate the filter chain. */
        AVFilter *buffersink = avfilter_get_by_name("buffersink");
        Q_ASSERT(buffersink);
        if ((ret = avfilter_graph_create_filter(&out_filter_ctx, buffersink, "out",
                                           NULL, NULL, filter_graph)) < 0) {
            qWarning("Can not create buffer sink: %s", av_err2str(ret));
            status = LibAVFilter::ConfigureFailed;
            return false;
        }
        /* Endpoints for the filter graph. */
        AVFilterInOut *outputs = avfilter_inout_alloc();
        AVFilterInOut *inputs  = avfilter_inout_alloc();
        outputs->name       = av_strdup("in");
        outputs->filter_ctx = in_filter_ctx;
        outputs->pad_idx    = 0;
        outputs->next       = NULL;

        inputs->name       = av_strdup("out");
        inputs->filter_ctx = out_filter_ctx;
        inputs->pad_idx    = 0;
        inputs->next       = NULL;

        //avfilter_graph_parse, avfilter_graph_parse2?
#if QTAV_USE_FFMPEG(LIBAVFILTER)
        if ((ret = avfilter_graph_parse_ptr(filter_graph, options.toUtf8().constData(), &inputs, &outputs, NULL)) < 0) {
#else
        if ((ret = avfilter_graph_parse(filter_graph, options.toUtf8().constData(), inputs, outputs, NULL))) {
#endif
            qWarning("avfilter_graph_parse_ptr fail: %s", av_err2str(ret));
            avfilter_inout_free(&outputs);
            avfilter_inout_free(&inputs);
            status = LibAVFilter::ConfigureFailed;
            return false;
        }
        if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0) {
            qWarning("avfilter_graph_config fail: %s", av_err2str(ret));
            avfilter_inout_free(&outputs);
            avfilter_inout_free(&inputs);
            status = LibAVFilter::ConfigureFailed;
            return false;
        }
        avfilter_inout_free(&outputs);
        avfilter_inout_free(&inputs);
        avframe = av_frame_alloc();
        status = LibAVFilter::ConfigreOk;
        return true;
#else
        return false;
#endif //QTAV_HAVE(AVFILTER)
    }
#if QTAV_HAVE(AVFILTER)
    AVFilterGraph *filter_graph;
    AVFilterContext *in_filter_ctx;
    AVFilterContext *out_filter_ctx;
#endif //QTAV_HAVE(AVFILTER)
    AVPixelFormat pixfmt;
    int width, height;
    AVFrame *avframe;

    QString options;
    LibAVFilter::Status status;
};

LibAVFilter::LibAVFilter(QObject *parent):
    VideoFilter(*new LibAVFilterPrivate(), parent)
{
}

LibAVFilter::~LibAVFilter()
{
}

void LibAVFilter::setOptions(const QString &options)
{
    if (!d_func().setOptions(options))
        return;
    emit optionsChanged();
    emit statusChanged();
}

QString LibAVFilter::options() const
{
    return d_func().options;
}

LibAVFilter::Status LibAVFilter::status() const
{
    return d_func().status;
}

void LibAVFilter::setStatus(Status value)
{
    DPTR_D(LibAVFilter);
    if (d.status == value)
        return;
    d.status = value;
    emit statusChanged();
}

void LibAVFilter::process(Statistics *statistics, VideoFrame *frame)
{
    Q_UNUSED(statistics);
    if (status() == ConfigureFailed)
        return;
    DPTR_D(LibAVFilter);
    Status old = status();
    bool ok = d.push(frame, statistics->video_only.pts());
    if (old != status())
        emit statusChanged();
    if (!ok)
        return;
    d.pull(frame);
}


bool LibAVFilterPrivate::push(Frame *frame, qreal pts)
{
#if QTAV_HAVE(AVFILTER)
    VideoFrame *vf = static_cast<VideoFrame*>(frame);
    if (status == LibAVFilter::NotConfigured || !avframe || width != vf->width() || height != vf->height() || pixfmt != vf->pixelFormatFFmpeg()) {
        width = vf->width();
        height = vf->height();
        pixfmt = (AVPixelFormat)vf->pixelFormatFFmpeg();
        if (!setup()) {
            qWarning("setup filter graph error");
            //enabled = false; // skip this filter and avoid crash
            return false;
        }
    }
    avframe->pts = pts * 1000000.0; // time_base is 1/1000000
    avframe->width = vf->width();
    avframe->height = vf->height();
    avframe->format = pixfmt = (AVPixelFormat)vf->pixelFormatFFmpeg();
    for (int i = 0; i < vf->planeCount(); ++i) {
        avframe->data[i] =vf->bits(i);
        avframe->linesize[i] = vf->bytesPerLine(i);
    }
    //int ret = av_buffersrc_add_frame_flags(in_filter_ctx, avframe, AV_BUFFERSRC_FLAG_KEEP_REF);
    /*
     * av_buffersrc_write_frame equals to av_buffersrc_add_frame_flags with AV_BUFFERSRC_FLAG_KEEP_REF.
     * av_buffersrc_write_frame is more compatible, while av_buffersrc_add_frame_flags only exists in ffmpeg >=2.0
     * add a ref if frame is ref counted
     * TODO: libav < 10.0 will copy the frame, prefer to use av_buffersrc_buffer
     */
    int ret = av_buffersrc_write_frame(in_filter_ctx, avframe);
    if (ret != 0) {
        qWarning("av_buffersrc_add_frame error: %s", av_err2str(ret));
        return false;
    }
    return true;
#else
    Q_UNUSED(frame);
    Q_UNUSED(pts);
    enabled = false;
    return false;
#endif
}

#if QTAV_HAVE(AVFILTER)
// local types can not be used as template parameters
class AVFrameHolder {
public:
    AVFrameHolder() {
        m_frame = av_frame_alloc();
#if !QTAV_HAVE_av_buffersink_get_frame
        picref = 0;
#endif
    }
    ~AVFrameHolder() {
        av_frame_free(&m_frame);
#if !QTAV_HAVE_av_buffersink_get_frame
        avfilter_unref_bufferp(&picref);
#endif
    }
    AVFrame* frame() { return m_frame;}
#if !QTAV_HAVE_av_buffersink_get_frame
    AVFilterBufferRef** bufferRef() { return &picref;}
    void copyBufferToFrame() { avfilter_copy_buf_props(m_frame, picref);}
#endif
private:
    AVFrame *m_frame;
#if !QTAV_HAVE_av_buffersink_get_frame
    AVFilterBufferRef *picref;
#endif
};
typedef QSharedPointer<AVFrameHolder> AVFrameHolderRef;
#endif //QTAV_HAVE(AVFILTER)

bool LibAVFilterPrivate::pull(Frame *f)
{
#if QTAV_HAVE(AVFILTER)
    AVFrameHolderRef frame_ref(new AVFrameHolder());
#if QTAV_HAVE_av_buffersink_get_frame
    int ret = av_buffersink_get_frame(out_filter_ctx, frame_ref->frame());
#else
    int ret = av_buffersink_read(out_filter_ctx, frame_ref->bufferRef());
#endif //QTAV_HAVE_av_buffersink_get_frame
    if (ret < 0) {
        qWarning("av_buffersink_get_frame error: %s", av_err2str(ret));
        return false;
    }
#if !QTAV_HAVE_av_buffersink_get_frame
    frame_ref->copyBufferToFrame();
#endif
    VideoFrame vf(frame_ref->frame()->width, frame_ref->frame()->height, VideoFormat(frame_ref->frame()->format));
    vf.setBits(frame_ref->frame()->data);
    vf.setBytesPerLine(frame_ref->frame()->linesize);
    vf.setMetaData("avframe_hoder_ref", QVariant::fromValue(frame_ref));
    *f = vf;
    return true;
#else
    Q_UNUSED(f);
    return false;
#endif //QTAV_HAVE(AVFILTER)
}

} //namespace QtAV

#if QTAV_HAVE(AVFILTER)
Q_DECLARE_METATYPE(QtAV::AVFrameHolderRef)
#endif
