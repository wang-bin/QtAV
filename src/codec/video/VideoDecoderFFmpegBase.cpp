/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2014-2015 Wang Bin <wbsecg1@gmail.com>

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

#include "VideoDecoderFFmpegBase.h"
#include "QtAV/Packet.h"
#include "utils/Logger.h"

namespace QtAV {

extern ColorSpace colorSpaceFromFFmpeg(AVColorSpace cs);

void VideoDecoderFFmpegBasePrivate::updateColorDetails(VideoFrame *f)
{
    ColorSpace cs = colorSpaceFromFFmpeg(av_frame_get_colorspace(frame));
    if (cs != ColorSpace_Unknow)
        cs = colorSpaceFromFFmpeg(codec_ctx->colorspace);
    f->setColorSpace(cs);
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
// always keep the following code
//TODO: use ipp, cuda decode and yuv functions. is sws_scale necessary?
bool VideoDecoderFFmpegBase::decode(const QByteArray &encoded)
{
    if (!isAvailable())
        return false;
    DPTR_D(VideoDecoderFFmpegBase);
    AVPacket packet;
#if NO_PADDING_DATA
    /*!
      larger than the actual read bytes because some optimized bitstream readers read 32 or 64 bits at once and could read over the end.
      The end of the input buffer avpkt->data should be set to 0 to ensure that no overreading happens for damaged MPEG streams
     */
    // auto released by av_buffer_default_free
    av_new_packet(&packet, encoded.size());
    memcpy(packet.data, encoded.data(), encoded.size());
#else
    av_init_packet(&packet);
    packet.size = encoded.size();
    packet.data = (uint8_t*)encoded.constData();
#endif //NO_PADDING_DATA
    int got_frame_ptr = 0;
    //TODO: some decoders might in addition need other fields like flags&AV_PKT_FLAG_KEY
    int ret = avcodec_decode_video2(d.codec_ctx, d.frame, &got_frame_ptr, &packet);
    //qDebug("pic_type=%c", av_get_picture_type_char(d.frame->pict_type));
    d.undecoded_size = qMin(encoded.size() - ret, encoded.size());
    //TODO: decoded format is YUV420P, YUV422P?
    av_free_packet(&packet);
    if (ret < 0) {
        qWarning("[VideoDecoderFFmpegBase] %s", av_err2str(ret));
        return false;
    }
    if (!got_frame_ptr) {
        //qWarning("no frame could be decompressed: %s", av_err2str(ret));
        return true;
    }
    if (!d.codec_ctx->width || !d.codec_ctx->height)
        return false;
    //qDebug("codec %dx%d, frame %dx%d", d.codec_ctx->width, d.codec_ctx->height, d.frame->width, d.frame->height);
    d.width = d.frame->width;
    d.height = d.frame->height;
    //avcodec_align_dimensions2(d.codec_ctx, &d.width_align, &d.height_align, aligns);
    return true;
}

bool VideoDecoderFFmpegBase::decode(const Packet &packet)
{
    if (!isAvailable())
        return false;
    DPTR_D(VideoDecoderFFmpegBase);
    // some decoders might in addition need other fields like flags&AV_PKT_FLAG_KEY
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
        qWarning("[VideoDecoderFFmpegBase] %s", av_err2str(ret));
        return false;
    }
    if (!got_frame_ptr) {
        qWarning("no frame could be decompressed: %s %d/%d", av_err2str(ret), d.undecoded_size, packet.data.size());
        return !packet.isEOF();
    }
    if (!d.codec_ctx->width || !d.codec_ctx->height)
        return false;
    //qDebug("codec %dx%d, frame %dx%d", d.codec_ctx->width, d.codec_ctx->height, d.frame->width, d.frame->height);
    d.width = d.frame->width;
    d.height = d.frame->height;
    //avcodec_align_dimensions2(d.codec_ctx, &d.width_align, &d.height_align, aligns);
    return true;
}

} //namespace QtAV
