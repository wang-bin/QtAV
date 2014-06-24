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

#include <assert.h>
#include <algorithm>
#include <QtCore/QLibrary>
#include <QtCore/QMap>
#include <QtCore/QStringList>
#include <QtAV/Packet.h>
#include "QtAV/SurfaceInterop.h"
#include <QtAV/QtAV_Compat.h>
#include "utils/GPUMemCopy.h"
#include "prepost.h"
#include <va/va.h>
#include <libavcodec/vaapi.h>

//TODO: use dllapi
#include <fcntl.h> //open()
#include <unistd.h> //close()
//#include <xf86drm.h>
//#include <va/va_drm.h>
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

// TODO: add to QtAV_Compat.h?
// FF_API_PIX_FMT
#ifdef PixelFormat
#undef PixelFormat
#endif

#define VERSION_CHK(major, minor, patch) \
    (((major&0xff)<<16) | ((minor&0xff)<<8) | (patch&0xff))

//ffmpeg_vaapi patch: http://lists.libav.org/pipermail/libav-devel/2013-November/053515.html

namespace QtAV {

class VideoDecoderVAAPIPrivate;
class VideoDecoderVAAPI : public VideoDecoderFFmpegHW
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(VideoDecoderVAAPI)
    Q_PROPERTY(bool SSE4 READ SSE4 WRITE setSSE4)
    Q_PROPERTY(bool derive READ derive WRITE setDerive)
    Q_PROPERTY(int surfaces READ surfaces WRITE setSurfaces)
    Q_PROPERTY(QStringList displayPriority READ displayPriority WRITE setDisplayPriority)
    Q_PROPERTY(DisplayType display READ display WRITE setDisplay)
    Q_ENUMS(DisplayType)
public:
    enum DisplayType {
        X11,
        GLX,
        DRM
    };
    VideoDecoderVAAPI();
    virtual VideoDecoderId id() const;
    virtual QString description() const;
    virtual VideoFrame frame();

    // TODO: QObject property
    void setSSE4(bool y);
    bool SSE4() const;
    void setDerive(bool y);
    bool derive() const;
    void setSurfaces(int num);
    int surfaces() const;
    void setDisplayPriority(const QStringList& priority);
    QStringList displayPriority() const;
    DisplayType display() const;
    void setDisplay(DisplayType disp);
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

class VAAPI_DRM
{
public:
    VAAPI_DRM() {
        m_lib.setFileName("va-drm");
        if (m_lib.load()) {
            qDebug("libva-drm loaded");
        }
    }
    ~VAAPI_DRM() {
        if (m_lib.isLoaded()){
            m_lib.unload();
        }
    }

    bool isLoaded() const {
        return m_lib.isLoaded();
    }
    VADisplay vaGetDisplayDRM(int fd) {
        typedef VADisplay vaGetDisplayDRM_t(int fd);
        static vaGetDisplayDRM_t* fp_vaGetDisplayDRM = (vaGetDisplayDRM_t*)m_lib.resolve("vaGetDisplayDRM");
        assert(fp_vaGetDisplayDRM);
        return fp_vaGetDisplayDRM(fd);
    }
private:
    QLibrary m_lib;
};

class VAAPISurfaceInterop : public VideoSurfaceInterop//, public VAAPI_GLX
{
public:
    VAAPISurfaceInterop(VADisplay display)
        : mDisplay(display)
        , mpSurface(0)
        , mWidth(0)
        , mHeight(0)
    {
    }
    ~VAAPISurfaceInterop() {
#if QTAV_HAVE(VAAPI_GLX)
        if (tmp_surfaces.isEmpty())
            return;
        QMap<GLuint*,void*>::iterator it(tmp_surfaces.begin());
        while (it != tmp_surfaces.end()) {
            delete it.key();
            void *glxSurface = it.value();
            if (glxSurface) {
                vaDestroySurfaceGLX(mDisplay, glxSurface);
            }
            it = tmp_surfaces.erase(it);
        }
        it = glx_surfaces.begin();
        while (it != glx_surfaces.end()) {
            void *glxSurface = it.value();
            if (glxSurface) {
                vaDestroySurfaceGLX(mDisplay, glxSurface);
            }
            it = tmp_surfaces.erase(it);
        }
#endif
    }
    void setSurface(va_surface_t* surface, int width, int height) {
        mpSurface = surface;
        mWidth = width;
        mHeight = height;
    }
    // return glx surface
    void* createGLXSurface(void* handle) {
#if QTAV_HAVE(VAAPI_GLX)
        GLuint tex = *((GLuint*)handle);
        void *glxSurface = 0;
        VAStatus status = vaCreateSurfaceGLX(mDisplay, GL_TEXTURE_2D, tex, &glxSurface);
        if (status != VA_STATUS_SUCCESS) {
            qWarning("vaCreateSurfaceGLX(%p, GL_TEXTURE_2D, %u, %p) == %#x", mDisplay, tex, &glxSurface, status);
            return 0;
        }
        glx_surfaces[(GLuint*)handle] = glxSurface;
        return glxSurface;
#endif
        return 0;
    }
    virtual void* map(SurfaceType type, const VideoFormat& fmt, void* handle, int plane) {
        if (!fmt.isRGB())
            return 0;
        if (!handle)
            handle = createHandle(type, fmt, plane);
#if QTAV_HAVE(VAAPI_GLX)
        VAStatus status = VA_STATUS_SUCCESS;
        if (type == GLTextureSurface) {
            void *glxSurface = glx_surfaces[(GLuint*)handle];
            if (!glxSurface) {
                glxSurface = createGLXSurface(handle);
                if (!glxSurface) {
                    qWarning("Fail to create vaapi glx surface");
                    return 0;
                }
            }
            int flags = VA_FRAME_PICTURE | VA_SRC_BT709;
            //qDebug("vaCopySurfaceGLX(glSurface:%p, VASurfaceID:%#x) ref: %d", glxSurface, mpSurface->i_id, mpSurface->i_refcount);
            status = vaCopySurfaceGLX(mDisplay, glxSurface, mpSurface->i_id, flags);
            if (status != VA_STATUS_SUCCESS) {
                qWarning("vaCopySurfaceGLX(VADisplay:%p, glSurface:%p, VASurfaceID:%#x, flags:%d) == %d", mDisplay, glxSurface, mpSurface->i_id, flags, status);
                return 0;
            }
            if ((status = vaSyncSurface(mDisplay, mpSurface->i_id)) != VA_STATUS_SUCCESS) {
                qWarning("vaCopySurfaceGLX: %#x", status);
            }
            return handle;
        } else
#endif
        if (type == HostMemorySurface) {
        } else {
            return 0;
        }
        return handle;
    }
    virtual void unmap(void *handle) {
#if QTAV_HAVE(VAAPI_GLX)
        QMap<GLuint*,void*>::iterator it(tmp_surfaces.find((GLuint*)handle));
        if (it == tmp_surfaces.end())
            return;
        delete it.key();
        void *glxSurface = it.value();
        if (glxSurface) {
            vaDestroySurfaceGLX(mDisplay, glxSurface);
        }
        tmp_surfaces.erase(it);
#endif
    }
    virtual void* createHandle(SurfaceType type, const VideoFormat& fmt, int plane = 0) {
        Q_UNUSED(plane);
        if (type == GLTextureSurface) {
            if (!fmt.isRGB()) {
                return 0;
            }
#if QTAV_HAVE(VAAPI_GLX) //TODO: remove the macro if we use qtopengl not glx
            GLuint *tex = new GLuint;
            glGenTextures(1, tex);
            glBindTexture(GL_TEXTURE_2D, *tex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mWidth, mHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
            tmp_surfaces[tex] = 0;
            return tex;
#endif
        }
        return 0;
    }
private:
    VADisplay mDisplay;
    va_surface_t* mpSurface; //TODO: shared_ptr
    int mWidth, mHeight;
#if QTAV_HAVE(VAAPI_GLX)
    QMap<GLuint*,void*> glx_surfaces, tmp_surfaces;
#endif
};


class VideoDecoderVAAPIPrivate : public VideoDecoderFFmpegHWPrivate, public VAAPI_DRM
{
public:
    VideoDecoderVAAPIPrivate()
        : support_4k(true)
        , surface_interop(0)
    {
        if (VAAPI_DRM::isLoaded()) {
            display_type = VideoDecoderVAAPI::DRM;
        }
        drm_fd = -1;
#if QTAV_HAVE(VAAPI_X11)
        display_type = VideoDecoderVAAPI::X11;
        display_x11 = 0;
#endif
#if QTAV_HAVE(VAAPI_GLX)
        display_type = VideoDecoderVAAPI::GLX;
#endif
        display = 0;
        config_id = VA_INVALID_ID;
        context_id = VA_INVALID_ID;
        version_major = 0;
        version_minor = 0;
        surfaces = 0;
        surface_order = 0;
        surface_width = 0;
        surface_height = 0;
        surface_chroma = QTAV_PIX_FMT_C(NONE);
        image.image_id = VA_INVALID_ID;
        supports_derive = false;
        va_pixfmt = QTAV_PIX_FMT_C(VAAPI_VLD);
        // set by user. don't reset in when call destroy
        surface_auto = true;
        nb_surfaces = 0;
        disable_derive = false;
        copy_uswc = true;
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

    bool support_4k;
    VideoDecoderVAAPI::DisplayType display_type;
    QList<VideoDecoderVAAPI::DisplayType> display_priority;
#if QTAV_HAVE(VAAPI_X11) || QTAV_HAVE(VAAPI_GLX)
    Display *display_x11;
#endif
    int drm_fd;
    VADisplay     display;

    VAConfigID    config_id;
    VAContextID   context_id;

    struct vaapi_context hw_ctx;

    /* */
    int version_major;
    int version_minor;

    /* */
    QMutex  mutex;
    bool surface_auto;
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

    QString vendor;
    // false for not intel gpu. my test result is intel gpu is supper fast and lower cpu usage if use optimized uswc copy. but nv is worse.
    bool copy_uswc;
    GPUMemCopy gpu_mem;
    VAAPISurfaceInterop *surface_interop;
};


VideoDecoderVAAPI::VideoDecoderVAAPI()
    : VideoDecoderFFmpegHW(*new VideoDecoderVAAPIPrivate())
{
    setDisplayPriority(QStringList() << "GLX" << "X11" << "DRM");
}

VideoDecoderId VideoDecoderVAAPI::id() const
{
    return VideoDecoderId_VAAPI;
}

QString VideoDecoderVAAPI::description() const
{
    if (!d_func().description.isEmpty())
        return d_func().description;
    return "Video Acceleration API";
}

void VideoDecoderVAAPI::setSSE4(bool y)
{
    d_func().copy_uswc = y;
}

bool VideoDecoderVAAPI::SSE4() const
{
    return d_func().copy_uswc;
}

void VideoDecoderVAAPI::setDerive(bool y)
{
    d_func().disable_derive = !y;
}

bool VideoDecoderVAAPI::derive() const
{
    return !d_func().disable_derive;
}

void VideoDecoderVAAPI::setSurfaces(int num)
{
    DPTR_D(VideoDecoderVAAPI);
    d.nb_surfaces = num;
    d.surface_auto = num <= 0;
}

int VideoDecoderVAAPI::surfaces() const
{
    return d_func().nb_surfaces;
}

VideoDecoderVAAPI::DisplayType VideoDecoderVAAPI::display() const
{
    return d_func().display_type;
}

void VideoDecoderVAAPI::setDisplay(DisplayType disp)
{
    DPTR_D(VideoDecoderVAAPI);
    d.display_priority.clear();
    d.display_priority.append(disp);
    d.display_type = disp;
}

VideoFrame VideoDecoderVAAPI::frame()
{
    DPTR_D(VideoDecoderVAAPI);
    if (!d.frame->opaque || !d.frame->data[0])
        return VideoFrame();
    VASurfaceID surface_id = (VASurfaceID)(uintptr_t)d.frame->data[3];
    VAStatus status = VA_STATUS_SUCCESS;
    if (display() == GLX) {
        d.surface_interop->setSurface((va_surface_t*)d.frame->opaque, d.surface_width, d.surface_height);
        VideoFrame f(d.surface_width, d.surface_height, VideoFormat::Format_RGB32);
        f.setBytesPerLine(d.surface_width*4); //used by gl to compute texture size
        f.setSurfaceInterop(d.surface_interop);
        return f;
    }
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

    VideoFormat::PixelFormat pixfmt = VideoFormat::Format_Invalid;
    bool swap_uv = false;
    switch (d.image.format.fourcc) {
    case VA_FOURCC_YV12:
        swap_uv |= d.disable_derive || !d.supports_derive;
        pixfmt = VideoFormat::Format_YUV420P;
        break;
    case VA_FOURCC_IYUV:
        swap_uv = true;
        pixfmt = VideoFormat::Format_YUV420P;
        break;
    case VA_FOURCC_NV12:
        pixfmt = VideoFormat::Format_NV12;
        break;
    default:
        break;
    }
    if (pixfmt == VideoFormat::Format_Invalid) {
        qWarning("unsupported vaapi pixel format: %#x", d.image.format.fourcc);
        return VideoFrame();
    }
    const VideoFormat fmt(pixfmt);
    uint8_t *src[3];
    int pitch[3];
    for (int i = 0; i < fmt.planeCount(); ++i) {
        src[i] = (uint8_t*)p_base + d.image.offsets[i];
        pitch[i] = d.image.pitches[i];
    }
    if (swap_uv) {
        std::swap(src[1], src[2]);
        std::swap(pitch[1], pitch[2]);
    }
    VideoFrame frame;
    if (d.copy_uswc && d.gpu_mem.isReady()) {
        int yuv_size = 0;
        if (pixfmt == VideoFormat::Format_NV12)
            yuv_size = pitch[0]*d.surface_height*3/2;
        else
            yuv_size = pitch[0]*d.surface_height + pitch[1]*d.surface_height/2 + pitch[2]*d.surface_height/2;
        // additional 15 bytes to ensure 16 bytes aligned
        QByteArray buf(15 + yuv_size, 0);
        const int offset_16 = (16 - ((uintptr_t)buf.data() & 0x0f)) & 0x0f;
        // plane 1, 2... is aligned?
        uchar* plane_ptr = (uchar*)buf.data() + offset_16;
        QVector<uchar*> dst(fmt.planeCount(), 0);
        for (int i = 0; i < dst.size(); ++i) {
            dst[i] = plane_ptr;
            // TODO: add VideoFormat::planeWidth/Height() ?
            const int plane_w = pitch[i];//(i == 0 || pixfmt == VideoFormat::Format_NV12) ? d.surface_width : fmt.chromaWidth(d.surface_width);
            const int plane_h = i == 0 ? d.surface_height : fmt.chromaHeight(d.surface_height);
            plane_ptr += pitch[i] * plane_h;
            d.gpu_mem.copyFrame(src[i], dst[i], plane_w, plane_h, pitch[i]);
        }
        frame = VideoFrame(buf, d.width, d.height, fmt);
        frame.setBits(dst);
        frame.setBytesPerLine(pitch);
    } else {
        frame = VideoFrame(d.width, d.height, fmt);
        frame.setBits(src);
        frame.setBytesPerLine(pitch);
        // TODO: why clone is faster()?
        frame = frame.clone();
    }

    if ((status = vaUnmapBuffer(d.display, d.image.buf)) != VA_STATUS_SUCCESS) {
        qWarning("vaUnmapBuffer(VADisplay:%p, VABufferID:%#x) == %#x", d.display, d.image.buf, status);
        return VideoFrame();
    }

    if (!d.disable_derive && d.supports_derive) {
        vaDestroyImage(d.display, d.image.image_id);
        d.image.image_id = VA_INVALID_ID;
    }
    return frame;
}

struct display_names_t {
    VideoDecoderVAAPI::DisplayType display;
    QString name;
};
static const display_names_t display_names[] = {
    { VideoDecoderVAAPI::GLX, "GLX" },
    { VideoDecoderVAAPI::X11, "X11" },
    { VideoDecoderVAAPI::DRM, "DRM" }
};

static VideoDecoderVAAPI::DisplayType displayFromName(QString name) {
    for (unsigned int i = 0; i < sizeof(display_names)/sizeof(display_names[0]); ++i) {
        if (name.toUpper().contains(display_names[i].name.toUpper())) {
            return display_names[i].display;
        }
    }
    return VideoDecoderVAAPI::X11;
}

static QString displayToName(VideoDecoderVAAPI::DisplayType t) {
    for (unsigned int i = 0; i < sizeof(display_names)/sizeof(display_names[0]); ++i) {
        if (t == display_names[i].display) {
            return display_names[i].name;
        }
    }
    return QString();
}

void VideoDecoderVAAPI::setDisplayPriority(const QStringList &priority)
{
    DPTR_D(VideoDecoderVAAPI);
    d.display_priority.clear();
    foreach (QString disp, priority) {
        d.display_priority.push_back(displayFromName(disp));
    }
}

QStringList VideoDecoderVAAPI::displayPriority() const
{
    QStringList names;
    foreach (DisplayType disp, d_func().display_priority) {
        names.append(displayToName(disp));
    }
    return names;
}

bool VideoDecoderVAAPIPrivate::open()
{
    if (va_pixfmt != QTAV_PIX_FMT_C(NONE))
        codec_ctx->pix_fmt = va_pixfmt;
#define VAProfileNone ((VAProfile)-1) //maybe not defined for old va
    VAProfile i_profile = VAProfileNone;
    int i_surfaces = 0;
    switch (codec_ctx->codec_id) {
    case QTAV_CODEC_ID(MPEG1VIDEO):
    case QTAV_CODEC_ID(MPEG2VIDEO):
        i_profile = VAProfileMPEG2Main;
        i_surfaces = 2+1;
        break;
    case QTAV_CODEC_ID(MPEG4):
        i_profile = VAProfileMPEG4AdvancedSimple;
        i_surfaces = 2+1;
        break;
    case QTAV_CODEC_ID(WMV3):
        i_profile = VAProfileVC1Main;
        i_surfaces = 2+1;
        break;
    case QTAV_CODEC_ID(VC1):
        i_profile = VAProfileVC1Advanced;
        i_surfaces = 2+1;
        break;
    case QTAV_CODEC_ID(H264):
        i_profile = VAProfileH264High;
        i_surfaces = 16+1;
        break;
    default:
        return false;
    }
    if (surface_auto) {
        nb_surfaces = i_surfaces;
    }
    if (nb_surfaces <= 0) {
        qWarning("internal error: wrong surface count.  %u auto=%d", nb_surfaces, surface_auto);
        nb_surfaces = 17;
    }
    config_id  = VA_INVALID_ID;
    context_id = VA_INVALID_ID;
    image.image_id = VA_INVALID_ID;
    /* Create a VA display */
    display = 0;
    foreach (VideoDecoderVAAPI::DisplayType dt, display_priority) {
        if (dt == VideoDecoderVAAPI::DRM) {
            if (VAAPI_DRM::isLoaded()) {
                qDebug("vaGetDisplay DRM...............");
    // get drm use udev: https://gitorious.org/hwdecode-demos/hwdecode-demos/commit/d591cf14b83bedc8a5fa9f2fcb53d279e2f76d7f?diffmode=sidebyside
                // try drmOpen()?
                drm_fd = ::open("/dev/dri/card0", O_RDWR);
                if(drm_fd == -1) {
                    qWarning("Could not access rendering device");
                    continue;
                }
                display = vaGetDisplayDRM(drm_fd);
                display_type = VideoDecoderVAAPI::DRM;
            }
        } else if (dt == VideoDecoderVAAPI::X11) {
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
            display_type = VideoDecoderVAAPI::X11;
        } else if (dt == VideoDecoderVAAPI::GLX) {
            qDebug("vaGetDisplay GLX...............");
#if QTAV_HAVE(VAAPI_GLX)
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
            display = vaGetDisplayGLX(display_x11);
#endif
            display_type = VideoDecoderVAAPI::GLX;
        }
        if (display)
            break;
    }

    if (!display/* || vaDisplayIsValid(display) != 0*/) {
        qWarning("Could not get a VAAPI device");
        return false;
    }
    if (surface_interop) {
        delete surface_interop;
        surface_interop = 0;
    }
    surface_interop = new VAAPISurfaceInterop(display);
    if (vaInitialize(display, &version_major, &version_minor)) {
        qWarning("Failed to initialize the VAAPI device");
        return false;
    }
    vendor = vaQueryVendorString(display);
    if (!vendor.toLower().contains("intel"))
        copy_uswc = false;

    //disable_derive = !copy_uswc;

    description = QString("VA API version %1.%2; Vendor: %3;").arg(version_major).arg(version_minor).arg(vendor);
    description += " Display: " + displayToName(display_type);

    // check 4k support. from xbmc
    int major, minor, micro;
    if (sscanf(vendor.toUtf8().constData(), "Intel i965 driver - %d.%d.%d", &major, &minor, &micro) == 3) {
        /* older version will crash and burn */
        if (VERSION_CHK(major, minor, micro) < VERSION_CHK(1, 0, 17)) {
            qDebug("VAAPI - deinterlace not support on this intel driver version");
        }
        // do the same check for 4K decoding: version < 1.2.0 (stable) and 1.0.21 (staging)
        // cannot decode 4K and will crash the GPU
        if (VERSION_CHK(major, minor, micro) < VERSION_CHK(1, 2, 0) && VERSION_CHK(major, minor, micro) < VERSION_CHK(1, 0, 21)) {
            support_4k = false;
        }
    }
    if (!support_4k && (codec_ctx->width > 1920 || codec_ctx->height > 1088)) {
        qWarning("VAAPI: frame size (%dx%d) is too large", codec_ctx->width, codec_ctx->height);
        return false;
    }

    /* Check if the selected profile is supported */
    int i_profiles_nb = vaMaxNumProfiles(display);
    VAProfile *p_profiles_list = (VAProfile*)calloc(i_profiles_nb, sizeof(VAProfile));
    if (!p_profiles_list)
        return false;

    bool b_supported_profile = false;
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
    supports_derive = false;
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
    width = w;
    height = h;
    surface_width = FFALIGN(w, 16);
    surface_height = FFALIGN(h, 16);
    /* Create surfaces */
    VASurfaceID pi_surface_id[nb_surfaces];
    VAStatus status = VA_STATUS_SUCCESS;
    if ((status = vaCreateSurfaces(display, VA_RT_FORMAT_YUV420, surface_width, surface_height, pi_surface_id, nb_surfaces, NULL, 0)) != VA_STATUS_SUCCESS) {
        qWarning("vaCreateSurfaces(VADisplay:%p, VA_RT_FORMAT_YUV420, %d, %d, VASurfaceID*:%p, surfaces:%d, VASurfaceAttrib:NULL, num_attrib:0) == %#x", display, surface_width, surface_height, pi_surface_id, nb_surfaces, status);
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
    if ((status = vaCreateContext(display, config_id, surface_width, surface_height, VA_PROGRESSIVE, pi_surface_id, nb_surfaces, &context_id)) != VA_STATUS_SUCCESS) {
        qWarning("vaCreateContext(VADisplay:%p, VAConfigID:%#x, %d, %d, VA_PROGRESSIVE, VASurfaceID*:%p, surfaces:%d, VAContextID*:%p) == %#x", display, config_id, surface_width, surface_height, pi_surface_id, nb_surfaces, &context_id, status);
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
    for (int i = 0; i < i_fmt_count; i++) {
        if (p_fmt[i].fourcc == VA_FOURCC_YV12 ||
            p_fmt[i].fourcc == VA_FOURCC_IYUV ||
            p_fmt[i].fourcc == VA_FOURCC_NV12) {
            qDebug("vaCreateImage: %c%c%c%c", p_fmt[i].fourcc<<24>>24, p_fmt[i].fourcc<<16>>24, p_fmt[i].fourcc<<8>>24, p_fmt[i].fourcc>>24);
            if (vaCreateImage(display, &p_fmt[i], surface_width, surface_height, &image)) {
                image.image_id = VA_INVALID_ID;
                qDebug("vaCreateImage error: %c%c%c%c", p_fmt[i].fourcc<<24>>24, p_fmt[i].fourcc<<16>>24, p_fmt[i].fourcc<<8>>24, p_fmt[i].fourcc>>24);
                continue;
            }
            /* Validate that vaGetImage works with this format */
            if (vaGetImage(display, pi_surface_id[0], 0, 0, surface_width, surface_height, image.image_id)) {
                vaDestroyImage(display, image.image_id);
                qDebug("vaGetImage error: %c%c%c%c", p_fmt[i].fourcc<<24>>24, p_fmt[i].fourcc<<16>>24, p_fmt[i].fourcc<<8>>24, p_fmt[i].fourcc>>24);
                image.image_id = VA_INVALID_ID;
                continue;
            }
            //see vlc chroma.c map to AVPixelFormat. Can used by VideoFormat::PixelFormat
            i_chroma = QTAV_PIX_FMT_C(YUV420P);// VLC_CODEC_YV12; //VideoFormat::PixelFormat
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

    if (copy_uswc) {
        if (!gpu_mem.initCache(surface_width)) {
            // only set by user (except disabling copy_uswc for non-intel gpu)
            //copy_uswc = false;
            //disable_derive = true;
        }
    }

    /* Setup the ffmpeg hardware context */
    *pp_hw_ctx = &hw_ctx;

    memset(&hw_ctx, 0, sizeof(hw_ctx));
    hw_ctx.display    = display;
    hw_ctx.config_id  = config_id;
    hw_ctx.context_id = context_id;

    /* */
    surface_chroma = i_chroma;
    return true;
}

void VideoDecoderVAAPIPrivate::destroySurfaces()
{
    if (image.image_id != VA_INVALID_ID) {
        vaDestroyImage(display, image.image_id);
    }
    if (copy_uswc) {
        gpu_mem.cleanCache();
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
    if (surface_width == FFALIGN(w, 16) && surface_height == FFALIGN(h, 16)) {
        width = w;
        height = h;
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
    if (surface_interop) {
        delete surface_interop;
        surface_interop = 0;
    }
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
    if (drm_fd >= 0) {
        ::close(drm_fd);
        drm_fd = -1;
    }
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


// used by .moc QMetaType::Bool
#ifdef Bool
#undef Bool
#endif
#include "VideoDecoderVAAPI.moc"
