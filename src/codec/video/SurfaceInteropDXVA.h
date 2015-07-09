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
#if QTAV_HAVE(EGL_CAPI) // always use dynamic load
#if defined(QT_OPENGL_DYNAMIC) || defined(QT_OPENGL_ES_2) || defined(QT_OPENGL_ES_2_ANGLE)
#define QTAV_HAVE_DXVA_EGL 1
#endif
#endif //QTAV_HAVE(EGL_CAPI)
#if defined(QT_OPENGL_DYNAMIC) || !defined(QT_OPENGL_ES_2)
#define QTAV_HAVE_DXVA_GL 1
#endif

namespace QtAV {
namespace dxva {

class InteropResource
{
public:
    InteropResource(IDirect3DDevice9 * d3device);
    virtual ~InteropResource();
    /*!
     * \brief map
     * \param surface dxva decoded surface
     * \param tex opengl texture
     * \param w frame width(visual width) without alignment, <= dxva surface width
     * \param h frame height(visual height)
     * \param plane useless now
     * \return true if success
     */
    virtual bool map(IDirect3DSurface9* surface, GLuint tex, int w, int h, int plane) = 0;
    virtual bool unmap(GLuint tex) { Q_UNUSED(tex); return true;}
protected:
    void releaseDX();

    IDirect3DDevice9 *d3ddev;
    IDirect3DTexture9 *dx_texture;
    IDirect3DSurface9 *dx_surface; // size is frame size(visual size) for display
    int width, height; // video frame width and dx_surface width without alignment, not dxva decoded surface width
};
typedef QSharedPointer<InteropResource> InteropResourcePtr;

class SurfaceInteropDXVA Q_DECL_FINAL: public VideoSurfaceInterop
{
public:
    SurfaceInteropDXVA(const InteropResourcePtr& res) : m_surface(0), m_resource(res), frame_width(0), frame_height(0) {}
    ~SurfaceInteropDXVA();
    /*!
     * \brief setSurface
     * \param surface dxva decoded surface
     * \param frame_w frame width(visual width) without alignment, <= dxva surface width
     * \param frame_h frame height(visual height)
     */
    void setSurface(IDirect3DSurface9* surface, int frame_w, int frame_h);
    /// GLTextureSurface only supports rgb32
    void* map(SurfaceType type, const VideoFormat& fmt, void* handle, int plane) Q_DECL_OVERRIDE;
    void unmap(void *handle) Q_DECL_OVERRIDE;
protected:
    /// copy from gpu (optimized if possible) and convert to target format if necessary
    void* mapToHost(const VideoFormat &format, void *handle, int plane);
private:
    IDirect3DSurface9 *m_surface;
    InteropResourcePtr m_resource;
    int frame_width, frame_height;
};

#if QTAV_HAVE(DXVA_EGL)
class EGL;
class EGLInteropResource Q_DECL_FINAL: public InteropResource
{
public:
    EGLInteropResource(IDirect3DDevice9 * d3device);
    ~EGLInteropResource();
    bool map(IDirect3DSurface9 *surface, GLuint tex, int w, int h, int) Q_DECL_OVERRIDE;

private:
    void releaseEGL();
    bool ensureSurface(int w, int h);

    EGL* egl;
    IDirect3DQuery9 *dx_query;
};
#endif //QTAV_HAVE(DXVA_EGL)

#if QTAV_HAVE(DXVA_GL)
struct WGL;
class GLInteropResource Q_DECL_FINAL: public InteropResource
{
public:
    GLInteropResource(IDirect3DDevice9 * d3device);
    ~GLInteropResource();
    bool map(IDirect3DSurface9 *surface, GLuint tex, int frame_w, int frame_h, int) Q_DECL_OVERRIDE;
    bool unmap(GLuint tex) Q_DECL_OVERRIDE;
private:
    bool ensureWGL();
    bool ensureResource(int w, int h, GLuint tex);

    HANDLE interop_dev;
    HANDLE interop_obj;
    WGL *wgl;
};
#endif //QTAV_HAVE(DXVA_GL)
} //namespace dxva
} //namespace QtAV


#endif // QTAV_SURFACEINTEROPDXVA_H
