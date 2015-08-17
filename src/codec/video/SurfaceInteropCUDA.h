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
#include <d3d9.h>
#include "QtAV/SurfaceInterop.h"
#include "utils/OpenGLHelper.h"
// no need to check qt4 because no ANGLE there
#if QTAV_HAVE(EGL_CAPI) // always use dynamic load
#if defined(QT_OPENGL_DYNAMIC) || defined(QT_OPENGL_ES_2) || defined(QT_OPENGL_ES_2_ANGLE)
#define QTAV_HAVE_CUDA_EGL 1
#endif
#endif //QTAV_HAVE(EGL_CAPI)
#if defined(QT_OPENGL_DYNAMIC) || !defined(QT_OPENGL_ES_2)
#define QTAV_HAVE_CUDA_GL 1
#endif

namespace QtAV {
namespace cuda {

class InteropResource : protected cuda_api
{
public:
    InteropResource(CUdevice d, CUvideodecoder* decoder, CUvideoctxlock *declock);
    ~InteropResource();
    /// copy from gpu (optimized if possible) and convert to target format if necessary
    // mapToHost
    /*!
     * \brief map
     * \param surface cuda decoded surface
     * \param tex opengl texture
     * \param w frame width(visual width) without alignment, <= cuda surface width
     * \param h frame height(visual height)
     * \param plane useless now
     * \return true if success
     */
    virtual bool map(int picIndex, const CUVIDPROCPARAMS& param, GLuint tex, int w, int h, int plane) = 0;
    virtual bool unmap(GLuint tex) { Q_UNUSED(tex); return true;}
protected:
    int width, height; // video frame width and dx_surface width without alignment, not cuda decoded surface width
    CUdevice dev;
    CUcontext ctx;
    CUvideodecoder *dec; //store the ptr to check null *dec before use, e.g. decoder is deleted but frame is still alive. or use weak_ptr
    CUvideoctxlock *lock;

    typedef struct {
       GLuint texture;
       int width, height;
       CUgraphicsResource cuRes;
    } TexRes;
    TexRes res[2];
};
typedef QSharedPointer<InteropResource> InteropResourcePtr;

// avoid inherits cuda_api because SurfaceInteropCUDA is frequently created and cuda functions are only used in mapToHost()
class SurfaceInteropCUDA Q_DECL_FINAL: public VideoSurfaceInterop
{
public:
    SurfaceInteropCUDA(const InteropResourcePtr& res) : m_index(-1), m_resource(res), frame_width(0), frame_height(0) {}
    ~SurfaceInteropCUDA() {}
    /*!
     * \brief setSurface
     * \param surface cuda decoded picture index in cuda decoder.
     * If use CUdeviceptr, we must cuMemAllocPitch() and cuMemcpyDtoD for each plane every time. But we can not know whether
     * the decoded picture the index pointed to is changed when rendering, so the displayed images may out of order.
     * CUdeviceptr is associated with context
     * \param frame_w frame width(visual width) without alignment, <= cuda surface width
     * \param frame_h frame height(visual height)
     */
    void setSurface(int picIndex, CUVIDPROCPARAMS param, int frame_w, int frame_h);
    /// GLTextureSurface only supports rgb32
    void* map(SurfaceType type, const VideoFormat& fmt, void* handle, int plane) Q_DECL_OVERRIDE;
    void unmap(void *handle) Q_DECL_OVERRIDE;
private:
    //CUdeviceptr m_surface;
    int m_index;
    CUVIDPROCPARAMS m_param;
    InteropResourcePtr m_resource; //TODO: weak_ptr
    int frame_width, frame_height;
};

#if 0 //QTAV_HAVE(CUDA_EGL)
class EGL;
class EGLInteropResource Q_DECL_FINAL: public InteropResource
{
public:
    EGLInteropResource(CUvideodecoder* d3device);
    ~EGLInteropResource();
    bool map(int picIndex, const CUVIDPROCPARAMS& param, GLuint tex, int w, int h, int plane) Q_DECL_OVERRIDE;

private:
    void releaseEGL();
    bool ensureSurface(int w, int h);

    EGL* egl;
};
#endif //QTAV_HAVE(CUDA_EGL)

#if 1//QTAV_HAVE(CUDA_GL)
class GLInteropResource Q_DECL_FINAL: public InteropResource
{
public:
    GLInteropResource(CUdevice d, CUvideodecoder* decoder, CUvideoctxlock* lk);
    //~GLInteropResource();
    bool map(int picIndex, const CUVIDPROCPARAMS& param, GLuint tex, int frame_w, int frame_h, int plane) Q_DECL_OVERRIDE;
    bool unmap(GLuint tex) Q_DECL_OVERRIDE;
private:
    bool ensureResource(int w, int h, GLuint tex, int plane);
};
#endif //QTAV_HAVE(CUDA_GL)
} //namespace cuda
} //namespace QtAV

#endif // QTAV_SURFACEINTEROPCUDA_H
