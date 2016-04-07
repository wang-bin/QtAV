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
//#define QTAV_HAVE_D3D11_GL 1
#endif

namespace QtAV {
namespace d3d11 {

bool InteropResource::isSupported()
{
#if QTAV_HAVE(EGL_CAPI)
    // runtime check gles for dynamic gl
    if (OpenGLHelper::isOpenGLES()) {
        return true;
    }
#endif
    return false;
}

extern InteropResource* CreateInteropEGL();
InteropResource* InteropResource::create(InteropType type)
{
    if (type == InteropAuto) {
#if QTAV_HAVE(EGL_CAPI)
        // runtime check gles for dynamic gl
        if (OpenGLHelper::isOpenGLES()) {
            return CreateInteropEGL();
        }
#endif
    }
#ifdef _MSC_VER
#pragma warning(disable:4065) //vc: switch has default but no case
#endif //_MSC_VER
    switch (type) {
#if QTAV_HAVE(EGL_CAPI)
    case InteropEGL: return CreateInteropEGL();
#endif
    default: return NULL;
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
        if (!fmt.isRGB())
            return NULL;
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
    //TODO: share code
    return NULL;
}
} //namespace d3d11
} //namespace QtAV
