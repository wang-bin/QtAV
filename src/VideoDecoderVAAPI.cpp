/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013 Wang Bin <wbsecg1@gmail.com>

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

#include "QtAV/VideoDecoderFFmpeg.h"
#include "private/VideoDecoder_p.h"
#include <QtAV/Packet.h>
#include <QtAV/QtAV_Compat.h>
#include "prepost.h"

#include <va/va.h>

#if HAVE_VAAPI_DRM
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <xf86drm.h>
#include <sys/stat.h>
#include <va/va_drm.h>
#endif

#include <libavcodec/avcodec.h>
#include <libavcodec/vaapi.h>
#if 1//HAVE_VAAPI_X11
#include <X11/Xlib.h>
#include <va/va_x11.h>
#endif

/* LIBAVCODEC_VERSION_CHECK checks for the right version of libav and FFmpeg
 * a is the major version
 * b and c the minor and micro versions of libav
 * d and e the minor and micro versions of FFmpeg */
#define LIBAVCODEC_VERSION_CHECK(a, b, c, d, e) \
    ((LIBAVCODEC_VERSION_MICRO <  100 && LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(a, b, c)) || \
      (LIBAVCODEC_VERSION_MICRO >= 100 && LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(a, d, e)))

#ifndef VA_SURFACE_ATTRIB_SETTABLE
#define vaCreateSurfaces(d, f, w, h, s, ns, a, na) \
    vaCreateSurfaces(d, w, h, f, ns, s)
#endif

namespace QtAV {

class VideoDecoderVAAPIPrivate;
class VideoDecoderVAAPI : public VideoDecoder
{
    DPTR_DECLARE_PRIVATE(VideoDecoderVAAPI)
public:
    VideoDecoderVAAPI();
    virtual ~VideoDecoderVAAPI();
    virtual bool prepare();
    virtual bool open();
    virtual bool close();
    virtual bool decode(const QByteArray &encoded);
    virtual VideoFrame frame();

};

extern VideoDecoderId VideoDecoderId_VAAPI;
FACTORY_REGISTER_ID_AUTO(VideoDecoder, VAAPI, "VAAPI")

void RegisterVideoDecoderVAAPI_Man()
{
    FACTORY_REGISTER_ID_MAN(VideoDecoder, VAAPI, "VAAPI")
}


typedef struct
{
    VASurfaceID  i_id;
    int          i_refcount;
    unsigned int i_order;
    //vlc_mutex_t *p_lock;
} vlc_va_surface_t;


class VideoDecoderVAAPIPrivate : public VideoDecoderPrivate
{
public:
    VideoDecoderVAAPIPrivate() {
        //TODO:
    }

    ~VideoDecoderVAAPIPrivate() {
        //TODO:
    }

    bool createSurfaces(void **hwctx, AVPixelFormat *chroma, int w, int h);
    void destroySurfaces();

    bool setup(void **hwctx, AVPixelFormat *chroma, int w, int h);
    bool open();
    void close();

    AVPixelFormat getFormat(struct AVCodecContext *p_context, const AVPixelFormat *pi_fmt);
    int getBuffer(void **opaque, uint8_t **data);
    void releaseBuffer(void *opaque, uint8_t *data);

    Display      *display_x11;
    VADisplay     display;

    VAConfigID    config_id;
    VAContextID   context_id;

    struct vaapi_context hw_ctx;

    /* */
    int version_major;
    int version_minor;

    /* */
    QMutex  mutex;
    int          nb_surfaces;
    unsigned int surface_order;
    int          surface_width;
    int          surface_height;
    AVPixelFormat surface_chroma;

    //QVector<vlc_va_surface_t*> surfaces;
    vlc_va_surface_t *surfaces;

    VAImage      image;
    //copy_cache_t image_cache;

    bool supports_derive;

    AVPixelFormat va_pixfmt;
    QString description;
};

static AVPixelFormat ffmpeg_get_vaapi_format(struct AVCodecContext *c, const AVPixelFormat * ff)
{
    VideoDecoderVAAPIPrivate *va = (VideoDecoderVAAPIPrivate*)c->opaque;
    return va->getFormat(c, ff);
}

static int ffmpeg_get_vaapi_buffer(struct AVCodecContext *c, AVFrame *ff)//vlc_va_t *external, AVFrame *ff)
{
    VideoDecoderVAAPIPrivate *va = (VideoDecoderVAAPIPrivate*)c->opaque;
    /* hwaccel_context is not present in old ffmpeg version */
    if (va->setup(&c->hwaccel_context, &c->pix_fmt, c->coded_width, c->coded_height)) {
        qWarning("va Setup failed");
        return -1;
    }
    if (va->getBuffer(&c->opaque, &ff->data[0]) != 0)
        return -1;

    //ffmpeg_va_GetFrameBuf
    ff->data[3] = ff->data[0];
    ff->type = FF_BUFFER_TYPE_USER;
    return 0;
}

static void ffmpeg_release_buffer(struct AVCodecContext *c, AVFrame *ff)
{
    VideoDecoderVAAPIPrivate *va = (VideoDecoderVAAPIPrivate*)c->opaque;
    va->releaseBuffer(c->opaque, ff->data[0]);
    memset(ff->data, 0, sizeof(ff->data));
}


VideoDecoderVAAPI::VideoDecoderVAAPI()
    : VideoDecoder(*new VideoDecoderVAAPIPrivate())
{
}

VideoDecoderVAAPI::~VideoDecoderVAAPI()
{
    setCodecContext(0);
}

bool VideoDecoderVAAPI::prepare()
{
    /* Only VLD supported */
    DPTR_D(VideoDecoderVAAPI);
    d.va_pixfmt = QTAV_PIX_FMT_C(VAAPI_VLD); //in prepare()?

    //vlc_xlib_init: XInitThreads();
    return VideoDecoder::prepare();
}

//prepareHW
bool VideoDecoderVAAPI::open()
{
    DPTR_D(VideoDecoderVAAPI);
    if (!d.codec_ctx) {
        qWarning("FFmpeg codec context not ready");
        return false;
    }
    AVCodec *codec = 0;
    if (!d.name.isEmpty()) {
        codec = avcodec_find_decoder_by_name(d.name.toUtf8().constData());
    } else {
        codec = avcodec_find_decoder(d.codec_ctx->codec_id);
    }
    if (!codec) {
        if (d.name.isEmpty()) {
            qWarning("No codec could be found with id %d", d.codec_ctx->codec_id);
        } else {
            qWarning("No codec could be found with name %s", d.name.toUtf8().constData());
        }
        return false;
    }
    if (!d.open()) {
        qWarning("Open vaapi decoder failed!");
        return false;
    }
    //TODO: neccesary?
#if 0
    if (!d.setup(&d.codec_ctx->hwaccel_context, &d.codec_ctx->pix_fmt, d.codec_ctx->width, d.codec_ctx->height)) {
        qWarning("Setup vaapi failed.");
        return false;
    }
#endif
    d.codec_ctx->opaque = &d; //is it ok?
    d.codec_ctx->get_format = ffmpeg_get_vaapi_format;
//#if LIBAVCODEC_VERSION_MAJOR >= 55
    //d.codec_ctx->get_buffer2 = lavc_GetFrame;
//#else
    d.codec_ctx->get_buffer = ffmpeg_get_vaapi_buffer;//ffmpeg_GetFrameBuf;
    d.codec_ctx->reget_buffer = avcodec_default_reget_buffer;
    d.codec_ctx->release_buffer = ffmpeg_release_buffer;//ffmpeg_ReleaseFrameBuf;
//#endif

    //d.codec_ctx->strict_std_compliance = FF_COMPLIANCE_STRICT;
    //d.codec_ctx->slice_flags |= SLICE_FLAG_ALLOW_FIELD;
// lavfilter
    //d.codec_ctx->slice_flags |= SLICE_FLAG_ALLOW_FIELD; //lavfilter
    //d.codec_ctx->strict_std_compliance = FF_COMPLIANCE_STRICT;


    //HAVE_AVCODEC_MT
    if (d.threads == -1)
        d.threads = qMax(0, QThread::idealThreadCount());
    if (d.threads > 0)
        d.codec_ctx->thread_count = d.threads;
    if (d.threads > 1)
        d.codec_ctx->thread_type = d.thread_slice ? FF_THREAD_SLICE : FF_THREAD_FRAME;
    d.codec_ctx->thread_safe_callbacks = true;
    switch (d.codec_ctx->codec_id) {
        case AV_CODEC_ID_MPEG4:
        case AV_CODEC_ID_H263:
            d.codec_ctx->thread_type = 0;
            break;
        case AV_CODEC_ID_MPEG1VIDEO:
        case AV_CODEC_ID_MPEG2VIDEO:
            d.codec_ctx->thread_type &= ~FF_THREAD_SLICE;
            /* fall through */
# if (LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55, 1, 0))
        case AV_CODEC_ID_H264:
        case AV_CODEC_ID_VC1:
        case AV_CODEC_ID_WMV3:
            d.codec_ctx->thread_type &= ~FF_THREAD_FRAME;
# endif
    }
/*
    if (d.codec_ctx->thread_type & FF_THREAD_FRAME)
        p_dec->i_extra_picture_buffers = 2 * p_sys->p_context->thread_count;
*/


    d.codec_ctx->pix_fmt = QTAV_PIX_FMT_C(VAAPI_VLD); ///TODO: move

    int ret = avcodec_open2(d.codec_ctx, codec, NULL);
    if (ret < 0) {
        qWarning("open video codec failed: %s", av_err2str(ret));
        return false;
    }
    d.is_open = true;
    return true;
}

bool VideoDecoderVAAPI::close()
{
    if (!isOpen()) {
        return true;
    }
    DPTR_D(VideoDecoderVAAPI);
    d.is_open = false;
    d.close();
    return VideoDecoder::close();
}

bool VideoDecoderVAAPI::decode(const QByteArray &encoded)
{
    if (!isAvailable())
        return false;
    DPTR_D(VideoDecoder);
    AVPacket packet;
    av_new_packet(&packet, encoded.size());
    memcpy(packet.data, encoded.data(), encoded.size());
//TODO: use AVPacket directly instead of Packet?
    //AVStream *stream = format_context->streams[stream_idx];

    //TODO: some decoders might in addition need other fields like flags&AV_PKT_FLAG_KEY
    int ret = avcodec_decode_video2(d.codec_ctx, d.frame, &d.got_frame_ptr, &packet);
    //TODO: decoded format is YUV420P, YUV422P?
    av_free_packet(&packet);
    if (ret < 0) {
        qWarning("[VideoDecoder] %s", av_err2str(ret));
        return false;
    }
    if (!d.got_frame_ptr) {
        qWarning("no frame could be decompressed: %s", av_err2str(ret));
        return false; //FIXME
    }
    // TODO: wait key frame?
    if (!d.codec_ctx->width || !d.codec_ctx->height)
        return false;
    d.width = d.codec_ctx->width;
    d.height = d.codec_ctx->height;
    return true;
}

VideoFrame VideoDecoderVAAPI::frame()
{
    DPTR_D(VideoDecoderVAAPI);
    VASurfaceID surface_id = (VASurfaceID)(uintptr_t)d.frame->data[3];
#if VA_CHECK_VERSION(0,31,0)
    if (vaSyncSurface(d.display, surface_id))
#else
    if (vaSyncSurface(d.display, d.context_id, surface_id))
#endif
        return VideoFrame();

    if (d.supports_derive) {
        if (vaDeriveImage(d.display, surface_id, &(d.image)) != VA_STATUS_SUCCESS)
            return VideoFrame();
    } else {
        if (vaGetImage(d.display, surface_id, 0, 0, d.surface_width, d.surface_height, d.image.image_id))
            return VideoFrame();
    }

    void *p_base;
    if (vaMapBuffer(d.display, d.image.buf, &p_base))
        return VideoFrame();

    VideoFrame frame;
    const uint32_t i_fourcc = d.image.format.fourcc;
    if (i_fourcc == VA_FOURCC('Y','V','1','2') ||
        i_fourcc == VA_FOURCC('I','4','2','0')) {
        bool b_swap_uv = i_fourcc == VA_FOURCC('I','4','2','0');
        uint8_t *pp_plane[3];
        int pi_pitch[3];

        for (int i = 0; i < 3; i++) {
            const int i_src_plane = (b_swap_uv && i != 0) ?  (3 - i) : i;
            pp_plane[i] = (uint8_t*)p_base + d.image.offsets[i_src_plane];
            pi_pitch[i] = d.image.pitches[i_src_plane];
        }
        frame = VideoFrame(d.surface_width, d.surface_height, VideoFormat(VideoFormat::Format_YUV420P));
        frame.setBits(pp_plane);
        frame.setBytesPerLine(pi_pitch);
    } else {
        Q_ASSERT(i_fourcc == VA_FOURCC('N','V','1','2'));
        uint8_t *pp_plane[2];
        int pi_pitch[2];

        for (int i = 0; i < 2; i++) {
            pp_plane[i] = (uint8_t*)p_base + d.image.offsets[i];
            pi_pitch[i] = d.image.pitches[i];
        }
        frame = VideoFrame(d.surface_width, d.surface_height, VideoFormat(VideoFormat::Format_NV12));
        frame.setBits(pp_plane);
        frame.setBytesPerLine(pi_pitch);
    }

    if (vaUnmapBuffer(d.display, d.image.buf))
        return VideoFrame();

    if (d.supports_derive) {
        vaDestroyImage(d.display, d.image.image_id);
        d.image.image_id = VA_INVALID_ID;
    }
    return frame;
}


///////
bool VideoDecoderVAAPIPrivate::open()
{
    VAProfile i_profile, *p_profiles_list;
    bool b_supported_profile = false;
    int i_profiles_nb = 0;
    int i_nb_surfaces;
    /* */
    switch (codec_ctx->codec_id) {
    case AV_CODEC_ID_MPEG1VIDEO:
    case AV_CODEC_ID_MPEG2VIDEO:
        i_profile = VAProfileMPEG2Main;
        i_nb_surfaces = 2+1;
        break;
    case AV_CODEC_ID_MPEG4:
        i_profile = VAProfileMPEG4AdvancedSimple;
        i_nb_surfaces = 2+1;
        break;
    case AV_CODEC_ID_WMV3:
        i_profile = VAProfileVC1Main;
        i_nb_surfaces = 2+1;
        break;
    case AV_CODEC_ID_VC1:
        i_profile = VAProfileVC1Advanced;
        i_nb_surfaces = 2+1;
        break;
    case AV_CODEC_ID_H264:
        i_profile = VAProfileH264High;
        i_nb_surfaces = 16+1;
        break;
    default:
        return false;
    }
    config_id  = VA_INVALID_ID;
    context_id = VA_INVALID_ID;
    image.image_id = VA_INVALID_ID;
    /* Create a VA display */
    display_x11 = XOpenDisplay(NULL);
    if (!display_x11) {
        qWarning("Could not connect to X server");
        return false;
    }
    display = vaGetDisplay(display_x11);
    if (!display) {
        qWarning("Could not get a VAAPI device");
        return false;
    }
    if (vaInitialize(display, &version_major, &version_minor)) {
        qWarning("Failed to initialize the VAAPI device");
        return false;
    }
    /* Check if the selected profile is supported */
    i_profiles_nb = vaMaxNumProfiles(display);
    p_profiles_list = (VAProfile*)calloc(i_profiles_nb, sizeof(VAProfile));
    if (!p_profiles_list)
        return false;

    VAStatus i_status = vaQueryConfigProfiles(display, p_profiles_list, &i_profiles_nb);
    if (i_status == VA_STATUS_SUCCESS) {
        for (int i = 0; i < i_profiles_nb; i++) {
            if (p_profiles_list[i] == i_profile) {
                b_supported_profile = true;
                break;
            }
        }
    }
    free(p_profiles_list);
    if (!b_supported_profile) {
        qDebug("Codec and profile not supported by the hardware");
        return false;
    }
    /* Create a VA configuration */
    VAConfigAttrib attrib;
    memset(&attrib, 0, sizeof(attrib));
    attrib.type = VAConfigAttribRTFormat;
    if (vaGetConfigAttributes(display, i_profile, VAEntrypointVLD, &attrib, 1))
        return false;
    /* Not sure what to do if not, I don't have a way to test */
    if ((attrib.value & VA_RT_FORMAT_YUV420) == 0)
        return false;
    //vaCreateConfig(display, i_profile, VAEntrypointVLD, NULL, 0, &config_id)
    if (vaCreateConfig(display, i_profile, VAEntrypointVLD, &attrib, 1, &config_id)) {
        config_id = VA_INVALID_ID;
        return false;
    }
    nb_surfaces = i_nb_surfaces;
    supports_derive = false;

    description = QString("VA API version %1.%2").arg(version_major).arg(version_minor);
    return true;
}

bool VideoDecoderVAAPIPrivate::createSurfaces(void **pp_hw_ctx, AVPixelFormat *chroma, int w, int h)
{
    Q_ASSERT(w > 0 && h > 0);
    /* */
    surfaces = (vlc_va_surface_t*)calloc(nb_surfaces, sizeof(vlc_va_surface_t));
    if (!surfaces)
        return false;
    image.image_id = VA_INVALID_ID;
    context_id = VA_INVALID_ID;

    /* Create surfaces */
    VASurfaceID pi_surface_id[nb_surfaces];
    if (vaCreateSurfaces(display, VA_RT_FORMAT_YUV420, w, h, pi_surface_id, nb_surfaces, NULL, 0)) {
        for (int i = 0; i < nb_surfaces; i++)
            surfaces[i].i_id = VA_INVALID_SURFACE;
        destroySurfaces();
        return false;
    }
    for (int i = 0; i < nb_surfaces; i++) {
        vlc_va_surface_t *surface = &surfaces[i];
        surface->i_id = pi_surface_id[i];
        surface->i_refcount = 0;
        surface->i_order = 0;
        ////surface->p_lock = &sys->lock;
    }
    /* Create a context */
    if (vaCreateContext(display, config_id, w, h, VA_PROGRESSIVE, pi_surface_id, nb_surfaces, &context_id)) {
        context_id = VA_INVALID_ID;
        destroySurfaces();
        return false;
    }
    /* Find and create a supported image chroma */
    int i_fmt_count = vaMaxNumImageFormats(display);
    //av_mallocz_array
    VAImageFormat *p_fmt = (VAImageFormat*)calloc(i_fmt_count, sizeof(*p_fmt));
    if (!p_fmt) {
        destroySurfaces();
        return false;
    }

    if (vaQueryImageFormats(display, p_fmt, &i_fmt_count)) {
        free(p_fmt);
        destroySurfaces();
        return false;
    }

    VAImage test_image;
    if (vaDeriveImage(display, pi_surface_id[0], &test_image) == VA_STATUS_SUCCESS) {
        supports_derive = true;
        vaDestroyImage(display, test_image.image_id);
    }

    AVPixelFormat i_chroma = QTAV_PIX_FMT_C(NONE);
    VAImageFormat fmt;
    for (int i = 0; i < i_fmt_count; i++) {
        if (p_fmt[i].fourcc == VA_FOURCC('Y', 'V', '1', '2') ||
            p_fmt[i].fourcc == VA_FOURCC('I', '4', '2', '0') ||
            p_fmt[i].fourcc == VA_FOURCC('N', 'V', '1', '2')) {
            if (vaCreateImage(display, &p_fmt[i], w, h, &image)) {
                image.image_id = VA_INVALID_ID;
                continue;
            }
            /* Validate that vaGetImage works with this format */
            if (vaGetImage(display, pi_surface_id[0], 0, 0, w, h, image.image_id)) {
                vaDestroyImage(display, image.image_id);
                image.image_id = VA_INVALID_ID;
                continue;
            }
            //see vlc chroma.c map to AVPixelFormat. Can used by VideoFormat::PixelFormat
            i_chroma = QTAV_PIX_FMT_C(YUV420P);// VLC_CODEC_YV12; //VideoFormat::PixelFormat
            fmt = p_fmt[i];
            break;
        }
    }
    free(p_fmt);
    if (!i_chroma) {
        destroySurfaces();
        return false;
    }
    *chroma = i_chroma;

    if (supports_derive) {
        vaDestroyImage(display, image.image_id);
        image.image_id = VA_INVALID_ID;
    }
/*
    if (unlikely(CopyInitCache(&sys->image_cache, i_width)))
        goto error;
*/
    /* Setup the ffmpeg hardware context */
    *pp_hw_ctx = &hw_ctx;

    memset(&hw_ctx, 0, sizeof(hw_ctx));
    hw_ctx.display    = display;
    hw_ctx.config_id  = config_id;
    hw_ctx.context_id = context_id;

    /* */
    surface_chroma = i_chroma;
    surface_width = w;
    surface_height = h;
    return true;
}

void VideoDecoderVAAPIPrivate::destroySurfaces()
{
    if (image.image_id != VA_INVALID_ID) {
        //CopyCleanCache(&sys->image_cache);
        vaDestroyImage(display, image.image_id);
    } else if (supports_derive) {
        //CopyCleanCache(&sys->image_cache);
    }

    if (context_id != VA_INVALID_ID)
        vaDestroyContext(display, context_id);

    for (int i = 0; i < nb_surfaces && surfaces; i++) {
        vlc_va_surface_t *surface = &surfaces[i];
        if (surface->i_id != VA_INVALID_SURFACE)
            vaDestroySurfaces(display, &surface->i_id, 1);
    }
    //qDeleteAll(surfaces);
    //surfaces.clear();
    free(surfaces);
    surfaces = 0;
    /* */
    image.image_id = VA_INVALID_ID;
    context_id = VA_INVALID_ID;
    surface_width = 0;
    surface_height = 0;
    //vlc_mutex_destroy(&sys->lock);
}

bool VideoDecoderVAAPIPrivate::setup(void **hwctx, AVPixelFormat *chroma, int w, int h)
{
    if (surface_width == w && surface_height == h) {
        *hwctx = &hw_ctx;
        *chroma = surface_chroma;
        return true;
    }
    *hwctx = NULL;
    *chroma = QTAV_PIX_FMT_C(NONE);
    if (surface_width || surface_height)
        destroySurfaces();
    if (w > 0 && h > 0)
        return createSurfaces(hwctx, chroma, w, h);
    return false;
}

AVPixelFormat VideoDecoderVAAPIPrivate::getFormat(struct AVCodecContext *p_context, const AVPixelFormat *pi_fmt)
{
    bool can_hwaccel = false;
    for (size_t i = 0; pi_fmt[i] != QTAV_PIX_FMT_C(NONE); i++) {
        const AVPixFmtDescriptor *dsc = av_pix_fmt_desc_get(pi_fmt[i]);
        if (dsc == NULL)
            continue;
        bool hwaccel = (dsc->flags & AV_PIX_FMT_FLAG_HWACCEL) != 0;

        qDebug("available %sware decoder output format %d (%s)",
                 hwaccel ? "hard" : "soft", pi_fmt[i], dsc->name);
        if (hwaccel)
            can_hwaccel = true;
    }

    if (!can_hwaccel)
        goto end;

    /* Profile and level information is needed now.
     * TODO: avoid code duplication with avcodec.c */
#if 0
    if (p_context->profile != FF_PROFILE_UNKNOWN)
        p_dec->fmt_in.i_profile = p_context->profile;
    if (p_context->level != FF_LEVEL_UNKNOWN)
        p_dec->fmt_in.i_level = p_context->level;
#endif
    for (size_t i = 0; pi_fmt[i] != PIX_FMT_NONE; i++) {
        if (va_pixfmt != pi_fmt[i])
            continue;

        /* We try to call vlc_va_Setup when possible to detect errors when
         * possible (later is too late) */
        if (p_context->width > 0 && p_context->height > 0
         && !setup(&p_context->hwaccel_context, &p_context->pix_fmt, p_context->width, p_context->height)) {
            qWarning("acceleration setup failure");
            break;
        }

        qDebug("Using %s for hardware decoding.", qPrintable(description));

        /* FIXME this will disable direct rendering
         * even if a new pixel format is renegotiated
         */
        //p_sys->b_direct_rendering = false;
        //p_sys->p_va = p_va;
        p_context->draw_horiz_band = NULL;
        return pi_fmt[i];
    }

    close();
    //vlc_va_Delete(p_va);

end:
    /* Fallback to default behaviour */
    //p_sys->p_va = NULL;
    return avcodec_default_get_format(p_context, pi_fmt);
}

void VideoDecoderVAAPIPrivate::close()
{
    if (surface_width || surface_height)
        destroySurfaces();

    if (config_id != VA_INVALID_ID)
        vaDestroyConfig(display, config_id);
    if (display)
        vaTerminate(display);
    if (display_x11)
        XCloseDisplay(display_x11);
}

int VideoDecoderVAAPIPrivate::getBuffer(void **opaque, uint8_t **data)
{
    int i_old;
    int i;

    //vlc_mutex_lock(&sys->lock);
    /* Grab an unused surface, in case none are, try the oldest
     * XXX using the oldest is a workaround in case a problem happens with ffmpeg */
    for (i = 0, i_old = 0; i < nb_surfaces; i++) {
        vlc_va_surface_t *p_surface = &surfaces[i];

        if (!p_surface->i_refcount)
            break;

        if (p_surface->i_order < surfaces[i_old].i_order)
            i_old = i;
    }
    if (i >= nb_surfaces)
        i = i_old;
    //vlc_mutex_unlock(&sys->lock);

    vlc_va_surface_t *p_surface = &surfaces[i];

    p_surface->i_refcount = 1;
    p_surface->i_order = surface_order++;
    *data = (uint8_t*)p_surface->i_id; ///??
    *opaque = p_surface;

    return true;
}

void VideoDecoderVAAPIPrivate::releaseBuffer(void *opaque, uint8_t *data)
{
    Q_UNUSED(data);
    vlc_va_surface_t *p_surface = (vlc_va_surface_t*)opaque;

    //vlc_mutex_lock(p_surface->p_lock);
    p_surface->i_refcount--;
    //vlc_mutex_unlock(p_surface->p_lock);
}

} // namespace QtAV
