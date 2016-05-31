/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

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
#include <initguid.h> //IID_ID3D11VideoContext
#include "VideoDecoderD3D.h"
#include "QtAV/private/factory.h"
#include "QtAV/private/mkid.h"
#define DX_LOG_COMPONENT "D3D11VA"
#include "utils/DirectXHelper.h"
#include "directx/dxcompat.h"
#include <d3d11.h> //include before <libavcodec/d3d11va.h> because d3d11va.h also includes d3d11.h but as a c header (for msvc)
#include <wrl/client.h>
extern "C" {
#include <libavcodec/d3d11va.h>
}
using namespace Microsoft::WRL; //ComPtr
#include "directx/SurfaceInteropD3D11.h"
#include "utils/Logger.h"

// define __mingw_uuidof
#ifdef __CRT_UUID_DECL
__CRT_UUID_DECL(ID3D11VideoContext,0x61F21C45,0x3C0E,0x4a74,0x9C,0xEA,0x67,0x10,0x0D,0x9A,0xD5,0xE4)
__CRT_UUID_DECL(ID3D11VideoDevice,0x10EC4D5B,0x975A,0x4689,0xB9,0xE4,0xD0,0xAA,0xC3,0x0F,0xE3,0x33)
#endif //__CRT_UUID_DECL

namespace QtAV {
static QString sD3D11Description;

struct dxgi_fcc {
    int fourcc;
    DXGI_FORMAT dxgi;
} dxgi_formats[] = {
{MAKEFOURCC('N','V','1','2'), DXGI_FORMAT_NV12},
{MAKEFOURCC('P','0','1','0'), DXGI_FORMAT_P010},
{MAKEFOURCC('P','0','1','6'), DXGI_FORMAT_P016},
};

DXGI_FORMAT fourccToDXGI(int fourcc)
{
    for (size_t i = 0; i < sizeof(dxgi_formats)/sizeof(dxgi_formats[0]); ++i) {
        if (dxgi_formats[i].fourcc == fourcc)
            return dxgi_formats[i].dxgi;
    }
    return DXGI_FORMAT_UNKNOWN;
}

int fourccFromDXGI(DXGI_FORMAT fmt)
{
    for (const dxgi_fcc* f = dxgi_formats; f < dxgi_formats + sizeof(dxgi_formats)/sizeof(dxgi_formats[0]); ++f) {
        if (f->dxgi == fmt)
            return f->fourcc;
    }
    return 0;
}

class VideoDecoderD3D11Private;
class VideoDecoderD3D11 : public VideoDecoderD3D
{
    DPTR_DECLARE_PRIVATE(VideoDecoderD3D11)
public:
    VideoDecoderD3D11();
    VideoDecoderId id() const Q_DECL_OVERRIDE;
    QString description() const Q_DECL_OVERRIDE;
    VideoFrame frame() Q_DECL_OVERRIDE;
};

extern VideoDecoderId VideoDecoderId_D3D11;
FACTORY_REGISTER(VideoDecoder, D3D11, "D3D11")


VideoDecoderId VideoDecoderD3D11::id() const
{
    return VideoDecoderId_D3D11;
}

QString VideoDecoderD3D11::description() const
{
    if (!sD3D11Description.isEmpty())
        return sD3D11Description;
    return QStringLiteral("D3D11 Video Acceleration");
}

struct d3d11_surface_t : public va_surface_t {
    d3d11_surface_t(): va_surface_t(), view(0) {}
    void setSurface(IUnknown* s) {
        view = (ID3D11VideoDecoderOutputView*)s;
    }
    IUnknown* getSurface() const { return view.Get();}

private:
    ComPtr<ID3D11VideoDecoderOutputView> view;
};

class VideoDecoderD3D11Private Q_DECL_FINAL : public VideoDecoderD3DPrivate
{
public:
    VideoDecoderD3D11Private()
        : dll(0)
    {
        //GetModuleHandle()
#ifndef Q_OS_WINRT
        dll = LoadLibrary(TEXT("d3d11.dll"));
        available = !!dll;
#endif
        if (d3d11::InteropResource::isSupported(d3d11::InteropEGL))
            copy_mode = VideoDecoderFFmpegHW::ZeroCopy;
    }
    ~VideoDecoderD3D11Private() {
#ifndef Q_OS_WINRT
        if (dll) {
            FreeLibrary(dll);
        }
#endif
    }

    AVPixelFormat vaPixelFormat() const Q_DECL_OVERRIDE { return QTAV_PIX_FMT_C(D3D11VA_VLD);}
private:
    bool createDevice() Q_DECL_OVERRIDE;
    void destroyDevice() Q_DECL_OVERRIDE; //d3d device and it's resources
    bool createDecoder(AVCodecID codec_id, int w, int h, QVector<va_surface_t*>& surf) Q_DECL_OVERRIDE;
    void destroyDecoder() Q_DECL_OVERRIDE;
    bool setupSurfaceInterop() Q_DECL_OVERRIDE;
    void* setupAVVAContext() Q_DECL_OVERRIDE;
    QVector<GUID> getSupportedCodecs() const Q_DECL_OVERRIDE;
    int fourccFor(const GUID *guid) const Q_DECL_OVERRIDE;

    HMODULE dll;
    ComPtr<ID3D11Device> d3ddev;
    ComPtr<ID3D11VideoDevice> d3dviddev;
    ComPtr<ID3D11VideoDecoder> d3ddec;
    ComPtr<ID3D11VideoContext> d3dvidctx;
    D3D11_VIDEO_DECODER_CONFIG cfg;
    struct AVD3D11VAContext hw;
public:
    ComPtr<ID3D11DeviceContext> d3dctx;
    ComPtr<ID3D11Texture2D> texture_cpu; // used by copy mode. d3d11 texture can not be accessed by both gpu and cpu
    d3d11::InteropResourcePtr interop_res; //may be still used in video frames when decoder is destroyed
};

VideoFrame VideoDecoderD3D11::frame()
{
    DPTR_D(VideoDecoderD3D11);
    //qDebug("frame size: %dx%d", d.frame->width, d.frame->height);
    if (!d.frame->opaque || !d.frame->data[0])
        return VideoFrame();
    if (d.frame->width <= 0 || d.frame->height <= 0 || !d.codec_ctx)
        return VideoFrame();

    ID3D11VideoDecoderOutputView *surface = (ID3D11VideoDecoderOutputView*)(uintptr_t)d.frame->data[3];
    if (!surface) {
        qWarning("Get D3D11 surface error");
        return VideoFrame();
    }
    ComPtr<ID3D11Texture2D> texture;// = ((d3d11_surface_t*)(uintptr_t)d.frame->opaque)->texture;
    surface->GetResource((ID3D11Resource**)texture.GetAddressOf());
    if (!texture) {
        qWarning("Get D3D11 texture error");
        return VideoFrame();
    }
    D3D11_VIDEO_DECODER_OUTPUT_VIEW_DESC view_desc;
    surface->GetDesc(&view_desc);
    D3D11_TEXTURE2D_DESC tex_desc;
    texture->GetDesc(&tex_desc);
    if (copyMode() == VideoDecoderFFmpegHW::ZeroCopy && d.interop_res) {
        d3d11::SurfaceInterop *interop = new d3d11::SurfaceInterop(d.interop_res);
        interop->setSurface(texture, view_desc.Texture2D.ArraySlice, d.width, d.height);
        VideoFormat fmt(d.interop_res->format(tex_desc.Format));
        VideoFrame f(d.width, d.height, fmt);
        for (int i = 0; i < fmt.planeCount(); ++i) {
            f.setBytesPerLine(fmt.bytesPerLine(d.width, i), i); //used by gl to compute texture size
        }
        f.setMetaData(QStringLiteral("surface_interop"), QVariant::fromValue(VideoSurfaceInteropPtr(interop)));
        f.setTimestamp(d.frame->pkt_pts/1000.0);
        f.setDisplayAspectRatio(d.getDAR(d.frame));
        return f;
    }
//    qDebug("process for view: %p, texture: %p", surface, texture.Get());
    d.d3dctx->CopySubresourceRegion(d.texture_cpu.Get(), 0, 0, 0, 0,
                                    texture.Get()
                                    , view_desc.Texture2D.ArraySlice
                                    , NULL);
    struct ScopedMap {
        ScopedMap(ComPtr<ID3D11DeviceContext> ctx, ComPtr<ID3D11Texture2D> res, D3D11_MAPPED_SUBRESOURCE *mapped): c(ctx), r(res) {
            DX_ENSURE(c->Map(r.Get(), 0, D3D11_MAP_READ, 0, mapped)); //TODO: check error
        }
        ~ScopedMap() { c->Unmap(r.Get(), 0);}
        ComPtr<ID3D11DeviceContext> c;
        ComPtr<ID3D11Texture2D> r;
    };

    D3D11_MAPPED_SUBRESOURCE mapped;
    ScopedMap sm(d.d3dctx, d.texture_cpu, &mapped); //mingw error if ComPtr<T> constructs from ComPtr<U> [T=ID3D11Resource, U=ID3D11Texture2D]
    Q_UNUSED(sm);
    int pitch[3] = { (int)mapped.RowPitch, 0, 0}; //compute chroma later
    uint8_t *src[] = { (uint8_t*)mapped.pData, 0, 0}; //compute chroma later
    const VideoFormat format = pixelFormatFromFourcc(d.format_fcc); //tex_desc
    return copyToFrame(format, tex_desc.Height, src, pitch, false);
}

VideoDecoderD3D11::VideoDecoderD3D11()
    : VideoDecoderD3D(*new VideoDecoderD3D11Private())
{
}

bool VideoDecoderD3D11Private::createDevice()
{
    // if (d3dviddev) return true;
    PFN_D3D11_CREATE_DEVICE fCreateDevice = NULL;
#if defined(Q_OS_WINRT)
    fCreateDevice = ::D3D11CreateDevice;
#else
    fCreateDevice = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(dll, "D3D11CreateDevice");
    if (!fCreateDevice) {
        qWarning("Can not resolve symbol D3D11CreateDevice");
    }
#endif
    // TODO: feature levels
    DX_ENSURE(fCreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
                            D3D11_CREATE_DEVICE_VIDEO_SUPPORT, NULL, 0,
                            D3D11_SDK_VERSION, d3ddev.ReleaseAndGetAddressOf(), NULL,
                            d3dctx.ReleaseAndGetAddressOf()), false);

    ComPtr<ID3D10Multithread> mt;
    DX_ENSURE(d3ddev.As(&mt), false);
    mt->SetMultithreadProtected(true); //TODO: check threads
    DX_ENSURE(d3dctx.As(&d3dvidctx), false);
    DX_ENSURE(d3ddev.As(&d3dviddev), false);

    ComPtr<IDXGIDevice> dxgi_dev;
    DX_ENSURE(d3ddev.As(&dxgi_dev), false);
    ComPtr<IDXGIAdapter> dxgi_adapter;
    DX_ENSURE(dxgi_dev->GetAdapter(dxgi_adapter.GetAddressOf()), false);
    DXGI_ADAPTER_DESC desc;
    DX_ENSURE(dxgi_adapter->GetDesc(&desc), false);
    sD3D11Description = QStringLiteral("D3D11 Video Acceleration (%1, vendor %2(%3), device %4, revision %5)")
            .arg(QString::fromWCharArray(desc.Description))
            .arg(desc.VendorId)
            .arg(QString::fromUtf8(DXHelper::vendorName(desc.VendorId)))
            .arg(desc.DeviceId)
            .arg(desc.Revision)
            ;
    qDebug() << sD3D11Description;
    description = sD3D11Description;
    return true;
}

void VideoDecoderD3D11Private::destroyDevice()
{
    d3dviddev.Reset();
    d3dvidctx.Reset();
    d3dctx.Reset();
    d3ddev.Reset();
}

int VideoDecoderD3D11Private::fourccFor(const GUID *guid) const
{
    BOOL is_supported = FALSE;
    for (size_t i = 0; i < sizeof(dxgi_formats)/sizeof(dxgi_formats[i]); ++i) {
        const dxgi_fcc& f = dxgi_formats[i];
        DX_ENSURE(d3dviddev->CheckVideoDecoderFormat(guid, f.dxgi, &is_supported), 0);
        if (is_supported)
            return f.fourcc;
    }
    return 0;
}

QVector<GUID> VideoDecoderD3D11Private::getSupportedCodecs() const
{
    UINT nb_profiles = d3dviddev->GetVideoDecoderProfileCount();
    QVector<GUID> guids(nb_profiles);
    for (UINT i = 0; i < nb_profiles; ++i) {
        DX_ENSURE(d3dviddev->GetVideoDecoderProfile(i, &guids[i]), QVector<GUID>());
    }
    return guids;
}

bool VideoDecoderD3D11Private::createDecoder(AVCodecID codec_id, int w, int h, QVector<va_surface_t *> &surf)
{
    const int nb_surfaces = surf.size();
    D3D11_TEXTURE2D_DESC texDesc;
    ZeroMemory(&texDesc, sizeof(texDesc));
    texDesc.Width = aligned(w);
    texDesc.Height = aligned(h);
    texDesc.MipLevels = 1;
    texDesc.Format = fourccToDXGI(format_fcc);
    texDesc.SampleDesc.Count = 1;
    texDesc.MiscFlags = 0;
    texDesc.ArraySize = nb_surfaces;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_DECODER;
    texDesc.CPUAccessFlags = 0;
    ComPtr<ID3D11Texture2D> tex;
    DX_ENSURE(d3ddev->CreateTexture2D(&texDesc, NULL, tex.GetAddressOf()), false);

    if (copy_mode != VideoDecoderFFmpegHW::ZeroCopy || !interop_res) { //copy
        tex->GetDesc(&texDesc);
        texDesc.MipLevels = 1;
        texDesc.MiscFlags = 0;
        texDesc.ArraySize = 1;
        texDesc.Usage = D3D11_USAGE_STAGING;
        texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        texDesc.BindFlags = 0; //?
        DX_ENSURE(d3ddev->CreateTexture2D(&texDesc, NULL, &texture_cpu), false);
    }
    D3D11_VIDEO_DECODER_OUTPUT_VIEW_DESC viewDesc;
    ZeroMemory(&viewDesc, sizeof(viewDesc));
    viewDesc.DecodeProfile = codec_guid;
    viewDesc.ViewDimension = D3D11_VDOV_DIMENSION_TEXTURE2D;

    for (int i = 0; i < nb_surfaces; ++i) {
        viewDesc.Texture2D.ArraySlice = i;
        ComPtr<ID3D11VideoDecoderOutputView> view;
        DX_ENSURE(d3dviddev->CreateVideoDecoderOutputView(tex.Get(), &viewDesc, &view), false);
        d3d11_surface_t *s = new d3d11_surface_t();
        s->setSurface(view.Get());
        surf[i] = s;
    }
    qDebug("ID3D11VideoDecoderOutputView %d surfaces (%dx%d)", nb_surfaces, aligned(w), aligned(h));

    D3D11_VIDEO_DECODER_DESC decoderDesc;
    ZeroMemory(&decoderDesc, sizeof(decoderDesc));
    decoderDesc.Guid = codec_guid;
    decoderDesc.SampleWidth = w;
    decoderDesc.SampleHeight = h;
    decoderDesc.OutputFormat = fourccToDXGI(format_fcc);

    UINT cfg_count;
    DX_ENSURE(d3dviddev->GetVideoDecoderConfigCount(&decoderDesc, &cfg_count), false);

    /* List all configurations available for the decoder */
    QVector<D3D11_VIDEO_DECODER_CONFIG> cfg_list(cfg_count);
    for (unsigned i = 0; i < cfg_count; i++) {
        DX_ENSURE(d3dviddev->GetVideoDecoderConfig(&decoderDesc, i, &cfg_list[i]), false);
    }
    if (SelectConfig(codec_id, cfg_list.constData(), cfg_count, &cfg) <= 0)
        return false;
    // not associated with surfaces, so surfaces can be dynamicall added?
    DX_ENSURE(d3dviddev->CreateVideoDecoder(&decoderDesc, &cfg, &d3ddec), false);
    return true;
}

void VideoDecoderD3D11Private::destroyDecoder()
{
    texture_cpu.Reset();
    d3ddec.Reset();
}

bool VideoDecoderD3D11Private::setupSurfaceInterop()
{
    if (copy_mode == VideoDecoderFFmpegHW::ZeroCopy)
        interop_res = d3d11::InteropResourcePtr(d3d11::InteropResource::create());
    if (interop_res)
        interop_res->setDevice(d3ddev);
    return true;
}

void* VideoDecoderD3D11Private::setupAVVAContext()
{
    // TODO: FF_DXVA2_WORKAROUND_SCALING_LIST_ZIGZAG
    if (isIntelClearVideo(&codec_guid)) {
#ifdef FF_DXVA2_WORKAROUND_INTEL_CLEARVIDEO //2014-03-07 - 8b2a130 - lavc 55.50.0 / 55.53.100 - dxva2.h
        qDebug("FF_DXVA2_WORKAROUND_INTEL_CLEARVIDEO");
        hw.workaround |= FF_DXVA2_WORKAROUND_INTEL_CLEARVIDEO;
#endif
    } else {
        hw.workaround = 0;
    }
    hw.video_context = d3dvidctx.Get();
    hw.decoder = d3ddec.Get();
    hw.cfg = &cfg;
    hw.surface_count = hw_surfaces.size();
    hw.surface = (ID3D11VideoDecoderOutputView**)hw_surfaces.constData();
    return &hw;
}
} //namespace QtAV
