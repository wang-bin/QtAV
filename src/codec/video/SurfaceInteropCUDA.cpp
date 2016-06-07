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

#include "SurfaceInteropCUDA.h"
#include "QtAV/VideoFrame.h"
#include "utils/Logger.h"
#include "helper_cuda.h"

#define WORKAROUND_UNMAP_CONTEXT_SWITCH 1
#define USE_STREAM 1

namespace QtAV {
namespace cuda {

InteropResource::InteropResource()
    : cuda_api()
    , dev(0)
    , ctx(0)
    , dec(0)
    , lock(0)
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
    if (!share_ctx && ctx)
        CUDA_ENSURE(cuCtxDestroy(ctx));
}

void* InteropResource::mapToHost(const VideoFormat &format, void *handle, int picIndex, const CUVIDPROCPARAMS &param, int width, int height, int coded_height)
{
    AutoCtxLock locker((cuda_api*)this, lock);
    Q_UNUSED(locker);
    CUdeviceptr devptr;
    unsigned int pitch;

    CUDA_ENSURE(cuvidMapVideoFrame(dec, picIndex, &devptr, &pitch, const_cast<CUVIDPROCPARAMS*>(&param)), NULL);
    CUVIDAutoUnmapper unmapper(this, dec, devptr);
    Q_UNUSED(unmapper);
    uchar* host_data = NULL;
    const unsigned int host_size = pitch*coded_height*3/2;
    CUDA_ENSURE(cuMemAllocHost((void**)&host_data, host_size), NULL);
    // copy to the memory not allocated by cuda is possible but much slower
    CUDA_ENSURE(cuMemcpyDtoH(host_data, devptr, host_size), NULL);

    VideoFrame frame(width, height, VideoFormat::Format_NV12);
    uchar *planes[] = {
        host_data,
        host_data + pitch * coded_height
    };
    frame.setBits(planes);
    int pitches[] = { (int)pitch, (int)pitch };
    frame.setBytesPerLine(pitches);

    VideoFrame *f = reinterpret_cast<VideoFrame*>(handle);
    frame.setTimestamp(f->timestamp());
    frame.setDisplayAspectRatio(f->displayAspectRatio());
    if (format == frame.format())
        *f = frame.clone();
    else
        *f = frame.to(format);

    CUDA_ENSURE(cuMemFreeHost(host_data), f);
    return f;
}

#ifndef QT_NO_OPENGL
HostInteropResource::HostInteropResource()
    : InteropResource()
{
    memset(&host_mem, 0, sizeof(host_mem));
    host_mem.index = -1;
}

HostInteropResource::~HostInteropResource()
{
    if (ctx) { //cuMemFreeHost need the context of mem allocated, it's shared context, or own context
        CUDA_WARN(cuCtxPushCurrent(ctx));
    }
    if (host_mem.data) { //FIXME: CUDA_ERROR_INVALID_VALUE
        CUDA_ENSURE(cuMemFreeHost(host_mem.data));
        host_mem.data = NULL;
    }
    if (ctx) {
        CUDA_WARN(cuCtxPopCurrent(NULL));
    }
}

bool HostInteropResource::map(int picIndex, const CUVIDPROCPARAMS &param, GLuint tex, int w, int h, int H, int plane)
{
    Q_UNUSED(w);
    if (host_mem.index != picIndex || !host_mem.data) {
        AutoCtxLock locker((cuda_api*)this, lock);
        Q_UNUSED(locker);

        CUdeviceptr devptr;
        unsigned int pitch;
        //qDebug("index: %d=>%d, plane: %d", host_mem.index, picIndex, plane);
        CUDA_ENSURE(cuvidMapVideoFrame(dec, picIndex, &devptr, &pitch, const_cast<CUVIDPROCPARAMS*>(&param)), false);
        CUVIDAutoUnmapper unmapper(this, dec, devptr);
        Q_UNUSED(unmapper);
        if (!ensureResource(pitch, H)) //copy height is coded height
            return false;
        // the same thread (context) as cuMemAllocHost, so no ccontext switch is needed
        CUDA_ENSURE(cuMemcpyDtoH(host_mem.data, devptr, pitch*H*3/2), NULL);
        host_mem.index = picIndex;
    }
    // map to texture
    //qDebug("map plane %d @%d", plane, picIndex);
    GLint iformat[2];
    GLenum format[2], dtype[2];
    OpenGLHelper::videoFormatToGL(VideoFormat::Format_NV12, iformat, format, dtype);
    DYGL(glBindTexture(GL_TEXTURE_2D, tex));
    const int chroma = plane != 0;
    // chroma pitch for gl is 1/2 (gl_rg)
    // texture height is not coded height!
    DYGL(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, host_mem.pitch>>chroma, h>>chroma, format[plane], dtype[plane], host_mem.data + chroma*host_mem.pitch*host_mem.height));
    //DYGL(glTexImage2D(GL_TEXTURE_2D, 0, iformat[plane], host_mem.pitch>>chroma, h>>chroma, 0, format[plane], dtype[plane], host_mem.data + chroma*host_mem.pitch*host_mem.height));
    return true;
}

bool HostInteropResource::unmap(GLuint)
{
    return true;
}

bool HostInteropResource::ensureResource(int pitch, int height)
{
    if (host_mem.data && host_mem.pitch == pitch && host_mem.height == height)
        return true;
    if (host_mem.data) {
        CUDA_ENSURE(cuMemFreeHost(host_mem.data), false);
        host_mem.data = NULL;
    }
    qDebug("allocate cuda host mem. %dx%d=>%dx%d", host_mem.pitch, host_mem.height, pitch, height);
    host_mem.pitch = pitch;
    host_mem.height = height;
    if (!ctx) {
        CUDA_ENSURE(cuCtxCreate(&ctx, CU_CTX_SCHED_BLOCKING_SYNC, dev), false);
        CUDA_WARN(cuCtxPopCurrent(&ctx));
        share_ctx = false;
    }
    if (!share_ctx) // cuMemFreeHost will be called in dtor which is not the current thread.
        CUDA_WARN(cuCtxPushCurrent(ctx));
    // NV12
    CUDA_ENSURE(cuMemAllocHost((void**)&host_mem.data, pitch*height*3/2), NULL);
    if (!share_ctx)
        CUDA_WARN(cuCtxPopCurrent(NULL)); //can be null or &ctx
    return true;
}
#endif //QT_NO_OPENGL

void SurfaceInteropCUDA::setSurface(int picIndex, CUVIDPROCPARAMS param, int width, int height, int surface_height)
{
    m_index = picIndex;
    m_param = param;
    w = width;
    h = height;
    H = surface_height;
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
#ifndef QT_NO_OPENGL
        // FIXME: to strong ref may delay the delete and cuda resource maybe already destoryed after strong ref is finished
        if (m_resource.toStrongRef()->map(m_index, m_param, *((GLuint*)handle), w, h, H, plane))
            return handle;
#endif //QT_NO_OPENGL
    } else if (type == HostMemorySurface) {
        return m_resource.toStrongRef()->mapToHost(fmt, handle, m_index, m_param, w, h, H);
    }
    return NULL;
}

void SurfaceInteropCUDA::unmap(void *handle)
{
    if (m_resource.isNull())
        return;
#ifndef QT_NO_OPENGL
    // FIXME: to strong ref may delay the delete and cuda resource maybe already destoryed after strong ref is finished
    m_resource.toStrongRef()->unmap(*((GLuint*)handle));
#endif
}
} //namespace cuda
} //namespace QtAV

#if QTAV_HAVE(CUDA_EGL)
#ifdef QT_OPENGL_ES_2_ANGLE_STATIC
#define CAPI_LINK_EGL
#else
#define EGL_CAPI_NS
#endif //QT_OPENGL_ES_2_ANGLE_STATIC
#include "capi/egl_api.h"
#include <EGL/eglext.h> //include after egl_capi.h to match types
#define DX_LOG_COMPONENT "CUDA.D3D"
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

EGLInteropResource::EGLInteropResource()
    : InteropResource()
    , egl(new EGL())
    , dll9(NULL)
    , d3d9(NULL)
    , device9(NULL)
    , texture9(NULL)
    , surface9(NULL)
    , texture9_nv12(NULL)
    , surface9_nv12(NULL)
    , query9(NULL)
{
    ctx = NULL; //need a context created with d3d (TODO: check it?)
    share_ctx = false;
}

EGLInteropResource::~EGLInteropResource()
{
    releaseEGL();
    if (egl) {
        delete egl;
        egl = NULL;
    }
    SafeRelease(&query9);
    SafeRelease(&surface9_nv12);
    SafeRelease(&texture9_nv12);
    SafeRelease(&surface9);
    SafeRelease(&texture9);
    SafeRelease(&device9);
    SafeRelease(&d3d9);
    if (dll9)
        FreeLibrary(dll9);
}

bool EGLInteropResource::ensureD3DDevice()
{
    if (device9)
        return true;
    if (!dll9)
        dll9 = LoadLibrary(TEXT("D3D9.DLL"));
    if (!dll9) {
        qWarning("cuda::EGLInteropResource cannot load d3d9.dll");
        return false;
    }
    D3DADAPTER_IDENTIFIER9 ai9;
    ZeroMemory(&ai9, sizeof(ai9));
    device9 = DXHelper::CreateDevice9Ex(dll9, (IDirect3D9Ex**)(&d3d9), &ai9);
    if (!device9) {
        qWarning("Failed to create d3d9 device ex, fallback to d3d9 device");
        device9 = DXHelper::CreateDevice9(dll9, &d3d9, &ai9);
    }
    if (!device9)
        return false;
    qDebug() << QString().sprintf("CUDA.D3D9 (%.*s, vendor %lu, device %lu, revision %lu)",
                                    sizeof(ai9.Description), ai9.Description,
                                    ai9.VendorId, ai9.DeviceId, ai9.Revision);

    // move to ensureResouce
    DX_ENSURE(device9->CreateQuery(D3DQUERYTYPE_EVENT, &query9), false);
    query9->Issue(D3DISSUE_END);
    return !!device9;
}

void EGLInteropResource::releaseEGL() {
    if (egl->surface != EGL_NO_SURFACE) {
        eglReleaseTexImage(egl->dpy, egl->surface, EGL_BACK_BUFFER);
        eglDestroySurface(egl->dpy, egl->surface);
        egl->surface = EGL_NO_SURFACE;
    }
}

bool EGLInteropResource::ensureResource(int w, int h, int W, int H, GLuint tex)
{
    TexRes &r = res[0];// 1 NV12 texture
    if (ensureD3D9CUDA(w, h, W, H) && ensureD3D9EGL(w, h)) {
        r.texture = tex;
        r.w = w;
        r.h = h;
        r.W = W;
        r.H = H;
        return true;
    }
    releaseEGL();
    //releaseDX();
    SafeRelease(&query9);
    SafeRelease(&surface9);
    SafeRelease(&texture9);
    SafeRelease(&surface9_nv12);
    SafeRelease(&texture9_nv12);
    return false;
}

bool EGLInteropResource::ensureD3D9CUDA(int w, int h, int W, int H)
{
    TexRes &r = res[0];// 1 NV12 texture
    if (r.w == w && r.h == h && r.W == W && r.H == H && r.cuRes)
        return true;
    if (share_ctx) {
        share_ctx = false;
        ctx = NULL;
    }
    if (!ctx) {
        // TODO: how to use pop/push decoder's context without the context in opengl context
        if (!ensureD3DDevice())
            return false;
        // CUdevice is different from decoder's
        CUDA_ENSURE(cuD3D9CtxCreate(&ctx, &dev, CU_CTX_SCHED_BLOCKING_SYNC, device9), false);
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

    // create d3d resource for interop
    if (!surface9_nv12) {
        // TODO: need pitch from cuvid to ensure cuMemcpy2D can copy the whole pitch
        DX_ENSURE(device9->CreateTexture(W
                                         //, H
                                         , H*3/2
                                         , 1
                                         , D3DUSAGE_DYNAMIC //D3DUSAGE_DYNAMIC is lockable // 0 is from NV example. cudaD3D9.h says The primary rendertarget may not be registered with CUDA. So can not be D3DUSAGE_RENDERTARGET?
                                         //, D3DUSAGE_RENDERTARGET
                                         , D3DFMT_L8
                                         //, (D3DFORMAT)MAKEFOURCC('N','V','1','2') // can not create nv12. use 2 textures L8+A8L8?
                                         , D3DPOOL_DEFAULT // must be D3DPOOL_DEFAULT for cuda?
                                         , &texture9_nv12
                                         , NULL) // - Resources allocated as shared may not be registered with CUDA.
                  , false);
        DX_ENSURE(device9->CreateOffscreenPlainSurface(W, H, (D3DFORMAT)MAKEFOURCC('N','V','1','2'), D3DPOOL_DEFAULT, &surface9_nv12, NULL), false); //TODO: createrendertarget
    }

    // TODO: cudaD3D9.h says NV12 is not supported
    // CUDA_ERROR_INVALID_HANDLE if register D3D9 surface
    // TODO: why flag CU_GRAPHICS_REGISTER_FLAGS_WRITE_DISCARD is invalid while it's fine for opengl
    CUDA_ENSURE(cuGraphicsD3D9RegisterResource(&r.cuRes, texture9_nv12, CU_GRAPHICS_REGISTER_FLAGS_NONE), false);
    return true;
}

bool EGLInteropResource::ensureD3D9EGL(int w, int h) {
    if (egl->surface && res[0].w == w && res[0].h == h)
        return true;
    releaseEGL();
    egl->dpy = eglGetCurrentDisplay();
    qDebug("EGL version: %s, client api: %s", eglQueryString(egl->dpy, EGL_VERSION), eglQueryString(egl->dpy, EGL_CLIENT_APIS));
    EGLint cfg_attribs[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8, //
        EGL_BIND_TO_TEXTURE_RGBA, EGL_TRUE, //remove?
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_NONE
    };
    EGLint nb_cfgs;
    EGLConfig egl_cfg;
    if (!eglChooseConfig(egl->dpy, cfg_attribs, &egl_cfg, 1, &nb_cfgs)) {
        qWarning("Failed to create EGL configuration");
        return false;
    }
    // check extensions
    QList<QByteArray> extensions = QByteArray(eglQueryString(egl->dpy, EGL_EXTENSIONS)).split(' ');
    // ANGLE_d3d_share_handle_client_buffer will be used if possible
    const bool kEGL_ANGLE_d3d_share_handle_client_buffer = extensions.contains("EGL_ANGLE_d3d_share_handle_client_buffer");
    const bool kEGL_ANGLE_query_surface_pointer = extensions.contains("EGL_ANGLE_query_surface_pointer");
    if (!kEGL_ANGLE_d3d_share_handle_client_buffer && !kEGL_ANGLE_query_surface_pointer) {
        qWarning("EGL extension 'kEGL_ANGLE_query_surface_pointer' or 'ANGLE_d3d_share_handle_client_buffer' is required!");
        return false;
    }
    GLint has_alpha = 1; //QOpenGLContext::currentContext()->format().hasAlpha()
    eglGetConfigAttrib(egl->dpy, egl_cfg, EGL_BIND_TO_TEXTURE_RGBA, &has_alpha); //EGL_ALPHA_SIZE
    qDebug("choose egl display:%p config: %p/%d, has alpha: %d", egl->dpy, egl_cfg, nb_cfgs, has_alpha);
    EGLint attribs[] = {
        EGL_WIDTH, w,
        EGL_HEIGHT, h,
        EGL_TEXTURE_FORMAT, has_alpha ? EGL_TEXTURE_RGBA : EGL_TEXTURE_RGB,
        EGL_TEXTURE_TARGET, EGL_TEXTURE_2D,
        EGL_NONE
    };

    HANDLE share_handle = NULL;
    if (!kEGL_ANGLE_d3d_share_handle_client_buffer && kEGL_ANGLE_query_surface_pointer) {
        EGL_ENSURE((egl->surface = eglCreatePbufferSurface(egl->dpy, egl_cfg, attribs)) != EGL_NO_SURFACE, false);
        qDebug("pbuffer surface: %p", egl->surface);
        PFNEGLQUERYSURFACEPOINTERANGLEPROC eglQuerySurfacePointerANGLE = reinterpret_cast<PFNEGLQUERYSURFACEPOINTERANGLEPROC>(eglGetProcAddress("eglQuerySurfacePointerANGLE"));
        if (!eglQuerySurfacePointerANGLE) {
            qWarning("EGL_ANGLE_query_surface_pointer is not supported");
            return false;
        }
        EGL_ENSURE(eglQuerySurfacePointerANGLE(egl->dpy, egl->surface, EGL_D3D_TEXTURE_2D_SHARE_HANDLE_ANGLE, &share_handle), false);
    }

    SafeRelease(&surface9);
    SafeRelease(&texture9);
    // _A8 for a yuv plane
    /*
     * d3d resource share requires windows >= vista: https://msdn.microsoft.com/en-us/library/windows/desktop/bb219800(v=vs.85).aspx
     * from extension files:
     * d3d9: level must be 1, dimensions must match EGL surface's
     * d3d9ex or d3d10:
     */
    DX_ENSURE(device9->CreateTexture(w, h, 1,
                                        D3DUSAGE_RENDERTARGET,
                                        has_alpha ? D3DFMT_A8R8G8B8 : D3DFMT_X8R8G8B8,
                                        D3DPOOL_DEFAULT,
                                        &texture9,
                                        &share_handle) , false);
    DX_ENSURE(texture9->GetSurfaceLevel(0, &surface9), false);

    if (kEGL_ANGLE_d3d_share_handle_client_buffer) {
        // requires extension EGL_ANGLE_d3d_share_handle_client_buffer
        // egl surface size must match d3d texture's
        // d3d9ex or d3d10 is required
        EGL_ENSURE((egl->surface = eglCreatePbufferFromClientBuffer(egl->dpy, EGL_D3D_TEXTURE_2D_SHARE_HANDLE_ANGLE, share_handle, egl_cfg, attribs)), false);
        qDebug("pbuffer surface from client buffer: %p", egl->surface);
    }
    return true;
}

bool EGLInteropResource::map(int picIndex, const CUVIDPROCPARAMS &param, GLuint tex, int w, int h, int H, int plane)
{
    // plane is always 0 because frame is rgb
    AutoCtxLock locker((cuda_api*)this, lock);
    Q_UNUSED(locker);
    if (!ensureResource(w, h, param.Reserved[0], H, tex)) // TODO surface size instead of frame size because we copy the device data
        return false;
    //CUDA_ENSURE(cuCtxPushCurrent(ctx), false);
    CUdeviceptr devptr;
    unsigned int pitch;

    CUDA_ENSURE(cuvidMapVideoFrame(dec, picIndex, &devptr, &pitch, const_cast<CUVIDPROCPARAMS*>(&param)), false);
    CUVIDAutoUnmapper unmapper(this, dec, devptr);
    Q_UNUSED(unmapper);
    // TODO: why can not use res[plane].stream? CUDA_ERROR_INVALID_HANDLE
    CUDA_ENSURE(cuGraphicsMapResources(1, &res[plane].cuRes, 0), false);
    CUarray array;
    CUDA_ENSURE(cuGraphicsSubResourceGetMappedArray(&array, res[plane].cuRes, 0, 0), false);
    CUDA_ENSURE(cuGraphicsUnmapResources(1, &res[plane].cuRes, 0), false); // mapped array still accessible!

    CUDA_MEMCPY2D cu2d;
    memset(&cu2d, 0, sizeof(cu2d));
    // Y plane
    cu2d.srcDevice = devptr;
    cu2d.srcMemoryType = CU_MEMORYTYPE_DEVICE;
    cu2d.srcPitch = pitch;
    cu2d.dstArray = array;
    cu2d.dstMemoryType = CU_MEMORYTYPE_ARRAY;
    cu2d.dstPitch = pitch;
    // the whole size or copy size?
    cu2d.WidthInBytes = res[plane].W; // the same value as texture9_nv12
    cu2d.Height = H*3/2;
    if (res[plane].stream)
        CUDA_ENSURE(cuMemcpy2DAsync(&cu2d, res[plane].stream), false);
    else
        CUDA_ENSURE(cuMemcpy2D(&cu2d), false);
    //TODO: delay cuCtxSynchronize && unmap. do it in unmap(tex)?
    // map to an already mapped resource will crash. sometimes I can not unmap the resource in unmap(tex) because if context switch error
    // so I simply unmap the resource here
    if (WORKAROUND_UNMAP_CONTEXT_SWITCH) {
        if (res[plane].stream) {
            //CUDA_WARN(cuCtxSynchronize(), false); //wait too long time? use cuStreamQuery?
            CUDA_WARN(cuStreamSynchronize(res[plane].stream)); //slower than CtxSynchronize
        }
        /*
         * This function provides the synchronization guarantee that any CUDA work issued
         * in \p stream before ::cuGraphicsUnmapResources() will complete before any
         * subsequently issued graphics work begins.
         * The graphics API from which \p resources were registered
         * should not access any resources while they are mapped by CUDA. If an
         * application does so, the results are undefined.
         */
//        CUDA_ENSURE(cuGraphicsUnmapResources(1, &res[plane].cuRes, 0), false);
    }
    D3DLOCKED_RECT rect_src, rect_dst;
    DX_ENSURE(texture9_nv12->LockRect(0, &rect_src, NULL, D3DLOCK_READONLY), false);
    DX_ENSURE(surface9_nv12->LockRect(&rect_dst, NULL, D3DLOCK_DISCARD), false);
    memcpy(rect_dst.pBits, rect_src.pBits, res[plane].W*H*3/2); // exactly w and h
    DX_ENSURE(surface9_nv12->UnlockRect(), false);
    DX_ENSURE(texture9_nv12->UnlockRect(0), false);
#if 0
    //IDirect3DSurface9 *raw_surface = NULL;
    //DX_ENSURE(texture9_nv12->GetSurfaceLevel(0, &raw_surface), false);
    const RECT src = { 0, 0, (~0-1)&w, (~0-1)&(h*3/2)};
    DX_ENSURE(device9->StretchRect(raw_surface, &src, surface9_nv12, NULL, D3DTEXF_NONE), false);
#endif
    if (!map(surface9_nv12, tex, w, h, H))
        return false;
    return true;
}

bool EGLInteropResource::map(IDirect3DSurface9* surface, GLuint tex, int w, int h, int H)
{
    Q_UNUSED(H);
    D3DSURFACE_DESC dxvaDesc;
    surface->GetDesc(&dxvaDesc);
    const RECT src = { 0, 0, (~0-1)&w, (~0-1)&h}; //StretchRect does not supports odd values
    DX_ENSURE(device9->StretchRect(surface, &src, surface9, NULL, D3DTEXF_NONE), false);
    if (query9) {
        // Flush the draw command now. Ideally, this should be done immediately before the draw call that uses the texture. Flush it once here though.
        query9->Issue(D3DISSUE_END);
        // ensure data is copied to egl surface. Solution and comment is from chromium
        // The DXVA decoder has its own device which it uses for decoding. ANGLE has its own device which we don't have access to.
        // The above code attempts to copy the decoded picture into a surface which is owned by ANGLE.
        // As there are multiple devices involved in this, the StretchRect call above is not synchronous.
        // We attempt to flush the batched operations to ensure that the picture is copied to the surface owned by ANGLE.
        // We need to do this in a loop and call flush multiple times.
        // We have seen the GetData call for flushing the command buffer fail to return success occassionally on multi core machines, leading to an infinite loop.
        // Workaround is to have an upper limit of 10 on the number of iterations to wait for the Flush to finish.
        int k = 0;
        // skip at decoder.close()
        while (/*!skip_dx.load() && */(query9->GetData(NULL, 0, D3DGETDATA_FLUSH) == FALSE) && ++k < 10) {
            Sleep(1);
        }
    }
    DYGL(glBindTexture(GL_TEXTURE_2D, tex));
    eglBindTexImage(egl->dpy, egl->surface, EGL_BACK_BUFFER);
    DYGL(glBindTexture(GL_TEXTURE_2D, 0));
    return true;
}

} //namespace cuda
} //namespace QtAV
#endif //QTAV_HAVE(CUDA_EGL)
#if QTAV_HAVE(CUDA_GL)
namespace QtAV {
namespace cuda {
//TODO: cuGLMapBufferObject: get cudeviceptr from pbo, then memcpy2d
bool GLInteropResource::map(int picIndex, const CUVIDPROCPARAMS &param, GLuint tex, int w, int h, int H, int plane)
{
    AutoCtxLock locker((cuda_api*)this, lock);
    Q_UNUSED(locker);
    if (!ensureResource(w, h, H, tex, plane)) // TODO surface size instead of frame size because we copy the device data
        return false;
    //CUDA_ENSURE(cuCtxPushCurrent(ctx), false);
    CUdeviceptr devptr;
    unsigned int pitch;

    CUDA_ENSURE(cuvidMapVideoFrame(dec, picIndex, &devptr, &pitch, const_cast<CUVIDPROCPARAMS*>(&param)), false);
    CUVIDAutoUnmapper unmapper(this, dec, devptr);
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
        cu2d.srcXInBytes = 0;// +srcY*srcPitch + srcXInBytes
        cu2d.srcY = H; // skip the padding height
        cu2d.Height /= 2;
    }
    if (res[plane].stream)
        CUDA_ENSURE(cuMemcpy2DAsync(&cu2d, res[plane].stream), false);
    else
        CUDA_ENSURE(cuMemcpy2D(&cu2d), false);
    //TODO: delay cuCtxSynchronize && unmap. do it in unmap(tex)?
    // map to an already mapped resource will crash. sometimes I can not unmap the resource in unmap(tex) because if context switch error
    // so I simply unmap the resource here
    if (WORKAROUND_UNMAP_CONTEXT_SWITCH) {
        if (res[plane].stream) {
            //CUDA_WARN(cuCtxSynchronize(), false); //wait too long time? use cuStreamQuery?
            CUDA_WARN(cuStreamSynchronize(res[plane].stream)); //slower than CtxSynchronize
        }
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
        CUDA_ENSURE(cuCtxPopCurrent(&ctx), false); // not required
    }
    return true;
}

bool GLInteropResource::unmap(GLuint tex)
{
    Q_UNUSED(tex);
    if (WORKAROUND_UNMAP_CONTEXT_SWITCH)
        return true;
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
    return true;
}

bool GLInteropResource::ensureResource(int w, int h, int H, GLuint tex, int plane)
{
    Q_ASSERT(plane < 2 && "plane number must be 0 or 1 for NV12");
    TexRes &r = res[plane];
    if (r.texture == tex && r.w == w && r.h == h && r.H == H && r.cuRes)
        return true;
    if (!ctx) {
        // TODO: how to use pop/push decoder's context without the context in opengl context
        CUDA_ENSURE(cuCtxCreate(&ctx, CU_CTX_SCHED_BLOCKING_SYNC, dev), false);
        if (USE_STREAM) {
            CUDA_WARN(cuStreamCreate(&res[0].stream, CU_STREAM_DEFAULT));
            CUDA_WARN(cuStreamCreate(&res[1].stream, CU_STREAM_DEFAULT));
        }
        qDebug("cuda contex on gl thread: %p", ctx);
        CUDA_ENSURE(cuCtxPopCurrent(&ctx), false); // TODO: why cuMemcpy2D need this
    }
    if (r.cuRes) {
        CUDA_ENSURE(cuGraphicsUnregisterResource(r.cuRes), false);
        r.cuRes = NULL;
    }
    // CU_GRAPHICS_REGISTER_FLAGS_WRITE_DISCARD works too for opengl, but not d3d
    CUDA_ENSURE(cuGraphicsGLRegisterImage(&r.cuRes, tex, GL_TEXTURE_2D, CU_GRAPHICS_REGISTER_FLAGS_NONE), false);
    r.texture = tex;
    r.w = w;
    r.h = h;
    r.H = H;
    return true;
}
} //namespace cuda
} //namespace QtAV
#endif //QTAV_HAVE(CUDA_GL)
