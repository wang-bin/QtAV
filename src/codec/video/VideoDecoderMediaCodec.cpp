/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2017 Wang Bin <wbsecg1@gmail.com>

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

#include "VideoDecoderFFmpegHW.h"
#include "VideoDecoderFFmpegHW_p.h"
#if FFMPEG_MODULE_CHECK(LIBAVCODEC, 57, 28, 100)
#define QTAV_HAVE_MEDIACODEC 1
#endif
#if QTAV_HAVE(MEDIACODEC)
#include "QtAV/private/factory.h"
#include "QtAV/private/mkid.h"
#include <QtAndroidExtras>
extern "C" {
#include <libavcodec/jni.h>
#include <libavcodec/mediacodec.h>
}

// QtAV's fastest mediacodec decoding/rendering code is private because all other projects I know require java code or is opengl incompatible(chrome, firefox, xbmc, vlc etc.). Maybe my code is the best implementation.
#ifdef MEDIACODEC_TEXTURE
#include "QtAV/SurfaceInterop.h"
#include "opengl/OpenGLHelper.h"
#include "MediaCodecTextureStandalone.h"
#endif

namespace QtAV {
class VideoDecoderMediaCodecPrivate;
class VideoDecoderMediaCodec Q_DECL_FINAL : public VideoDecoderFFmpegHW
{
    DPTR_DECLARE_PRIVATE(VideoDecoderMediaCodec)
public:
    VideoDecoderMediaCodec();

    VideoDecoderId id() const Q_DECL_OVERRIDE;
    QString description() const Q_DECL_OVERRIDE;
    VideoFrame frame() Q_DECL_OVERRIDE;

    void flush() Q_DECL_OVERRIDE { // workaroudn for EAGAIN error in avcodec_receive_frame
        if (copyMode() == ZeroCopy) {

        } else {
            VideoDecoderFFmpegHW::flush();
        }
    }
};

extern VideoDecoderId VideoDecoderId_MediaCodec;
FACTORY_REGISTER(VideoDecoder, MediaCodec, "MediaCodec")

class VideoDecoderMediaCodecPrivate Q_DECL_FINAL : public VideoDecoderFFmpegHWPrivate
{
public:
    ~VideoDecoderMediaCodecPrivate() {
#ifdef MEDIACODEC_TEXTURE
        if (pool_)
            api_->texture_pool_release(&pool_);
#endif
    }
    void close() Q_DECL_OVERRIDE {
        restore();
#ifdef MEDIACODEC_TEXTURE
        if (codec_ctx)
            av_mediacodec_default_free(codec_ctx);
#endif
    }
    bool enableFrameRef() const Q_DECL_OVERRIDE { return true;}

    void* setup(AVCodecContext *avctx) Q_DECL_OVERRIDE {
        if (copy_mode != VideoDecoderFFmpegHW::ZeroCopy)
            return nullptr;
#ifdef MEDIACODEC_TEXTURE
        api_->set_jvm(QAndroidJniEnvironment::javaVM());
        if (!pool_)
            pool_ = api_->texture_pool_create();
        av_mediacodec_default_free(avctx);
        AVMediaCodecContext *mc = av_mediacodec_alloc_context();
        AV_ENSURE(av_mediacodec_default_init(avctx, mc, api_->texture_pool_ensure_surface(pool_)), nullptr);
#endif
        return avctx->hwaccel_context; // set in av_mediacodec_default_init
    }

    bool getBuffer(void **, uint8_t **data) {return true;}
    void releaseBuffer(void *, uint8_t *) {}
    AVPixelFormat vaPixelFormat() const {
        if (copy_mode == VideoDecoderFFmpegHW::ZeroCopy)
            return AV_PIX_FMT_MEDIACODEC;
        return AV_PIX_FMT_NONE;
    }
#ifdef MEDIACODEC_TEXTURE
    const MdkMediaCodecTextureAPI* api_ = mdk_mediacodec_get_api("qtav.nonfree.mediacodec");
    MdkMediaCodecTextureAPI::TexturePool *pool_ = nullptr;
#endif
};

VideoDecoderMediaCodec::VideoDecoderMediaCodec()
    : VideoDecoderFFmpegHW(*new VideoDecoderMediaCodecPrivate())
{
    setProperty("hwaccel", "mediacodec");
#ifdef MEDIACODEC_TEXTURE
    setProperty("copyMode", "ZeroCopy");
#endif
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

static void av_mediacodec_render_buffer(void *buf)
{
    av_mediacodec_release_buffer((AVMediaCodecBuffer*)buf, 1);
}

static void av_mediacodec_buffer_unref(void* buf)
{
    av_buffer_unref((AVBufferRef**)&buf);
}

VideoFrame VideoDecoderMediaCodec::frame()
{
    DPTR_D(VideoDecoderMediaCodec);
    if (d.frame->width <= 0 || d.frame->height <= 0 || !d.codec_ctx)
        return VideoFrame();
    // it's safe if width, height, pixfmt will not change, only data change
    if (copyMode() != ZeroCopy) {
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
// print width height
    VideoFrame frame(d.frame->width, d.frame->height, VideoFormat::Format_RGB32);
    frame.setBytesPerLine(d.frame->width*4);
    frame.setDisplayAspectRatio(d.getDAR(d.frame));
    frame.setTimestamp(d.frame->pkt_pts/1000.0);
#ifdef MEDIACODEC_TEXTURE
    class MediaCodecTextureInterop : public VideoSurfaceInterop
    {
        const MdkMediaCodecTextureAPI* api_ = nullptr;
        MdkMediaCodecTextureAPI::Texture *tex_ = nullptr;
    public:
        MediaCodecTextureInterop(const MdkMediaCodecTextureAPI *api, MdkMediaCodecTextureAPI::Texture *mt) : api_(api), tex_(mt) {}
        ~MediaCodecTextureInterop() {
            api_->texture_release(&tex_);
        }

        void* map(SurfaceType, const VideoFormat &, void *handle, int plane) {
            Q_UNUSED(plane);
            GLuint* t = reinterpret_cast<GLuint*>(handle);
            *t = api_->texture_to_gl(tex_, nullptr, nullptr);
            return t;
        }
    };
    assert(d.frame->buf[0] && d.frame->data[3] && "No AVMediaCodecBuffer or ref in AVFrame");
    AVBufferRef* bufref = av_buffer_ref(d.frame->buf[0]);
    AVMediaCodecBuffer *mcbuf = (AVMediaCodecBuffer*)d.frame->data[3];
    MdkMediaCodecTextureAPI::Texture* mt = d.api_->texture_pool_feed_avbuffer(d.pool_, d.frame->width, d.frame->height, av_mediacodec_buffer_unref, bufref, av_mediacodec_render_buffer, mcbuf);

    MediaCodecTextureInterop *interop = new MediaCodecTextureInterop(d.api_, mt);
    frame.setMetaData(QStringLiteral("surface_interop"), QVariant::fromValue(VideoSurfaceInteropPtr((interop))));
#endif
    return frame;
}
} //namespace QtAV
#endif //QTAV_HAVE(MEDIACODEC)
