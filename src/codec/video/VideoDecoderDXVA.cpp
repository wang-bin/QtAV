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
#include "directx/SurfaceInteropD3D9.h"
#include <QtCore/QSysInfo>
#define DX_LOG_COMPONENT "DXVA2"
#include "utils/DirectXHelper.h"

// d3d9ex: http://dxr.mozilla.org/mozilla-central/source/dom/media/wmf/DXVA2Manager.cpp

// to use c api, add define COBJMACROS and CINTERFACE
#define DXVA2API_USE_BITFIELDS
extern "C" {
#include <libavcodec/dxva2.h> //will include d3d9.h, dxva2api.h
}

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

class VideoDecoderDXVAPrivate;
class VideoDecoderDXVA : public VideoDecoderD3D
{
    DPTR_DECLARE_PRIVATE(VideoDecoderDXVA)
public:
    VideoDecoderDXVA();
    VideoDecoderId id() const Q_DECL_OVERRIDE;
    QString description() const Q_DECL_OVERRIDE;
    VideoFrame frame() Q_DECL_OVERRIDE;
};

extern VideoDecoderId VideoDecoderId_DXVA;
FACTORY_REGISTER(VideoDecoder, DXVA, "DXVA")

struct d3d9_surface_t : public va_surface_t {
    d3d9_surface_t() : va_surface_t(), d3d(0) {}
    ~d3d9_surface_t() { SafeRelease(&d3d);}
    void setSurface(IUnknown* s) Q_DECL_OVERRIDE {
        d3d = (IDirect3DSurface9*)s;
    }
    IUnknown* getSurface() const {return d3d;}
private:
    IDirect3DSurface9 *d3d;
};
/* */
//https://technet.microsoft.com/zh-cn/aa965266(v=vs.98).aspx
class VideoDecoderDXVAPrivate Q_DECL_FINAL: public VideoDecoderD3DPrivate
{
public:
    VideoDecoderDXVAPrivate():
        VideoDecoderD3DPrivate()
    {
        // d3d9+gl interop may not work on optimus moble platforms, 0-copy is enabled only for egl interop
        if (d3d9::InteropResource::isSupported(d3d9::InteropEGL) && QSysInfo::windowsVersion() >= QSysInfo::WV_VISTA)
            copy_mode = VideoDecoderFFmpegHW::ZeroCopy;

        hd3d9_dll = 0;
        hdxva2_dll = 0;
        d3dobj = 0;
        d3ddev = 0;
        token = 0;
        devmng = 0;
        device = 0;
        vs = 0;
        decoder = 0;
        available = loadDll();
    }
    virtual ~VideoDecoderDXVAPrivate() // can not unload dlls because dx resource will be released in VideoDecoderD3DPrivate::close
    {
        unloadDll();
    }
    AVPixelFormat vaPixelFormat() const Q_DECL_OVERRIDE { return QTAV_PIX_FMT_C(DXVA2_VLD);}
private:
    bool loadDll();
    bool unloadDll();
    bool createDevice() Q_DECL_OVERRIDE;
    void destroyDevice() Q_DECL_OVERRIDE; //d3d device and it's resources, device manager, video device and decoder service
    QVector<GUID> getSupportedCodecs() const Q_DECL_OVERRIDE;
    bool checkDevice() Q_DECL_OVERRIDE;
    bool createDecoder(AVCodecID codec_id, int w, int h, QVector<va_surface_t*>& surf) Q_DECL_OVERRIDE;
    void destroyDecoder() Q_DECL_OVERRIDE;
    bool setupSurfaceInterop() Q_DECL_OVERRIDE;
    void* setupAVVAContext() Q_DECL_OVERRIDE;
    int fourccFor(const GUID *guid) const Q_DECL_OVERRIDE;

    /* DLL */
    HINSTANCE hd3d9_dll;
    HINSTANCE hdxva2_dll;
    IDirect3D9 *d3dobj;
    IDirect3DDevice9 *d3ddev; // can be Ex
    /* Device manager */
    UINT token;
    IDirect3DDeviceManager9 *devmng;
    HANDLE device;
    /* Video service */
    IDirectXVideoDecoderService *vs;
    /* Video decoder */
    DXVA2_ConfigPictureDecode cfg;
    IDirectXVideoDecoder *decoder;

    struct dxva_context hw_ctx;
    QString vendor;
public:
    d3d9::InteropResourcePtr interop_res; //may be still used in video frames when decoder is destroyed
};

static D3DFORMAT fourccToD3D(int fcc) {
    return (D3DFORMAT)fcc;
}

VideoDecoderDXVA::VideoDecoderDXVA()
    : VideoDecoderD3D(*new VideoDecoderDXVAPrivate())
{
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
        d3d9::SurfaceInterop *interop = new d3d9::SurfaceInterop(d.interop_res);
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
    const VideoFormat fmt = VideoFormat(pixelFormatFromFourcc(desc.Format));
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
    return copyToFrame(fmt, desc.Height, src, pitch, swap_uv);
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
        FreeLibrary(hd3d9_dll); //TODO: don't unload. maybe it's used by others
    return true;
}

bool VideoDecoderDXVAPrivate::createDevice()
{
    // check whether they are created?
    D3DADAPTER_IDENTIFIER9 d3dai;
    ZeroMemory(&d3dai, sizeof(d3dai));
    d3ddev = DXHelper::CreateDevice9Ex(hd3d9_dll, (IDirect3D9Ex**)(&d3dobj), &d3dai);
    if (!d3ddev) {
        qWarning("Failed to create d3d9 device ex, fallback to d3d9 device");
        d3ddev = DXHelper::CreateDevice9(hd3d9_dll, &d3dobj, &d3dai);
    }
    if (!d3ddev) {
        qWarning("Failed to create d3d9 device");
        return false;
    }
    vendor = QString::fromLatin1(DXHelper::vendorName(d3dai.VendorId));
    description = QString().sprintf("DXVA2 (%.*s, vendor %lu(%s), device %lu, revision %lu)",
                                    sizeof(d3dai.Description), d3dai.Description,
                                    d3dai.VendorId, qPrintable(vendor), d3dai.DeviceId, d3dai.Revision);

    //if (copy_uswc)
      //  copy_uswc = vendor.toLower() == "intel";
    qDebug("DXVA2 description:  %s", description.toUtf8().constData());

    if (!d3ddev)
        return false;
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
    DX_ENSURE_OK(devmng->OpenDeviceHandle(&device), false);
    DX_ENSURE_OK(devmng->GetVideoService(device, IID_IDirectXVideoDecoderService, (void**)&vs), false);
    return true;
}

void VideoDecoderDXVAPrivate::destroyDevice()
{
    SafeRelease(&vs);
    if (devmng && device && device != INVALID_HANDLE_VALUE) {
        devmng->CloseDeviceHandle(device);
        device = 0;
    }
    SafeRelease(&devmng);

    SafeRelease(&d3ddev);
    SafeRelease(&d3dobj);
}

QVector<GUID> VideoDecoderDXVAPrivate::getSupportedCodecs() const
{
    /* Retreive supported modes from the decoder service */
    UINT input_count = 0;
    GUID *input_list = NULL;
    QVector<GUID> guids;
    DX_ENSURE_OK(vs->GetDecoderDeviceGuids(&input_count, &input_list), guids);
    guids.resize(input_count);
    memcpy(guids.data(), input_list, input_count*sizeof(GUID));
    CoTaskMemFree(input_list);
    return guids;
}

int VideoDecoderDXVAPrivate::fourccFor(const GUID *guid) const
{
    UINT output_count = 0;
    D3DFORMAT *output_list = NULL;
    if (FAILED(vs->GetDecoderRenderTargets(*guid, &output_count, &output_list))) {
        qWarning("IDirectXVideoDecoderService_GetDecoderRenderTargets failed");
        return 0;
    }
    int fmt = getSupportedFourcc((int*)output_list, output_count);
    CoTaskMemFree(output_list);
    return fmt;
}

bool VideoDecoderDXVAPrivate::checkDevice()
{
    // check pix fmt DXVA2_VLD and h264 profile?
    HRESULT hr = devmng->TestDevice(device);
    if (hr == DXVA2_E_NEW_VIDEO_DEVICE) {
        //https://technet.microsoft.com/zh-cn/aa965266(v=vs.98).aspx
        // CloseDeviceHandle,Release service&decoder, open a new device handle, create a new decoder
        qWarning("DXVA2_E_NEW_VIDEO_DEVICE. Video decoder reset is not implemeted");
            return false;
    } else if (FAILED(hr)) {
        qWarning() << "IDirect3DDeviceManager9.TestDevice (" << hr << "): " << qt_error_string(hr);
        return false;
    }
    return true;
}

bool VideoDecoderDXVAPrivate::createDecoder(AVCodecID codec_id, int w, int h, QVector<va_surface_t*>& surf)
{
    if (!vs || !d3ddev) {
        qWarning("dx video surface is not ready. IDirectXVideoService: %p, IDirect3DDevice9: %p", vs, d3ddev);
        return false;
    }
    const int nb_surfaces = surf.size();
    static const int kMaxSurfaceCount = 64;
    IDirect3DSurface9* surface_list[kMaxSurfaceCount];
    qDebug("IDirectXVideoDecoderService=%p nb_surfaces=%d surface %dx%d", vs, nb_surfaces, aligned(w), aligned(h));
    DX_ENSURE_OK(vs->CreateSurface(aligned(w),
                                 aligned(h),
                                 nb_surfaces - 1, //The number of back buffers. The method creates BackBuffers + 1 surfaces.
                                 fourccToD3D(format_fcc),
                                 D3DPOOL_DEFAULT,
                                 0,
                                 DXVA2_VideoDecoderRenderTarget,
                                 surface_list,
                                 NULL)
            , false);

    for (int i = 0; i < nb_surfaces; i++) {
        d3d9_surface_t* s = new d3d9_surface_t();
        s->setSurface(surface_list[i]);
        surf[i] = s;
    }
    qDebug("IDirectXVideoAccelerationService_CreateSurface succeed with %d surfaces (%dx%d)", nb_surfaces, w, h);

    /* */
    DXVA2_VideoDesc dsc;
    ZeroMemory(&dsc, sizeof(dsc));
    dsc.SampleWidth     = w; //coded_width
    dsc.SampleHeight    = h; //coded_height
    dsc.Format          = fourccToD3D(format_fcc);
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
    DX_ENSURE_OK(vs->GetDecoderConfigurations(codec_guid,
                                            &dsc,
                                            NULL,
                                            &cfg_count,
                                            &cfg_list)
                 , false);
    const int score = SelectConfig(codec_id, cfg_list, cfg_count, &cfg);
    CoTaskMemFree(cfg_list);
    if (score <= 0)
        return false;
    /* Create the decoder */
    DX_ENSURE_OK(vs->CreateVideoDecoder(codec_guid, &dsc, &cfg, surface_list, nb_surfaces, &decoder), false);
    qDebug("IDirectXVideoDecoderService.CreateVideoDecoder succeed. decoder=%p", decoder);
    return true;
}

void VideoDecoderDXVAPrivate::destroyDecoder()
{
    SafeRelease(&decoder);
}

void* VideoDecoderDXVAPrivate::setupAVVAContext()
{
    // TODO: FF_DXVA2_WORKAROUND_SCALING_LIST_ZIGZAG
    if (isIntelClearVideo(&codec_guid)) {
#ifdef FF_DXVA2_WORKAROUND_INTEL_CLEARVIDEO //2014-03-07 - 8b2a130 - lavc 55.50.0 / 55.53.100 - dxva2.h
        qDebug("FF_DXVA2_WORKAROUND_INTEL_CLEARVIDEO");
        hw_ctx.workaround |= FF_DXVA2_WORKAROUND_INTEL_CLEARVIDEO;
#endif
    } else {
        hw_ctx.workaround = 0;
    }
    hw_ctx.decoder = decoder;
    hw_ctx.cfg = &cfg;
    hw_ctx.surface_count = hw_surfaces.size();
    hw_ctx.surface = (IDirect3DSurface9**)hw_surfaces.constData();
    return &hw_ctx;
}

bool VideoDecoderDXVAPrivate::setupSurfaceInterop()
{
    if (copy_mode == VideoDecoderFFmpegHW::ZeroCopy)
        interop_res = d3d9::InteropResourcePtr(d3d9::InteropResource::create(d3ddev));
    return true;
}
} //namespace QtAV
