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

#include "SurfaceInteropCUDA.h"
#include "QtAV/VideoFrame.h"
#include "utils/Logger.h"
#include "helper_cuda.h"

#define WORKAROUND_UNMAP_CONTEXT_SWITCH 1
#define USE_STREAM 0

namespace QtAV {
namespace cuda {

InteropResource::InteropResource(CUdevice d, CUvideodecoder decoder, CUvideoctxlock declock)
    : cuda_api()
    , dev(d)
    , ctx(NULL)
    , dec(decoder)
    , lock(declock)
{
    memset(res, 0, sizeof(res));
}

InteropResource::~InteropResource()
{
    //CUDA_WARN(cuCtxPushCurrent(ctx)); //error invalid value
    if (res[0].cuRes)
        CUDA_WARN(cuGraphicsUnregisterResource(res[0].cuRes));
    if (res[1].cuRes)
        CUDA_WARN(cuGraphicsUnregisterResource(res[1].cuRes));
    if (res[0].stream)
        CUDA_WARN(cuStreamDestroy(res[0].stream));
    if (res[1].stream)
        CUDA_WARN(cuStreamDestroy(res[1].stream));

    // FIXME: we own the context. But why crash to destroy ctx? CUDA_ERROR_INVALID_VALUE
    //CUDA_ENSURE(cuCtxDestroy(ctx));
}

void SurfaceInteropCUDA::setSurface(int picIndex, CUVIDPROCPARAMS param, int height, int coded_height)
{
    m_index = picIndex;
    m_param = param;
    h = height;
    H = coded_height;
}

void* SurfaceInteropCUDA::map(SurfaceType type, const VideoFormat &fmt, void *handle, int plane)
{
    Q_UNUSED(fmt);
    if (m_resource.isNull())
        return NULL;
    if (!handle)
        return NULL;

    if (m_index < 0)
        return 0;
    if (type == GLTextureSurface) {
        // FIXME: to strong ref may delay the delete and cuda resource maybe already destoryed after strong ref is finished
        if (m_resource.toStrongRef()->map(m_index, m_param, *((GLuint*)handle), h, H, plane))
            return handle;
    } else if (type == HostMemorySurface) {
        return NULL;//mapToHost(fmt, handle, plane);
    }
    return NULL;
}

void SurfaceInteropCUDA::unmap(void *handle)
{
    if (m_resource.isNull())
        return;
    // FIXME: to strong ref may delay the delete and cuda resource maybe already destoryed after strong ref is finished
    m_resource.toStrongRef()->unmap(*((GLuint*)handle));
}
} //namespace cuda
} //namespace QtAV

#if QTAV_HAVE(CUDA_EGL)
#define EGL_ENSURE(x, ...) \
    do { \
        if (!(x)) { \
            EGLint err = eglGetError(); \
            qWarning("EGL error@%d<<%s. " #x ": %#x %s", __LINE__, __FILE__, err, eglQueryString(eglGetCurrentDisplay(), err)); \
            return __VA_ARGS__; \
        } \
    } while(0)

#if QTAV_HAVE(GUI_PRIVATE)
#include <qpa/qplatformnativeinterface.h>
#include <QtGui/QGuiApplication>
#endif //QTAV_HAVE(GUI_PRIVATE)
#ifdef QT_OPENGL_ES_2_ANGLE_STATIC
#define CAPI_LINK_EGL
#else
#define EGL_CAPI_NS
#endif //QT_OPENGL_ES_2_ANGLE_STATIC
#include "capi/egl_api.h"
#include <EGL/eglext.h> //include after egl_capi.h to match types
#define DX_LOG_COMPONENT "CUDA"
#include "utils/DirectXHelper.h"

namespace QtAV {
namespace cuda {
class EGL {
public:
    EGL() : dpy(EGL_NO_DISPLAY), surface(EGL_NO_SURFACE) {}
    EGLDisplay dpy;
    EGLSurface surface; //only support rgb. then we must use CUDA kernel
#ifdef EGL_VERSION_1_5
    // eglCreateImageKHR does not support EGL_NATIVE_PIXMAP_KHR, only 2d, 3d, render buffer
    //EGLImageKHR image[2];
    //EGLImage image[2]; //not implemented yet
#endif //EGL_VERSION_1_5
};

EGLInteropResource::EGLInteropResource(CUdevice d, CUvideodecoder decoder, CUvideoctxlock declock)
    : InteropResource(d, decoder, declock)
    , egl(new EGL())
    , d3d9(NULL)
    , texture9(NULL)
    , surface9(NULL)
{
}


EGLInteropResource::~EGLInteropResource()
{
    releaseEGL();
    if (egl) {
        delete egl;
        egl = NULL;
    }
    SafeRelease(&surface9);
    SafeRelease(&texture9);
    SafeRelease(&d3d9);
}

void EGLInteropResource::releaseEGL() {
    if (egl->surface != EGL_NO_SURFACE) {
        eglReleaseTexImage(egl->dpy, egl->surface, EGL_BACK_BUFFER);
        eglDestroySurface(egl->dpy, egl->surface);
        egl->surface = EGL_NO_SURFACE;
    }
}
} //namespace cuda
} //namespace QtAV
#endif //QTAV_HAVE(CUDA_EGL)
#if QTAV_HAVE(CUDA_GL)
namespace QtAV {
namespace cuda {
GLInteropResource::GLInteropResource(CUdevice d, CUvideodecoder decoder, CUvideoctxlock lk)
    : InteropResource(d, decoder, lk)
{}

bool GLInteropResource::map(int picIndex, const CUVIDPROCPARAMS &param, GLuint tex, int h, int ch, int plane)
{
    AutoCtxLock locker((cuda_api*)this, lock);
    Q_UNUSED(locker);
    if (!ensureResource(h, ch, tex, plane)) // TODO surface size instead of frame size because we copy the device data
        return false;
    //CUDA_ENSURE(cuCtxPushCurrent(ctx), false);
    CUdeviceptr devptr;
    unsigned int pitch;

    CUDA_ENSURE(cuvidMapVideoFrame(dec, picIndex, &devptr, &pitch, const_cast<CUVIDPROCPARAMS*>(&param)), false);
    class AutoUnmapper {
        cuda_api *api;
        CUvideodecoder dec;
        CUdeviceptr devptr;
    public:
        AutoUnmapper(cuda_api *a, CUvideodecoder d, CUdeviceptr p) : api(a), dec(d), devptr(p) {}
        ~AutoUnmapper() {
            CUDA_WARN2(api->cuvidUnmapVideoFrame(dec, devptr));
        }
    };
    AutoUnmapper unmapper(this, dec, devptr);
    Q_UNUSED(unmapper);
    // TODO: why can not use res[plane].stream? CUDA_ERROR_INVALID_HANDLE
    CUDA_ENSURE(cuGraphicsMapResources(1, &res[plane].cuRes, 0), false);
    CUarray array;
    CUDA_ENSURE(cuGraphicsSubResourceGetMappedArray(&array, res[plane].cuRes, 0, 0), false);

    CUDA_MEMCPY2D cu2d;
    memset(&cu2d, 0, sizeof(cu2d));
    cu2d.srcDevice = devptr;
    cu2d.srcMemoryType = CU_MEMORYTYPE_DEVICE;
    cu2d.srcPitch = pitch;
    cu2d.dstArray = array;
    cu2d.dstMemoryType = CU_MEMORYTYPE_ARRAY;
    cu2d.dstPitch = pitch;
    // the whole size or copy size?
    cu2d.WidthInBytes = pitch;
    cu2d.Height = h;
    if (plane == 1) {
        cu2d.srcXInBytes = 0;// TODO: why not pitch*ch?
        cu2d.srcY = ch; // skip the padding height
        cu2d.Height /= 2;
    }
#if USE_STREAM
    CUDA_ENSURE(cuMemcpy2DAsync(&cu2d, res[plane].stream), false);
#else
    CUDA_ENSURE(cuMemcpy2D(&cu2d), false);
#endif
    //TODO: delay cuCtxSynchronize && unmap. do it in unmap(tex)?
    // map to an already mapped resource will crash. sometimes I can not unmap the resource in unmap(tex) because if context switch error
    // so I simply unmap the resource here
    if (WORKAROUND_UNMAP_CONTEXT_SWITCH) {
#if USE_STREAM
        //CUDA_WARN(cuCtxSynchronize(), false); //wait too long time? use cuStreamQuery?
        CUDA_WARN(cuStreamSynchronize(res[plane].stream)); //slower than CtxSynchronize
#endif
        /*
         * This function provides the synchronization guarantee that any CUDA work issued
         * in \p stream before ::cuGraphicsUnmapResources() will complete before any
         * subsequently issued graphics work begins.
         * The graphics API from which \p resources were registered
         * should not access any resources while they are mapped by CUDA. If an
         * application does so, the results are undefined.
         */
        CUDA_ENSURE(cuGraphicsUnmapResources(1, &res[plane].cuRes, 0), false);
    } else {
        // call it at last. current context will be used by other cuda calls (unmap() for example)
        CUDA_ENSURE(cuCtxPopCurrent(&ctx), false);
    }
    return true;
}

bool GLInteropResource::unmap(GLuint tex)
{
    Q_UNUSED(tex);
#if !WORKAROUND_UNMAP_CONTEXT_SWITCH
    int plane = -1;
    if (res[0].texture == tex)
        plane = 0;
    else if (res[1].texture == tex)
        plane = 1;
    else
        return false;
    // FIXME: why cuCtxPushCurrent gives CUDA_ERROR_INVALID_CONTEXT if opengl viewport changed?
    CUDA_WARN(cuCtxPushCurrent(ctx));
    CUDA_WARN(cuStreamSynchronize(res[plane].stream));
    // FIXME: need a correct context. But why we have to push context even though map/unmap are called in the same thread
    // Because the decoder switch the context in another thread so we have to switch the context back?
    // to workaround the context issue, we must pop the context that valid in map() and push it here
    CUDA_ENSURE(cuGraphicsUnmapResources(1, &res[plane].cuRes, 0), false);
    CUDA_ENSURE(cuCtxPopCurrent(&ctx), false);
#endif //WORKAROUND_UNMAP_CONTEXT_SWITCH
    return true;
}

bool GLInteropResource::ensureResource(int h, int ch, GLuint tex, int plane)
{
    Q_ASSERT(plane < 2 && "plane number must be 0 or 1 for NV12");
    TexRes &r = res[plane];
    if (r.texture == tex && r.h == h && r.H == ch && r.cuRes)
        return true;
    if (!ctx) {
        // TODO: how to use pop/push decoder's context without the context in opengl context
        CUDA_ENSURE(cuCtxCreate(&ctx, CU_CTX_SCHED_BLOCKING_SYNC, dev), false);
#if USE_STREAM
        CUDA_WARN(cuStreamCreate(&res[0].stream, CU_STREAM_DEFAULT));
        CUDA_WARN(cuStreamCreate(&res[1].stream, CU_STREAM_DEFAULT));
#endif //USE_STREAM
        qDebug("cuda contex on gl thread: %p", ctx);
        CUDA_ENSURE(cuCtxPopCurrent(&ctx), false); // TODO: why cuMemcpy2D need this
    }
    if (r.cuRes) {
        CUDA_ENSURE(cuGraphicsUnregisterResource(r.cuRes), false);
        r.cuRes = NULL;
    }
    CUDA_ENSURE(cuGraphicsGLRegisterImage(&r.cuRes, tex, GL_TEXTURE_2D, CU_GRAPHICS_REGISTER_FLAGS_NONE), false);
    r.texture = tex;
    r.h = h;
    r.H = ch;
    return true;
}
} //namespace cuda
} //namespace QtAV
#endif //QTAV_HAVE(CUDA_GL)
