/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013-2014 Wang Bin <wbsecg1@gmail.com>

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

#include "QtAV/VideoDecoderFFmpegHW.h"
#include "private/VideoDecoderFFmpegHW_p.h"

#include <algorithm>

#include <QtAV/Packet.h>
#include <QtAV/QtAV_Compat.h>
#include "prepost.h"
#include <va/va.h>
#include <libavcodec/avcodec.h>
#include <libavcodec/vaapi.h>

//TODO: use dllapi
#if QTAV_HAVE(VAAPI_DRM)
#include <fcntl.h> //open()
#include <unistd.h> //close()
//#include <xf86drm.h>
#include <va/va_drm.h>
#endif
#if QTAV_HAVE(VAAPI_X11)
#include <X11/Xlib.h>
#include <va/va_x11.h>
#endif
#if QTAV_HAVE(VAAPI_GLX)
#include <va/va_glx.h>
#include <GL/gl.h>
#endif

#ifndef VA_SURFACE_ATTRIB_SETTABLE
#define vaCreateSurfaces(d, f, w, h, s, ns, a, na) \
    vaCreateSurfaces(d, w, h, f, ns, s)
#endif

namespace QtAV {

class VideoDecoderVAAPIPrivate;
class VideoDecoderVAAPI : public VideoDecoderFFmpegHW
{
    DPTR_DECLARE_PRIVATE(VideoDecoderVAAPI)
public:
    enum DisplayType {
        Display_X11,
        Display_GLX,
        Display_DRM
    };
    VideoDecoderVAAPI();
    virtual VideoDecoderId id() const;
    virtual VideoFrame frame();

    // TODO: QObject property
    void setDisplayTypePriority(const QStringList& priority);
    DisplayType displayType() const;
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
} va_surface_t;


class VideoDecoderVAAPIPrivate : public VideoDecoderFFmpegHWPrivate
{
public:
    VideoDecoderVAAPIPrivate() {
        display_type = VideoDecoderVAAPI::Display_X11;
#if QTAV_HAVE(VAAPI_X11)
        display_x11 = 0;
#endif
#if QTAV_HAVE(VAAPI_DRM)
        drm_fd = -1;
#endif
#if QTAV_HAVE(VAAPI_GLX)
        glxSurface = 0;
        texture = 0;
#endif
        display = 0;
        config_id = VA_INVALID_ID;
        context_id = VA_INVALID_ID;
        version_major = 0;
        version_minor = 0;
        nb_surfaces = 0;
        surface_order = 0;
        surface_width = 0;
        surface_height = 0;
        surface_chroma = QTAV_PIX_FMT_C(NONE);
        surfaces = 0;
        image.image_id = VA_INVALID_ID;
        disable_derive = true;
        supports_derive = false;
        va_pixfmt = QTAV_PIX_FMT_C(VAAPI_VLD);
    }

    ~VideoDecoderVAAPIPrivate() {
        //TODO:
    }
    virtual bool open();
    virtual void close();
    bool createSurfaces(void **hwctx, AVPixelFormat *chroma, int w, int h);
    void destroySurfaces();

    virtual bool setup(void **hwctx, AVPixelFormat *chroma, int w, int h);
    virtual bool getBuffer(void **opaque, uint8_t **data);
    virtual void releaseBuffer(void *opaque, uint8_t *data);

    VideoDecoderVAAPI::DisplayType display_type;
    QVector<VideoDecoderVAAPI::DisplayType> display_priority;
#if QTAV_HAVE(VAAPI_X11)
    Display *display_x11;
#endif
#if QTAV_HAVE(VAAPI_DRM)
    int drm_fd;
#endif
#if QTAV_HAVE(VAAPI_GLX)
    void* glxSurface;
    GLuint texture;
#endif

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

    //QVector<va_surface_t*> surfaces;
    va_surface_t *surfaces;

    VAImage      image;

    bool disable_derive;
    bool supports_derive;
};


VideoDecoderVAAPI::VideoDecoderVAAPI()
    : VideoDecoderFFmpegHW(*new VideoDecoderVAAPIPrivate())
{
    setDisplayTypePriority(QStringList() << "glx" << "x11" << "drm");
}

VideoDecoderId VideoDecoderVAAPI::id() const
{
    return VideoDecoderId_VAAPI;
}

VideoFrame VideoDecoderVAAPI::frame()
{
    DPTR_D(VideoDecoderVAAPI);
    if (!d.frame->opaque || !d.frame->data[0])
        return VideoFrame();
    VASurfaceID surface_id = (VASurfaceID)(uintptr_t)d.frame->data[3];
    VAStatus status = VA_STATUS_SUCCESS;
#if QTAV_HAVE(VAAPI_GLX)
    if (displayType() == Display_GLX) {
        if (!d.glxSurface) {
            // FIXME: wrong gl context
            glGenTextures(1, &d.texture);
            qDebug("==============glGenTextures(1, %d)", d.texture);

            glBindTexture(GL_TEXTURE_2D, d.texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, d.surface_width, d.surface_height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
            status = vaCreateSurfaceGLX(d.display, GL_TEXTURE_2D, d.texture, &d.glxSurface);
            if (status != VA_STATUS_SUCCESS) {
                qWarning("vaCreateSurfaceGLX(%p, GL_TEXTURE_2D, %u, %p) == %#x", d.display, d.texture, &d.glxSurface, status);
                return VideoFrame();
            }
        }
        int flags = VA_FRAME_PICTURE | VA_SRC_BT601;
        status = vaCopySurfaceGLX(d.display, d.glxSurface, surface_id, flags);
        if (status != VA_STATUS_SUCCESS) {
            qWarning("vaCopySurfaceGLX(VADisplay:%p, glSurface:%p, VASurfaceID:%#x, flags:%d) == %d", d.display, d.glxSurface, surface_id, flags, status);
            return VideoFrame();
        }
        if ((status = vaSyncSurface(d.display, surface_id)) != VA_STATUS_SUCCESS) {
            qWarning("vaCopySurfaceGLX: %#x", status);
        }
        return VideoFrame(QVector<int>() << d.texture, d.surface_width, d.surface_height, VideoFormat::Format_ARGB32);
    }
#endif
#if VA_CHECK_VERSION(0,31,0)
    if ((status = vaSyncSurface(d.display, surface_id)) != VA_STATUS_SUCCESS) {
        qWarning("vaSyncSurface(VADisplay:%p, VASurfaceID:%#x) == %#x", d.display, surface_id, status);
#else
    if (vaSyncSurface(d.display, d.context_id, surface_id)) {
        qWarning("vaSyncSurface(VADisplay:%#x, VAContextID:%#x, VASurfaceID:%#x) == %#x", d.display, d.context_id, surface_id, status);
#endif
        return VideoFrame();
    }

    if (!d.disable_derive && d.supports_derive) {
        /*
         * http://web.archiveorange.com/archive/v/OAywENyq88L319OcRnHI
         * vaDeriveImage is faster than vaGetImage. But VAImage is uncached memory and copying from it would be terribly slow
         * TODO: copy from USWC, see vlc and https://github.com/OpenELEC/OpenELEC.tv/pull/2937.diff
         * https://software.intel.com/en-us/articles/increasing-memory-throughput-with-intel-streaming-simd-extensions-4-intel-sse4-streaming-load
         */
        status = vaDeriveImage(d.display, surface_id, &d.image);
        if (status != VA_STATUS_SUCCESS) {
            qWarning("vaDeriveImage(VADisplay:%p, VASurfaceID:%#x, VAImage*:%p) == %#x", d.display, surface_id, &d.image, status);
            return VideoFrame();
        }
    } else {
        status = vaGetImage(d.display, surface_id, 0, 0, d.surface_width, d.surface_height, d.image.image_id);
        if (status != VA_STATUS_SUCCESS) {
            qWarning("vaGetImage(VADisplay:%p, VASurfaceID:%#x, 0,0, %d, %d, VAImageID:%#x) == %#x", d.display, surface_id, d.surface_width, d.surface_height, d.image.image_id, status);
            return VideoFrame();
        }
    }

    void *p_base;
    if ((status = vaMapBuffer(d.display, d.image.buf, &p_base)) != VA_STATUS_SUCCESS) {
        qWarning("vaMapBuffer(VADisplay:%p, VABufferID:%#x, pBuf:%p) == %#x", d.display, d.image.buf, &p_base, status);
        return VideoFrame();
    }

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
        //swap U V if use vaGetImage. have not confirmed why
        if (d.disable_derive || !d.supports_derive) {
            std::swap(pp_plane[1], pp_plane[2]);
            std::swap(pi_pitch[1], pi_pitch[2]);
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

    if ((status = vaUnmapBuffer(d.display, d.image.buf)) != VA_STATUS_SUCCESS) {
        qWarning("vaUnmapBuffer(VADisplay:%p, VABufferID:%#x) == %#x", d.display, d.image.buf, status);
        return VideoFrame();
    }

    if (!d.disable_derive && d.supports_derive) {
        vaDestroyImage(d.display, d.image.image_id);
        d.image.image_id = VA_INVALID_ID;
    }
    // TODO: why clone is faster()?
    // http://software.intel.com/en-us/articles/copying-accelerated-video-decode-frame-buffers
    return frame.clone();
}

void VideoDecoderVAAPI::setDisplayTypePriority(const QStringList &priority)
{
    DPTR_D(VideoDecoderVAAPI);
    d.display_priority.clear();
    foreach (QString disp, priority) {
        if (disp.toLower() == "drm")
            d.display_priority.push_back(Display_DRM);
        else if (disp.toLower() == "glx")
            d.display_priority.push_back(Display_GLX);
        else
            d.display_priority.push_back(Display_X11);
    }
}

VideoDecoderVAAPI::DisplayType VideoDecoderVAAPI::displayType() const
{
    return d_func().display_type;
}


bool VideoDecoderVAAPIPrivate::open()
{
    if (va_pixfmt != QTAV_PIX_FMT_C(NONE))
        codec_ctx->pix_fmt = va_pixfmt;
    VAProfile i_profile, *p_profiles_list;
    bool b_supported_profile = false;
    int i_profiles_nb = 0;
    int i_nb_surfaces;
    /* */
    switch (codec_ctx->codec_id) {
    case CODEC_ID_MPEG1VIDEO:
    case CODEC_ID_MPEG2VIDEO:
        i_profile = VAProfileMPEG2Main;
        i_nb_surfaces = 2+1;
        break;
    case CODEC_ID_MPEG4:
        i_profile = VAProfileMPEG4AdvancedSimple;
        i_nb_surfaces = 2+1;
        break;
    case CODEC_ID_WMV3:
        i_profile = VAProfileVC1Main;
        i_nb_surfaces = 2+1;
        break;
    case CODEC_ID_VC1:
        i_profile = VAProfileVC1Advanced;
        i_nb_surfaces = 2+1;
        break;
    case CODEC_ID_H264:
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
    foreach (VideoDecoderVAAPI::DisplayType dt, display_priority) {
        if (dt == VideoDecoderVAAPI::Display_DRM) {
            qDebug("vaGetDisplay DRM...............");
// get drm use udev: https://gitorious.org/hwdecode-demos/hwdecode-demos/commit/d591cf14b83bedc8a5fa9f2fcb53d279e2f76d7f?diffmode=sidebyside
#if QTAV_HAVE(VAAPI_DRM)
            drm_fd = ::open("/dev/dri/card0", O_RDWR);
            if(drm_fd == -1) {
                qWarning("Could not access rendering device");
                continue;
            }
            display = vaGetDisplayDRM(drm_fd);
#endif //QTAV_HAVE(VAAPI_DRM)
            display_type = VideoDecoderVAAPI::Display_DRM;
        } else if (dt == VideoDecoderVAAPI::Display_X11) {
            qDebug("vaGetDisplay X11...............");
#if QTAV_HAVE(VAAPI_X11)
            // TODO: lock
            if (!XInitThreads()) {
                qWarning("XInitThreads failed!");
                continue;
            }
            display_x11 = XOpenDisplay(NULL);;
            if (!display_x11) {
                qWarning("Could not connect to X server");
                continue;
            }
            display = vaGetDisplay(display_x11);
#endif //QTAV_HAVE(VAAPI_X11)
            display_type = VideoDecoderVAAPI::Display_X11;
        } else if (dt == VideoDecoderVAAPI::Display_GLX) {
            qDebug("vaGetDisplay GLX...............");
#if QTAV_HAVE(VAAPI_GLX)
            display_x11 = XOpenDisplay(NULL);;
            if (!display_x11) {
                qWarning("Could not connect to X server");
                continue;
            }
            display = vaGetDisplayGLX(display_x11);
#endif
            display_type = VideoDecoderVAAPI::Display_GLX;
        }
        if (display)
            break;
    }

    if (!display/* || vaDisplayIsValid(display) != 0*/) {
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

    VAStatus status = vaQueryConfigProfiles(display, p_profiles_list, &i_profiles_nb);
    if (status == VA_STATUS_SUCCESS) {
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
    if ((status = vaGetConfigAttributes(display, i_profile, VAEntrypointVLD, &attrib, 1)) != VA_STATUS_SUCCESS) {
        qWarning("vaGetConfigAttributes(VADisplay:%p, VAProfile:%d, VAEntrypointVLD, VAConfigAttrib*:%p, num_attrib:1) == %#x", display, i_profile, &attrib, status);
        return false;
    }
    /* Not sure what to do if not, I don't have a way to test */
    if ((attrib.value & VA_RT_FORMAT_YUV420) == 0)
        return false;
    //vaCreateConfig(display, i_profile, VAEntrypointVLD, NULL, 0, &config_id)
    if ((status = vaCreateConfig(display, i_profile, VAEntrypointVLD, &attrib, 1, &config_id)) != VA_STATUS_SUCCESS) {
        qWarning("vaCreateConfig(VADisplay:%p, VAProfile:%d, VAEntrypointVLD, VAConfigAttrib*:%p, num_attrib:1, VAConfigID*:%p) == %#x", display, i_profile, &attrib, &config_id, status);
        config_id = VA_INVALID_ID;
        return false;
    }
    nb_surfaces = i_nb_surfaces;
    supports_derive = false;

    description = QString("VA API version %1.%2; Vendor: %3;").arg(version_major).arg(version_minor).arg(vaQueryVendorString(display));
    switch (display_type) {
    case VideoDecoderVAAPI::Display_X11:
        description += " Display: X11";
        break;
    case VideoDecoderVAAPI::Display_DRM:
        description += " Display: DRM";
        break;
    case VideoDecoderVAAPI::Display_GLX:
        description += " Display: GLX";
        break;
    default:
        break;
    }
    return true;
}

bool VideoDecoderVAAPIPrivate::createSurfaces(void **pp_hw_ctx, AVPixelFormat *chroma, int w, int h)
{
    Q_ASSERT(w > 0 && h > 0);
    /* */
    surfaces = (va_surface_t*)calloc(nb_surfaces, sizeof(va_surface_t));
    if (!surfaces)
        return false;
    image.image_id = VA_INVALID_ID;
    context_id = VA_INVALID_ID;

    /* Create surfaces */
    VASurfaceID pi_surface_id[nb_surfaces];
    VAStatus status = VA_STATUS_SUCCESS;
    if ((status = vaCreateSurfaces(display, VA_RT_FORMAT_YUV420, w, h, pi_surface_id, nb_surfaces, NULL, 0)) != VA_STATUS_SUCCESS) {
        qWarning("vaCreateSurfaces(VADisplay:%p, VA_RT_FORMAT_YUV420, %d, %d, VASurfaceID*:%p, surfaces:%d, VASurfaceAttrib:NULL, num_attrib:0) == %#x", display, w, h, pi_surface_id, nb_surfaces, status);
        for (int i = 0; i < nb_surfaces; i++)
            surfaces[i].i_id = VA_INVALID_SURFACE;
        destroySurfaces();
        return false;
    }
    for (int i = 0; i < nb_surfaces; i++) {
        va_surface_t *surface = &surfaces[i];
        surface->i_id = pi_surface_id[i];
        surface->i_refcount = 0;
        surface->i_order = 0;
        ////surface->p_lock = &sys->lock;
    }
    /* Create a context */
    if ((status = vaCreateContext(display, config_id, w, h, VA_PROGRESSIVE, pi_surface_id, nb_surfaces, &context_id)) != VA_STATUS_SUCCESS) {
        qWarning("vaCreateContext(VADisplay:%p, VAConfigID:%#x, %d, %d, VA_PROGRESSIVE, VASurfaceID*:%p, surfaces:%d, VAContextID*:%p) == %#x", display, config_id, w, h, pi_surface_id, nb_surfaces, &context_id, status);
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

    if (!disable_derive) {
        if (vaDeriveImage(display, pi_surface_id[0], &test_image) == VA_STATUS_SUCCESS) {
            qDebug("vaDeriveImage supported");
            supports_derive = true;
            vaDestroyImage(display, test_image.image_id);
        }
    }

    AVPixelFormat i_chroma = QTAV_PIX_FMT_C(NONE);
    VAImageFormat fmt;
    for (int i = 0; i < i_fmt_count; i++) {
        if (p_fmt[i].fourcc == VA_FOURCC('Y', 'V', '1', '2') ||
            p_fmt[i].fourcc == VA_FOURCC('I', '4', '2', '0') ||
            p_fmt[i].fourcc == VA_FOURCC('N', 'V', '1', '2')) {
            qDebug("vaCreateImage: %c%c%c%c", p_fmt[i].fourcc<<24>>24, p_fmt[i].fourcc<<16>>24, p_fmt[i].fourcc<<8>>24, p_fmt[i].fourcc>>24);
            if (vaCreateImage(display, &p_fmt[i], w, h, &image)) {
                image.image_id = VA_INVALID_ID;
                qDebug("vaCreateImage error: %c%c%c%c", p_fmt[i].fourcc<<24>>24, p_fmt[i].fourcc<<16>>24, p_fmt[i].fourcc<<8>>24, p_fmt[i].fourcc>>24);
                continue;
            }
            /* Validate that vaGetImage works with this format */
            if (vaGetImage(display, pi_surface_id[0], 0, 0, w, h, image.image_id)) {
                vaDestroyImage(display, image.image_id);
                qDebug("vaGetImage error: %c%c%c%c", p_fmt[i].fourcc<<24>>24, p_fmt[i].fourcc<<16>>24, p_fmt[i].fourcc<<8>>24, p_fmt[i].fourcc>>24);
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
    if (i_chroma == QTAV_PIX_FMT_C(NONE)) {
        destroySurfaces();
        return false;
    }
    *chroma = i_chroma;

    if (!disable_derive && supports_derive) {
        vaDestroyImage(display, image.image_id);
        image.image_id = VA_INVALID_ID;
    }

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
        vaDestroyImage(display, image.image_id);
    }
    if (context_id != VA_INVALID_ID)
        vaDestroyContext(display, context_id);

    for (int i = 0; i < nb_surfaces && surfaces; i++) {
        va_surface_t *surface = &surfaces[i];
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
    //*chroma = QTAV_PIX_FMT_C(NONE);
    if (surface_width || surface_height)
        destroySurfaces();
    if (w > 0 && h > 0)
        return createSurfaces(hwctx, chroma, w, h);
    return false;
}

void VideoDecoderVAAPIPrivate::close()
{
    restore();
    if (surface_width || surface_height)
        destroySurfaces();

    if (config_id != VA_INVALID_ID) {
        vaDestroyConfig(display, config_id);
        config_id = VA_INVALID_ID;
    }
#if QTAV_HAVE(VAAPI_GLX)
    if (glxSurface) {
        vaDestroySurfaceGLX(display, glxSurface);
        glxSurface = 0;
    }
#endif
    if (display) {
        vaTerminate(display);
        display = 0;
    }
#if QTAV_HAVE(VAAPI_X11)
    if (display_x11) {
        XCloseDisplay(display_x11);
        display_x11 = 0;
    }
#endif
#if QTAV_HAVE(VAAPI_DRM)
    if (drm_fd >= 0) {
        ::close(drm_fd);
        drm_fd = -1;
    }
#endif
}

bool VideoDecoderVAAPIPrivate::getBuffer(void **opaque, uint8_t **data)
{
    int i_old;
    int i;

//    QMutexLocker lock(&mutex);
//    Q_UNUSED(lock);
    /* Grab an unused surface, in case none are, try the oldest
     * XXX using the oldest is a workaround in case a problem happens with ffmpeg */
    for (i = 0, i_old = 0; i < nb_surfaces; i++) {
        va_surface_t *p_surface = &surfaces[i];

        if (!p_surface->i_refcount)
            break;

        if (p_surface->i_order < surfaces[i_old].i_order)
            i_old = i;
    }
    if (i >= nb_surfaces)
        i = i_old;
    //vlc_mutex_unlock(&sys->lock);

    va_surface_t *p_surface = &surfaces[i];

    p_surface->i_refcount = 1;
    p_surface->i_order = surface_order++;
    //FIXME: warning: cast to pointer from integer of different size [-Wint-to-pointer-cast]
    *data = (uint8_t*)p_surface->i_id; ///??
    *opaque = p_surface;

    return true;
}

void VideoDecoderVAAPIPrivate::releaseBuffer(void *opaque, uint8_t *data)
{
    Q_UNUSED(data);
    va_surface_t *p_surface = (va_surface_t*)opaque;

//    QMutexLocker lock(&mutex);
//    Q_UNUSED(lock);
    p_surface->i_refcount--;
}

} // namespace QtAV
