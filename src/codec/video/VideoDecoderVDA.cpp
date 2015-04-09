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

#include "VideoDecoderFFmpegHW.h"
#include "VideoDecoderFFmpegHW_p.h"
#include "utils/GPUMemCopy.h"
#include "QtAV/SurfaceInterop.h"
#include "QtAV/private/AVCompat.h"
#include "QtAV/private/prepost.h"
#include "utils/OpenGLHelper.h"
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus
#include <libavcodec/vda.h>
#ifdef __cplusplus
}
#endif //__cplusplus
#include <VideoDecodeAcceleration/VDADecoder.h>
#include "utils/Logger.h"

// TODO: add to QtAV_Compat.h?
// FF_API_PIX_FMT
#ifdef PixelFormat
#undef PixelFormat
#endif
#ifdef MAC_OS_X_VERSION_MIN_REQUIRED
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1070 //MAC_OS_X_VERSION_10_7
#define OSX_TARGET_MIN_LION
#endif // 1070
#endif //MAC_OS_X_VERSION_MIN_REQUIRED

namespace QtAV {

class VideoDecoderVDAPrivate;
class VideoDecoderVDA Q_DECL_FINAL : public VideoDecoderFFmpegHW
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(VideoDecoderVDA)
    Q_PROPERTY(PixelFormat format READ format WRITE setFormat NOTIFY formatChanged)
    Q_PROPERTY(bool zeroCopy READ isZeroCopy WRITE setZeroCopy NOTIFY zeroCopyChanged)
    Q_ENUMS(PixelFormat)
public:
    enum PixelFormat {
        NV12 = '420v',
        //NV12Full = '420f',
        UYVY = '2vuy',
        YUV420P = 'y420',
        YUYV = 'yuvs'
    };
    VideoDecoderVDA();
    VideoDecoderId id() const Q_DECL_OVERRIDE;
    QString description() const Q_DECL_OVERRIDE;
    VideoFrame frame() Q_DECL_OVERRIDE;
    // QObject properties
    void setFormat(PixelFormat fmt);
    PixelFormat format() const;
    void setZeroCopy(bool value);
    bool isZeroCopy() const;
Q_SIGNALS:
    void formatChanged();
    void zeroCopyChanged();
};

extern VideoDecoderId VideoDecoderId_VDA;
FACTORY_REGISTER_ID_AUTO(VideoDecoder, VDA, "VDA")

void RegisterVideoDecoderVDA_Man()
{
    FACTORY_REGISTER_ID_MAN(VideoDecoder, VDA, "VDA")
}


class VideoDecoderVDAPrivate : public VideoDecoderFFmpegHWPrivate
{
public:
    VideoDecoderVDAPrivate()
        : VideoDecoderFFmpegHWPrivate()
        , zero_copy(true)
        , out_fmt(VideoDecoderVDA::NV12)
    {
        copy_uswc = false;
        description = "VDA";
        memset(&hw_ctx, 0, sizeof(hw_ctx));
    }
    ~VideoDecoderVDAPrivate() {}
    virtual bool open();
    virtual void close();

    virtual bool setup(AVCodecContext *avctx);
    virtual bool getBuffer(void **opaque, uint8_t **data);
    virtual void releaseBuffer(void *opaque, uint8_t *data);
    virtual AVPixelFormat vaPixelFormat() const { return QTAV_PIX_FMT_C(VDA_VLD);}


    bool zero_copy;
    VideoDecoderVDA::PixelFormat out_fmt;
    struct vda_context  hw_ctx;
};

typedef struct {
    int  code;
    const char *str;
} vda_error;

static const vda_error vda_errors[] = {
    { kVDADecoderHardwareNotSupportedErr,
        "Hardware doesn't support accelerated decoding" },
    { kVDADecoderFormatNotSupportedErr,
        "Hardware doesn't support requested output format" },
    { kVDADecoderConfigurationError,
        "Invalid configuration provided to VDADecoderCreate" },
    { kVDADecoderDecoderFailedErr,
        "Generic error returned by the decoder layer. The cause can range from"
        " VDADecoder finding errors in the bitstream to another application"
        " using VDA at the moment. Only one application can use VDA at a"
        " givent time." },
    { 0, NULL },
};

static const char* vda_err_str(int err)
{
    for (int i = 0; vda_errors[i].code; ++i) {
        if (vda_errors[i].code != err)
            continue;
        return vda_errors[i].str;
    }
    return 0;
}

typedef struct {
    int cv_pixfmt;
    VideoFormat::PixelFormat pixfmt;
} cv_format;

//https://developer.apple.com/library/Mac/releasenotes/General/MacOSXLionAPIDiffs/CoreVideo.html
/* use fourcc '420v', 'yuvs' for NV12 and yuyv to avoid build time version check
 * qt4 targets 10.6, so those enum values is not valid in build time, while runtime is supported.
 */
static const cv_format cv_formats[] = {
    { 'y420', VideoFormat::Format_YUV420P }, //kCVPixelFormatType_420YpCbCr8Planar
    { '2vuy', VideoFormat::Format_UYVY }, //kCVPixelFormatType_422YpCbCr8
//#ifdef OSX_TARGET_MIN_LION
    { '420f' , VideoFormat::Format_NV12 }, // kCVPixelFormatType_420YpCbCr8BiPlanarFullRange
    { '420v', VideoFormat::Format_NV12 }, //kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange
    { 'yuvs', VideoFormat::Format_YUYV }, //kCVPixelFormatType_422YpCbCr8_yuvs
//#endif
    { 0, VideoFormat::Format_Invalid }
};

static VideoFormat::PixelFormat format_from_cv(int cv)
{
    for (int i = 0; cv_formats[i].cv_pixfmt; ++i) {
        if (cv_formats[i].cv_pixfmt == cv)
            return cv_formats[i].pixfmt;
    }
    return VideoFormat::Format_Invalid;
}

int format_to_cv(VideoFormat::PixelFormat fmt)
{
    for (int i = 0; cv_formats[i].cv_pixfmt; ++i) {
        if (cv_formats[i].pixfmt == fmt)
            return cv_formats[i].cv_pixfmt;
    }
    return 0;
}

VideoDecoderVDA::VideoDecoderVDA()
    : VideoDecoderFFmpegHW(*new VideoDecoderVDAPrivate())
{
    // dynamic properties about static property details. used by UI
    // format: detail_property
    const QString note(tr("Reopen to apply"));
    setProperty("detail_SSE4", tr("Optimized copy decoded data from USWC memory using SSE4.1 if possible.") + " " + tr("Crash for some videos.") + "\n" + note);
    setProperty("detail_format", tr("Output pixel format from decoder. NV12 and UYVY is fast. Some are available since OSX 10.7, e.g. NV12.") + "\n" + note);
}

VideoDecoderId VideoDecoderVDA::id() const
{
    return VideoDecoderId_VDA;
}

QString VideoDecoderVDA::description() const
{
    return "Video Decode Acceleration";
}

VideoFrame VideoDecoderVDA::frame()
{
    DPTR_D(VideoDecoderVDA);
    CVPixelBufferRef cv_buffer = (CVPixelBufferRef)d.frame->data[3];
    if (!cv_buffer) {
        qDebug("Frame buffer is empty.");
        return VideoFrame();
    }
    if (CVPixelBufferGetDataSize(cv_buffer) <= 0) {
        qDebug("Empty frame buffer");
        return VideoFrame();
    }
    VideoFormat::PixelFormat pixfmt = format_from_cv(d.hw_ctx.cv_pix_fmt_type);
    if (pixfmt == VideoFormat::Format_Invalid) {
        qWarning("unsupported vda pixel format: %#x", d.hw_ctx.cv_pix_fmt_type);
        return VideoFrame();
    }
    // we can map the cv buffer addresses to video frame in SurfaceInteropCVBuffer. (may need VideoSurfaceInterop::mapToTexture()
    class SurfaceInteropCVBuffer : public VideoSurfaceInterop {
        bool glinterop;
        CVPixelBufferRef cvbuf; // keep ref until video frame is destroyed
    public:
        SurfaceInteropCVBuffer(CVPixelBufferRef cv, bool gl) : glinterop(gl), cvbuf(cv) {}
        ~SurfaceInteropCVBuffer() {
            CVPixelBufferRelease(cvbuf);
        }
        virtual void* map(SurfaceType type, const VideoFormat& fmt, void* handle = 0, int plane = 0) {
            Q_UNUSED(plane);
            Q_UNUSED(fmt);
            if (!glinterop)
                return 0;
            if (type == HostMemorySurface) {

            }
            if (type != GLTextureSurface)
                return 0;
            IOSurfaceRef surface  = CVPixelBufferGetIOSurface(cvbuf);
            //OSType format_type = CVPixelBufferGetPixelFormatType(cvbuf);
            // TODO: other target. other formats(only 1 surface, so uyvy is the only format supported?)
            DYGL(glBindTexture(GL_TEXTURE_RECTANGLE, *((GLuint*)handle)));
            CGLTexImageIOSurface2D(CGLGetCurrentContext(), GL_TEXTURE_RECTANGLE, GL_RGB8, IOSurfaceGetWidth(surface), IOSurfaceGetHeight(surface), GL_YCBCR_422_APPLE, GL_UNSIGNED_SHORT_8_8_APPLE, surface, 0);
            DYGL(glBindTexture(GL_TEXTURE_RECTANGLE, 0));
            return handle;
        }
    };

    uint8_t *src[3];
    int pitch[3];
    bool gl = false;
    if (isZeroCopy() && format() == UYVY) {// TODO: NV12 zero copy test
       // gl = true;
        pitch[0] = 2*width();
    }
    const VideoFormat fmt(pixfmt);
    if (!gl) {
        CVPixelBufferLockBaseAddress(cv_buffer, 0);
        for (int i = 0; i <fmt.planeCount(); ++i) {
            src[i] = (uint8_t*)CVPixelBufferGetBaseAddressOfPlane(cv_buffer, i);
            pitch[i] = CVPixelBufferGetBytesPerRowOfPlane(cv_buffer, i);
        }
        CVPixelBufferUnlockBaseAddress(cv_buffer, 0);
        //CVPixelBufferRelease(cv_buffer); // release when video frame is destroyed
    }
    VideoFrame f;
    if (isZeroCopy()) {
        f = VideoFrame(width(), height(), fmt);
        f.setBits(src);
        f.setBytesPerLine(pitch);
        f.setTimestamp(double(d.frame->pkt_pts)/1000.0);
    } else {
        f = copyToFrame(fmt, d.height, src, pitch, false);
    }
    f.setMetaData("surface_interop", QVariant::fromValue(VideoSurfaceInteropPtr(new SurfaceInteropCVBuffer(cv_buffer, gl))));
    if (gl)
        f.setMetaData("target", "rect");
    return f;
}

void VideoDecoderVDA::setFormat(PixelFormat fmt)
{
    DPTR_D(VideoDecoderVDA);
    if (d.out_fmt == fmt)
        return;
    d.out_fmt = fmt;
    emit formatChanged();
#ifndef kCFCoreFoundationVersionNumber10_7
#define kCFCoreFoundationVersionNumber10_7      635.00
#endif
    if (kCFCoreFoundationVersionNumber >= kCFCoreFoundationVersionNumber10_7)
        return;
    if (fmt != YUV420P && fmt != UYVY)
        qWarning("format is not supported on OSX < 10.7");
}

VideoDecoderVDA::PixelFormat VideoDecoderVDA::format() const
{
    return d_func().out_fmt;
}

void VideoDecoderVDA::setZeroCopy(bool value)
{
    DPTR_D(VideoDecoderVDA);
    if (d.zero_copy == value)
        return;
    d.zero_copy = value;
    Q_EMIT zeroCopyChanged();
}

bool VideoDecoderVDA::isZeroCopy() const
{
    return d_func().zero_copy;
}

bool VideoDecoderVDAPrivate::setup(AVCodecContext *avctx)
{
    const int w = codedWidth(avctx);
    const int h = codedHeight(avctx);
    if (hw_ctx.width == w && hw_ctx.height == h && hw_ctx.decoder) {
        avctx->hwaccel_context = &hw_ctx;
        return true;
    }
    if (hw_ctx.decoder) {
        ff_vda_destroy_decoder(&hw_ctx);
        releaseUSWC();
    } else {
        memset(&hw_ctx, 0, sizeof(hw_ctx));
        hw_ctx.format = 'avc1'; //fourcc
        hw_ctx.cv_pix_fmt_type = out_fmt; // has the same value as cv pixel format
    }
    /* Setup the libavcodec hardware context */
    hw_ctx.width = w;
    hw_ctx.height = h;
    width = avctx->width; // not necessary. set in decode()
    height = avctx->height;
    avctx->hwaccel_context = NULL;
    /* create the decoder */
    int status = ff_vda_create_decoder(&hw_ctx, codec_ctx->extradata, codec_ctx->extradata_size);
    if (status) {
        qWarning("Failed to create decoder (%i): %s", status, vda_err_str(status));
        return false;
    }
    avctx->hwaccel_context = &hw_ctx;
    initUSWC(hw_ctx.width);
    qDebug("VDA decoder created");
    return true;
}

bool VideoDecoderVDAPrivate::getBuffer(void **opaque, uint8_t **data)
{
    Q_UNUSED(data);
    //qDebug("%s @%d data=%p", __PRETTY_FUNCTION__, __LINE__, *data);
    // FIXME: why *data == 0?
    //*data = (uint8_t *)1; // dummy
    Q_UNUSED(opaque);
    return true;
}

void VideoDecoderVDAPrivate::releaseBuffer(void *opaque, uint8_t *data)
{
    Q_UNUSED(opaque);
    Q_UNUSED(data)
#if 0
    // released in getBuffer?
    CVPixelBufferRef cv_buffer = (CVPixelBufferRef)data;
    if (!cv_buffer)
        return;
    CVPixelBufferRelease(cv_buffer);
#endif
}

bool VideoDecoderVDAPrivate::open()
{
    qDebug("opening VDA module");
    if (codec_ctx->codec_id != AV_CODEC_ID_H264) {
        qWarning("input codec (%s) isn't H264, canceling VDA decoding", avcodec_get_name(codec_ctx->codec_id));
        return false;
    }
#if 0
    if (!codec_ctx->extradata || codec_ctx->extradata_size < 7) {
        qWarning("VDA requires extradata.");
        return false;
    }
#endif
    // TODO: check whether VDA is in use
    return true;
}

void VideoDecoderVDAPrivate::close()
{
    restore(); //IMPORTANT. can not call restore in dtor because ctx is 0 there
    qDebug("destroying VDA decoder");
    ff_vda_destroy_decoder(&hw_ctx);
    releaseUSWC();
}

} //namespace QtAV
#include "VideoDecoderVDA.moc"
