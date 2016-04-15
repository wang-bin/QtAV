/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2016)

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

#include "VideoDecoderFFmpegBase.h"
#if FFMPEG_MODULE_CHECK(LIBAVCODEC, 57, 28, 100)
#define QTAV_HAVE_MEDIACODEC 1
#endif
#if QTAV_HAVE(MEDIACODEC)
#include "QtAV/private/factory.h"
#include "QtAV/private/mkid.h"
#include <QtAndroidExtras>
extern "C" {
#include <libavcodec/jni.h>
}

namespace QtAV {
class VideoDecoderMediaCodecPrivate;
class VideoDecoderMediaCodec : public VideoDecoderFFmpegBase
{
    DPTR_DECLARE_PRIVATE(VideoDecoderMediaCodec)
public:
    VideoDecoderMediaCodec();
    VideoDecoderId id() const Q_DECL_OVERRIDE;
    QString description() const Q_DECL_OVERRIDE;
    VideoFrame frame() Q_DECL_OVERRIDE;
};

extern VideoDecoderId VideoDecoderId_MediaCodec;
FACTORY_REGISTER(VideoDecoder, MediaCodec, "MediaCodec")

class VideoDecoderMediaCodecPrivate Q_DECL_FINAL : public VideoDecoderFFmpegBasePrivate
{
public:
};

VideoDecoderMediaCodec::VideoDecoderMediaCodec()
    : VideoDecoderFFmpegBase(*new VideoDecoderMediaCodecPrivate())
{
    setCodecName("h264_mediacodec");
    av_jni_set_java_vm(QAndroidJniEnvironment::javaVM(), NULL);
}

VideoDecoderId VideoDecoderMediaCodec::id() const
{
    return VideoDecoderId_MediaCodec;
}

QString VideoDecoderMediaCodec::description() const
{
    return QStringLiteral("MediaCodec");
}

VideoFrame VideoDecoderMediaCodec::frame()
{
    DPTR_D(VideoDecoderMediaCodec);
    if (d.frame->width <= 0 || d.frame->height <= 0 || !d.codec_ctx)
        return VideoFrame();
    // it's safe if width, height, pixfmt will not change, only data change
    VideoFrame frame(d.frame->width, d.frame->height, VideoFormat((int)d.codec_ctx->pix_fmt));
    frame.setDisplayAspectRatio(d.getDAR(d.frame));
    frame.setBits(d.frame->data);
    frame.setBytesPerLine(d.frame->linesize);
    // in s. TODO: what about AVFrame.pts? av_frame_get_best_effort_timestamp? move to VideoFrame::from(AVFrame*)
    frame.setTimestamp((double)d.frame->pkt_pts/1000.0);
    frame.setMetaData(QStringLiteral("avbuf"), QVariant::fromValue(AVFrameBuffersRef(new AVFrameBuffers(d.frame))));
    d.updateColorDetails(&frame);
    return frame;
}
} //namespace QtAV
#endif //QTAV_HAVE(MEDIACODEC)
