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
#include <QtAV/Packet.h>
#include <QtAV/QtAV_Compat.h>
#include "QtAV/prepost.h"

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
    Q_PROPERTY(bool skip_loop_filter READ skipLoopFilter WRITE setSkipLoopFilter)
    Q_PROPERTY(bool skip_idct READ skipIDCT WRITE setSkipIDCT)
    Q_PROPERTY(StrictType strict READ strict WRITE setStrict)
    Q_PROPERTY(SkipFrameType skip_frame READ skipFrameType WRITE setSkipFrameType)
    Q_PROPERTY(int threads READ threads WRITE setThreads) // 0 is auto
    Q_PROPERTY(ThreadFlags thread_type READ threadFlags WRITE setThreadFlags)
    Q_PROPERTY(MotionVectorVisFlags vismv READ motionVectorVisFlags WRITE setMotionVectorVisFlags)
    Q_PROPERTY(BugFlags bug READ bugFlags WRITE setBugFlags)
    Q_ENUMS(StrictType)
    Q_ENUMS(SkipFrameType)
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

    enum SkipFrameType {
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
    //virtual bool prepare();

    // TODO: av_opt_set in setter
    void setSkipLoopFilter(bool y);
    bool skipLoopFilter() const;
    void setSkipIDCT(bool y);
    bool skipIDCT() const;
    void setStrict(StrictType s);
    StrictType strict() const;
    void setSkipFrameType(SkipFrameType s);
    SkipFrameType skipFrameType() const;
    void setThreads(int t);
    int threads() const;
    void setThreadFlags(ThreadFlags tt);
    ThreadFlags threadFlags() const;
    void setMotionVectorVisFlags(MotionVectorVisFlags mv);
    MotionVectorVisFlags motionVectorVisFlags() const;
    void setBugFlags(BugFlags b);
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
      , skip_loop_filter(AVDISCARD_DEFAULT)
      , skip_idct(AVDISCARD_DEFAULT)
      , strict(VideoDecoderFFmpeg::Normal)
      , skip_frame(VideoDecoderFFmpeg::Default)
      , thread_type(VideoDecoderFFmpeg::DefaultType)
      , threads(0)
      , debug_mv(VideoDecoderFFmpeg::No)
      , bug(VideoDecoderFFmpeg::autodetect)
    {}

    bool skip_loop_filter;
    bool skip_idct;
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
}

VideoDecoderId VideoDecoderFFmpeg::id() const
{
    return VideoDecoderId_FFmpeg;
}

void VideoDecoderFFmpeg::setSkipLoopFilter(bool y)
{
    DPTR_D(VideoDecoderFFmpeg);
    d.skip_loop_filter = y;
    //av_opt_set_int
}

bool VideoDecoderFFmpeg::skipLoopFilter() const
{
    return d_func().skip_loop_filter;
}

void VideoDecoderFFmpeg::setSkipIDCT(bool y)
{
    DPTR_D(VideoDecoderFFmpeg);
    d.skip_idct = y;
    //av_opt_set_int
}

bool VideoDecoderFFmpeg::skipIDCT() const
{
    return d_func().skip_idct;
}

void VideoDecoderFFmpeg::setStrict(StrictType s)
{
    DPTR_D(VideoDecoderFFmpeg);
    d.strict = (int)s;
    //av_opt_set_int(d.codec_ctx, "strict", )
}

VideoDecoderFFmpeg::StrictType VideoDecoderFFmpeg::strict() const
{
    return (StrictType)d_func().strict;
}

void VideoDecoderFFmpeg::setSkipFrameType(SkipFrameType s)
{
    DPTR_D(VideoDecoderFFmpeg);
    d.skip_frame = (int)s;
    //av_opt_set_int
}

VideoDecoderFFmpeg::SkipFrameType VideoDecoderFFmpeg::skipFrameType() const
{
    return (SkipFrameType)d_func().skip_frame;
}

void VideoDecoderFFmpeg::setThreads(int t)
{
    DPTR_D(VideoDecoderFFmpeg);
    d.threads = t;
    //av_opt_set_int
}

int VideoDecoderFFmpeg::threads() const
{
    return d_func().threads;
}

void VideoDecoderFFmpeg::setThreadFlags(ThreadFlags tt)
{
    DPTR_D(VideoDecoderFFmpeg);
    d.thread_type = (int)tt;
    //av_opt_set_int
}

VideoDecoderFFmpeg::ThreadFlags VideoDecoderFFmpeg::threadFlags() const
{
    return (ThreadFlags)d_func().thread_type;
}

void VideoDecoderFFmpeg::setMotionVectorVisFlags(MotionVectorVisFlags mv)
{
    DPTR_D(VideoDecoderFFmpeg);
    d.debug_mv = (int)mv;
    //av_opt_set_int
}

VideoDecoderFFmpeg::MotionVectorVisFlags VideoDecoderFFmpeg::motionVectorVisFlags() const
{
    return (MotionVectorVisFlags)d_func().debug_mv;
}

void VideoDecoderFFmpeg::setBugFlags(BugFlags b)
{
    DPTR_D(VideoDecoderFFmpeg);
    d.bug = (int)b;
    //av_opt_set_int
}

VideoDecoderFFmpeg::BugFlags VideoDecoderFFmpeg::bugFlags() const
{
    return (BugFlags)d_func().bug;
}

} //namespace QtAV

#include "VideoDecoderFFmpeg.moc"
