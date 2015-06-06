/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2015 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_SURFACEINTEROPDXVA_H
#define QTAV_SURFACEINTEROPDXVA_H
#include <d3d9.h>
#include "QtAV/SurfaceInterop.h"
#include "utils/OpenGLHelper.h"
// no need to check qt4 because no ANGLE there
#if defined(QT_OPENGL_DYNAMIC) || defined(QT_OPENGL_ES_2) || defined(QT_OPENGL_ES_2_ANGLE)
#define QTAV_HAVE_DXVA_EGL 1
#endif
#if QTAV_HAVE(DXVA_EGL)
# ifdef QT_OPENGL_ES_2_ANGLE_STATIC
#   define CAPI_LINK_EGL
# else
#   define EGL_CAPI_NS
# endif //QT_OPENGL_ES_2_ANGLE_STATIC
#include "capi/egl_api.h"
#include <EGL/eglext.h> //include after egl_capi.h to match types
#endif //QTAV_HAVE(DXVA_EGL)

namespace QtAV {
namespace dxva {


class InteropResource
{
public:
    virtual ~InteropResource() {}
    virtual bool map(IDirect3DSurface9* surface, GLuint tex, int plane) = 0;
    virtual bool unmap(GLuint tex) { Q_UNUSED(tex); return true;}
};
typedef QSharedPointer<InteropResource> InteropResourcePtr;

class SurfaceInteropDXVA Q_DECL_FINAL: public VideoSurfaceInterop
{
public:
    SurfaceInteropDXVA(const InteropResourcePtr& res) : m_surface(0), m_resource(res) {}
    ~SurfaceInteropDXVA();
    void setSurface(IDirect3DSurface9* surface) { m_surface = surface; m_surface->AddRef();}
    void* map(SurfaceType type, const VideoFormat& fmt, void* handle, int plane) Q_DECL_OVERRIDE;
    void unmap(void *handle) Q_DECL_OVERRIDE;
protected:
    void* mapToHost(const VideoFormat &format, void *handle, int plane);
private:
    IDirect3DSurface9 *m_surface;
    InteropResourcePtr m_resource;
};

#if QTAV_HAVE(DXVA_EGL)
class EGLInteropResource Q_DECL_FINAL: public InteropResource
#ifndef EGL_CAPI_NS
                        , public egl::api
#endif //EGL_CAPI_NS
{
public:
    EGLInteropResource(IDirect3DDevice9 * d3device);
    ~EGLInteropResource();
    bool map(IDirect3DSurface9 *surface, GLuint tex, int) Q_DECL_OVERRIDE;

private:
    void releaseResource();
    bool ensureSurface(int w, int h);

    IDirect3DDevice9 *d3ddev;
    IDirect3DTexture9 *dx_texture;
    IDirect3DSurface9 *dx_surface;
    EGLDisplay egl_dpy;
    EGLSurface egl_surface;
    int width, height;
};
#endif //QTAV_HAVE(DXVA_EGL)
} //namespace dxva
} //namespace QtAV


#endif // QTAV_SURFACEINTEROPDXVA_H
