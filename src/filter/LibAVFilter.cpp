/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2014)

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
#include "QtAV/AudioFrame.h"
#include "QtAV/VideoFrame.h"
#include "QtAV/private/AVCompat.h"
#include "utils/internal.h"
#include "utils/Logger.h"

/*
 * libav10.x, ffmpeg2.x: av_buffersink_read deprecated
 * libav9.x: only av_buffersink_read can be used
 * ffmpeg<2.0: av_buffersink_get_buffer_ref and av_buffersink_read
 */
// TODO: enabled = false if no libavfilter
// TODO: filter_complex
// NO COPY in push/pull
#define QTAV_HAVE_av_buffersink_get_frame (LIBAV_MODULE_CHECK(LIBAVFILTER, 4, 2, 0) || FFMPEG_MODULE_CHECK(LIBAVFILTER, 3, 79, 100)) //3.79.101: ff2.0.4

namespace QtAV {

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
    // copy properties and data ptrs(no deep copy).
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


class LibAVFilter::Private
{
public:
    Private()
        : avframe(0)
        , status(LibAVFilter::NotConfigured)
    {
#if QTAV_HAVE(AVFILTER)
        filter_graph = 0;
        in_filter_ctx = 0;
        out_filter_ctx = 0;
        avfilter_register_all();
#endif //QTAV_HAVE(AVFILTER)
    }
    ~Private() {
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
    bool pushAudioFrame(Frame *frame, bool changed, const QString& args);
    bool pushVideoFrame(Frame *frame, bool changed, const QString& args);

    bool setup(const QString& args, bool video) {
        if (avframe) {
            av_frame_free(&avframe);
            avframe = 0;
        }
        status = LibAVFilter::ConfigureFailed;
#if QTAV_HAVE(AVFILTER)
        avfilter_graph_free(&filter_graph);
        filter_graph = avfilter_graph_alloc();
        //QString sws_flags_str;
        // pixel_aspect==sar, pixel_aspect is more compatible
        QString buffersrc_args = args;
        qDebug("buffersrc_args=%s", buffersrc_args.toUtf8().constData());
#if LIBAVFILTER_VERSION_INT >= AV_VERSION_INT(7,0,0)
        const
#endif
        AVFilter *buffersrc = avfilter_get_by_name(video ? "buffer" : "abuffer");
        Q_ASSERT(buffersrc);
        AV_ENSURE_OK(avfilter_graph_create_filter(&in_filter_ctx,
                                               buffersrc,
                                               "in", buffersrc_args.toUtf8().constData(), NULL,
                                               filter_graph)
                     , false);
        /* buffer video sink: to terminate the filter chain. */
#if LIBAVFILTER_VERSION_INT >= AV_VERSION_INT(7,0,0)
        const
#endif
        AVFilter *buffersink = avfilter_get_by_name(video ? "buffersink" : "abuffersink");
        Q_ASSERT(buffersink);
        AV_ENSURE_OK(avfilter_graph_create_filter(&out_filter_ctx, buffersink, "out",
                                           NULL, NULL, filter_graph)
                , false);
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

        struct delete_helper {
            AVFilterInOut **x;
            delete_helper(AVFilterInOut **io) : x(io) {}
            ~delete_helper() {
                // libav always free it in avfilter_graph_parse. so we does nothing
#if QTAV_USE_FFMPEG(LIBAVFILTER)
                avfilter_inout_free(x);
#endif
            }
        } scoped_in(&inputs), scoped_out(&outputs);
        //avfilter_graph_parse, avfilter_graph_parse2?
        AV_ENSURE_OK(avfilter_graph_parse_ptr(filter_graph, options.toUtf8().constData(), &inputs, &outputs, NULL), false);
        AV_ENSURE_OK(avfilter_graph_config(filter_graph, NULL), false);
        avframe = av_frame_alloc();
        status = LibAVFilter::ConfigureOk;
#if DBG_GRAPH
        //not available in libav9
        const char* g = avfilter_graph_dump(filter_graph, NULL);
        if (g)
            qDebug().nospace() << "filter graph:\n" << g; // use << to not print special chars in qt5.5
        av_freep(&g);
#endif //DBG_GRAPH
        return true;
#endif //QTAV_HAVE(AVFILTER)
        return false;
    }
#if QTAV_HAVE(AVFILTER)
    AVFilterGraph *filter_graph;
    AVFilterContext *in_filter_ctx;
    AVFilterContext *out_filter_ctx;
#endif //QTAV_HAVE(AVFILTER)
    AVFrame *avframe;
    QString options;
    LibAVFilter::Status status;
};

QStringList LibAVFilter::videoFilters()
{
    static const QStringList list(LibAVFilter::registeredFilters(AVMEDIA_TYPE_VIDEO));
    return list;
}

QStringList LibAVFilter::audioFilters()
{
    static const QStringList list(LibAVFilter::registeredFilters(AVMEDIA_TYPE_AUDIO));
    return list;
}

QString LibAVFilter::filterDescription(const QString &filterName)
{
    QString s;
#if QTAV_HAVE(AVFILTER)
    avfilter_register_all();
    const AVFilter *f = avfilter_get_by_name(filterName.toUtf8().constData());
    if (!f)
        return s;
    if (f->description)
        s.append(QString::fromUtf8(f->description));
#if AV_MODULE_CHECK(LIBAVFILTER, 3, 7, 0, 8, 100)
    return s.append(QLatin1String("\n")).append(QObject::tr("Options:"))
            .append(Internal::optionsToString((void*)&f->priv_class));
#endif
#endif //QTAV_HAVE(AVFILTER)
    Q_UNUSED(filterName);
    return s;
}

LibAVFilter::LibAVFilter()
    : priv(new Private())
{
}

LibAVFilter::~LibAVFilter()
{
    delete priv;
}

void LibAVFilter::setOptions(const QString &options)
{
    if (!priv->setOptions(options))
        return;
    Q_EMIT optionsChanged();
}

QString LibAVFilter::options() const
{
    return priv->options;
}

LibAVFilter::Status LibAVFilter::status() const
{
    return priv->status;
}

bool LibAVFilter::pushVideoFrame(Frame *frame, bool changed)
{
    return priv->pushVideoFrame(frame, changed, sourceArguments());
}

bool LibAVFilter::pushAudioFrame(Frame *frame, bool changed)
{
    return priv->pushAudioFrame(frame, changed, sourceArguments());
}

void* LibAVFilter::pullFrameHolder()
{
#if QTAV_HAVE(AVFILTER)
    AVFrameHolder *holder = NULL;
    holder = new AVFrameHolder();
#if QTAV_HAVE_av_buffersink_get_frame
    int ret = av_buffersink_get_frame(priv->out_filter_ctx, holder->frame());
#else
    int ret = av_buffersink_read(priv->out_filter_ctx, holder->bufferRef());
#endif //QTAV_HAVE_av_buffersink_get_frame
    if (ret < 0) {
        qWarning("av_buffersink_get_frame error: %s", av_err2str(ret));
        delete holder;
        return 0;
    }
#if !QTAV_HAVE_av_buffersink_get_frame
    holder->copyBufferToFrame();
#endif
    return holder;
#endif //QTAV_HAVE(AVFILTER)
    return 0;
}

QStringList LibAVFilter::registeredFilters(int type)
{
    QStringList filters;
#if QTAV_HAVE(AVFILTER)
    avfilter_register_all();
    const AVFilter* f = NULL;
    AVFilterPad* fp = NULL; // no const in avfilter_pad_get_name() for ffmpeg<=1.2 libav<=9
#if AV_MODULE_CHECK(LIBAVFILTER, 3, 8, 0, 53, 100)
    while ((f = avfilter_next(f))) {
#else
    AVFilter** ff = NULL;
    while ((ff = av_filter_next(ff)) && *ff) {
        f = (*ff);
#endif
        fp = (AVFilterPad*)f->inputs;
        // only check the 1st pad
        if (!fp || !avfilter_pad_get_name(fp, 0) || avfilter_pad_get_type(fp, 0) != (AVMediaType)type)
            continue;
        fp = (AVFilterPad*)f->outputs;
        // only check the 1st pad
        if (!fp || !avfilter_pad_get_name(fp, 0) || avfilter_pad_get_type(fp, 0) != (AVMediaType)type)
            continue;
        filters.append(QLatin1String(f->name));
    }
#endif //QTAV_HAVE(AVFILTER)
    return filters;
}

class LibAVFilterVideoPrivate : public VideoFilterPrivate
{
public:
    LibAVFilterVideoPrivate()
        : VideoFilterPrivate()
        , pixfmt(QTAV_PIX_FMT_C(NONE))
        , width(0)
        , height(0)
    {}
    AVPixelFormat pixfmt;
    int width, height;
};

LibAVFilterVideo::LibAVFilterVideo(QObject *parent)
    : VideoFilter(*new LibAVFilterVideoPrivate(), parent)
    , LibAVFilter()
{
}

QStringList LibAVFilterVideo::filters() const
{
    return LibAVFilter::videoFilters();
}

void LibAVFilterVideo::process(Statistics *statistics, VideoFrame *frame)
{
    Q_UNUSED(statistics);
#if QTAV_HAVE(AVFILTER)
    if (status() == ConfigureFailed)
        return;
    DPTR_D(LibAVFilterVideo);
    //Status old = status();
    bool changed = false;
    if (d.width != frame->width() || d.height != frame->height() || d.pixfmt != frame->pixelFormatFFmpeg()) {
        changed = true;
        d.width = frame->width();
        d.height = frame->height();
        d.pixfmt = (AVPixelFormat)frame->pixelFormatFFmpeg();
    }
    bool ok = pushVideoFrame(frame, changed);
    //if (old != status())
      //  emit statusChanged();
    if (!ok)
        return;

    AVFrameHolderRef ref((AVFrameHolder*)pullFrameHolder());
    if (!ref)
        return;
    const AVFrame *f = ref->frame();
    VideoFrame vf(f->width, f->height, VideoFormat(f->format));
    vf.setBits((quint8**)f->data);
    vf.setBytesPerLine((int*)f->linesize);
    vf.setMetaData(QStringLiteral("avframe_hoder_ref"), QVariant::fromValue(ref));
    vf.setTimestamp(ref->frame()->pts/1000000.0); //pkt_pts?
    //vf.setMetaData(frame->availableMetaData());
    *frame = vf;
#else
    Q_UNUSED(frame);
#endif //QTAV_HAVE(AVFILTER)
}

QString LibAVFilterVideo::sourceArguments() const
{
    DPTR_D(const LibAVFilterVideo);
#if QTAV_USE_LIBAV(LIBAVFILTER)
    return QStringLiteral("%1:%2:%3:%4:%5:%6:%7")
#else
    return QStringLiteral("video_size=%1x%2:pix_fmt=%3:time_base=%4/%5:pixel_aspect=%6/%7")
#endif
            .arg(d.width).arg(d.height).arg(d.pixfmt)
            .arg(1).arg(AV_TIME_BASE) //time base 1/1?
            .arg(1).arg(1) //sar
            ;
}

class LibAVFilterAudioPrivate : public AudioFilterPrivate
{
public:
    LibAVFilterAudioPrivate()
        : AudioFilterPrivate()
        , sample_rate(0)
        , sample_fmt(AV_SAMPLE_FMT_NONE)
        , channel_layout(0)
    {}
    int sample_rate;
    AVSampleFormat sample_fmt;
    qint64 channel_layout;
};

LibAVFilterAudio::LibAVFilterAudio(QObject *parent)
    : AudioFilter(*new LibAVFilterAudioPrivate(), parent)
    , LibAVFilter()
{}

QStringList LibAVFilterAudio::filters() const
{
    return LibAVFilter::audioFilters();
}

QString LibAVFilterAudio::sourceArguments() const
{
    DPTR_D(const LibAVFilterAudio);
    return QStringLiteral("time_base=%1/%2:sample_rate=%3:sample_fmt=%4:channel_layout=0x%5")
            .arg(1)
            .arg(AV_TIME_BASE)
            .arg(d.sample_rate)
            //ffmpeg new: AV_OPT_TYPE_SAMPLE_FMT
            //libav, ffmpeg old: AV_OPT_TYPE_STRING
            .arg(QLatin1String(av_get_sample_fmt_name(d.sample_fmt)))
            .arg(d.channel_layout, 0, 16) //AV_OPT_TYPE_STRING
            ;
}

void LibAVFilterAudio::process(Statistics *statistics, AudioFrame *frame)
{
    Q_UNUSED(statistics);
#if QTAV_HAVE(AVFILTER)
    if (status() == ConfigureFailed)
        return;
    DPTR_D(LibAVFilterAudio);
    //Status old = status();
    bool changed = false;
    const AudioFormat afmt(frame->format());
    if (d.sample_rate != afmt.sampleRate() || d.sample_fmt != afmt.sampleFormatFFmpeg() || d.channel_layout != afmt.channelLayoutFFmpeg()) {
        changed = true;
        d.sample_rate = afmt.sampleRate();
        d.sample_fmt = (AVSampleFormat)afmt.sampleFormatFFmpeg();
        d.channel_layout = afmt.channelLayoutFFmpeg();
    }
    bool ok = pushAudioFrame(frame, changed);
    //if (old != status())
      //  emit statusChanged();
    if (!ok)
        return;

    AVFrameHolderRef ref((AVFrameHolder*)pullFrameHolder());
    if (!ref)
        return;
    const AVFrame *f = ref->frame();
    AudioFormat fmt;
    fmt.setSampleFormatFFmpeg(f->format);
    fmt.setChannelLayoutFFmpeg(f->channel_layout);
    fmt.setSampleRate(f->sample_rate);
    if (!fmt.isValid()) {// need more data to decode to get a frame
        return;
    }
    AudioFrame af(fmt);
    //af.setBits((quint8**)f->extended_data);
    //af.setBytesPerLine((int*)f->linesize);
    af.setBits(f->extended_data); // TODO: ref
    af.setBytesPerLine(f->linesize[0], 0); // for correct alignment
    af.setSamplesPerChannel(f->nb_samples);
    af.setMetaData(QStringLiteral("avframe_hoder_ref"), QVariant::fromValue(ref));
    af.setTimestamp(ref->frame()->pts/1000000.0); //pkt_pts?
    //af.setMetaData(frame->availableMetaData());
    *frame = af;
#else
    Q_UNUSED(frame);
#endif //QTAV_HAVE(AVFILTER)
}

bool LibAVFilter::Private::pushVideoFrame(Frame *frame, bool changed, const QString &args)
{
#if QTAV_HAVE(AVFILTER)
    VideoFrame *vf = static_cast<VideoFrame*>(frame);
    if (status == LibAVFilter::NotConfigured || !avframe || changed) {
        if (!setup(args, true)) {
            qWarning("setup video filter graph error");
            //enabled = false; // skip this filter and avoid crash
            return false;
        }
    }
    if (!vf->constBits(0)) {
        *vf = vf->to(vf->format());
    }
    avframe->pts = frame->timestamp() * 1000000.0; // time_base is 1/1000000
    avframe->width = vf->width();
    avframe->height = vf->height();
    avframe->format = (AVPixelFormat)vf->pixelFormatFFmpeg();
    for (int i = 0; i < vf->planeCount(); ++i) {
        avframe->data[i] = (uint8_t*)vf->constBits(i);
        avframe->linesize[i] = vf->bytesPerLine(i);
    }
    //TODO: side data for vf_codecview etc
    //int ret = av_buffersrc_add_frame_flags(in_filter_ctx, avframe, AV_BUFFERSRC_FLAG_KEEP_REF);
    /*
     * av_buffersrc_write_frame equals to av_buffersrc_add_frame_flags with AV_BUFFERSRC_FLAG_KEEP_REF.
     * av_buffersrc_write_frame is more compatible, while av_buffersrc_add_frame_flags only exists in ffmpeg >=2.0
     * add a ref if frame is ref counted
     * TODO: libav < 10.0 will copy the frame, prefer to use av_buffersrc_buffer
     */
    AV_ENSURE_OK(av_buffersrc_write_frame(in_filter_ctx, avframe), false);
    return true;
#endif //QTAV_HAVE(AVFILTER)
    Q_UNUSED(frame);
    return false;
}


bool LibAVFilter::Private::pushAudioFrame(Frame *frame, bool changed, const QString &args)
{
#if QTAV_HAVE(AVFILTER)
    if (status == LibAVFilter::NotConfigured || !avframe || changed) {
        if (!setup(args, false)) {
            qWarning("setup audio filter graph error");
            //enabled = false; // skip this filter and avoid crash
            return false;
        }
    }
    AudioFrame *af = static_cast<AudioFrame*>(frame);
    const AudioFormat afmt(af->format());
    avframe->pts = frame->timestamp() * 1000000.0; // time_base is 1/1000000
    avframe->sample_rate = afmt.sampleRate();
    avframe->channel_layout = afmt.channelLayoutFFmpeg();
#if QTAV_USE_FFMPEG(LIBAVCODEC) || QTAV_USE_FFMPEG(LIBAVUTIL) //AVFrame was in avcodec
    avframe->channels = afmt.channels(); //MUST set because av_buffersrc_write_frame will compare channels and layout
#endif
    avframe->format = (AVSampleFormat)afmt.sampleFormatFFmpeg();
    avframe->nb_samples = af->samplesPerChannel();
    for (int i = 0; i < af->planeCount(); ++i) {
        //avframe->data[i] = (uint8_t*)af->constBits(i);
        avframe->extended_data[i] = (uint8_t*)af->constBits(i);
        avframe->linesize[i] = af->bytesPerLine(i);
    }
    AV_ENSURE_OK(av_buffersrc_write_frame(in_filter_ctx, avframe), false);
    return true;
#endif //QTAV_HAVE(AVFILTER)
    Q_UNUSED(frame);
    return false;
}

} //namespace QtAV

#if QTAV_HAVE(AVFILTER)
Q_DECLARE_METATYPE(QtAV::AVFrameHolderRef)
#endif
