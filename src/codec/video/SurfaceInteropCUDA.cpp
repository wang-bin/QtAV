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
#define DX_LOG_COMPONENT "CUDA2"
#include "utils/DirectXHelper.h"
#include "helper_cuda.h"

#define WORKAROUND_UNMAP_CONTEXT_SWITCH 1

namespace QtAV {
namespace cuda {

InteropResource::InteropResource(CUdevice d, CUvideodecoder *decoder, CUvideoctxlock *declock)
    : cuda_api()
    , width(0)
    , height(0)
    , dev(d)
    , ctx(NULL)
    , dec(decoder)
    , lock(declock)
{
    memset(res, 0, sizeof(res));
}

InteropResource::~InteropResource()
{
    if (!dec || !*dec)
        return;
    //CUDA_ENSURE(cuCtxPushCurrent(ctx));
    if (res[0].cuRes)
        cuGraphicsUnregisterResource(res[0].cuRes);
    if (res[1].cuRes)
        cuGraphicsUnregisterResource(res[1].cuRes);
}

void SurfaceInteropCUDA::setSurface(int picIndex, CUVIDPROCPARAMS param, int frame_w, int frame_h)
{
    m_index = picIndex;
    m_param = param;
    frame_width = frame_w;
    frame_height = frame_h;
}

void* SurfaceInteropCUDA::map(SurfaceType type, const VideoFormat &fmt, void *handle, int plane)
{
    if (!handle)
        return NULL;

    if (m_index < 0)
        return 0;
    if (type == GLTextureSurface) {
        if (m_resource->map(m_index, m_param, *((GLuint*)handle), frame_width, frame_height, plane))
            return handle;
    } else if (type == HostMemorySurface) {
        return NULL;//mapToHost(fmt, handle, plane);
    }
    return NULL;
}

void SurfaceInteropCUDA::unmap(void *handle)
{
    m_resource->unmap(*((GLuint*)handle));
}

GLInteropResource::GLInteropResource(CUdevice d, CUvideodecoder *decoder, CUvideoctxlock *lk)
    : InteropResource(d, decoder, lk)
{}

bool GLInteropResource::map(int picIndex, const CUVIDPROCPARAMS &param, GLuint tex, int frame_w, int frame_h, int plane)
{
    // TODO: check dec/lock before use
    if (!dec || !*dec || !lock || !*lock)
        return false;

    AutoCtxLock locker((cuda_api*)this, *lock);
    Q_UNUSED(locker);
    if (!ensureResource(frame_w, frame_h, tex, plane)) // TODO surface size instead of frame size because we copy the device data
        return false;
    //CUDA_ENSURE(cuCtxPushCurrent(ctx), false);
    CUdeviceptr devptr;
    unsigned int pitch;

    CUDA_ENSURE(cuvidMapVideoFrame(*dec, picIndex, &devptr, &pitch, const_cast<CUVIDPROCPARAMS*>(&param)), false);
    class AutoUnmapper {
        cuda_api *api;
        CUvideodecoder dec;
        CUdeviceptr devptr;
    public:
        AutoUnmapper(cuda_api *a, CUvideodecoder d, CUdeviceptr p) : api(a), dec(d), devptr(p) {}
        ~AutoUnmapper() {
            api->cuvidUnmapVideoFrame(dec, devptr);
        }
    };
    AutoUnmapper unmapper(this, *dec, devptr);
    Q_UNUSED(unmapper);
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
    cu2d.dstXInBytes = 0;
    cu2d.dstY = 0;
    // the whole size or copy size?
    cu2d.WidthInBytes = pitch;
    cu2d.Height = frame_h;
    if (plane == 1) {
        cu2d.srcXInBytes = 0;// why not pitch*frame_h??
        cu2d.srcY = frame_h;
        cu2d.Height /= 2;
    }
    CUDA_ENSURE(cuMemcpy2DAsync(&cu2d, 0), false);
    CUDA_ENSURE(cuCtxSynchronize(), false);
    //TODO: delay cuCtxSynchronize && unmap. do it in unmap(tex)?
    // map to an already mapped resource will crash. sometimes I can not unmap the resource in unmap(tex) because if context switch error
    // so I simply unmap the resource here
    if (WORKAROUND_UNMAP_CONTEXT_SWITCH)
        CUDA_ENSURE(cuGraphicsUnmapResources(1, &res[plane].cuRes, 0), false);

    // call it at last. current context will be used by other cuda calls (unmap() for example)
    CUDA_ENSURE(cuCtxPopCurrent(&ctx), false);
    return true;
}

bool GLInteropResource::unmap(GLuint tex)
{
#if !WORKAROUND_UNMAP_CONTEXT_SWITCH
    int plane = -1;
    if (res[0].texture == tex)
        plane = 0;
    else if (res[1].texture == tex)
        plane = 1;
    else
        return false;
    // FIXME: why cuCtxPushCurrent gives CUDA_ERROR_INVALID_CONTEXT if opengl viewport changed?
    CUDA_ENSURE(cuCtxPushCurrent(ctx), false);
    // FIXME: need a correct context. But why we have to push context even though map/unmap are called in the same thread
    // Because the decoder switch the context in another thread so we have to switch the context back?
    // to workaround the context issue, we must pop the context that valid in map() and push it here
    CUDA_ENSURE(cuGraphicsUnmapResources(1, &res[plane].cuRes, 0), false);
    CUDA_ENSURE(cuCtxPopCurrent(&ctx), false);
#endif //WORKAROUND_UNMAP_CONTEXT_SWITCH
    return true;
}

bool GLInteropResource::ensureResource(int w, int h, GLuint tex, int plane)
{
    Q_ASSERT(plane < 2 && "plane number must be 0 or 1 for NV12");
    TexRes &r = res[plane];
    if (r.texture == tex && r.width == w && r.height == h && r.cuRes)
        return true;
    if (!ctx) {
        // TODO: how to use pop/push decoder's context without the context in opengl context
        CUDA_ENSURE(cuCtxCreate(&ctx, CU_CTX_SCHED_BLOCKING_SYNC, dev), false);
        CUDA_ENSURE(cuCtxPopCurrent(&ctx), false); // TODO: why cuMemcpy2D need this
        qDebug("cuda contex on gl thread: %p", ctx);
    }
    if (r.cuRes) {
        CUDA_ENSURE(cuGraphicsUnregisterResource(r.cuRes), false);
        r.cuRes = NULL;
    }
    // TODO: CUDA_ENSURE
    CUDA_ENSURE(cuGraphicsGLRegisterImage(&r.cuRes, tex, GL_TEXTURE_2D, CU_GRAPHICS_REGISTER_FLAGS_NONE), false);
    qDebug("cuGraphicsGLRegisterImage done");
    r.texture = tex;
    r.width = w;
    r.height = h;
    return true;
}

} //namespace cuda
} //namespace QtAV
