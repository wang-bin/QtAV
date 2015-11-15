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

#include "VideoDecoderFFmpegHW.h"
#include "VideoDecoderFFmpegHW_p.h"
#include "utils/GPUMemCopy.h"
#include "QtAV/SurfaceInterop.h"
#include "QtAV/private/AVCompat.h"
#include "QtAV/private/factory.h"
#include "utils/OpenGLHelper.h"
#include <assert.h>
#ifdef __cplusplus
extern "C" {
#endif //__cplusplus
#include <libavcodec/videotoolbox.h>
#ifdef __cplusplus
}
#endif //__cplusplus
#include "utils/Logger.h"

#ifdef MAC_OS_X_VERSION_MIN_REQUIRED
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1070 //MAC_OS_X_VERSION_10_7
#define OSX_TARGET_MIN_LION
#endif // 1070
#endif //MAC_OS_X_VERSION_MIN_REQUIRED
#ifndef kCFCoreFoundationVersionNumber10_7
#define kCFCoreFoundationVersionNumber10_7      635.00
#endif

namespace QtAV {

class VideoDecoderVideoToolboxPrivate;
// qt4 moc can not correctly process Q_DECL_FINAL here
class VideoDecoderVideoToolbox : public VideoDecoderFFmpegHW
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(VideoDecoderVideoToolbox)
    Q_PROPERTY(PixelFormat format READ format WRITE setFormat NOTIFY formatChanged)
    // TODO: try async property
    Q_ENUMS(PixelFormat)
public:
    enum PixelFormat {
        NV12 = '420v',
        //NV12Full = '420f',
        UYVY = '2vuy',
        YUV420P = 'y420',
        YUYV = 'yuvs'
    };
    VideoDecoderVideoToolbox();
    VideoDecoderId id() const Q_DECL_OVERRIDE;
    QString description() const Q_DECL_OVERRIDE;
    VideoFrame frame() Q_DECL_OVERRIDE;
    // QObject properties
    void setFormat(PixelFormat fmt);
    PixelFormat format() const;
Q_SIGNALS:
    void formatChanged();
};
extern VideoDecoderId VideoDecoderId_VideoToolbox;
FACTORY_REGISTER(VideoDecoder, VideoToolbox, "VideoToolbox")

class VideoDecoderVideoToolboxPrivate Q_DECL_FINAL: public VideoDecoderFFmpegHWPrivate
{
public:
    VideoDecoderVideoToolboxPrivate()
        : VideoDecoderFFmpegHWPrivate()
        , out_fmt(VideoDecoderVideoToolbox::NV12)
    {
        if (kCFCoreFoundationVersionNumber < kCFCoreFoundationVersionNumber10_7)
            out_fmt = VideoDecoderVideoToolbox::UYVY;
        copy_mode = VideoDecoderFFmpegHW::ZeroCopy;
        description = QStringLiteral("VideoToolbox");
    }
    ~VideoDecoderVideoToolboxPrivate() {qDebug("~VideoDecoderVideoToolboxPrivate");}
    bool open() Q_DECL_OVERRIDE;
    void close() Q_DECL_OVERRIDE;

    bool setup(AVCodecContext *avctx) Q_DECL_OVERRIDE;
    bool getBuffer(void **opaque, uint8_t **data) Q_DECL_OVERRIDE;
    void releaseBuffer(void *opaque, uint8_t *data) Q_DECL_OVERRIDE;
    AVPixelFormat vaPixelFormat() const Q_DECL_OVERRIDE { return AV_PIX_FMT_VIDEOTOOLBOX;}

    VideoDecoderVideoToolbox::PixelFormat out_fmt;
};

typedef struct {
    int  code;
    const char *str;
} videotoolbox_error;

static const videotoolbox_error videotoolbox_errors[] = {
    { AVERROR(ENOSYS),
      "Hardware doesn't support accelerated decoding for this stream"
      " or Videotoolbox decoder is not available at the moment (another"
      " application is using it)."
    },
    { AVERROR(EINVAL), "Invalid configuration provided to VTDecompressionSessionCreate" },
    { AVERROR_INVALIDDATA,
      "Generic error returned by the decoder layer. The cause can be Videotoolbox"
      " found errors in the bitstream." },
    { 0, NULL },
};

static const char* videotoolbox_err_str(int err)
{
    for (int i = 0; videotoolbox_errors[i].code; ++i) {
        if (videotoolbox_errors[i].code != err)
            continue;
        return videotoolbox_errors[i].str;
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

VideoDecoderVideoToolbox::VideoDecoderVideoToolbox()
    : VideoDecoderFFmpegHW(*new VideoDecoderVideoToolboxPrivate())
{
    // dynamic properties about static property details. used by UI
    setProperty("detail_format", tr("Output pixel format from decoder. Performance NV12 > UYVY > YUV420P > YUYV.\nOSX < 10.7 only supports UYVY and YUV420p"));
}

VideoDecoderId VideoDecoderVideoToolbox::id() const
{
    return VideoDecoderId_VideoToolbox;
}

QString VideoDecoderVideoToolbox::description() const
{
    return QStringLiteral("Video Decode Acceleration");
}

VideoFrame VideoDecoderVideoToolbox::frame()
{
    DPTR_D(VideoDecoderVideoToolbox);
    CVPixelBufferRef cv_buffer = (CVPixelBufferRef)d.frame->data[3];
    if (!cv_buffer) {
        qDebug("Frame buffer is empty.");
        return VideoFrame();
    }
    if (CVPixelBufferGetDataSize(cv_buffer) <= 0) {
        qDebug("Empty frame buffer");
        return VideoFrame();
    }
    VideoFormat::PixelFormat pixfmt = format_from_cv(CVPixelBufferGetPixelFormatType(cv_buffer));
    if (pixfmt == VideoFormat::Format_Invalid) {
        qWarning("unsupported videotoolbox pixel format: %#x", CVPixelBufferGetPixelFormatType(cv_buffer));
        return VideoFrame();
    }
    // we can map the cv buffer addresses to video frame in SurfaceInteropCVBuffer. (may need VideoSurfaceInterop::mapToTexture()
    class SurfaceInteropCVBuffer Q_DECL_FINAL: public VideoSurfaceInterop {
        bool glinterop;
        CVPixelBufferRef cvbuf; // keep ref until video frame is destroyed
    public:
        SurfaceInteropCVBuffer(CVPixelBufferRef cv, bool gl) : glinterop(gl), cvbuf(cv) {
            CVPixelBufferRetain(cvbuf); // videotoolbox need it for map and CVPixelBufferRelease
        }
        ~SurfaceInteropCVBuffer() {
            CVPixelBufferRelease(cvbuf);
        }
        void* mapToHost(const VideoFormat &format, void *handle, int plane) {
            Q_UNUSED(plane);
            CVPixelBufferLockBaseAddress(cvbuf, 0);
            const VideoFormat fmt(format_from_cv(CVPixelBufferGetPixelFormatType(cvbuf)));
            if (!fmt.isValid()) {
                CVPixelBufferUnlockBaseAddress(cvbuf, 0);
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
            CVPixelBufferUnlockBaseAddress(cvbuf, 0);
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
            pitch[0] = 2*d.width; //
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
        CVPixelBufferUnlockBaseAddress(cv_buffer, 0);
        //CVPixelBufferRelease(cv_buffer); // release when video frame is destroyed
    }
    VideoFrame f;
    if (zero_copy || copyMode() == VideoDecoderFFmpegHW::LazyCopy) {
        f = VideoFrame(d.width, d.height, fmt);
        f.setBytesPerLine(pitch);
        f.setTimestamp(double(d.frame->pkt_pts)/1000.0);
        f.setDisplayAspectRatio(d.getDAR(d.frame));
        if (zero_copy) {
            f.setMetaData(QStringLiteral("target"), QByteArrayLiteral("rect"));
        } else {
            f.setBits(src); // only set for copy back mode
        }
    } else {
        f = copyToFrame(fmt, d.height, src, pitch, false);
    }
    f.setMetaData(QStringLiteral("surface_interop"), QVariant::fromValue(VideoSurfaceInteropPtr(new SurfaceInteropCVBuffer(cv_buffer, zero_copy))));
    return f;
}

void VideoDecoderVideoToolbox::setFormat(PixelFormat fmt)
{
    DPTR_D(VideoDecoderVideoToolbox);
    if (d.out_fmt == fmt)
        return;
    d.out_fmt = fmt;
    emit formatChanged();
    if (kCFCoreFoundationVersionNumber >= kCFCoreFoundationVersionNumber10_7)
        return;
    if (fmt != YUV420P && fmt != UYVY)
        qWarning("format is not supported on OSX < 10.7");
}

VideoDecoderVideoToolbox::PixelFormat VideoDecoderVideoToolbox::format() const
{
    return d_func().out_fmt;
}

bool VideoDecoderVideoToolboxPrivate::setup(AVCodecContext *avctx)
{
    releaseUSWC();

    if (avctx->hwaccel_context) {
        AVVideotoolboxContext *vt = reinterpret_cast<AVVideotoolboxContext*>(avctx->hwaccel_context);
        const CMVideoDimensions dim = CMVideoFormatDescriptionGetDimensions(vt->cm_fmt_desc);
        qDebug("AVVideotoolboxContext ready: %dx%d", dim.width, dim.height);
        return true;
    }
    AVVideotoolboxContext *vtctx = av_videotoolbox_alloc_context();
    vtctx->cv_pix_fmt_type = out_fmt;

    qDebug("AVVideotoolboxContext: %p", vtctx);
    int err = av_videotoolbox_default_init2(codec_ctx, vtctx);
    if (err < 0) {
        qWarning("Failed to init videotoolbox decoder (%#x): %s", err, videotoolbox_err_str(err));
        return false;
    }
    initUSWC(codedWidth(avctx));
    qDebug() << "VideoToolbox decoder created. format: " << format_from_cv(out_fmt);
    return true;
}

bool VideoDecoderVideoToolboxPrivate::getBuffer(void **opaque, uint8_t **data)
{
    *data = (uint8_t *)1; // dummy. it's AVFrame.data[0], must be non null required by ffmpeg
    Q_UNUSED(opaque);
    return true;
}

void VideoDecoderVideoToolboxPrivate::releaseBuffer(void *opaque, uint8_t *data)
{
    Q_UNUSED(opaque);
    Q_UNUSED(data);
}

bool VideoDecoderVideoToolboxPrivate::open()
{
    if (!prepare())
        return false;
    codec_ctx->thread_count = 1; // to avoid crash at av_videotoolbox_alloc_context/av_videotoolbox_default_free. I have no idea how the are called
    qDebug("opening VideoToolbox module");
    // codec/profile check?
    return setup(codec_ctx);
}

void VideoDecoderVideoToolboxPrivate::close()
{
    restore(); //IMPORTANT. can not call restore in dtor because ctx is 0 there
    qDebug("destroying VideoToolbox decoder");
    if (codec_ctx) {
        av_videotoolbox_default_free(codec_ctx);
    }
    releaseUSWC();
}

} //namespace QtAV
#include "VideoDecoderVideoToolbox.moc"
