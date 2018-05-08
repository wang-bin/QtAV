/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2018 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2015)

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
#include "QtAV/private/factory.h"
#include "QtAV/version.h"
#include "utils/Logger.h"

#if AV_MODULE_CHECK(LIBAVUTIL, 55, 13, 0, 27, 100)
#define HAVE_AVHWCTX
extern "C" {
#include <libavutil/hwcontext.h>
}
#endif

/*!
 * options (properties) are from libavcodec/options_table.h
 * enum name here must convert to lower case to fit the names in avcodec. done in AVEncoder.setOptions()
 * Don't use lower case here because the value name may be "default" in avcodec which is a keyword of C++
 */

namespace QtAV {

#ifdef HAVE_AVHWCTX
struct {
    AVHWDeviceType type;
    const char* name;
} hwdevs[] = {
{AV_HWDEVICE_TYPE_VDPAU, "vdpau"},
{AV_HWDEVICE_TYPE_CUDA, "cuda"},
{AV_HWDEVICE_TYPE_VAAPI, "vaapi"},
{AV_HWDEVICE_TYPE_DXVA2, "dxva2"},
};

AVHWDeviceType fromHWAName(const char* name)
{
    for (size_t i = 0; i < sizeof(hwdevs)/sizeof(hwdevs[0]); ++i) {
        if (qstrcmp(name, hwdevs[i].name) == 0)
            return hwdevs[i].type;
    }
    return AVHWDeviceType(-1);
}
#endif // HAVE_AVHWCTX

class VideoEncoderFFmpegPrivate;
class VideoEncoderFFmpeg : public VideoEncoder
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(VideoEncoderFFmpeg)
    Q_PROPERTY(QString hwdevice READ hwDevice WRITE setHWDevice NOTIFY hwDeviceChanged)
public:
    VideoEncoderFFmpeg();
    VideoEncoderId id() const Q_DECL_OVERRIDE;
    bool encode(const VideoFrame &frame = VideoFrame()) Q_DECL_OVERRIDE;

    void setHWDevice(const QString& name);
    QString hwDevice() const;
Q_SIGNALS:
    void hwDeviceChanged();
};

static const VideoEncoderId VideoEncoderId_FFmpeg = mkid::id32base36_6<'F', 'F', 'm', 'p', 'e', 'g'>::value;
FACTORY_REGISTER(VideoEncoder, FFmpeg, "FFmpeg")

class VideoEncoderFFmpegPrivate Q_DECL_FINAL: public VideoEncoderPrivate
{
public:
    VideoEncoderFFmpegPrivate()
        : VideoEncoderPrivate()
        , nb_encoded(0)
    {
#if !AVCODEC_STATIC_REGISTER
        avcodec_register_all();
#endif
        // NULL: codec-specific defaults won't be initialized, which may result in suboptimal default settings (this is important mainly for encoders, e.g. libx264).
        avctx = avcodec_alloc_context3(NULL);
    }
    bool open() Q_DECL_OVERRIDE;
    bool close() Q_DECL_OVERRIDE;

    qint64 nb_encoded;
    QByteArray buffer;
    QString hwdev;
#ifdef HAVE_AVHWCTX
    AVBufferRef *hw_device_ctx;

    AVBufferRef *hwframes_ref;
    AVHWFramesContext *hwframes;
    QVector<AVPixelFormat> sw_fmts;
#endif
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
    format_used = VideoFormat::Format_Invalid;
    AVPixelFormat fffmt = (AVPixelFormat)format.pixelFormatFFmpeg();
    if (codec->pix_fmts && format.isValid()) {
        for (int i = 0; codec->pix_fmts[i] != AVPixelFormat(-1); ++i) {
            if (fffmt == codec->pix_fmts[i]) {
                format_used = format.pixelFormat();
                break;
            }
        }
    }
    //avctx->sample_aspect_ratio =
    AVPixelFormat hwfmt = AVPixelFormat(-1);
    if (av_pix_fmt_desc_get(codec->pix_fmts[0])->flags & AV_PIX_FMT_FLAG_HWACCEL)
        hwfmt = codec->pix_fmts[0];
    bool use_hwctx = false;
    if (hwfmt != AVPixelFormat(-1)) {
#ifdef HAVE_AVHWCTX
        const AVHWDeviceType dt = fromHWAName(codec_name.section(QChar('_'), -1).toUtf8().constData());
        if (dt != AVHWDeviceType(-1)) {
            use_hwctx = true;
            avctx->pix_fmt = hwfmt;
            hw_device_ctx = NULL;
            AV_ENSURE(av_hwdevice_ctx_create(&hw_device_ctx, dt, hwdev.toLatin1().constData(), NULL, 0), false);
            avctx->hw_frames_ctx = av_hwframe_ctx_alloc(hw_device_ctx);
            if (!avctx->hw_frames_ctx) {
                qWarning("Failed to create hw frame context for '%s'", codec_name.toLatin1().constData());
                return false;
            }
            // get sw formats
            const void *hwcfg = NULL;
            AVHWFramesConstraints *constraints = av_hwdevice_get_hwframe_constraints(hw_device_ctx, hwcfg);
            const AVPixelFormat* in_fmts = constraints->valid_sw_formats;
            AVPixelFormat sw_fmt = AVPixelFormat(-1);
            if (in_fmts) {
                sw_fmt = in_fmts[0];
                while (*in_fmts != AVPixelFormat(-1)) {
                    if (*in_fmts == fffmt)
                        sw_fmt = *in_fmts;
                    sw_fmts.append(*in_fmts);
                    ++in_fmts;
                }
            } else {
                sw_fmt = QTAV_PIX_FMT_C(YUV420P);
            }
            av_hwframe_constraints_free(&constraints);
            format_used = VideoFormat::pixelFormatFromFFmpeg(sw_fmt);
            // encoder surface pool parameters
            AVHWFramesContext* hwfs = (AVHWFramesContext*)avctx->hw_frames_ctx->data;
            hwfs->format = hwfmt; // must the same as avctx->pix_fmt
            hwfs->sw_format = sw_fmt; // if it's not set, vaapi will choose the last valid_sw_formats, but that's wrong for vaGetImage/DeriveImage. nvenc always need sw_format
            // hw upload parameters. encoder's hwframes is just for parameter checking, will never be intialized, so we allocate an individual one.
            hwframes_ref = av_hwframe_ctx_alloc(hw_device_ctx);
            if (!hwframes_ref) {
                qWarning("Failed to create hw frame context for uploading '%s'", codec_name.toLatin1().constData());
            } else {
                hwframes = (AVHWFramesContext*)hwframes_ref->data;
                hwframes->format = hwfmt;
            }
        }
#endif //HAVE_AVHWCTX
    }

    if (!use_hwctx) { // no hw device (videotoolbox, wrong device name etc.), or old ffmpeg
        // TODO: check frame is hw frame
        if (hwfmt == AVPixelFormat(-1)) { // sw enc
            if (format_used == VideoFormat::Format_Invalid) {// requested format is not supported by sw enc
                if (codec->pix_fmts) { //pix_fmts[0] is always a sw format here
                    qDebug("use first supported pixel format '%d' for sw encoder", codec->pix_fmts[0]);
                    format_used = VideoFormat::pixelFormatFromFFmpeg((int)codec->pix_fmts[0]);
                }
            }
        } else {
            if (format_used == VideoFormat::Format_Invalid) { // requested format is not supported by hw enc
                qDebug("use first supported sw pixel format '%d' for hw encoder", codec->pix_fmts[1]);
                if (codec->pix_fmts && codec->pix_fmts[1] != AVPixelFormat(-1))
                    format_used = VideoFormat::pixelFormatFromFFmpeg(codec->pix_fmts[1]);
            }
        }
        if (format_used == VideoFormat::Format_Invalid) {
            qWarning("fallback to yuv420p");
            format_used = VideoFormat::Format_YUV420P;
        }
        avctx->pix_fmt = (AVPixelFormat)VideoFormat::pixelFormatToFFmpeg(format_used);
    }
    if (frame_rate > 0)
        avctx->time_base = av_d2q(1.0/frame_rate, frame_rate*1001.0+2);
    else
        avctx->time_base = av_d2q(1.0/VideoEncoder::defaultFrameRate(), VideoEncoder::defaultFrameRate()*1001.0+2);
    qDebug("size: %dx%d tbc: %f=%d/%d", width, height, av_q2d(avctx->time_base), avctx->time_base.num, avctx->time_base.den);
    avctx->bit_rate = bit_rate;
    //AVDictionary *dict = 0;
    if(avctx->codec_id == QTAV_CODEC_ID(H264)) {
        avctx->gop_size = 10;
        //avctx->max_b_frames = 3;//h264
        av_dict_set(&dict, "preset", "fast", 0); //x264
        av_dict_set(&dict, "tune", "zerolatency", 0);  //x264
        //av_dict_set(&dict, "profile", "main", 0); // conflict with vaapi (int values)
    }
    if(avctx->codec_id == AV_CODEC_ID_HEVC){
        av_dict_set(&dict, "preset", "ultrafast", 0);
        av_dict_set(&dict, "tune", "zero-latency", 0);
    }
    if (avctx->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
        av_dict_set(&dict, "strict", "-2", 0); // mpeg2 arbitrary fps
    }
    applyOptionsForContext();
    AV_ENSURE_OK(avcodec_open2(avctx, codec, &dict), false);
    // from mpv ao_lavc
    const int buffer_size = qMax<int>(qMax<int>(width*height*6+200, AV_INPUT_BUFFER_MIN_SIZE), sizeof(AVPicture));//??
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

void VideoEncoderFFmpeg::setHWDevice(const QString &name)
{
    DPTR_D(VideoEncoderFFmpeg);
    if (d.hwdev == name)
        return;
    d.hwdev = name;
    Q_EMIT hwDeviceChanged();
}

QString VideoEncoderFFmpeg::hwDevice() const
{
    return d_func().hwdev;
}

struct ScopedAVFrameDeleter
{
    static inline void cleanup(void *pointer) {
        av_frame_free((AVFrame**)&pointer);
    }
};

bool VideoEncoderFFmpeg::encode(const VideoFrame &frame)
{
    DPTR_D(VideoEncoderFFmpeg);
    QScopedPointer<AVFrame, ScopedAVFrameDeleter> f;
    // hwupload
    AVPixelFormat pixfmt = AVPixelFormat(frame.pixelFormatFFmpeg());
    if (frame.isValid()) {
        f.reset(av_frame_alloc());
        f->format = pixfmt;
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
#ifdef HAVE_AVHWCTX
        if (d.avctx->hw_frames_ctx) {
            // TODO: try to map to SourceSurface
            // checl valid sw_formats
            if (!d.hwframes_ref) {
                qWarning("no hw frame context for uploading");
                return false;
            }
            if (pixfmt != d.hwframes->sw_format) {
                // reinit or got an unsupported format. assume parameters will not change, so it's  the 1st init
                // check constraints
                bool init_frames_ctx = d.hwframes->sw_format == AVPixelFormat(-1);
                if (d.sw_fmts.contains(pixfmt)) { // format changed
                    init_frames_ctx = true;
                } else { // convert to supported sw format
                    pixfmt = d.sw_fmts[0];
                    f->format = pixfmt;
                    VideoFrame converted = frame.to(VideoFormat::pixelFormatFromFFmpeg(pixfmt));
                    for (int i = 0; i < converted.planeCount(); ++i) {
                        f->linesize[i] = converted.bytesPerLine(i);
                        f->data[i] = (uint8_t*)frame.constBits(i);
                    }
                }
                if (init_frames_ctx) {
                    d.hwframes->sw_format = pixfmt;
                    d.hwframes->width = frame.width();
                    d.hwframes->height = frame.height();
                    AV_ENSURE(av_hwframe_ctx_init(d.hwframes_ref), false);
                }
            }
            // upload
            QScopedPointer<AVFrame, ScopedAVFrameDeleter> hwf( av_frame_alloc());
            AV_ENSURE(av_hwframe_get_buffer(d.hwframes_ref, hwf.data(), 0), false);
            //hwf->format = d.hwframes->format; // not necessary
            //hwf->width = f->width;
            //hwf->height = f->height;
            AV_ENSURE(av_hwframe_transfer_data(hwf.data(), f.data(), 0), false);
            AV_ENSURE(av_frame_copy_props(hwf.data(), f.data()), false);
            av_frame_unref(f.data());
            av_frame_move_ref(f.data(), hwf.data());
        }
#endif //HAVE_AVHWCTX
    }
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = (uint8_t*)d.buffer.constData();
    pkt.size = d.buffer.size();
    int got_packet = 0;
    int ret = avcodec_encode_video2(d.avctx, &pkt, f.data(), &got_packet);
    if (ret < 0) {
        qWarning("error avcodec_encode_video2: %s" ,av_err2str(ret));
        return false; //false
    }
    d.nb_encoded++;
    if (!got_packet) {
        qWarning("no packet got");
        d.packet = Packet();
        // invalid frame means eof
        return frame.isValid();
    }
   // qDebug("enc avpkt.pts: %lld, dts: %lld.", pkt.pts, pkt.dts);
    d.packet = Packet::fromAVPacket(&pkt, av_q2d(d.avctx->time_base));
   // qDebug("enc packet.pts: %.3f, dts: %.3f.", d.packet.pts, d.packet.dts);
    return true;
}

} //namespace QtAV
#include "VideoEncoderFFmpeg.moc"
