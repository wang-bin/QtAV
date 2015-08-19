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

#ifndef QTAV_SURFACEINTEROPCUDA_H
#define QTAV_SURFACEINTEROPCUDA_H

#include "cuda_api.h"
#include <QtCore/QWeakPointer>
#include "QtAV/SurfaceInterop.h"
#include "utils/OpenGLHelper.h"
#ifdef Q_OS_WIN
// no need to check qt4 because no ANGLE there
#if QTAV_HAVE(EGL_CAPI) // always use dynamic load
#if defined(QT_OPENGL_DYNAMIC) || defined(QT_OPENGL_ES_2) || defined(QT_OPENGL_ES_2_ANGLE)
#define QTAV_HAVE_CUDA_EGL 1
#include <d3d9.h>
#endif
#endif //QTAV_HAVE(EGL_CAPI)
#endif //Q_OS_WIN
#if defined(QT_OPENGL_DYNAMIC) || !defined(QT_OPENGL_ES_2)
#define QTAV_HAVE_CUDA_GL 1
#endif

namespace QtAV {
namespace cuda {

class InteropResource : protected cuda_api
{
public:
    InteropResource(CUdevice d, CUvideodecoder decoder, CUvideoctxlock declock);
    ~InteropResource();
    /// copy from gpu (optimized if possible) and convert to target format if necessary
    // mapToHost
    /*!
     * \brief map
     * \param surface cuda decoded surface
     * \param tex opengl texture
     * \param h frame height(visual height) without alignment, <= cuda decoded surface(picture) height
     * \param ch cuda decoded surface(picture) height
     * \param plane useless now
     * \return true if success
     */
    virtual bool map(int picIndex, const CUVIDPROCPARAMS& param, GLuint tex, int h, int ch, int plane) = 0;
    virtual bool unmap(GLuint tex) { Q_UNUSED(tex); return true;}
protected:
    CUdevice dev;
    CUcontext ctx;
    CUvideodecoder dec;
    CUvideoctxlock lock;

    typedef struct {
       GLuint texture;
       int h, H;
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
     * \param height frame height(visual height) without alignment, <= cuda decoded surface(picture) height
     * \param coded_height cuda decoded surface(picture) height
     */
    void setSurface(int picIndex, CUVIDPROCPARAMS param, int height, int coded_height);
    /// GLTextureSurface only supports rgb32
    void* map(SurfaceType type, const VideoFormat& fmt, void* handle, int plane) Q_DECL_OVERRIDE;
    void unmap(void *handle) Q_DECL_OVERRIDE;
private:
    //CUdeviceptr m_surface; // invalid in a different context
    int m_index;
    CUVIDPROCPARAMS m_param;
    // decoder is deleted but interop is still alive
    QWeakPointer<InteropResource> m_resource;
    int h, H;
};

#if QTAV_HAVE(CUDA_EGL)
class EGL;
class EGLInteropResource Q_DECL_FINAL: public InteropResource
{
public:
    EGLInteropResource(CUdevice d, CUvideodecoder decoder, CUvideoctxlock declock);
    ~EGLInteropResource();
    //bool map(int picIndex, const CUVIDPROCPARAMS& param, GLuint tex, int h, int ch, int plane) Q_DECL_OVERRIDE { return false;}
private:
    void releaseEGL();

    EGL* egl;
    IDirect3D9 *d3d9;
    IDirect3DTexture9 *texture9;
    IDirect3DSurface9 *surface9; // size is frame size(visual size) for display
};
#endif //QTAV_HAVE(CUDA_EGL)

#if QTAV_HAVE(CUDA_GL)
class GLInteropResource Q_DECL_FINAL: public InteropResource
{
public:
    GLInteropResource(CUdevice d, CUvideodecoder decoder, CUvideoctxlock lk);
    bool map(int picIndex, const CUVIDPROCPARAMS& param, GLuint tex, int h, int ch, int plane) Q_DECL_OVERRIDE;
    bool unmap(GLuint tex) Q_DECL_OVERRIDE;
private:
    /*
     * TODO: do we need to check h, ch etc? interop is created by decoder and frame size does not change in the playback.
     * playing a new stream will recreate the decoder and interop
     * All we need to ensure is register when texture changed. But there's no way to check the texture change.
     */
    bool ensureResource(int h, int ch, GLuint tex, int plane);
};
#endif //QTAV_HAVE(CUDA_GL)
} //namespace cuda
} //namespace QtAV

#endif // QTAV_SURFACEINTEROPCUDA_H
