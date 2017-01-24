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

#include "VideoDecoderFFmpegHW.h"
#include "VideoDecoderFFmpegHW_p.h"
#include "utils/GPUMemCopy.h"
#include "QtAV/SurfaceInterop.h"
#include "QtAV/private/AVCompat.h"
#include "QtAV/private/factory.h"
#include "opengl/OpenGLHelper.h"
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

// av_vda_default_init2: 2015-05-13 - cc48409 / e7c5e17 - lavc 56.39.100 / 56.23.0
// I(wang bin) found the nv12 is the best format on mac. Then mpv developers confirmed it and pigoz added it to ffmpeg
#if AV_MODULE_CHECK(LIBAVCODEC, 56, 23, 0, 39, 100)
#define AV_VDA_NEW
#endif

#ifdef MAC_OS_X_VERSION_MIN_REQUIRED
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1070 //MAC_OS_X_VERSION_10_7
#define OSX_TARGET_MIN_LION
#endif // 1070
#endif //MAC_OS_X_VERSION_MIN_REQUIRED
#ifndef kCFCoreFoundationVersionNumber10_7
#define kCFCoreFoundationVersionNumber10_7      635.00
#endif

// hwaccel 1.2: av_vda_xxx
namespace QtAV {
namespace cv {
VideoFormat::PixelFormat format_from_cv(int cv);
}
class VideoDecoderVDAPrivate;
// qt4 moc can not correctly process Q_DECL_FINAL here
class VideoDecoderVDA : public VideoDecoderFFmpegHW
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(VideoDecoderVDA)
    Q_PROPERTY(PixelFormat format READ format WRITE setFormat NOTIFY formatChanged)
    // TODO: try async property
    Q_ENUMS(PixelFormat)
public:
    enum PixelFormat {
        NV12 = '420f',
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
Q_SIGNALS:
    void formatChanged();
};
extern VideoDecoderId VideoDecoderId_VDA;
FACTORY_REGISTER(VideoDecoder, VDA, "VDA")

class VideoDecoderVDAPrivate Q_DECL_FINAL: public VideoDecoderFFmpegHWPrivate
{
public:
    VideoDecoderVDAPrivate()
        : VideoDecoderFFmpegHWPrivate()
        , out_fmt(VideoDecoderVDA::NV12)
    {
        if (kCFCoreFoundationVersionNumber < kCFCoreFoundationVersionNumber10_7)
            out_fmt = VideoDecoderVDA::UYVY;
        copy_mode = VideoDecoderFFmpegHW::ZeroCopy;
        description = QStringLiteral("VDA");
#ifndef AV_VDA_NEW
        memset(&hw_ctx, 0, sizeof(hw_ctx));
#endif
    }
    ~VideoDecoderVDAPrivate() {qDebug("~VideoDecoderVDAPrivate");}
    bool open() Q_DECL_OVERRIDE;
    void close() Q_DECL_OVERRIDE;

    void* setup(AVCodecContext *avctx) Q_DECL_OVERRIDE;
    bool getBuffer(void **opaque, uint8_t **data) Q_DECL_OVERRIDE;
    void releaseBuffer(void *opaque, uint8_t *data) Q_DECL_OVERRIDE;
    AVPixelFormat vaPixelFormat() const Q_DECL_OVERRIDE {
#ifdef AV_VDA_NEW
        return AV_PIX_FMT_VDA;
#else
        return QTAV_PIX_FMT_C(VDA_VLD);
#endif
    }

    VideoDecoderVDA::PixelFormat out_fmt;
#ifndef AV_VDA_NEW
    struct vda_context  hw_ctx;
#endif
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

VideoDecoderVDA::VideoDecoderVDA()
    : VideoDecoderFFmpegHW(*new VideoDecoderVDAPrivate())
{
    // dynamic properties about static property details. used by UI
    setProperty("detail_format", tr("Output pixel format from decoder. Performance NV12 > UYVY > YUV420P > YUYV.\nOSX < 10.7 only supports UYVY and YUV420p"));
}

VideoDecoderId VideoDecoderVDA::id() const
{
    return VideoDecoderId_VDA;
}

QString VideoDecoderVDA::description() const
{
    return QStringLiteral("Video Decode Acceleration");
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
    VideoFormat::PixelFormat pixfmt = cv::format_from_cv(CVPixelBufferGetPixelFormatType(cv_buffer));
    if (pixfmt == VideoFormat::Format_Invalid) {
        qWarning("unsupported vda pixel format: %#x", CVPixelBufferGetPixelFormatType(cv_buffer));
        return VideoFrame();
    }
    // we can map the cv buffer addresses to video frame in SurfaceInteropCVBuffer. (may need VideoSurfaceInterop::mapToTexture()
    class SurfaceInteropCVBuffer Q_DECL_FINAL: public VideoSurfaceInterop {
        bool glinterop;
        CVPixelBufferRef cvbuf; // keep ref until video frame is destroyed
    public:
        SurfaceInteropCVBuffer(CVPixelBufferRef cv, bool gl) : glinterop(gl), cvbuf(cv) {
#ifdef AV_VDA_NEW
            CVPixelBufferRetain(cvbuf);
#endif
        }
        ~SurfaceInteropCVBuffer() {
            CVPixelBufferRelease(cvbuf);
        }
        void* mapToHost(const VideoFormat &format, void *handle, int plane) {
            Q_UNUSED(plane);
            CVPixelBufferLockBaseAddress(cvbuf, kCVPixelBufferLock_ReadOnly);
            const VideoFormat fmt(cv::format_from_cv(CVPixelBufferGetPixelFormatType(cvbuf)));
            if (!fmt.isValid()) {
                CVPixelBufferUnlockBaseAddress(cvbuf, kCVPixelBufferLock_ReadOnly);
                return NULL;
            }
            const int w = CVPixelBufferGetWidth(cvbuf);
            const int h = CVPixelBufferGetHeight(cvbuf);
            uint8_t *src[3];
            int pitch[3];
            for (int i = 0; i <fmt.planeCount(); ++i) {
                // get address results in internal copy
                src[i] = (uint8_t*)CVPixelBufferGetBaseAddressOfPlane(cvbuf, i);
                pitch[i] = CVPixelBufferGetBytesPerRowOfPlane(cvbuf, i);
            }
            CVPixelBufferUnlockBaseAddress(cvbuf, kCVPixelBufferLock_ReadOnly);
            //CVPixelBufferRelease(cv_buffer); // release when video frame is destroyed
            VideoFrame frame(VideoFrame::fromGPU(fmt, w, h, h, src, pitch));
            if (fmt != format)
                frame = frame.to(format);
            VideoFrame *f = reinterpret_cast<VideoFrame*>(handle);
            frame.setTimestamp(f->timestamp());
            frame.setDisplayAspectRatio(f->displayAspectRatio());
            *f = frame;
            return f;
        }
        virtual void* map(SurfaceType type, const VideoFormat& fmt, void* handle = 0, int plane = 0) Q_DECL_OVERRIDE {
            Q_UNUSED(fmt);
            if (!glinterop)
                return 0;
            if (type == HostMemorySurface) {
                return mapToHost(fmt, handle, plane);
            }
            if (type == SourceSurface) {
                CVPixelBufferRef *b = reinterpret_cast<CVPixelBufferRef*>(handle);
                *b = cvbuf;
                return handle;
            }
            if (type != GLTextureSurface)
                return 0;
            // https://www.opengl.org/registry/specs/APPLE/rgb_422.txt
            // TODO: check extension GL_APPLE_rgb_422 and rectangle?
            IOSurfaceRef surface  = CVPixelBufferGetIOSurface(cvbuf);
            int w = IOSurfaceGetWidthOfPlane(surface, plane);
            int h = IOSurfaceGetHeightOfPlane(surface, plane);
            //qDebug("plane:%d, iosurface %dx%d, ctx: %p", plane, w, h, CGLGetCurrentContext());
            OSType pixfmt = IOSurfaceGetPixelFormat(surface); //CVPixelBufferGetPixelFormatType(cvbuf);
            GLenum iformat = GL_RGBA8;
            GLenum format = GL_BGRA;
            GLenum dtype = GL_UNSIGNED_INT_8_8_8_8_REV;
            const GLenum target = GL_TEXTURE_RECTANGLE;
            // TODO: GL_RED, GL_RG is better for gl >= 3.0
            if (pixfmt == NV12) {
                dtype = GL_UNSIGNED_BYTE;
                if (plane == 0) {
                    iformat = format = OpenGLHelper::useDeprecatedFormats() ? GL_LUMINANCE : GL_RED;
                } else {
                    iformat = format = OpenGLHelper::useDeprecatedFormats() ? GL_LUMINANCE_ALPHA : GL_RG;
                }
            } else if (pixfmt == UYVY || pixfmt == YUYV) {
                w /= 2; //rgba texture
            } else if (pixfmt == YUV420P) {
                dtype = GL_UNSIGNED_BYTE;
                iformat = format = OpenGLHelper::useDeprecatedFormats() ? GL_LUMINANCE : GL_RED;
                if (plane > 1 && format == GL_LUMINANCE)
                    iformat = format = GL_ALPHA;
            }
            DYGL(glBindTexture(target, *((GLuint*)handle)));
            CGLError err = CGLTexImageIOSurface2D(CGLGetCurrentContext(), target, iformat, w, h, format, dtype, surface, plane);
            if (err != kCGLNoError) {
                qWarning("error creating IOSurface texture at plane %d: %s", plane, CGLErrorString(err));
            }
            DYGL(glBindTexture(target, 0));
            return handle;
        }
        void* createHandle(void* handle, SurfaceType type, const VideoFormat &fmt, int plane, int planeWidth, int planeHeight) Q_DECL_OVERRIDE {
            Q_UNUSED(type);
            Q_UNUSED(fmt);
            Q_UNUSED(plane);
            Q_UNUSED(planeWidth);
            Q_UNUSED(planeHeight);
            if (!glinterop)
                return 0;
            GLuint *tex = (GLuint*)handle;
            DYGL(glGenTextures(1, tex));
            // no init required
            return handle;
        }
    };

    uint8_t *src[3];
    int pitch[3];
    const bool zero_copy = copyMode() == VideoDecoderFFmpegHW::ZeroCopy;
    if (zero_copy) {
        // make sure VideoMaterial can correctly setup parameters
        switch (format()) {
        case UYVY:
            pitch[0] = 2*d.width;
            pixfmt = VideoFormat::Format_VYUY; //FIXME: VideoShader assume uyvy is uploaded as rgba, but apple limits the result to bgra
            break;
        case NV12:
            pitch[0] = d.width;
            pitch[1] = d.width;
            break;
        case YUV420P:
            pitch[0] = d.width;
            pitch[1] = pitch[2] = d.width/2;
            break;
        case YUYV:
            pitch[0] = 2*d.width; //
            //pixfmt = VideoFormat::Format_YVYU; //
            break;
        default:
            break;
        }
    }
    const VideoFormat fmt(pixfmt);
    if (!zero_copy) {
        CVPixelBufferLockBaseAddress(cv_buffer, 0);
        for (int i = 0; i <fmt.planeCount(); ++i) {
            // get address results in internal copy
            src[i] = (uint8_t*)CVPixelBufferGetBaseAddressOfPlane(cv_buffer, i);
            pitch[i] = CVPixelBufferGetBytesPerRowOfPlane(cv_buffer, i);
        }
        CVPixelBufferUnlockBaseAddress(cv_buffer, kCVPixelBufferLock_ReadOnly);
        //CVPixelBufferRelease(cv_buffer); // release when video frame is destroyed
    }
    VideoFrame f;
    if (zero_copy) {
        f = VideoFrame(d.width, d.height, fmt);
        f.setBytesPerLine(pitch);
        f.setTimestamp(double(d.frame->pkt_pts)/1000.0);
        f.setDisplayAspectRatio(d.getDAR(d.frame));
        if (zero_copy) {
            f.setMetaData(QStringLiteral("target"), QByteArrayLiteral("rect"));
        } else {
            f.setBits(src); // only set for copy back mode
        }
        d.updateColorDetails(&f);
    } else {
        f = copyToFrame(fmt, d.height, src, pitch, false);
    }
    f.setMetaData(QStringLiteral("surface_interop"), QVariant::fromValue(VideoSurfaceInteropPtr(new SurfaceInteropCVBuffer(cv_buffer, zero_copy))));
    return f;
}

void VideoDecoderVDA::setFormat(PixelFormat fmt)
{
    DPTR_D(VideoDecoderVDA);
    if (d.out_fmt == fmt)
        return;
    d.out_fmt = fmt;
    emit formatChanged();
    if (kCFCoreFoundationVersionNumber >= kCFCoreFoundationVersionNumber10_7)
        return;
    if (fmt != YUV420P && fmt != UYVY)
        qWarning("format is not supported on OSX < 10.7");
}

VideoDecoderVDA::PixelFormat VideoDecoderVDA::format() const
{
    return d_func().out_fmt;
}

void* VideoDecoderVDAPrivate::setup(AVCodecContext *avctx)
{
    releaseUSWC();
#ifdef AV_VDA_NEW
    av_vda_default_free(avctx);
    AVVDAContext* hw_ctx = av_vda_alloc_context();
    hw_ctx->cv_pix_fmt_type = out_fmt;
    int status = av_vda_default_init2(avctx, hw_ctx);
#else
    const int w = codedWidth(avctx);
    const int h = codedHeight(avctx);
    if (hw_ctx.decoder) {
        ff_vda_destroy_decoder(&hw_ctx);
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
    /* create the decoder */
    int status = ff_vda_create_decoder(&hw_ctx, codec_ctx->extradata, codec_ctx->extradata_size);
#endif
    if (status) {
        qWarning("Failed to init VDA decoder (%#x %s): %s", status, av_err2str(status), vda_err_str(status));
        return NULL;
    }
    initUSWC(codedWidth(avctx));
    qDebug() << "VDA decoder created. format: " << cv::format_from_cv(out_fmt);
#ifdef AV_VDA_NEW
    return hw_ctx;
#else
    return &hw_ctx;
#endif
}

bool VideoDecoderVDAPrivate::getBuffer(void **opaque, uint8_t **data)
{
    *data = (uint8_t *)1; // dummy. it's AVFrame.data[0], must be non null required by ffmpeg
    Q_UNUSED(opaque);
    return true;
}

void VideoDecoderVDAPrivate::releaseBuffer(void *opaque, uint8_t *data)
{
    Q_UNUSED(opaque);
    Q_UNUSED(data)
}

bool VideoDecoderVDAPrivate::open()
{
    if (!prepare())
        return false;
    qDebug("opening VDA module");
    if (codec_ctx->codec_id != AV_CODEC_ID_H264) {
        qWarning("input codec (%s) isn't H264, canceling VDA decoding", avcodec_get_name(codec_ctx->codec_id));
        return false;
    }
    switch (codec_ctx->profile) { //profile check code is from xbmc
    case FF_PROFILE_H264_HIGH_10:
    case FF_PROFILE_H264_HIGH_10_INTRA:
    case FF_PROFILE_H264_HIGH_422:
    case FF_PROFILE_H264_HIGH_422_INTRA:
    case FF_PROFILE_H264_HIGH_444_PREDICTIVE:
    case FF_PROFILE_H264_HIGH_444_INTRA:
    case FF_PROFILE_H264_CAVLC_444:
        return false;
    default:
        break;
    }
    return true;
    //return setup(codec_ctx); // must be in get_format() in hwa 1.2
}

void VideoDecoderVDAPrivate::close()
{
    restore(); //IMPORTANT. can not call restore in dtor because ctx is 0 there
    qDebug("destroying VDA decoder");
#ifdef AV_VDA_NEW
    if (codec_ctx)
        av_vda_default_free(codec_ctx);
#else
    ff_vda_destroy_decoder(&hw_ctx);
#endif
    releaseUSWC();
}

} //namespace QtAV
#include "VideoDecoderVDA.moc"
