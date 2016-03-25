/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2015)

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

#ifndef QTAV_SURFACEINTEROPCUDA_H
#define QTAV_SURFACEINTEROPCUDA_H

#include "cuda_api.h"
#include <QtCore/QWeakPointer>
#include "QtAV/SurfaceInterop.h"
#include "opengl/OpenGLHelper.h"
#ifndef QT_NO_OPENGL
#ifdef Q_OS_WIN
// no need to check qt4 because no ANGLE there
#if QTAV_HAVE(EGL_CAPI) // always use dynamic load
#if (defined(QT_OPENGL_DYNAMIC) || defined(QT_OPENGL_ES_2) || defined(QT_OPENGL_ES_2_ANGLE)) && !defined(Q_OS_WINRT)
#define QTAV_HAVE_CUDA_EGL 1
#include <d3d9.h>
#endif
#endif //QTAV_HAVE(EGL_CAPI)
#endif //Q_OS_WIN
#if defined(QT_OPENGL_DYNAMIC) || !defined(QT_OPENGL_ES_2)
#define QTAV_HAVE_CUDA_GL 1
#endif
#endif //QT_NO_OPENGL
namespace QtAV {
namespace cuda {

class InteropResource : protected cuda_api
{
public:
    InteropResource();
    void setDevice(CUdevice d) { dev = d;}
    void setShareContext(CUcontext c) {
        ctx = c;
        share_ctx = !!c;
    }
    void setDecoder(CUvideodecoder d) { dec = d;}
    void setLock(CUvideoctxlock l) { lock = l;}
    virtual ~InteropResource();
    /// copy from gpu (optimized if possible) and convert to target format if necessary
    // mapToHost
    /*!
     * \brief map
     * \param surface cuda decoded surface
     * \param tex opengl texture
     * \param h frame height(visual height) without alignment, <= cuda decoded surface(picture) height
     * \param H cuda decoded surface(picture) height
     * \param plane useless now
     * \return true if success
     */
    virtual bool map(int picIndex, const CUVIDPROCPARAMS& param, GLuint tex, int w, int h, int H, int plane) = 0;
    virtual bool unmap(GLuint tex) { Q_UNUSED(tex); return true;}
    /// copy from gpu and convert to target format if necessary. used by VideoCapture
    void* mapToHost(const VideoFormat &format, void *handle, int picIndex, const CUVIDPROCPARAMS &param, int width, int height, int surface_height);
protected:
    bool share_ctx;
    CUdevice dev;
    CUcontext ctx;
    CUvideodecoder dec;
    CUvideoctxlock lock;

    typedef struct {
       GLuint texture;
       int w, h, W, H;
       CUgraphicsResource cuRes;
       CUstream stream; // for async works
    } TexRes;
    TexRes res[2];
};
typedef QSharedPointer<InteropResource> InteropResourcePtr;

// avoid inheriting cuda_api because SurfaceInteropCUDA is frequently created and cuda functions are only used in mapToHost()
class SurfaceInteropCUDA Q_DECL_FINAL: public VideoSurfaceInterop
{
public:
    SurfaceInteropCUDA(const QWeakPointer<InteropResource>& res) : m_index(-1), m_resource(res), h(0), H(0) {}
    ~SurfaceInteropCUDA() {}
    /*!
     * \brief setSurface
     * \param picIndex cuda decoded picture index in cuda decoder.
     * If use CUdeviceptr, we must cuMemAllocPitch() and cuMemcpyDtoD for each plane every time. But we can not know whether
     * the decoded picture the index pointed to is changed when rendering, so the displayed images may out of order.
     * CUdeviceptr is associated with context
     * \param width frame width(visual width) without alignment, <= cuda decoded surface(picture) width
     * \param height frame height(visual height) without alignment, <= cuda decoded surface(picture) height
     * \param surface_height cuda decoded surface(picture) height
     */
    void setSurface(int picIndex, CUVIDPROCPARAMS param, int width, int height, int surface_height);
    /// GLTextureSurface only supports rgb32
    void* map(SurfaceType type, const VideoFormat& fmt, void* handle, int plane) Q_DECL_OVERRIDE;
    void unmap(void *handle) Q_DECL_OVERRIDE;
private:
    //CUdeviceptr m_surface; // invalid in a different context
    int m_index;
    CUVIDPROCPARAMS m_param;
    // decoder is deleted but interop is still alive
    QWeakPointer<InteropResource> m_resource;
    int w, h, H;
};

#ifndef QT_NO_OPENGL
class HostInteropResource Q_DECL_FINAL: public InteropResource
{
public:
    HostInteropResource();
    ~HostInteropResource();
    bool map(int picIndex, const CUVIDPROCPARAMS& param, GLuint tex, int w, int h, int H, int plane) Q_DECL_OVERRIDE;
    bool unmap(GLuint) Q_DECL_OVERRIDE;
private:
    bool ensureResource(int pitch, int height);

    struct {
        int index;
        uchar* data;
        int height;
        int pitch;
    } host_mem;
};
#endif //QT_NO_OPENGL

#if QTAV_HAVE(CUDA_EGL)
class EGL;
/*!
 * \brief The EGLInteropResource class
 * Interop with NV12 texture, then convert NV12 to RGB texture like DXVA+EGL does.
 * TODO: use pixel shader to convert L8+A8L8 textures to a NV12 texture, or an rgb texture directly on pbuffer surface
 * The VideoFrame from CUDA decoder is RGB format
 */
class EGLInteropResource Q_DECL_FINAL: public InteropResource
{
public:
    EGLInteropResource();
    ~EGLInteropResource();
    bool map(int picIndex, const CUVIDPROCPARAMS& param, GLuint tex, int w, int h, int H, int plane) Q_DECL_OVERRIDE;
private:
    bool map(IDirect3DSurface9 *surface, GLuint tex, int w, int h, int H);
    bool ensureD3DDevice();
    void releaseEGL();
    bool ensureResource(int w, int h, int W, int H, GLuint tex);
    bool ensureD3D9CUDA(int w, int h, int W, int H); // check/update nv12 texture and register for cuda interoperation
    bool ensureD3D9EGL(int w, int h);

    EGL* egl;
    HINSTANCE dll9;
    IDirect3D9 *d3d9;
    IDirect3DDevice9 *device9; // for CUDA
    IDirect3DTexture9 *texture9; // for EGL.
    IDirect3DSurface9 *surface9; // for EGL. size is frame size(visual size) for display
    IDirect3DTexture9 *texture9_nv12;
    // If size is coded size, crop when StretchRect. If frame size, crop in cuMemcpy2D for each plane
    IDirect3DSurface9 *surface9_nv12;
    IDirect3DQuery9 *query9;
};
#endif //QTAV_HAVE(CUDA_EGL)

#if QTAV_HAVE(CUDA_GL)
class GLInteropResource Q_DECL_FINAL: public InteropResource
{
public:
    bool map(int picIndex, const CUVIDPROCPARAMS& param, GLuint tex, int w, int h, int H, int plane) Q_DECL_OVERRIDE;
    bool unmap(GLuint tex) Q_DECL_OVERRIDE;
private:
    /*
     * TODO: do we need to check h, H etc? interop is created by decoder and frame size does not change in the playback.
     * playing a new stream will recreate the decoder and interop
     * All we need to ensure is register when texture changed. But there's no way to check the texture change.
     */
    bool ensureResource(int w, int h, int H, GLuint tex, int plane);
};
#endif //QTAV_HAVE(CUDA_GL)
} //namespace cuda
} //namespace QtAV

#endif // QTAV_SURFACEINTEROPCUDA_H
