/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2017 Wang Bin <wbsecg1@gmail.com>

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

#include "VideoDecoderFFmpegBase.h"
#include "QtAV/Packet.h"
#include "utils/Logger.h"

namespace QtAV {

extern ColorSpace colorSpaceFromFFmpeg(AVColorSpace cs);
extern ColorRange colorRangeFromFFmpeg(AVColorRange cr);

static void SetColorDetailsByFFmpeg(VideoFrame *f, AVFrame* frame, AVCodecContext* codec_ctx)
{
    ColorSpace cs = colorSpaceFromFFmpeg(av_frame_get_colorspace(frame));
    if (cs == ColorSpace_Unknown)
        cs = colorSpaceFromFFmpeg(codec_ctx->colorspace);
    f->setColorSpace(cs);
    ColorRange cr = colorRangeFromFFmpeg(av_frame_get_color_range(frame));
    if (cr == ColorRange_Unknown) {
        // check yuvj format. TODO: deprecated, check only for old ffmpeg?
        const AVPixelFormat pixfmt = (AVPixelFormat)frame->format;
        switch (pixfmt) {
        //case QTAV_PIX_FMT_C(YUVJ411P): //not in ffmpeg<2 and libav
        case QTAV_PIX_FMT_C(YUVJ420P):
        case QTAV_PIX_FMT_C(YUVJ422P):
        case QTAV_PIX_FMT_C(YUVJ440P):
        case QTAV_PIX_FMT_C(YUVJ444P):
            cr = ColorRange_Full;
            break;
        default:
            break;
        }
    }
    if (cr == ColorRange_Unknown) {
        cr = colorRangeFromFFmpeg(codec_ctx->color_range);
        if (cr == ColorRange_Unknown) {
            if (f->format().isXYZ()){
                cr = ColorRange_Full;
                cs = ColorSpace_XYZ; // not here
            } else if (!f->format().isRGB()) {
                //qDebug("prefer limited yuv range");
                cr = ColorRange_Limited;
            }
        }
    }
    f->setColorRange(cr);
}

void VideoDecoderFFmpegBasePrivate::updateColorDetails(VideoFrame *f)
{
    if (f->format().pixelFormatFFmpeg() == frame->format) {
        SetColorDetailsByFFmpeg(f, frame, codec_ctx);
        return;
    }
    // hw decoder output frame may have a different format, e.g. gl interop frame may have rgb format for rendering(stored as yuv)
    const bool rgb_frame = f->format().isRGB();
    if (rgb_frame) {
        //qDebug("rgb output frame (yuv coded)");
        f->setColorSpace(f->format().isPlanar() ? ColorSpace_GBR : ColorSpace_RGB);
        f->setColorRange(ColorRange_Full);
        return;
    }
    // yuv frame. When happens?
    const bool rgb_coded = (av_pix_fmt_desc_get(codec_ctx->pix_fmt)->flags & AV_PIX_FMT_FLAG_RGB) == AV_PIX_FMT_FLAG_RGB;
    if (rgb_coded) {
        if (f->width() >= 1280 && f->height() >= 576)
            f->setColorSpace(ColorSpace_BT709);
        else
            f->setColorSpace(ColorSpace_BT601);
        f->setColorRange(ColorRange_Limited);
    } else {
        SetColorDetailsByFFmpeg(f, frame, codec_ctx);
    }
}

qreal VideoDecoderFFmpegBasePrivate::getDAR(AVFrame *f)
{
    // lavf 54.5.100 av_guess_sample_aspect_ratio: stream.sar > frame.sar
    qreal dar = 0;
    if (f->height > 0)
        dar = (qreal)f->width/(qreal)f->height;
    // prefer sar from AVFrame if sar != 1/1
    if (f->sample_aspect_ratio.num > 1)
        dar *= av_q2d(f->sample_aspect_ratio);
    else if (codec_ctx && codec_ctx->sample_aspect_ratio.num > 1) // skip 1/1
        dar *= av_q2d(codec_ctx->sample_aspect_ratio);
    return dar;
}

VideoDecoderFFmpegBase::VideoDecoderFFmpegBase(VideoDecoderFFmpegBasePrivate &d):
    VideoDecoder(d)
{
}

bool VideoDecoderFFmpegBase::decode(const Packet &packet)
{
    if (!isAvailable())
        return false;
    DPTR_D(VideoDecoderFFmpegBase);
    // some decoders might need other fields like flags&AV_PKT_FLAG_KEY
    // const AVPacket*: ffmpeg >= 1.0. no libav
    int got_frame_ptr = 0;
    int ret = 0;
    if (packet.isEOF()) {
        AVPacket eofpkt;
        av_init_packet(&eofpkt);
        eofpkt.data = NULL;
        eofpkt.size = 0;
        ret = avcodec_decode_video2(d.codec_ctx, d.frame, &got_frame_ptr, &eofpkt);
    } else {
        ret = avcodec_decode_video2(d.codec_ctx, d.frame, &got_frame_ptr, (AVPacket*)packet.asAVPacket());
    }
    //qDebug("pic_type=%c", av_get_picture_type_char(d.frame->pict_type));
    d.undecoded_size = qMin(packet.data.size() - ret, packet.data.size());
    if (ret < 0) {
        //qWarning("[VideoDecoderFFmpegBase] %s", av_err2str(ret));
        return false;
    }
    if (!got_frame_ptr) {
        qWarning("no frame could be decompressed: %s %d/%d", av_err2str(ret), d.undecoded_size, packet.data.size());
        return !packet.isEOF();
    }
    if (!d.codec_ctx->width || !d.codec_ctx->height)
        return false;
    //qDebug("codec %dx%d, frame %dx%d", d.codec_ctx->width, d.codec_ctx->height, d.frame->width, d.frame->height);
    d.width = d.frame->width; // TODO: remove? used in hwdec
    d.height = d.frame->height;
    //avcodec_align_dimensions2(d.codec_ctx, &d.width_align, &d.height_align, aligns);
    return true;
}

VideoFrame VideoDecoderFFmpegBase::frame()
{
    DPTR_D(VideoDecoderFFmpegBase);
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
    if (frame.format().hasPalette()) {
        frame.setMetaData(QStringLiteral("pallete"), QByteArray((const char*)d.frame->data[1], 256*4));
    }
    return frame;
}
} //namespace QtAV
