/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013-2014 Wang Bin <wbsecg1@gmail.com>

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

#include "QtAV/VideoDecoder.h"
#include "QtAV/private/VideoDecoder_p.h"
#include "QtAV/Packet.h"
#include "QtAV/private/AVCompat.h"
#include "QtAV/private/prepost.h"
#include "utils/Logger.h"

/*!
 * options (properties) are from libavcodec/options_table.h
 * enum name here must convert to lower case to fit the names in avcodec. done in AVDecoder.setOptions()
 * Don't use lower case here because the value name may be "default" in avcodec which is a keyword of C++
 */

namespace QtAV {

class VideoDecoderFFmpegPrivate;
class VideoDecoderFFmpeg : public VideoDecoder
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(VideoDecoderFFmpeg)
    Q_PROPERTY(DiscardType skip_loop_filter READ skipLoopFilter WRITE setSkipLoopFilter)
    Q_PROPERTY(DiscardType skip_idct READ skipIDCT WRITE setSkipIDCT)
    // Force a strict standard compliance when encoding (accepted values: -2 to 2)
    //Q_PROPERTY(StrictType strict READ strict WRITE setStrict)
    Q_PROPERTY(DiscardType skip_frame READ skipFrame WRITE setSkipFrame)
    Q_PROPERTY(int threads READ threads WRITE setThreads) // 0 is auto
    Q_PROPERTY(ThreadFlags thread_type READ threadFlags WRITE setThreadFlags)
    Q_PROPERTY(MotionVectorVisFlags vismv READ motionVectorVisFlags WRITE setMotionVectorVisFlags)
    //Q_PROPERTY(BugFlags bug READ bugFlags WRITE setBugFlags)
    Q_ENUMS(StrictType)
    Q_ENUMS(DiscardType)
    Q_ENUMS(ThreadFlag)
    Q_FLAGS(ThreadFlags)
    Q_ENUMS(MotionVectorVisFlag)
    Q_FLAGS(MotionVectorVisFlags)
    Q_ENUMS(BugFlag)
    Q_FLAGS(BugFlags)
public:
    enum StrictType {
        Very = FF_COMPLIANCE_VERY_STRICT,
        Strict = FF_COMPLIANCE_STRICT,
        Normal = FF_COMPLIANCE_NORMAL, //default
        Unofficial = FF_COMPLIANCE_UNOFFICIAL,
        Experimental = FF_COMPLIANCE_EXPERIMENTAL
    };

    enum DiscardType { // TODO: discard_type
        None = AVDISCARD_NONE,
        Default = AVDISCARD_DEFAULT, //default
        NoRef = AVDISCARD_NONREF,
        Bidir = AVDISCARD_BIDIR,
        NoKey = AVDISCARD_NONKEY,
        All = AVDISCARD_ALL
    };

    enum ThreadFlag {
        DefaultType = FF_THREAD_SLICE | FF_THREAD_FRAME,//default
        Slice = FF_THREAD_SLICE,
        Frame = FF_THREAD_FRAME
    };
    Q_DECLARE_FLAGS(ThreadFlags, ThreadFlag)
    // flags. visualize motion vectors (MVs)
    enum MotionVectorVisFlag {
        No = 0, //default
        PF = FF_DEBUG_VIS_MV_P_FOR,
        BF = FF_DEBUG_VIS_MV_B_FOR,
        BB = FF_DEBUG_VIS_MV_B_BACK
    };
    Q_DECLARE_FLAGS(MotionVectorVisFlags, MotionVectorVisFlag)
    enum BugFlag {
        autodetect = FF_BUG_AUTODETECT, //default
#if FF_API_OLD_MSMPEG4
        //old_msmpeg4 = FF_BUG_OLD_MSMPEG4, //moc does not support PP?
#endif
        xvid_ilace = FF_BUG_XVID_ILACE,
        ump4 = FF_BUG_UMP4,
        no_padding = FF_BUG_NO_PADDING,
        amv = FF_BUG_AMV,
#if FF_API_AC_VLC
        //ac_vlc = FF_BUG_AC_VLC, //moc does not support PP?
#endif
        qpel_chroma = FF_BUG_QPEL_CHROMA,
        std_qpel = FF_BUG_QPEL_CHROMA2,
        direct_blocksize = FF_BUG_DIRECT_BLOCKSIZE,
        edge = FF_BUG_EDGE,
        hpel_chroma = FF_BUG_HPEL_CHROMA,
        dc_clip = FF_BUG_DC_CLIP,
        ms = FF_BUG_MS,
        trunc = FF_BUG_TRUNCATED
    };
    Q_DECLARE_FLAGS(BugFlags, BugFlag)

    VideoDecoderFFmpeg();
    virtual VideoDecoderId id() const;
    virtual bool prepare() Q_DECL_OVERRIDE;

    // TODO: av_opt_set in setter
    void setSkipLoopFilter(DiscardType value);
    DiscardType skipLoopFilter() const;
    void setSkipIDCT(DiscardType value);
    DiscardType skipIDCT() const;
    void setStrict(StrictType value);
    StrictType strict() const;
    void setSkipFrame(DiscardType value);
    DiscardType skipFrame() const;
    void setThreads(int value);
    int threads() const;
    void setThreadFlags(ThreadFlags value);
    ThreadFlags threadFlags() const;
    void setMotionVectorVisFlags(MotionVectorVisFlags value);
    MotionVectorVisFlags motionVectorVisFlags() const;
    void setBugFlags(BugFlags value);
    BugFlags bugFlags() const;
};

extern VideoDecoderId VideoDecoderId_FFmpeg;
FACTORY_REGISTER_ID_AUTO(VideoDecoder, FFmpeg, "FFmpeg")

void RegisterVideoDecoderFFmpeg_Man()
{
    FACTORY_REGISTER_ID_MAN(VideoDecoder, FFmpeg, "FFmpeg")
}


class VideoDecoderFFmpegPrivate : public VideoDecoderPrivate
{
public:
    VideoDecoderFFmpegPrivate():
        VideoDecoderPrivate()
      , skip_loop_filter(VideoDecoderFFmpeg::Default)
      , skip_idct(VideoDecoderFFmpeg::Default)
      , strict(VideoDecoderFFmpeg::Normal)
      , skip_frame(VideoDecoderFFmpeg::Default)
      , thread_type(VideoDecoderFFmpeg::DefaultType)
      , threads(0)
      , debug_mv(VideoDecoderFFmpeg::No)
      , bug(VideoDecoderFFmpeg::autodetect)
    {}

    int skip_loop_filter;
    int skip_idct;
    int strict;
    int skip_frame;
    int thread_type;
    int threads;
    int debug_mv;
    int bug;
};


VideoDecoderFFmpeg::VideoDecoderFFmpeg():
    VideoDecoder(*new VideoDecoderFFmpegPrivate())
{
    // dynamic properties about static property details. used by UI
    // format: detail_property
    setProperty("detail_skip_loop_filter", tr("Skipping the loop filter (aka deblocking) usually has determinal effect on quality. However it provides a big speedup for hi definition streams"));
    // like skip_frame
    setProperty("detail_skip_idct", tr("Force skipping of idct to speed up decoding for frame types (-1=None, "
                                       "0=Default, 1=B-frames, 2=P-frames, 3=B+P frames, 4=all frames)"));
    setProperty("detail_skip_frame", tr("Force skipping frames for speed up decoding."));
}

VideoDecoderId VideoDecoderFFmpeg::id() const
{
    return VideoDecoderId_FFmpeg;
}

bool VideoDecoderFFmpeg::prepare()
{
    DPTR_D(VideoDecoderFFmpeg);
    if (!d.codec_ctx) {
        qWarning("call this after AVCodecContext is set!");
        return false;
    }
    av_opt_set_int(d.codec_ctx, "skip_loop_filter", (int64_t)skipLoopFilter(), 0);
    av_opt_set_int(d.codec_ctx, "skip_idct", (int64_t)skipIDCT(), 0);
    av_opt_set_int(d.codec_ctx, "strict", (int64_t)strict(), 0);
    av_opt_set_int(d.codec_ctx, "skip_frame", (int64_t)skipFrame(), 0);
    av_opt_set_int(d.codec_ctx, "threads", (int64_t)threads(), 0);
    av_opt_set_int(d.codec_ctx, "thread_type", (int64_t)threadFlags(), 0);
    av_opt_set_int(d.codec_ctx, "vismv", (int64_t)motionVectorVisFlags(), 0);
    av_opt_set_int(d.codec_ctx, "bug", (int64_t)bugFlags(), 0);

    //CODEC_FLAG_EMU_EDGE: deprecated in ffmpeg >=? & libav>=10. always set by ffmpeg
#if 0
    if (d.fast) {
        d.codec_ctx->flags2 |= CODEC_FLAG2_FAST; // TODO:
    } else {
        //d.codec_ctx->flags2 &= ~CODEC_FLAG2_FAST; //ffplay has no this
    }
// lavfilter
    //d.codec_ctx->slice_flags |= SLICE_FLAG_ALLOW_FIELD; //lavfilter
    //d.codec_ctx->strict_std_compliance = FF_COMPLIANCE_STRICT;
    d.codec_ctx->thread_safe_callbacks = true;
    switch (d.codec_ctx->codec_id) {
        case QTAV_CODEC_ID(MPEG4):
        case QTAV_CODEC_ID(H263):
            d.codec_ctx->thread_type = 0;
            break;
        case QTAV_CODEC_ID(MPEG1VIDEO):
        case QTAV_CODEC_ID(MPEG2VIDEO):
            d.codec_ctx->thread_type &= ~FF_THREAD_SLICE;
            /* fall through */
# if (LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55, 1, 0))
        case QTAV_CODEC_ID(H264):
        case QTAV_CODEC_ID(VC1):
        case QTAV_CODEC_ID(WMV3):
            d.codec_ctx->thread_type &= ~FF_THREAD_FRAME;
# endif
        default:
            break;
    }
#endif
    return true;
}

void VideoDecoderFFmpeg::setSkipLoopFilter(DiscardType value)
{
    DPTR_D(VideoDecoderFFmpeg);
    d.skip_loop_filter = value;
    if (d.codec_ctx)
        av_opt_set_int(d.codec_ctx, "skip_loop_filter", (int64_t)value, 0);
}

VideoDecoderFFmpeg::DiscardType VideoDecoderFFmpeg::skipLoopFilter() const
{
    return (DiscardType)d_func().skip_loop_filter;
}

void VideoDecoderFFmpeg::setSkipIDCT(DiscardType value)
{
    DPTR_D(VideoDecoderFFmpeg);
    d.skip_idct = (int)value;
    if (d.codec_ctx)
        av_opt_set_int(d.codec_ctx, "skip_idct", (int64_t)value, 0);
}

VideoDecoderFFmpeg::DiscardType VideoDecoderFFmpeg::skipIDCT() const
{
    return (DiscardType)d_func().skip_idct;
}

void VideoDecoderFFmpeg::setStrict(StrictType value)
{
    DPTR_D(VideoDecoderFFmpeg);
    d.strict = (int)value;
    if (d.codec_ctx)
        av_opt_set_int(d.codec_ctx, "strict", int64_t(value), 0);
}

VideoDecoderFFmpeg::StrictType VideoDecoderFFmpeg::strict() const
{
    return (StrictType)d_func().strict;
}

void VideoDecoderFFmpeg::setSkipFrame(DiscardType value)
{
    DPTR_D(VideoDecoderFFmpeg);
    d.skip_frame = (int)value;
    if (d.codec_ctx)
        av_opt_set_int(d.codec_ctx, "skip_frame", (int64_t)value, 0);
}

VideoDecoderFFmpeg::DiscardType VideoDecoderFFmpeg::skipFrame() const
{
    return (DiscardType)d_func().skip_frame;
}

void VideoDecoderFFmpeg::setThreads(int value)
{
    DPTR_D(VideoDecoderFFmpeg);
    d.threads = value;
    if (d.codec_ctx)
        av_opt_set_int(d.codec_ctx, "threads", (int64_t)value, 0);
}

int VideoDecoderFFmpeg::threads() const
{
    return d_func().threads;
}

void VideoDecoderFFmpeg::setThreadFlags(ThreadFlags value)
{
    DPTR_D(VideoDecoderFFmpeg);
    d.thread_type = (int)value;
    if (d.codec_ctx)
        av_opt_set_int(d.codec_ctx, "thread_type", (int64_t)value, 0);
}

VideoDecoderFFmpeg::ThreadFlags VideoDecoderFFmpeg::threadFlags() const
{
    return (ThreadFlags)d_func().thread_type;
}

void VideoDecoderFFmpeg::setMotionVectorVisFlags(MotionVectorVisFlags value)
{
    DPTR_D(VideoDecoderFFmpeg);
    d.debug_mv = (int)value;
    if (d.codec_ctx)
        av_opt_set_int(d.codec_ctx, "vismv", (int64_t)value, 0);
}

VideoDecoderFFmpeg::MotionVectorVisFlags VideoDecoderFFmpeg::motionVectorVisFlags() const
{
    return (MotionVectorVisFlags)d_func().debug_mv;
}

void VideoDecoderFFmpeg::setBugFlags(BugFlags value)
{
    DPTR_D(VideoDecoderFFmpeg);
    d.bug = (int)value;
    if (d.codec_ctx)
        av_opt_set_int(d.codec_ctx, "bug", (int64_t)value, 0);
}

VideoDecoderFFmpeg::BugFlags VideoDecoderFFmpeg::bugFlags() const
{
    return (BugFlags)d_func().bug;
}

namespace {
void i18n() {
    QObject::tr("skip_loop_filter");
    QObject::tr("skip_idct");
    QObject::tr("strict");
    QObject::tr("skip_frame");
    QObject::tr("threads");
    QObject::tr("thread_type");
    QObject::tr("vismv");
    QObject::tr("bug");
}
}
} //namespace QtAV

#include "VideoDecoderFFmpeg.moc"
