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
#include "QtAV/private/VideoDecoderFFmpegHW_p.h"
#include <algorithm>
#include <list>
#include <QtCore/QList>
#include <QtCore/QMetaEnum>
#include <QtCore/QSharedPointer>
#include <QtCore/QStringList>
extern "C" {
#include <libavcodec/vaapi.h>
}
#include <fcntl.h> //open()
#include <unistd.h> //close()
#include "QtAV/Packet.h"
#include "QtAV/SurfaceInterop.h"
#include "QtAV/private/AVCompat.h"
#include "utils/GPUMemCopy.h"
#include "QtAV/private/prepost.h"
#include "vaapi/vaapi_helper.h"
#include "vaapi/SurfaceInteropVAAPI.h"
#include "utils/Logger.h"

// TODO: add to AVCompat.h?
// FF_API_PIX_FMT
#ifdef PixelFormat
#undef PixelFormat
#endif

#define VERSION_CHK(major, minor, patch) \
    (((major&0xff)<<16) | ((minor&0xff)<<8) | (patch&0xff))

//ffmpeg_vaapi patch: http://lists.libav.org/pipermail/libav-devel/2013-November/053515.html

namespace QtAV {
using namespace vaapi;

class VideoDecoderVAAPIPrivate;
class VideoDecoderVAAPI : public VideoDecoderFFmpegHW
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(VideoDecoderVAAPI)
    Q_PROPERTY(bool SSE4 READ SSE4 WRITE setSSE4)
    Q_PROPERTY(bool derive READ derive WRITE setDerive)
    Q_PROPERTY(int surfaces READ surfaces WRITE setSurfaces)
    //Q_PROPERTY(QStringList displayPriority READ displayPriority WRITE setDisplayPriority)
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

    // QObject properties
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

class VideoDecoderVAAPIPrivate : public VideoDecoderFFmpegHWPrivate, public VAAPI_DRM, public VAAPI_X11, public VAAPI_GLX
        , public X11_API
{
    DPTR_DECLARE_PUBLIC(VideoDecoderVAAPI)
public:
    VideoDecoderVAAPIPrivate()
        : support_4k(true)
    {
        if (VAAPI_X11::isLoaded())
            display_type = VideoDecoderVAAPI::X11;
        if (VAAPI_DRM::isLoaded())
            display_type = VideoDecoderVAAPI::DRM;
        if (VAAPI_GLX::isLoaded())
            display_type = VideoDecoderVAAPI::GLX;
        drm_fd = -1;
        display_x11 = 0;
        config_id = VA_INVALID_ID;
        context_id = VA_INVALID_ID;
        version_major = 0;
        version_minor = 0;
        surface_width = 0;
        surface_height = 0;
        image.image_id = VA_INVALID_ID;
        supports_derive = false;
        // set by user. don't reset in when call destroy
        surface_auto = true;
        nb_surfaces = 0;
        disable_derive = false;
        copy_uswc = true;
    }
    ~VideoDecoderVAAPIPrivate() {}
    virtual bool open();
    virtual void close();
    bool createSurfaces(int count, void **hwctx, int w, int h);
    void destroySurfaces();

    virtual bool setup(void **hwctx, int w, int h);
    virtual bool getBuffer(void **opaque, uint8_t **data);
    virtual void releaseBuffer(void *opaque, uint8_t *data);
    virtual AVPixelFormat vaPixelFormat() const { return QTAV_PIX_FMT_C(VAAPI_VLD); }

    bool support_4k;
    VideoDecoderVAAPI::DisplayType display_type;
    QList<VideoDecoderVAAPI::DisplayType> display_priority;
    Display *display_x11;
    int drm_fd;
    display_ptr display;

    VAConfigID    config_id;
    VAContextID   context_id;

    struct vaapi_context hw_ctx;
    int version_major;
    int version_minor;
    bool surface_auto;
    int surface_width;
    int surface_height;

    int nb_surfaces;
    QVector<VASurfaceID> surfaces;
    std::list<surface_ptr> surfaces_free, surfaces_used;
    VAImage image;
    bool disable_derive;
    bool supports_derive;

    QString vendor;
    // false for not intel gpu. my test result is intel gpu is supper fast and lower cpu usage if use optimized uswc copy. but nv is worse.
    bool copy_uswc;
    GPUMemCopy gpu_mem;
    VideoSurfaceInteropPtr surface_interop; //may be still used in video frames when decoder is destroyed
};


VideoDecoderVAAPI::VideoDecoderVAAPI()
    : VideoDecoderFFmpegHW(*new VideoDecoderVAAPIPrivate())
{
    setDisplayPriority(QStringList() << "GLX" << "X11" << "DRM");
    // dynamic properties about static property details. used by UI
    // format: detail_property
    setProperty("detail_surfaces", tr("Decoding surfaces.") + " " + tr("0: auto"));
    setProperty("detail_SSE4", tr("Optimized copy decoded data from USWC memory using SSE4.1"));
    setProperty("detail_derive", tr("Maybe faster if display is not GLX"));
    setProperty("detail_display", tr("GLX is fastest. No data copyback from gpu."));
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
        surface_ptr p;
        std::list<surface_ptr>::iterator it = d.surfaces_used.begin();
        for (; it != d.surfaces_used.end() && !p; ++it) {
            if((*it)->get() == surface_id) {
                p = *it;
                break;
            }
        }
        if (!p) {
            for (it = d.surfaces_free.begin(); it != d.surfaces_free.end() && !p; ++it) {
                if((*it)->get() == surface_id) {
                    p = *it;
                    break;
                }
            }
        }
        if (!p) {
            qWarning("VAAPI - Unable to find surface");
            return VideoFrame();
        }
        ((SurfaceInteropVAAPI*)d.surface_interop.data())->setSurface(p);
        VideoFrame f(d.width, d.height, VideoFormat::Format_RGB32); //p->width()
        f.setBytesPerLine(d.width*4); //used by gl to compute texture size
        f.setMetaData("surface_interop", QVariant::fromValue(d.surface_interop));
        return f;
    }
#if VA_CHECK_VERSION(0,31,0)
    if ((status = vaSyncSurface(d.display->get(), surface_id)) != VA_STATUS_SUCCESS) {
        qWarning("vaSyncSurface(VADisplay:%p, VASurfaceID:%#x) == %#x", d.display->get(), surface_id, status);
#else
    if (vaSyncSurface(d.display->get(), d.context_id, surface_id)) {
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
        status = vaDeriveImage(d.display->get(), surface_id, &d.image);
        if (status != VA_STATUS_SUCCESS) {
            qWarning("vaDeriveImage(VADisplay:%p, VASurfaceID:%#x, VAImage*:%p) == %#x", d.display->get(), surface_id, &d.image, status);
            return VideoFrame();
        }
    } else {
        status = vaGetImage(d.display->get(), surface_id, 0, 0, d.width, d.height, d.image.image_id);
        if (status != VA_STATUS_SUCCESS) {
            qWarning("vaGetImage(VADisplay:%p, VASurfaceID:%#x, 0,0, %d, %d, VAImageID:%#x) == %#x", d.display->get(), surface_id, d.width, d.height, d.image.image_id, status);
            return VideoFrame();
        }
    }

    void *p_base;
    if ((status = vaMapBuffer(d.display->get(), d.image.buf, &p_base)) != VA_STATUS_SUCCESS) {
        qWarning("vaMapBuffer(VADisplay:%p, VABufferID:%#x, pBuf:%p) == %#x", d.display->get(), d.image.buf, &p_base, status);
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

    if ((status = vaUnmapBuffer(d.display->get(), d.image.buf)) != VA_STATUS_SUCCESS) {
        qWarning("vaUnmapBuffer(VADisplay:%p, VABufferID:%#x) == %#x", d.display->get(), d.image.buf, status);
        return VideoFrame();
    }

    if (!d.disable_derive && d.supports_derive) {
        vaDestroyImage(d.display->get(), d.image.image_id);
        d.image.image_id = VA_INVALID_ID;
    }
    return frame;
}

void VideoDecoderVAAPI::setDisplayPriority(const QStringList &priority)
{
    DPTR_D(VideoDecoderVAAPI);
    d.display_priority.clear();
    int idx = staticMetaObject.indexOfEnumerator("DisplayType");
    const QMetaEnum me = staticMetaObject.enumerator(idx);
    foreach (const QString& disp, priority) {
        d.display_priority.push_back((DisplayType)me.keyToValue(disp.toUtf8().constData()));
    }
}

QStringList VideoDecoderVAAPI::displayPriority() const
{
    QStringList names;
    int idx = staticMetaObject.indexOfEnumerator("DisplayType");
    const QMetaEnum me = staticMetaObject.enumerator(idx);
    foreach (DisplayType disp, d_func().display_priority) {
        names.append(me.valueToKey(disp));
    }    
    return names;
}

bool VideoDecoderVAAPIPrivate::open()
{
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
        i_surfaces = 16+2 + codec_ctx->thread_count;
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
    VADisplay disp = 0;
    foreach (VideoDecoderVAAPI::DisplayType dt, display_priority) {
        if (dt == VideoDecoderVAAPI::DRM) {
            if (!VAAPI_DRM::isLoaded())
                continue;
            qDebug("vaGetDisplay DRM...............");
// get drm use udev: https://gitorious.org/hwdecode-demos/hwdecode-demos/commit/d591cf14b83bedc8a5fa9f2fcb53d279e2f76d7f?diffmode=sidebyside
            // try drmOpen()?
            drm_fd = ::open("/dev/dri/card0", O_RDWR);
            if(drm_fd == -1) {
                qWarning("Could not access rendering device");
                continue;
            }
            disp = vaGetDisplayDRM(drm_fd);
            display_type = VideoDecoderVAAPI::DRM;
        } else if (dt == VideoDecoderVAAPI::X11) {
            qDebug("vaGetDisplay X11...............");
            if (!VAAPI_X11::isLoaded())
                continue;
            // TODO: lock
            if (!XInitThreads()) {
                qWarning("XInitThreads failed!");
                continue;
            }
            display_x11 =  XOpenDisplay(NULL);;
            if (!display_x11) {
                qWarning("Could not connect to X server");
                continue;
            }
            disp = vaGetDisplay(display_x11);
            display_type = VideoDecoderVAAPI::X11;
        } else if (dt == VideoDecoderVAAPI::GLX) {
            qDebug("vaGetDisplay GLX...............");
            if (!VAAPI_GLX::isLoaded())
                continue;
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
            disp = vaGetDisplayGLX(display_x11);
            display_type = VideoDecoderVAAPI::GLX;
        }
        if (disp)
            break;
    }
    if (!disp/* || vaDisplayIsValid(display) != 0*/) {
        qWarning("Could not get a VAAPI device");
        return false;
    }
    display = display_ptr(new display_t(disp));
    if (vaInitialize(disp, &version_major, &version_minor)) {
        qWarning("Failed to initialize the VAAPI device");
        return false;
    }
    vendor = vaQueryVendorString(disp);
    if (!vendor.toLower().contains("intel"))
        copy_uswc = false;

    //disable_derive = !copy_uswc;
    description = QString("VA API version %1.%2; Vendor: %3;").arg(version_major).arg(version_minor).arg(vendor);
    DPTR_P(VideoDecoderVAAPI);
    int idx = p.staticMetaObject.indexOfEnumerator("DisplayType");
    const QMetaEnum me = p.staticMetaObject.enumerator(idx);
    description += " Display: " + QString(me.valueToKey(display_type));
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
    int i_profiles_nb = vaMaxNumProfiles(disp);
    VAProfile *p_profiles_list = (VAProfile*)calloc(i_profiles_nb, sizeof(VAProfile));
    if (!p_profiles_list)
        return false;

    bool b_supported_profile = false;
    VAStatus status = vaQueryConfigProfiles(disp, p_profiles_list, &i_profiles_nb);
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
    if ((status = vaGetConfigAttributes(disp, i_profile, VAEntrypointVLD, &attrib, 1)) != VA_STATUS_SUCCESS) {
        qWarning("vaGetConfigAttributes(VADisplay:%p, VAProfile:%d, VAEntrypointVLD, VAConfigAttrib*:%p, num_attrib:1) == %#x", disp, i_profile, &attrib, status);
        return false;
    }
    /* Not sure what to do if not, I don't have a way to test */
    if ((attrib.value & VA_RT_FORMAT_YUV420) == 0)
        return false;
    //vaCreateConfig(display, i_profile, VAEntrypointVLD, NULL, 0, &config_id)
    if ((status = vaCreateConfig(disp, i_profile, VAEntrypointVLD, &attrib, 1, &config_id)) != VA_STATUS_SUCCESS) {
        qWarning("vaCreateConfig(VADisplay:%p, VAProfile:%d, VAEntrypointVLD, VAConfigAttrib*:%p, num_attrib:1, VAConfigID*:%p) == %#x", disp, i_profile, &attrib, &config_id, status);
        config_id = VA_INVALID_ID;
        return false;
    }
    supports_derive = false;
    surface_interop = VideoSurfaceInteropPtr(new SurfaceInteropVAAPI());
    return true;
}

bool VideoDecoderVAAPIPrivate::createSurfaces(int count, void **pp_hw_ctx, int w, int h)
{
    Q_ASSERT(w > 0 && h > 0);
    const int kMaxSurfaces = 32;
    if (count > kMaxSurfaces) {
        qWarning("VAAPI- Too many surfaces. requested: %d, maximun: %d", count, kMaxSurfaces);
        return false;
    }
    const int old_size = surfaces.size();
    const bool size_changed = (surface_width != FFALIGN(w, 16) || surface_height != FFALIGN(h, 16));
    if (count <= old_size && !size_changed)
        return true;
    surfaces.resize(count);
    image.image_id = VA_INVALID_ID;
    context_id = VA_INVALID_ID;
    width = w;
    height = h;
    surface_width = FFALIGN(w, 16);
    surface_height = FFALIGN(h, 16);
    VAStatus status = VA_STATUS_SUCCESS;
    if ((status = vaCreateSurfaces(display->get(), VA_RT_FORMAT_YUV420, width, height,  surfaces.data() + old_size, count - old_size, NULL, 0)) != VA_STATUS_SUCCESS) {
        qWarning("vaCreateSurfaces(VADisplay:%p, VA_RT_FORMAT_YUV420, %d, %d, VASurfaceID*:%p, surfaces:%d, VASurfaceAttrib:NULL, num_attrib:0) == %#x", display->get(), width, height, surfaces.constData(), surfaces.size(), status);
        destroySurfaces(); //?
        return false;
    }
    for (int i = old_size; i < surfaces.size(); ++i) {
        surfaces_free.push_back(surface_ptr(new surface_t(width, height, surfaces[i], display)));
    }
    /* Create a context */
    if (context_id && context_id != VA_INVALID_ID)
        VAWARN(vaDestroyContext(display->get(), context_id));
    context_id = VA_INVALID_ID;
    if ((status = vaCreateContext(display->get(), config_id, width, height, VA_PROGRESSIVE, surfaces.data(), surfaces.size(), &context_id)) != VA_STATUS_SUCCESS) {
        qWarning("vaCreateContext(VADisplay:%p, VAConfigID:%#x, %d, %d, VA_PROGRESSIVE, VASurfaceID*:%p, surfaces:%d, VAContextID*:%p) == %#x", display->get(), config_id, width, height, surfaces.constData(), surfaces.size(), &context_id, status);
        context_id = VA_INVALID_ID;
        destroySurfaces();
        return false;
    }
    if (!size_changed)
        return true;
    // TODO: move to other place?
    /* Find and create a supported image chroma */
    int i_fmt_count = vaMaxNumImageFormats(display->get());
    //av_mallocz_array
    VAImageFormat *p_fmt = (VAImageFormat*)calloc(i_fmt_count, sizeof(*p_fmt));
    if (!p_fmt) {
        destroySurfaces();
        return false;
    }
    if (vaQueryImageFormats(display->get(), p_fmt, &i_fmt_count)) {
        free(p_fmt);
        destroySurfaces();
        return false;
    }
    VAImage test_image;
    if (!disable_derive) {
        if (vaDeriveImage(display->get(), surfaces[0], &test_image) == VA_STATUS_SUCCESS) {
            qDebug("vaDeriveImage supported");
            supports_derive = true;
            vaDestroyImage(display->get(), test_image.image_id);
        }
    }
    AVPixelFormat i_chroma = QTAV_PIX_FMT_C(NONE);
    for (int i = 0; i < i_fmt_count; i++) {
        if (p_fmt[i].fourcc == VA_FOURCC_YV12 ||
            p_fmt[i].fourcc == VA_FOURCC_IYUV ||
            p_fmt[i].fourcc == VA_FOURCC_NV12) {
            qDebug("vaCreateImage: %c%c%c%c", p_fmt[i].fourcc<<24>>24, p_fmt[i].fourcc<<16>>24, p_fmt[i].fourcc<<8>>24, p_fmt[i].fourcc>>24);
            if (vaCreateImage(display->get(), &p_fmt[i], width, height, &image)) {
                image.image_id = VA_INVALID_ID;
                qDebug("vaCreateImage error: %c%c%c%c", p_fmt[i].fourcc<<24>>24, p_fmt[i].fourcc<<16>>24, p_fmt[i].fourcc<<8>>24, p_fmt[i].fourcc>>24);
                continue;
            }
            /* Validate that vaGetImage works with this format */
            if (vaGetImage(display->get(), surfaces[0], 0, 0, width, height, image.image_id)) {
                vaDestroyImage(display->get(), image.image_id);
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
    if (!disable_derive && supports_derive) {
        vaDestroyImage(display->get(), image.image_id);
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
    hw_ctx.display = display->get();
    hw_ctx.config_id = config_id;
    hw_ctx.context_id = context_id;
    return true;
}

void VideoDecoderVAAPIPrivate::destroySurfaces()
{
    if (image.image_id != VA_INVALID_ID) {
        VAWARN(vaDestroyImage(display->get(), image.image_id));
    }
    if (copy_uswc) {
        gpu_mem.cleanCache();
    }
    if (context_id != VA_INVALID_ID)
        VAWARN(vaDestroyContext(display->get(), context_id));
    nb_surfaces = 0;
    surfaces.clear();
    surfaces_free.clear();
    surfaces_used.clear();
    image.image_id = VA_INVALID_ID;
    context_id = VA_INVALID_ID;
    surface_width = 0;
    surface_height = 0;
}

bool VideoDecoderVAAPIPrivate::setup(void **hwctx, int w, int h)
{
    if (surface_width == FFALIGN(w, 16) && surface_height == FFALIGN(h, 16)) {
        width = w;
        height = h;
        *hwctx = &hw_ctx;
        return true;
    }
    *hwctx = NULL;
    //*chroma = QTAV_PIX_FMT_C(NONE);
    if (surface_width || surface_height)
        destroySurfaces();
    if (w > 0 && h > 0)
        return createSurfaces(nb_surfaces, hwctx, w, h);
    return false;
}

void VideoDecoderVAAPIPrivate::close()
{
    restore();
    if (surface_width || surface_height)
        destroySurfaces();

    if (config_id != VA_INVALID_ID) {
        VAWARN(vaDestroyConfig(display->get(), config_id));
        config_id = VA_INVALID_ID;
    }
    display.clear();
    if (display_x11) {
        //XCloseDisplay(display_x11); //FIXME: why crash?
        display_x11 = 0;
    }
    if (drm_fd >= 0) {
        ::close(drm_fd);
        drm_fd = -1;
    }
}

bool VideoDecoderVAAPIPrivate::getBuffer(void **opaque, uint8_t **data)
{
    VASurfaceID id = (VASurfaceID)(quintptr)*data;
    std::list<surface_ptr>::iterator it = surfaces_free.begin();
    if (id && id != VA_INVALID_ID) {
        bool found = false;
        for (; it != surfaces_free.end(); ++it) { //?
            if ((*it)->get() == id) {
                found = true;
                break;
            }
        }
        if (!found) {
            qWarning("surface not found!!!!!!!!!!!!!");
            return false;
        }
    } else {
        for (; it != surfaces_free.end() && it->count() > 1; ++it) {}
        if (it == surfaces_free.end()) {
            if (!surfaces_free.empty())
                qWarning("VAAPI - renderer still using all freed up surfaces by decoder. unable to find free surface, trying to allocate a new one");
            // Set itarator position to the newly allocated surface (end-1)
            createSurfaces(surfaces.size() + 1, &codec_ctx->hwaccel_context, width, height);
            it = surfaces_free.end();
            --it;
        }
    }
    id =(*it)->get();
    surface_t *s = it->get();
    // !erase later. otherwise it may be destroyed
    surfaces_used.push_back(*it);
    // TODO: why QList may erase an invalid iterator(first iterator)at the same position?
    surfaces_free.erase(it); //ref not increased, but can not be used.
//FIXME: warning: cast to pointer from integer of different size [-Wint-to-pointer-cast]
    *data = (uint8_t*)(quintptr)id;
    *opaque = s;
    return true;
}

void VideoDecoderVAAPIPrivate::releaseBuffer(void *opaque, uint8_t *data)
{
    Q_UNUSED(opaque);
    VASurfaceID id = (VASurfaceID)(uintptr_t)data;
    for (std::list<surface_ptr>::iterator it = surfaces_used.begin(); it != surfaces_used.end(); ++it) {
        if((*it)->get() == id) {
            surfaces_free.push_back(*it);
            surfaces_used.erase(it);
            break;
        }
    }
}

} // namespace QtAV
// used by .moc QMetaType::Bool
#ifdef Bool
#undef Bool
#endif
#include "VideoDecoderVAAPI.moc"
