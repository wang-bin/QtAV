/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
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

#include "SurfaceInteropD3D11.h"
#include <stdint.h> // uint8_t for windows phone
#include "QtAV/VideoFrame.h"
#define DX_LOG_COMPONENT "D3D11 Interop"
#include "utils/DirectXHelper.h"

#include "opengl/OpenGLHelper.h"
// no need to check qt4 because no ANGLE there
#if QTAV_HAVE(EGL_CAPI) // always use dynamic load
#if defined(QT_OPENGL_DYNAMIC) || defined(QT_OPENGL_ES_2) || defined(QT_OPENGL_ES_2_ANGLE)
#define QTAV_HAVE_D3D11_EGL 1
#endif
#endif //QTAV_HAVE(EGL_CAPI)
#if defined(QT_OPENGL_DYNAMIC) || !defined(QT_OPENGL_ES_2)
#define QTAV_HAVE_D3D11_GL 1
#endif

namespace QtAV {

int fourccFromDXGI(DXGI_FORMAT fmt); //FIXME: defined in d3d11 decoder
VideoFormat::PixelFormat pixelFormatFromFourcc(int format);
namespace d3d11 {

bool InteropResource::isSupported(InteropType type)
{
    if (type == InteropAuto || type == InteropEGL) {
#if QTAV_HAVE(D3D11_EGL)
        if (OpenGLHelper::isOpenGLES())
            return true;
#endif
    }
    if (type == InteropAuto || type == InteropGL) {
#if QTAV_HAVE(D3D11_GL)
        if (!OpenGLHelper::isOpenGLES())
            return true;
#endif
    }
    return false;
}

extern InteropResource* CreateInteropEGL();
extern InteropResource* CreateInteropGL();
InteropResource* InteropResource::create(InteropType type)
{
    if (type == InteropAuto || type == InteropEGL) {
#if QTAV_HAVE(D3D11_EGL)
        if (OpenGLHelper::isOpenGLES())
            return CreateInteropEGL();
#endif
    }
    if (type == InteropAuto || type == InteropGL) {
#if QTAV_HAVE(D3D11_GL)
        if (!OpenGLHelper::isOpenGLES())
            return CreateInteropGL();
#endif
    }
    return NULL;
}

void InteropResource::setDevice(ComPtr<ID3D11Device> dev)
{
    d3ddev = dev;
}

void SurfaceInterop::setSurface(ComPtr<ID3D11Texture2D> surface, int index, int frame_w, int frame_h)
{
    m_surface = surface;
    m_index = index;
    frame_width = frame_w;
    frame_height = frame_h;
}

void* SurfaceInterop::map(SurfaceType type, const VideoFormat &fmt, void *handle, int plane)
{
    if (!handle)
        return NULL;
    if (!m_surface)
        return 0;
    if (type == GLTextureSurface) {
        if (m_resource->map(m_surface, m_index, *((GLuint*)handle), frame_width, frame_height, plane))
            return handle;
    } else if (type == HostMemorySurface) {
        return mapToHost(fmt, handle, plane);
    }
    return NULL;
}

void SurfaceInterop::unmap(void *handle)
{
    m_resource->unmap(*((GLuint*)handle));
}

void* SurfaceInterop::mapToHost(const VideoFormat &format, void *handle, int plane)
{
    Q_UNUSED(plane);
    ComPtr<ID3D11Device> dev;
    m_surface->GetDevice(&dev);
    D3D11_TEXTURE2D_DESC desc;
    m_surface->GetDesc(&desc);
    desc.MipLevels = 1;
    desc.MiscFlags = 0;
    desc.ArraySize = 1;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.BindFlags = 0; //?
    ComPtr<ID3D11Texture2D> tex;
    DX_ENSURE(dev->CreateTexture2D(&desc, NULL, &tex), NULL);
    ComPtr<ID3D11DeviceContext> ctx;
    dev->GetImmediateContext(&ctx);
    ctx->CopySubresourceRegion(tex.Get(), 0, 0, 0, 0
                               , m_surface.Get()
                               , m_index
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
    ScopedMap sm(ctx, tex, &mapped); //mingw error if ComPtr<T> constructs from ComPtr<U> [T=ID3D11Resource, U=ID3D11Texture2D]
    Q_UNUSED(sm);
    int pitch[3] = { (int)mapped.RowPitch, 0, 0}; //compute chroma later
    uint8_t *src[] = { (uint8_t*)mapped.pData, 0, 0}; //compute chroma later
    const VideoFormat fmt = pixelFormatFromFourcc(fourccFromDXGI(desc.Format));
    VideoFrame frame = VideoFrame::fromGPU(fmt, frame_width, frame_height, desc.Height, src, pitch);
    if (fmt != format)
        frame = frame.to(format);
    VideoFrame *f = reinterpret_cast<VideoFrame*>(handle);
    frame.setTimestamp(f->timestamp());
    *f = frame;
    return f;
}
} //namespace d3d11
} //namespace QtAV
