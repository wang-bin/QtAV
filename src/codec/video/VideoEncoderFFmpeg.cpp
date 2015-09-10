/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2015 Wang Bin <wbsecg1@gmail.com>

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

#include "QtAV/VideoEncoder.h"
#include "QtAV/private/AVEncoder_p.h"
#include "QtAV/private/AVCompat.h"
#include "QtAV/private/mkid.h"
#include "QtAV/private/prepost.h"
#include "QtAV/version.h"
#include "utils/Logger.h"

/*!
 * options (properties) are from libavcodec/options_table.h
 * enum name here must convert to lower case to fit the names in avcodec. done in AVEncoder.setOptions()
 * Don't use lower case here because the value name may be "default" in avcodec which is a keyword of C++
 */

namespace QtAV {

class VideoEncoderFFmpegPrivate;
class VideoEncoderFFmpeg Q_DECL_FINAL: public VideoEncoder
{
    DPTR_DECLARE_PRIVATE(VideoEncoderFFmpeg)
public:
    VideoEncoderFFmpeg();
    VideoEncoderId id() const Q_DECL_OVERRIDE;
    bool encode(const VideoFrame &frame = VideoFrame()) Q_DECL_OVERRIDE;
};

static const VideoEncoderId VideoEncoderId_FFmpeg = mkid::id32base36_6<'F', 'F', 'm', 'p', 'e', 'g'>::value;
FACTORY_REGISTER_ID_AUTO(VideoEncoder, FFmpeg, "FFmpeg")

void RegisterVideoEncoderFFmpeg_Man()
{
    FACTORY_REGISTER_ID_MAN(VideoEncoder, FFmpeg, "FFmpeg")
}

class VideoEncoderFFmpegPrivate Q_DECL_FINAL: public VideoEncoderPrivate
{
public:
    VideoEncoderFFmpegPrivate()
        : VideoEncoderPrivate()
        , nb_encoded(0)
    {
        avcodec_register_all();
        // NULL: codec-specific defaults won't be initialized, which may result in suboptimal default settings (this is important mainly for encoders, e.g. libx264).
        avctx = avcodec_alloc_context3(NULL);
    }
    bool open() Q_DECL_OVERRIDE;
    bool close() Q_DECL_OVERRIDE;

    qint64 nb_encoded;
    QByteArray buffer;
};

bool VideoEncoderFFmpegPrivate::open()
{
    nb_encoded = 0LL;
    if (codec_name.isEmpty()) {
        // copy ctx from muxer by copyAVCodecContext
        AVCodec *codec = avcodec_find_encoder(avctx->codec_id);
        AV_ENSURE_OK(avcodec_open2(avctx, codec, &dict), false);
        return true;
    }
    AVCodec *codec = avcodec_find_encoder_by_name(codec_name.toUtf8().constData());
    if (!codec) {
        const AVCodecDescriptor* cd = avcodec_descriptor_get_by_name(codec_name.toUtf8().constData());
        if (cd) {
            codec = avcodec_find_encoder(cd->id);
        }
    }
    if (!codec) {
        qWarning() << "Can not find encoder for codec " << codec_name;
        return false;
    }
    if (avctx) {
        avcodec_free_context(&avctx);
        avctx = 0;
    }
    avctx = avcodec_alloc_context3(codec);
    avctx->width = width; // coded_width works, why?
    avctx->height = height;
    // reset format_used to user defined format. important to update default format if format is invalid
    format_used = format.pixelFormat();
    if (format.pixelFormat() == VideoFormat::Format_Invalid) {
        if (codec->pix_fmts) {
            qDebug("use first supported pixel format: %d", codec->pix_fmts[0]);
            format_used = VideoFormat::pixelFormatFromFFmpeg((int)codec->pix_fmts[0]);
        } else {
            qWarning("pixel format and supported pixel format are not set. use yuv420p");
            format_used = VideoFormat::Format_YUV420P;
        }
    }
    //avctx->sample_aspect_ratio =
    avctx->pix_fmt = (AVPixelFormat)VideoFormat::pixelFormatToFFmpeg(format_used);
    if (frame_rate > 0)
        avctx->time_base = av_d2q(1.0/frame_rate, frame_rate*1001.0+2);
    else
        avctx->time_base = av_d2q(1.0/VideoEncoder::defaultFrameRate(), VideoEncoder::defaultFrameRate()*1001.0+2);
    qDebug("size: %dx%d tbc: %f=%d/%d", width, height, av_q2d(avctx->time_base), avctx->time_base.num, avctx->time_base.den);
    avctx->bit_rate = bit_rate;
#if 1
    //AVDictionary *dict = 0;
    if(avctx->codec_id == QTAV_CODEC_ID(H264)) {
        avctx->gop_size = 10;
        //avctx->max_b_frames = 3;//h264
        av_dict_set(&dict, "preset", "fast", 0);
        av_dict_set(&dict, "tune", "zerolatency", 0);
        av_dict_set(&dict, "profile", "main", 0);
    }
#ifdef FF_PROFILE_HEVC_MAIN
    if(avctx->codec_id == AV_CODEC_ID_HEVC){
        av_dict_set(&dict, "preset", "ultrafast", 0);
        av_dict_set(&dict, "tune", "zero-latency", 0);
    }
#endif //FF_PROFILE_HEVC_MAIN
#endif
    applyOptionsForContext();
    AV_ENSURE_OK(avcodec_open2(avctx, codec, &dict), false);
    // from mpv ao_lavc
    const int buffer_size = qMax<int>(qMax<int>(width*height*6+200, FF_MIN_BUFFER_SIZE), sizeof(AVPicture));//??
    buffer.resize(buffer_size);
    return true;
}

bool VideoEncoderFFmpegPrivate::close()
{
    AV_ENSURE_OK(avcodec_close(avctx), false);
    return true;
}


VideoEncoderFFmpeg::VideoEncoderFFmpeg()
    : VideoEncoder(*new VideoEncoderFFmpegPrivate())
{
}

VideoEncoderId VideoEncoderFFmpeg::id() const
{
    return VideoEncoderId_FFmpeg;
}

bool VideoEncoderFFmpeg::encode(const VideoFrame &frame)
{
    DPTR_D(VideoEncoderFFmpeg);
    AVFrame *f = NULL;
    if (frame.isValid()) {
        f = av_frame_alloc();
        f->format = frame.format().pixelFormatFFmpeg();
        f->width = frame.width();
        f->height = frame.height();
//        f->quality = d.avctx->global_quality;
        switch (timestampMode()) {
        case TimestampCopy:
            f->pts = int64_t(frame.timestamp()*frameRate()); // TODO: check monotically increase and fix if not. or another mode?
            break;
        case TimestampMonotonic:
            f->pts = d.nb_encoded+1;
            break;
        default:
            break;
        }
        // pts is set in muxer
        const int nb_planes = frame.planeCount();
        for (int i = 0; i < nb_planes; ++i) {
            f->linesize[i] = frame.bytesPerLine(i);
            f->data[i] = (uint8_t*)frame.constBits(i);
        }
        if (d.avctx->width <= 0) {
            d.avctx->width = frame.width();
        }
        if (d.avctx->height <= 0) {
            d.avctx->height = frame.width();
        }
    }
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = (uint8_t*)d.buffer.constData();
    pkt.size = d.buffer.size();
    int got_packet = 0;
    int ret = avcodec_encode_video2(d.avctx, &pkt, f, &got_packet);
    av_frame_free(&f);
    if (ret < 0) {
        qWarning("error avcodec_encode_video2: %s" ,av_err2str(ret));
        return false; //false
    }
    d.nb_encoded++;
    if (!got_packet) {
        qWarning("no packet got");
        return false; //false
    }
   // qDebug("enc avpkt.pts: %lld, dts: %lld.", pkt.pts, pkt.dts);
    d.packet = Packet::fromAVPacket(&pkt, av_q2d(d.avctx->time_base));
   // qDebug("enc packet.pts: %.3f, dts: %.3f.", d.packet.pts, d.packet.dts);
    return true;
}

} //namespace QtAV
