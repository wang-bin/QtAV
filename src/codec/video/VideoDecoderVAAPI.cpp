/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013-2015 Wang Bin <wbsecg1@gmail.com>

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
#include <algorithm>
#include <list>
#include <QtCore/QList>
#include <QtCore/QMetaEnum>
#include <QtCore/QStringList>
extern "C" {
#include <libavcodec/vaapi.h>
}
#include <fcntl.h> //open()
#include <unistd.h> //close()
#include "QtAV/Packet.h"
#include "QtAV/private/AVCompat.h"
#include "QtAV/private/prepost.h"
#include "vaapi/SurfaceInteropVAAPI.h"
#include "utils/Logger.h"

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
    VideoDecoderId id() const Q_DECL_OVERRIDE;
    QString description() const Q_DECL_OVERRIDE;
    VideoFrame frame() Q_DECL_OVERRIDE;

    // QObject properties
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
const char* getProfileName(AVCodecID id, int profile)
{
    AVCodec *c = avcodec_find_decoder(id);
    if (!c)
        return "Unknow";
    return av_get_profile_name(c, profile);
}

typedef struct {
    AVCodecID codec;
    int profile;
    VAProfile va_profile;
} codec_profile_t;
#define VAProfileNone ((VAProfile)-1) //maybe not defined for old va

static const  codec_profile_t va_profiles[] = {
    { QTAV_CODEC_ID(MPEG1VIDEO), FF_PROFILE_UNKNOWN, VAProfileMPEG2Main }, //vlc
    { QTAV_CODEC_ID(MPEG2VIDEO), FF_PROFILE_MPEG2_MAIN, VAProfileMPEG2Main },
    { QTAV_CODEC_ID(MPEG2VIDEO),  FF_PROFILE_MPEG2_SIMPLE, VAProfileMPEG2Simple },
    { QTAV_CODEC_ID(H263), FF_PROFILE_UNKNOWN, VAProfileMPEG4AdvancedSimple }, //xbmc
    { QTAV_CODEC_ID(MPEG4), FF_PROFILE_MPEG4_ADVANCED_SIMPLE, VAProfileMPEG4AdvancedSimple },
    { QTAV_CODEC_ID(MPEG4), FF_PROFILE_MPEG4_MAIN, VAProfileMPEG4Main },
    { QTAV_CODEC_ID(MPEG4), FF_PROFILE_MPEG4_SIMPLE, VAProfileMPEG4Simple },
    { QTAV_CODEC_ID(H264), FF_PROFILE_H264_HIGH, VAProfileH264High },
    { QTAV_CODEC_ID(H264), FF_PROFILE_H264_MAIN, VAProfileH264Main },
    { QTAV_CODEC_ID(H264), FF_PROFILE_H264_BASELINE, VAProfileH264Baseline },
    { QTAV_CODEC_ID(H264), FF_PROFILE_H264_CONSTRAINED_BASELINE, VAProfileH264ConstrainedBaseline }, //mpv force main
    { QTAV_CODEC_ID(VC1), FF_PROFILE_VC1_ADVANCED, VAProfileVC1Advanced },
    { QTAV_CODEC_ID(VC1), FF_PROFILE_VC1_MAIN, VAProfileVC1Main },
    { QTAV_CODEC_ID(VC1), FF_PROFILE_VC1_SIMPLE, VAProfileVC1Simple },
    { QTAV_CODEC_ID(WMV3), FF_PROFILE_VC1_ADVANCED, VAProfileVC1Advanced },
    { QTAV_CODEC_ID(WMV3), FF_PROFILE_VC1_MAIN, VAProfileVC1Main },
    { QTAV_CODEC_ID(WMV3), FF_PROFILE_VC1_SIMPLE, VAProfileVC1Simple },
    { QTAV_CODEC_ID(NONE), FF_PROFILE_UNKNOWN, VAProfileNone }
};

static bool isProfileSupportedByRuntime(const VAProfile *profiles, int count, VAProfile p)
{
    for (int i = 0; i < count; ++i) {
        if (profiles[i] == p)
            return true;
    }
    return false;
}

const codec_profile_t* findProfileEntry(AVCodecID codec, int profile)
{
    if (codec == QTAV_CODEC_ID(NONE))
        return 0;
    for (int i = 0; va_profiles[i].codec != QTAV_CODEC_ID(NONE); ++i) {
        const codec_profile_t* p = &va_profiles[i];
        if (codec != p->codec || profile != p->profile)
            continue;
        // return the first profile entry if given profile is unknow
        if (profile == FF_PROFILE_UNKNOWN
                || p->profile == FF_PROFILE_UNKNOWN // force the profile
                || profile == p->profile)
            return p;
    }
    return 0;
}

class VideoDecoderVAAPIPrivate Q_DECL_FINAL: public VideoDecoderFFmpegHWPrivate, protected VAAPI_DRM, protected VAAPI_X11
#ifndef QT_NO_OPENGL
        , protected VAAPI_GLX
#endif //QT_NO_OPENGL
        , protected X11_API
{
    DPTR_DECLARE_PUBLIC(VideoDecoderVAAPI)
public:
    VideoDecoderVAAPIPrivate()
        : support_4k(true)
    {
        if (VAAPI_DRM::isLoaded())
            display_type = VideoDecoderVAAPI::DRM;
#ifndef QT_NO_OPENGL
        if (VAAPI_GLX::isLoaded()) {
            display_type = VideoDecoderVAAPI::GLX;
            copy_mode = VideoDecoderFFmpegHW::ZeroCopy;
        }
#endif //QT_NO_OPENGL
        if (VAAPI_X11::isLoaded()) {
            display_type = VideoDecoderVAAPI::X11;
#ifndef QT_NO_OPENGL
#if VA_X11_INTEROP
            copy_mode = VideoDecoderFFmpegHW::ZeroCopy;
#endif //VA_X11_INTEROP
#endif //QT_NO_OPENGL
        }
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
        disable_derive = true;
    }
    bool open() Q_DECL_OVERRIDE;
    void close() Q_DECL_OVERRIDE;
    bool createSurfaces(int count, void **hwctx, int w, int h);
    void destroySurfaces();

    bool setup(AVCodecContext *avctx) Q_DECL_OVERRIDE;
    bool getBuffer(void **opaque, uint8_t **data) Q_DECL_OVERRIDE;
    void releaseBuffer(void *opaque, uint8_t *data) Q_DECL_OVERRIDE;
    AVPixelFormat vaPixelFormat() const Q_DECL_OVERRIDE { return QTAV_PIX_FMT_C(VAAPI_VLD); }

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
    InteropResourcePtr interop_res; //may be still used in video frames when decoder is destroyed
};


VideoDecoderVAAPI::VideoDecoderVAAPI()
    : VideoDecoderFFmpegHW(*new VideoDecoderVAAPIPrivate())
{
    setDisplayPriority(QStringList() << QStringLiteral("X11") << QStringLiteral("GLX") <<  QStringLiteral("DRM"));
    // dynamic properties about static property details. used by UI
    // format: detail_property
    setProperty("detail_surfaces", tr("Decoding surfaces.") + QStringLiteral(" ") + tr("0: auto"));
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
    return QStringLiteral("Video Acceleration API");
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

    const int kMaxSurfaces = 32;
    if (num > kMaxSurfaces) {
        qWarning("VAAPI- Too many surfaces. requested: %d, maximun: %d", num, kMaxSurfaces);
    }
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

extern ColorSpace colorSpaceFromFFmpeg(AVColorSpace cs);
VideoFrame VideoDecoderVAAPI::frame()
{
    DPTR_D(VideoDecoderVAAPI);
    if (!d.frame->opaque || !d.frame->data[0])
        return VideoFrame();
    VASurfaceID surface_id = (VASurfaceID)(uintptr_t)d.frame->data[3];
    VAStatus status = VA_STATUS_SUCCESS;
    if (display() == GLX || (copyMode() == ZeroCopy && display() == X11)) {
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
        SurfaceInteropVAAPI *interop = new SurfaceInteropVAAPI(d.interop_res);
        interop->setSurface(p, d.width, d.height);

        VideoFrame f(d.width, d.height, VideoFormat::Format_RGB32); //p->width()
        f.setBytesPerLine(d.width*4); //used by gl to compute texture size
        f.setMetaData(QStringLiteral("surface_interop"), QVariant::fromValue(VideoSurfaceInteropPtr(interop)));
        f.setTimestamp(double(d.frame->pkt_pts)/1000.0);
        f.setDisplayAspectRatio(d.getDAR(d.frame));

        ColorSpace cs = colorSpaceFromFFmpeg(av_frame_get_colorspace(d.frame));
        if (cs != ColorSpace_Unknow)
            cs = colorSpaceFromFFmpeg(d.codec_ctx->colorspace);
        if (cs == ColorSpace_BT601)
            p->setColorSpace(VA_SRC_BT601);
        else
            p->setColorSpace(VA_SRC_BT709);
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
        VA_ENSURE_TRUE(vaDeriveImage(d.display->get(), surface_id, &d.image), VideoFrame());
    } else {
        VA_ENSURE_TRUE(vaGetImage(d.display->get(), surface_id, 0, 0, d.surface_width, d.surface_height, d.image.image_id), VideoFrame());
    }

    void *p_base;
    VA_ENSURE_TRUE(vaMapBuffer(d.display->get(), d.image.buf, &p_base), VideoFrame());

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
    VideoFrame frame(copyToFrame(fmt, d.surface_height, src, pitch, swap_uv));
    VAWARN(vaUnmapBuffer(d.display->get(), d.image.buf));
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
        names.append(QString::fromLatin1(me.valueToKey(disp)));
    }
    return names;
}

bool VideoDecoderVAAPIPrivate::open()
{
    if (!prepare())
        return false;
    const codec_profile_t* pe = findProfileEntry(codec_ctx->codec_id, codec_ctx->profile);
    // TODO: allow wrong profile
    // FIXME: sometimes get wrong profile (switch copyMode)
    if (!pe) {
        qWarning("codec(%s) or profile(%s) is not supported", avcodec_get_name(codec_ctx->codec_id), getProfileName(codec_ctx->codec_id, codec_ctx->profile));
        return false;
    }
    int surface_count = 2 + 1;
    if (codec_ctx->codec_id == QTAV_CODEC_ID(H264))
        surface_count = 16+2 + codec_ctx->thread_count;
    if (surface_auto)
        nb_surfaces = surface_count;
    if (nb_surfaces <= 0)
        nb_surfaces = surface_count;
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
#ifndef QT_NO_OPENGL
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
#else
            qWarning("No OpenGL support in Qt");
#endif //QT_NO_OPENGL
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
    vendor = QString::fromLatin1(vaQueryVendorString(disp));
    //if (!vendor.toLower().contains(QLatin1Strin("intel")))
      //  copy_uswc = false;

    //disable_derive = !copy_uswc;
    description = QObject::tr("VA API version %1.%2; Vendor: %3;").arg(version_major).arg(version_minor).arg(vendor);
    DPTR_P(VideoDecoderVAAPI);
    int idx = p.staticMetaObject.indexOfEnumerator("DisplayType");
    const QMetaEnum me = p.staticMetaObject.enumerator(idx);
    description += QStringLiteral(" Display: ") + QString::fromLatin1(me.valueToKey(display_type));
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
    qDebug("checking profile: %s, %s",  avcodec_get_name(codec_ctx->codec_id), getProfileName(pe->codec, pe->profile));
    int nb_profiles = vaMaxNumProfiles(disp);
    if (nb_profiles <= 0) {
        qWarning("No profile supported");
        return false;
    }
    QVector<VAProfile> supported_profiles(nb_profiles, VAProfileNone);
    VA_ENSURE_TRUE(vaQueryConfigProfiles(disp, supported_profiles.data(), &nb_profiles), false);
    if (!isProfileSupportedByRuntime(supported_profiles.constData(), nb_profiles, pe->va_profile)) {
        qDebug("Codec or profile is not supported by the hardware");
        return false;
    }
    /* Create a VA configuration */
    VAConfigAttrib attrib;
    memset(&attrib, 0, sizeof(attrib));
    attrib.type = VAConfigAttribRTFormat;
    VA_ENSURE_TRUE(vaGetConfigAttributes(disp, pe->va_profile, VAEntrypointVLD, &attrib, 1), false);
    /* Not sure what to do if not, I don't have a way to test */
    if ((attrib.value & VA_RT_FORMAT_YUV420) == 0)
        return false;
    //vaCreateConfig(display, pe->va_profile, VAEntrypointVLD, NULL, 0, &config_id)
    VA_ENSURE_TRUE(vaCreateConfig(disp, pe->va_profile, VAEntrypointVLD, &attrib, 1, &config_id), false);
    supports_derive = false;
    width = codec_ctx->width;
    height = codec_ctx->height;
    surface_width = codedWidth(codec_ctx);
    surface_height = codedHeight(codec_ctx);
    // create surfaces here to avoid creating a surface in threaded getBuffer()
    if (!createSurfaces(nb_surfaces, &codec_ctx->hwaccel_context, surface_width, surface_height)) {
        destroySurfaces();
        return false;
    }
#ifndef QT_NO_OPENGL
    if (display_type == VideoDecoderVAAPI::GLX)
        interop_res = InteropResourcePtr(new GLXInteropResource());
#endif //QT_NO_OPENGL
#if VA_X11_INTEROP
    if (display_type == VideoDecoderVAAPI::X11)
        interop_res = InteropResourcePtr(new X11InteropResource());
#endif //VA_X11_INTEROP
    return true;
}

bool VideoDecoderVAAPIPrivate::createSurfaces(int count, void **pp_hw_ctx, int w, int h)
{
    if (!display) {
        qWarning("no va display");
        return false;
    }
    qDebug("createSurfaces %dx%d", w, h);
    Q_ASSERT(w > 0 && h > 0);
    const int old_size = surfaces.size();
    if (count <= old_size)
        return true;
    surfaces.resize(count);
    image.image_id = VA_INVALID_ID;
    context_id = VA_INVALID_ID;
    VAStatus status = VA_STATUS_SUCCESS;
    status = vaCreateSurfaces(display->get(), VA_RT_FORMAT_YUV420, w, h,  surfaces.data() + old_size, count - old_size, NULL, 0); //VA_ENSURE_OK: travis-ci macro mismatch
    if (status != VA_STATUS_SUCCESS) {
        qWarning("vaCreateSurfaces error (%#x): %s", status, vaErrorStr(status));
        return false;
    }
    for (int i = old_size; i < surfaces.size(); ++i) {
        surfaces_free.push_back(surface_ptr(new surface_t(width, height, surfaces[i], display)));
    }
    /* Create a context */
    if (context_id && context_id != VA_INVALID_ID)
        VAWARN(vaDestroyContext(display->get(), context_id));
    context_id = VA_INVALID_ID;
    if ((status = vaCreateContext(display->get(), config_id, w, h, VA_PROGRESSIVE, surfaces.data(), surfaces.size(), &context_id)) != VA_STATUS_SUCCESS) {
        qWarning("vaCreateContext(VADisplay:%p, VAConfigID:%#x, %d, %d, VA_PROGRESSIVE, VASurfaceID*:%p, surfaces:%d, VAContextID*:%p) == %#x: %s", display->get(), config_id, width, height, surfaces.constData(), surfaces.size(), &context_id, status, vaErrorStr(status));
        context_id = VA_INVALID_ID;
        destroySurfaces();
        return false;
    }
    /* Setup the ffmpeg hardware context */
    memset(&hw_ctx, 0, sizeof(hw_ctx));
    hw_ctx.display = display->get();
    hw_ctx.config_id = config_id;
    hw_ctx.context_id = context_id;
    if (display_type == VideoDecoderVAAPI::GLX || (display_type == VideoDecoderVAAPI::X11 && copy_mode == VideoDecoderFFmpegHW::ZeroCopy)) {
        *pp_hw_ctx = &hw_ctx;
        return true;
    }
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
    bool found = false;
    for (int i = 0; i < i_fmt_count; i++) {
        qDebug("va image format: %c%c%c%c", p_fmt[i].fourcc<<24>>24, p_fmt[i].fourcc<<16>>24, p_fmt[i].fourcc<<8>>24, p_fmt[i].fourcc>>24);
        if (p_fmt[i].fourcc == VA_FOURCC_YV12 ||
                p_fmt[i].fourcc == VA_FOURCC_IYUV ||
                p_fmt[i].fourcc == VA_FOURCC_NV12) {
            VAStatus s = VA_STATUS_SUCCESS;
            if ((s = vaCreateImage(display->get(), &p_fmt[i], w, h, &image)) != VA_STATUS_SUCCESS) {
                image.image_id = VA_INVALID_ID;
                qDebug("vaCreateImage error: %c%c%c%c (%p) %s", p_fmt[i].fourcc<<24>>24, p_fmt[i].fourcc<<16>>24, p_fmt[i].fourcc<<8>>24, p_fmt[i].fourcc>>24, s, vaErrorStr(s));
                continue;
            }
            /* Validate that vaGetImage works with this format */
            if ((s = vaGetImage(display->get(), surfaces[0], 0, 0, w, h, image.image_id)) != VA_STATUS_SUCCESS) {
                vaDestroyImage(display->get(), image.image_id);
                qDebug("vaGetImage error: %c%c%c%c  (%p) %s", p_fmt[i].fourcc<<24>>24, p_fmt[i].fourcc<<16>>24, p_fmt[i].fourcc<<8>>24, p_fmt[i].fourcc>>24, s, vaErrorStr(s));
                image.image_id = VA_INVALID_ID;
                continue;
            }
            found = true;
            break;
        }
    }
    free(p_fmt);
    if (!found) {
        destroySurfaces();
        return false;
    }
    if (!disable_derive && supports_derive) {
        vaDestroyImage(display->get(), image.image_id);
        image.image_id = VA_INVALID_ID;
    }
    initUSWC(w);
    /* Setup the ffmpeg hardware context */
    *pp_hw_ctx = &hw_ctx;
    return true;
}

void VideoDecoderVAAPIPrivate::destroySurfaces()
{
    if (image.image_id != VA_INVALID_ID) {
        VAWARN(vaDestroyImage(display->get(), image.image_id));
    }
    releaseUSWC();
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

bool VideoDecoderVAAPIPrivate::setup(AVCodecContext *avctx)
{
    if (!display) {
        qWarning("no va display");
        return false;
    }
    const int w = codedWidth(avctx);
    const int h = codedHeight(avctx);
    if (surface_width == FFALIGN(w, 16) && surface_height == FFALIGN(h, 16)) {
        avctx->hwaccel_context = &hw_ctx;
        return true;
    }
    avctx->hwaccel_context = NULL;
    width = avctx->width; // not necessary. set in decode()
    height = avctx->height;
    if (surface_width || surface_height)
        destroySurfaces();
    if (!createSurfaces(nb_surfaces, &avctx->hwaccel_context, w, h)) {
        destroySurfaces();
        return false;
    }
    return true;
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

            const int kMaxSurfaces = 32;
            if (surfaces.size() + 1 > kMaxSurfaces) {
                qWarning("VAAPI- Too many surfaces. requested: %d, maximun: %d", surfaces.size() + 1, kMaxSurfaces);
            }
            // Set itarator position to the newly allocated surface (end-1)
            const int old_size = surfaces.size();
            if (!createSurfaces(old_size + 1, &codec_ctx->hwaccel_context, surface_width, surface_height)) {
                // destroy the new created surface. Surfaces can only be destroyed after the context associated has been destroyed?
                VAWARN(vaDestroySurfaces(display->get(), surfaces.data() + old_size, 1));
                surfaces.resize(old_size);
            }
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
