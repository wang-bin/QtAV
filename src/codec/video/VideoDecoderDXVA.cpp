/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2013)

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
/// egl support is added by: Andy Bell <andy.bell@displaynote.com>

#ifdef _MSC_VER
#pragma comment(lib, "ole32.lib") //CoTaskMemFree. why link failed?
#endif
#include "VideoDecoderD3D.h"
#include "QtAV/private/AVCompat.h"
#include "QtAV/private/factory.h"
//#include "QtAV/private/mkid.h"
#include "utils/Logger.h"
#include "SurfaceInteropDXVA.h"
#include <QtCore/QSysInfo>
#define DX_LOG_COMPONENT "DXVA2"
#include "utils/DirectXHelper.h"

// d3d9ex: http://dxr.mozilla.org/mozilla-central/source/dom/media/wmf/DXVA2Manager.cpp

// to use c api, add define COBJMACROS and CINTERFACE
#define DXVA2API_USE_BITFIELDS
extern "C" {
#include <libavcodec/dxva2.h> //will include d3d9.h, dxva2api.h
}
#define VA_DXVA2_MAX_SURFACE_COUNT (64)

#include <d3d9.h>
#include <dxva2api.h>

#include <initguid.h> /* must be last included to not redefine existing GUIDs */
/* dxva2api.h GUIDs: http://msdn.microsoft.com/en-us/library/windows/desktop/ms697067(v=vs100).aspx
 * assume that they are declared in dxva2api.h */
//#define MS_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) ///TODO: ???

#define MS_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    static const GUID name = { l, w1, w2, {b1, b2, b3, b4, b5, b6, b7, b8}}
#ifdef __MINGW32__
# include <_mingw.h>
# if !defined(__MINGW64_VERSION_MAJOR)
#  undef MS_GUID
#  define MS_GUID DEFINE_GUID /* dxva2api.h fails to declare those, redefine as static */
# else
#  include <dxva.h>
# endif
#endif /* __MINGW32__ */

namespace QtAV {
MS_GUID(IID_IDirectXVideoDecoderService, 0xfc51a551, 0xd5e7, 0x11d9, 0xaf,0x55,0x00,0x05,0x4e,0x43,0xff,0x02);
MS_GUID(IID_IDirectXVideoAccelerationService, 0xfc51a550, 0xd5e7, 0x11d9, 0xaf,0x55,0x00,0x05,0x4e,0x43,0xff,0x02);
MS_GUID(IID_IDirect3DDevice9Ex, 0xb18b10ce, 0x2649, 0x405a, 0x87, 0xf, 0x95, 0xf7, 0x77, 0xd4, 0x31, 0x3a);

class VideoDecoderDXVAPrivate;
class VideoDecoderDXVA : public VideoDecoderD3D
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(VideoDecoderDXVA)
    Q_PROPERTY(int surfaces READ surfaces WRITE setSurfaces)
public:
    VideoDecoderDXVA();
    VideoDecoderId id() const Q_DECL_OVERRIDE;
    QString description() const Q_DECL_OVERRIDE;
    VideoFrame frame() Q_DECL_OVERRIDE;
    // properties
    void setSurfaces(int num);
    int surfaces() const;
};

extern VideoDecoderId VideoDecoderId_DXVA;
FACTORY_REGISTER(VideoDecoder, DXVA, "DXVA")

typedef struct {
    IDirect3DSurface9 *d3d;
    int refcount;
    unsigned int order;
} va_surface_t;
/* */

class VideoDecoderDXVAPrivate Q_DECL_FINAL: public VideoDecoderD3DPrivate
{
public:
    VideoDecoderDXVAPrivate():
        VideoDecoderD3DPrivate()
    {
#if QTAV_HAVE(DXVA_EGL)
        if (OpenGLHelper::isOpenGLES() && QSysInfo::windowsVersion() >= QSysInfo::WV_VISTA)
            copy_mode = VideoDecoderFFmpegHW::ZeroCopy;
#endif
        hd3d9_dll = 0;
        hdxva2_dll = 0;
        d3dobj = 0;
        d3ddev = 0;
        token = 0;
        devmng = 0;
        device = 0;
        vs = 0;
        render = D3DFMT_UNKNOWN;
        decoder = 0;
        surface_order = 0;
        surface_width = surface_height = 0;
        available = loadDll();
        // set by user. don't reset in when call destroy
        surface_auto = true;
        surface_count = 0;
    }
    virtual ~VideoDecoderDXVAPrivate()
    {
        unloadDll();
    }

    bool loadDll();
    bool unloadDll();
    bool D3dCreateDevice();
    void D3dDestroyDevice();
    bool D3dCreateDeviceManager();
    void D3dDestroyDeviceManager();
    bool DxCreateVideoService();
    void DxDestroyVideoService();
    bool DxFindVideoServiceConversion(GUID *input, D3DFORMAT *output);
    bool DxCreateVideoDecoder(int codec_id, int w, int h);
    void DxDestroyVideoDecoder();
    bool DxResetVideoDecoder();
    bool isHEVCSupported() const;

    D3DFormat formatFor(const GUID *guid) const Q_DECL_OVERRIDE;
    bool setup(AVCodecContext *avctx) Q_DECL_OVERRIDE;
    bool open() Q_DECL_OVERRIDE;
    void close() Q_DECL_OVERRIDE;

    bool getBuffer(void **opaque, uint8_t **data) Q_DECL_OVERRIDE;
    void releaseBuffer(void *opaque, uint8_t *data) Q_DECL_OVERRIDE;
    AVPixelFormat vaPixelFormat() const Q_DECL_OVERRIDE { return QTAV_PIX_FMT_C(DXVA2_VLD);}
    /* DLL */
    HINSTANCE hd3d9_dll;
    HINSTANCE hdxva2_dll;

    /* Direct3D */
    IDirect3D9 *d3dobj;
    IDirect3DDevice9 *d3ddev; // can be Ex
    /* Device manager */
    UINT                     token;
    IDirect3DDeviceManager9  *devmng;
    HANDLE                   device;

    /* Video service */
    IDirectXVideoDecoderService  *vs;
    GUID                         input;
    D3DFORMAT                    render;

    /* Video decoder */
    DXVA2_ConfigPictureDecode    cfg;
    IDirectXVideoDecoder         *decoder;

    struct dxva_context hw_ctx;
    bool surface_auto;
    unsigned     surface_count;
    unsigned     surface_order;
    int          surface_width;
    int          surface_height;
    //AVPixelFormat surface_chroma;

    va_surface_t surfaces[VA_DXVA2_MAX_SURFACE_COUNT];
    IDirect3DSurface9* hw_surfaces[VA_DXVA2_MAX_SURFACE_COUNT];

    QString vendor;
    dxva::InteropResourcePtr interop_res; //may be still used in video frames when decoder is destroyed
};

VideoDecoderDXVA::VideoDecoderDXVA()
    : VideoDecoderD3D(*new VideoDecoderDXVAPrivate())
{
    // dynamic properties about static property details. used by UI
    // format: detail_property
    setProperty("detail_surfaces", tr("Decoding surfaces") + QStringLiteral(" ") + tr("0: auto"));
}

VideoDecoderId VideoDecoderDXVA::id() const
{
    return VideoDecoderId_DXVA;
}

QString VideoDecoderDXVA::description() const
{
    DPTR_D(const VideoDecoderDXVA);
    if (!d.description.isEmpty())
        return d.description;
    return QStringLiteral("DirectX Video Acceleration");
}

VideoFrame VideoDecoderDXVA::frame()
{
    DPTR_D(VideoDecoderDXVA);
    //qDebug("frame size: %dx%d", d.frame->width, d.frame->height);
    if (!d.frame->opaque || !d.frame->data[0])
        return VideoFrame();
    if (d.frame->width <= 0 || d.frame->height <= 0 || !d.codec_ctx)
        return VideoFrame();

    IDirect3DSurface9 *d3d = (IDirect3DSurface9*)(uintptr_t)d.frame->data[3];
    if (copyMode() == ZeroCopy && d.interop_res) {
        dxva::SurfaceInteropDXVA *interop = new dxva::SurfaceInteropDXVA(d.interop_res);
        interop->setSurface(d3d, d.width, d.height);
        VideoFrame f(d.width, d.height, VideoFormat::Format_RGB32);
        f.setBytesPerLine(d.width * 4); //used by gl to compute texture size
        f.setMetaData(QStringLiteral("surface_interop"), QVariant::fromValue(VideoSurfaceInteropPtr(interop)));
        f.setTimestamp(d.frame->pkt_pts/1000.0);
        f.setDisplayAspectRatio(d.getDAR(d.frame));
        return f;
    }
    class ScopedD3DLock {
        IDirect3DSurface9 *mpD3D;
    public:
        ScopedD3DLock(IDirect3DSurface9* d3d, D3DLOCKED_RECT *rect) : mpD3D(d3d) {
            if (FAILED(mpD3D->LockRect(rect, NULL, D3DLOCK_READONLY))) {
                qWarning("Failed to lock surface");
                mpD3D = 0;
            }
        }
        ~ScopedD3DLock() {
            if (mpD3D)
                mpD3D->UnlockRect();
        }
    };

    D3DLOCKED_RECT lock;
    ScopedD3DLock(d3d, &lock);
    if (lock.Pitch == 0) {
        return VideoFrame();
    }
    //picth >= desc.Width
    D3DSURFACE_DESC desc;
    d3d->GetDesc(&desc);
    const VideoFormat fmt = VideoFormat(pixelFormatFromD3D(desc.Format));
    if (!fmt.isValid()) {
        qWarning("unsupported dxva pixel format: %#x", desc.Format);
        return VideoFrame();
    }
    //YV12 need swap, not imc3?
    // imc3 U V pitch == Y pitch, but half of the U/V plane is space. we convert to yuv420p here
    // nv12 bpp(1)==1
    // 3rd plane is not used for nv12
    int pitch[3] = { lock.Pitch, 0, 0}; //compute chroma later
    uint8_t *src[] = { (uint8_t*)lock.pBits, 0, 0}; //compute chroma later
    const bool swap_uv = desc.Format ==  MAKEFOURCC('I','M','C','3');
    return copyToFrame(fmt, d.surface_height, src, pitch, swap_uv);
}

void VideoDecoderDXVA::setSurfaces(int num)
{
    DPTR_D(VideoDecoderDXVA);
    d.surface_count = num;
    d.surface_auto = num <= 0;
}

int VideoDecoderDXVA::surfaces() const
{
    return d_func().surface_count;
}

/* FIXME it is nearly common with VAAPI */
bool VideoDecoderDXVAPrivate::getBuffer(void **opaque, uint8_t **data)//vlc_va_t *external, AVFrame *ff)
{
    // check pix fmt DXVA2_VLD and h264 profile?
    HRESULT hr = devmng->TestDevice(device);
    if (hr == DXVA2_E_NEW_VIDEO_DEVICE) {
        if (!DxResetVideoDecoder())
            return false;
    } else if (FAILED(hr)) {
        qWarning() << "IDirect3DDeviceManager9.TestDevice (" << hr << "): " << qt_error_string(hr);
        return false;
    }
    /* Grab an unused surface, in case none are, try the oldest
     * XXX using the oldest is a workaround in case a problem happens with libavcodec */
    unsigned i, old;
    for (i = 0, old = 0; i < surface_count; i++) {
        va_surface_t *surface = &surfaces[i];
        if (!surface->refcount)
            break;
        if (surface->order < surfaces[old].order)
            old = i;
    }
    if (i >= surface_count)
        i = old;
    va_surface_t *surface = &surfaces[i];
    surface->refcount = 1;
    surface->order = surface_order++;
    *data = (uint8_t*)surface->d3d;/* Yummie */
    *opaque = surface;
    return true;
}

//(void *opaque, uint8_t *data)
void VideoDecoderDXVAPrivate::releaseBuffer(void *opaque, uint8_t *data)
{
    Q_UNUSED(data);
    va_surface_t *surface = (va_surface_t*)opaque;
    surface->refcount--;
}

bool VideoDecoderDXVAPrivate::loadDll() {
    hd3d9_dll = LoadLibrary(TEXT("D3D9.DLL"));
    if (!hd3d9_dll) {
        qWarning("cannot load d3d9.dll");
        return false;
    }
    hdxva2_dll = LoadLibrary(TEXT("DXVA2.DLL"));
    if (!hdxva2_dll) {
        qWarning("cannot load dxva2.dll");
        FreeLibrary(hd3d9_dll);
        return false;
    }
    return true;
}

bool VideoDecoderDXVAPrivate::unloadDll() {
    if (hdxva2_dll)
        FreeLibrary(hdxva2_dll);
    if (hd3d9_dll)
        FreeLibrary(hd3d9_dll);
    return true;
}

static const char* getVendorName(D3DADAPTER_IDENTIFIER9 *id) //vlc_va_dxva2_t *va
{
    static const struct {
        unsigned id;
        char     name[32];
    } vendors [] = {
        { 0x1002, "ATI" },
        { 0x10DE, "NVIDIA" },
        { 0x1106, "VIA" },
        { 0x8086, "Intel" },
        { 0x5333, "S3 Graphics" },
        { 0, "" }
    };
    const char *vendor = "Unknown";
    for (int i = 0; vendors[i].id != 0; i++) {
        if (vendors[i].id == id->VendorId) {
            vendor = vendors[i].name;
            break;
        }
    }
    return vendor;
}

bool VideoDecoderDXVAPrivate::D3dCreateDevice()
{
    D3DADAPTER_IDENTIFIER9 d3dai;
    ZeroMemory(&d3dai, sizeof(d3dai));
    d3ddev = DXHelper::CreateDevice9Ex(hd3d9_dll, (IDirect3D9Ex**)(&d3dobj), &d3dai);
    if (!d3ddev) {
        qWarning("Failed to create d3d9 device ex, fallback to d3d9 device");
        d3ddev = DXHelper::CreateDevice9(hd3d9_dll, &d3dobj, &d3dai);
    }
    if (!d3ddev)
        return false;
    vendor = QString::fromLatin1(getVendorName(&d3dai));
    description = QString().sprintf("DXVA2 (%.*s, vendor %lu(%s), device %lu, revision %lu)",
                                    sizeof(d3dai.Description), d3dai.Description,
                                    d3dai.VendorId, qPrintable(vendor), d3dai.DeviceId, d3dai.Revision);

    //if (copy_uswc)
      //  copy_uswc = vendor.toLower() == "intel";
    qDebug("DXVA2 description:  %s", description.toUtf8().constData());

    return !!d3ddev;
}

/**
 * It releases a Direct3D device and its resources.
 */
void VideoDecoderDXVAPrivate::D3dDestroyDevice()
{
    SafeRelease(&d3ddev);
    SafeRelease(&d3dobj);
}
/**
 * It creates a Direct3D device manager
 */
bool VideoDecoderDXVAPrivate::D3dCreateDeviceManager()
{
    typedef HRESULT (WINAPI *CreateDeviceManager9Func)(UINT *pResetToken, IDirect3DDeviceManager9 **);
    CreateDeviceManager9Func CreateDeviceManager9 = (CreateDeviceManager9Func)GetProcAddress(hdxva2_dll, "DXVA2CreateDirect3DDeviceManager9");
    if (!CreateDeviceManager9) {
        qWarning("cannot load function DXVA2CreateDirect3DDeviceManager9");
        return false;
    }
    qDebug("OurDirect3DCreateDeviceManager9 Success!");
    DX_ENSURE_OK(CreateDeviceManager9(&token, &devmng), false);
    qDebug("obtained IDirect3DDeviceManager9");
    //http://msdn.microsoft.com/en-us/library/windows/desktop/ms693525%28v=vs.85%29.aspx
    DX_ENSURE_OK(devmng->ResetDevice(d3ddev, token), false);
    return true;
}
void VideoDecoderDXVAPrivate::D3dDestroyDeviceManager()
{
    SafeRelease(&devmng);
}

bool VideoDecoderDXVAPrivate::DxCreateVideoService()
{
    DX_ENSURE_OK(devmng->OpenDeviceHandle(&device), false);
    DX_ENSURE_OK(devmng->GetVideoService(device, IID_IDirectXVideoDecoderService, (void**)&vs), false);
    return true;
}
void VideoDecoderDXVAPrivate::DxDestroyVideoService()
{
    if (devmng && device && device != INVALID_HANDLE_VALUE) {
        devmng->CloseDeviceHandle(device);
        device = 0;
    }
    SafeRelease(&vs);
}
/**
 * Find the best suited decoder mode GUID and render format.
 */
bool VideoDecoderDXVAPrivate::DxFindVideoServiceConversion(GUID *input, D3DFORMAT *output)
{
    /* Retreive supported modes from the decoder service */
    UINT input_count = 0;
    GUID *input_list = NULL;
    DX_ENSURE_OK(vs->GetDecoderDeviceGuids(&input_count, &input_list), false);
    const d3d_format_t *fmt = getFormat(input_list, input_count, input);
    CoTaskMemFree(input_list);
    if (!fmt)
        return false;
    *output = (D3DFORMAT)fmt->format;
    return true;
}

D3DFormat VideoDecoderDXVAPrivate::formatFor(const GUID *guid) const
{
    UINT output_count = 0;
    D3DFORMAT *output_list = NULL;
    if (FAILED(vs->GetDecoderRenderTargets(*guid, &output_count, &output_list))) {
        qWarning("IDirectXVideoDecoderService_GetDecoderRenderTargets failed");
        return 0;
    }
    qDebug("d3d decoder supprted output count: %d", output_count);
    D3DFormat fmt = select_d3d_format((D3DFormat*)output_list, output_count);
    CoTaskMemFree(output_list);
    return fmt;
}

bool VideoDecoderDXVAPrivate::DxCreateVideoDecoder(int codec_id, int w, int h)
{
    if (!vs || !d3ddev) {
        qWarning("d3d is not ready. IDirectXVideoService: %p, IDirect3DDevice9: %p", vs, d3ddev);
        return false;
    }
    qDebug("DxCreateVideoDecoder id %d %dx%d, surfaces: %u", codec_id, w, h, surface_count);
    /* Allocates all surfaces needed for the decoder */
    surface_width = aligned(w);
    surface_height = aligned(h);
    if (surface_auto) {
        switch (codec_id) {
        case QTAV_CODEC_ID(HEVC):
        case QTAV_CODEC_ID(H264):
            surface_count = 16 + 4;
            break;
        case QTAV_CODEC_ID(MPEG1VIDEO):
        case QTAV_CODEC_ID(MPEG2VIDEO):
            surface_count = 2 + 4;
        default:
            surface_count = 2 + 4;
            break;
        }
        if (codec_ctx->active_thread_type & FF_THREAD_FRAME)
            surface_count += codec_ctx->thread_count;
    }
    qDebug(">>>>>>>>>>>>>>>>>>>>>surfaces: %d, active_thread_type: %d, threads: %d, refs: %d", surface_count, codec_ctx->active_thread_type, codec_ctx->thread_count, codec_ctx->refs);
    if (surface_count == 0) {
        qWarning("internal error: wrong surface count.  %u auto=%d", surface_count, surface_auto);
        surface_count = 16 + 4;
    }

    IDirect3DSurface9* surface_list[VA_DXVA2_MAX_SURFACE_COUNT];
    qDebug("%s @%d vs=%p surface_count=%d surface_width=%d surface_height=%d"
           , __FUNCTION__, __LINE__, vs, surface_count, surface_width, surface_height);
    DX_ENSURE_OK(vs->CreateSurface(surface_width,
                                 surface_height,
                                 surface_count - 1,
                                 render,
                                 D3DPOOL_DEFAULT,
                                 0,
                                 DXVA2_VideoDecoderRenderTarget,
                                 surface_list,
                                 NULL)
            , false);
    memset(surfaces, 0, sizeof(surfaces));
    for (unsigned i = 0; i < surface_count; i++) {
        va_surface_t *surface = &this->surfaces[i];
        surface->d3d = surface_list[i];
        surface->refcount = 0;
        surface->order = 0;
    }
    qDebug("IDirectXVideoAccelerationService_CreateSurface succeed with %d surfaces (%dx%d)", surface_count, w, h);

    /* */
    DXVA2_VideoDesc dsc;
    ZeroMemory(&dsc, sizeof(dsc));
    dsc.SampleWidth     = w; //coded_width
    dsc.SampleHeight    = h; //coded_height
    dsc.Format          = render;
    dsc.InputSampleFreq.Numerator   = 0;
    dsc.InputSampleFreq.Denominator = 0;
    dsc.OutputFrameFreq = dsc.InputSampleFreq;
    dsc.UABProtectionLevel = FALSE;
    dsc.Reserved = 0;
// see xbmc
    /* FIXME I am unsure we can let unknown everywhere */
    DXVA2_ExtendedFormat *ext = &dsc.SampleFormat;
    ext->SampleFormat = 0;//DXVA2_SampleProgressiveFrame;//xbmc. DXVA2_SampleUnknown;
    ext->VideoChromaSubsampling = 0;//DXVA2_VideoChromaSubsampling_Unknown;
    ext->NominalRange = 0;//DXVA2_NominalRange_Unknown;
    ext->VideoTransferMatrix = 0;//DXVA2_VideoTransferMatrix_Unknown;
    ext->VideoLighting = 0;//DXVA2_VideoLighting_dim;//xbmc. DXVA2_VideoLighting_Unknown;
    ext->VideoPrimaries = 0;//DXVA2_VideoPrimaries_Unknown;
    ext->VideoTransferFunction = 0;//DXVA2_VideoTransFunc_Unknown;

    /* List all configurations available for the decoder */
    UINT                      cfg_count = 0;
    DXVA2_ConfigPictureDecode *cfg_list = NULL;
    DX_ENSURE_OK(vs->GetDecoderConfigurations(input,
                                            &dsc,
                                            NULL,
                                            &cfg_count,
                                            &cfg_list)
                 , false);
    qDebug("we got %d decoder configurations", cfg_count);
    /* Select the best decoder configuration */
    int cfg_score = 0;
    for (unsigned i = 0; i < cfg_count; i++) {
        const DXVA2_ConfigPictureDecode *cfg = &cfg_list[i];
        qDebug("configuration[%d] ConfigBitstreamRaw %d", i, cfg->ConfigBitstreamRaw);
        int score;
        if (cfg->ConfigBitstreamRaw == 1)
            score = 1;
        else if (codec_id ==  QTAV_CODEC_ID(H264) && cfg->ConfigBitstreamRaw == 2)
            score = 2;
        else
            continue;
        if (isNoEncrypt(&cfg->guidConfigBitstreamEncryption))
            score += 16;

        if (cfg_score < score) {
            this->cfg = *cfg;
            cfg_score = score;
        }
    }
    CoTaskMemFree(cfg_list);
    if (cfg_score <= 0) {
        qWarning("Failed to find a supported decoder configuration");
        return false;
    }
    /* Create the decoder */
    DX_ENSURE_OK(vs->CreateVideoDecoder(input, &dsc, &cfg, surface_list, surface_count, &decoder), false);
    qDebug("IDirectXVideoDecoderService_CreateVideoDecoder succeed. decoder=%p", decoder);
    return true;
}

void VideoDecoderDXVAPrivate::DxDestroyVideoDecoder()
{
    SafeRelease(&decoder);
    for (unsigned i = 0; i < surface_count; i++) {
        SafeRelease(&surfaces[i].d3d);
    }
}

bool VideoDecoderDXVAPrivate::DxResetVideoDecoder()
{
    qWarning("DxResetVideoDecoder unimplemented");
    return false;
}

static bool check_ffmpeg_hevc_dxva2()
{
    avcodec_register_all();
    AVHWAccel *hwa = av_hwaccel_next(0);
    while (hwa) {
        if (strncmp("hevc_dxva2", hwa->name, 10) == 0)
            return true;
        hwa = av_hwaccel_next(hwa);
    }
    return false;
}

bool VideoDecoderDXVAPrivate::isHEVCSupported() const
{
    static const bool support_hevc = check_ffmpeg_hevc_dxva2();
    return support_hevc;
}

// hwaccel_context
bool VideoDecoderDXVAPrivate::setup(AVCodecContext *avctx)
{
    const int w = codedWidth(avctx);
    const int h = codedHeight(avctx);
    if (decoder && surface_width == aligned(w) && surface_height == aligned(h)) {
        avctx->hwaccel_context = &hw_ctx;
        return true;
    }
    width = avctx->width; // not necessary. set in decode()
    height = avctx->height;
    releaseUSWC();
    DxDestroyVideoDecoder();
    avctx->hwaccel_context = NULL;
    /* FIXME transmit a video_format_t by VaSetup directly */
    if (!DxCreateVideoDecoder(avctx->codec_id, w, h))
        return false;
    avctx->hwaccel_context = &hw_ctx;
    // TODO: FF_DXVA2_WORKAROUND_SCALING_LIST_ZIGZAG
    if (isIntelClearVideo(&input)) {
#ifdef FF_DXVA2_WORKAROUND_INTEL_CLEARVIDEO //2014-03-07 - 8b2a130 - lavc 55.50.0 / 55.53.100 - dxva2.h
        qDebug("FF_DXVA2_WORKAROUND_INTEL_CLEARVIDEO");
        hw_ctx.workaround |= FF_DXVA2_WORKAROUND_INTEL_CLEARVIDEO;
#endif
    } else {
        hw_ctx.workaround = 0;
    }

    hw_ctx.decoder = decoder;
    hw_ctx.cfg = &cfg;
    hw_ctx.surface_count = surface_count;
    hw_ctx.surface = hw_surfaces;
    memset(hw_surfaces, 0, sizeof(hw_surfaces));
    for (unsigned i = 0; i < surface_count; i++)
        hw_ctx.surface[i] = surfaces[i].d3d;
    initUSWC(surface_width);
    return true;
}

bool VideoDecoderDXVAPrivate::open()
{
    if (!prepare())
        return false;
    if (codec_ctx->codec_id == QTAV_CODEC_ID(HEVC)) {
        // runtime hevc check
        if (isHEVCSupported()) {
            qWarning("HEVC DXVA2 is supported by current FFmpeg runtime.");
        } else {
            qWarning("HEVC DXVA2 is not supported by current FFmpeg runtime.");
            return false;
        }
    }
    if (!D3dCreateDevice()) {
        qWarning("Failed to create Direct3D device");
        goto error;
    }
    qDebug("D3dCreateDevice succeed");
    if (!D3dCreateDeviceManager()) {
        qWarning("D3dCreateDeviceManager failed");
        goto error;
    }
    if (!DxCreateVideoService()) {
        qWarning("DxCreateVideoService failed");
        goto error;
    }
    if (!DxFindVideoServiceConversion(&input, &render)) {
        qWarning("DxFindVideoServiceConversion failed");
        goto error;
    }
    IDirect3DDevice9Ex *devEx;
    d3ddev->QueryInterface(IID_IDirect3DDevice9Ex, (void**)&devEx);
    qDebug("using D3D9Ex: %d", !!devEx);
    // runtime check gles for dynamic gl
#if QTAV_HAVE(DXVA_EGL)
    if (OpenGLHelper::isOpenGLES()) {
        // d3d9ex is required to share d3d resource. It's available in vista and later. d3d9 can not CreateTexture with shared handle
        if (devEx)
            interop_res = dxva::InteropResourcePtr(new dxva::EGLInteropResource(d3ddev));
        else
            qDebug("D3D9Ex is not available. Disable 0-copy.");
    }
#endif
    SafeRelease(&devEx);
#if QTAV_HAVE(DXVA_GL)
    if (!OpenGLHelper::isOpenGLES())
        interop_res = dxva::InteropResourcePtr(new dxva::GLInteropResource(d3ddev));
#endif
    return true;
error:
    close();
    return false;
}

void VideoDecoderDXVAPrivate::close()
{
    restore();
    releaseUSWC();
    DxDestroyVideoDecoder();
    DxDestroyVideoService();
    D3dDestroyDeviceManager();
    D3dDestroyDevice();
}
} //namespace QtAV

#include "VideoDecoderDXVA.moc"
