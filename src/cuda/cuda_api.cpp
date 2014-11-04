/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>

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

#include <assert.h>
#include <QtCore/QLibrary>

#include "helper_cuda.h"
#include "cuda_api.h"
#include "QtAV/QtAV_Global.h"
#include "utils/Logger.h"
/*
#if !NV_CONFIG(DLLAPI_CUDA) && !defined(CUDA_LINK)
#define DEFINE_API() ...
#else
#define DEFINE_API() ... ::api()
#endif
*/
#if 0
#define DECLARE_API_CONTEXT(API, ..) \
    typedef API##_t (...); \
    API##_t* API##_fp; \
    void resolve_##API () { API##_fp = ... } \
    resolve_t *resolve_##API##_fp = resolve_##API;
// in ctor

    //memset()
    for (int i = 0; i < sizeof(api_t)/sizeof(void*); ++i) {
        (resolve_t*)(char*)(&api)+(2*i+1)*sizeof(void*))();
    }
#endif


class cuda_api::context {
public:
    context()
        : loaded(true) {
#if !NV_CONFIG(DLLAPI_CUDA) && !defined(CUDA_LINK)
        loaded = false;
        memset(&api, 0, sizeof(api));
        cuda_dll.setFileName("cuda");
        if (!cuda_dll.isLoaded())
            cuda_dll.load();
        if (!cuda_dll.isLoaded()) {
            cuda_dll.setFileName("nvcuda");
            cuda_dll.load();
        }
        if (!cuda_dll.isLoaded()) {
            qWarning("can not load cuda!");
            return;
        }
        cuvid_dll.setFileName("nvcuvid");
        cuvid_dll.load();
        if (!cuvid_dll.isLoaded()) {
            qWarning("can not load nvcuvid!");
            return;
        }
        loaded = true;
#endif // !NV_CONFIG(DLLAPI_CUDA) && !defined(CUDA_LINK)
    }
#if !NV_CONFIG(DLLAPI_CUDA) && !defined(CUDA_LINK)
    ~context() {
        loaded = false;
        cuvid_dll.unload();
        cuda_dll.unload();
    }

    QLibrary cuda_dll;
    QLibrary cuvid_dll;
    typedef struct {
        typedef CUresult CUDAAPI tcuInit(unsigned int);
        tcuInit* cuInit;
        typedef CUresult CUDAAPI tcuCtxCreate(CUcontext *, unsigned int, CUdevice);
        tcuCtxCreate* cuCtxCreate;
        typedef CUresult CUDAAPI tcuCtxDestroy(CUcontext);
        tcuCtxDestroy* cuCtxDestroy;
        typedef CUresult CUDAAPI tcuCtxPushCurrent(CUcontext);
        tcuCtxPushCurrent* cuCtxPushCurrent;
        typedef CUresult CUDAAPI tcuCtxPopCurrent(CUcontext *);
        tcuCtxPopCurrent* cuCtxPopCurrent;
        typedef CUresult CUDAAPI tcuMemAllocHost(void **pp, unsigned int bytesize);
        tcuMemAllocHost* cuMemAllocHost;
        typedef CUresult CUDAAPI tcuMemFreeHost(void *p);
        tcuMemFreeHost* cuMemFreeHost;
        typedef CUresult CUDAAPI tcuMemcpyDtoH(void *dstHost, CUdeviceptr srcDevice, unsigned int ByteCount );
        tcuMemcpyDtoH* cuMemcpyDtoH;
        typedef CUresult CUDAAPI tcuMemcpyDtoHAsync(void *dstHost, CUdeviceptr srcDevice, unsigned int ByteCount, CUstream hStream);
        tcuMemcpyDtoHAsync* cuMemcpyDtoHAsync;
        typedef CUresult CUDAAPI tcuStreamCreate(CUstream *phStream, unsigned int Flags);
        tcuStreamCreate* cuStreamCreate;
        typedef CUresult CUDAAPI tcuStreamDestroy(CUstream hStream);
        tcuStreamDestroy* cuStreamDestroy;
        typedef CUresult CUDAAPI tcuStreamQuery(CUstream hStream);
        tcuStreamQuery* cuStreamQuery;
        typedef CUresult CUDAAPI tcuDeviceGetCount(int *count);
        tcuDeviceGetCount* cuDeviceGetCount;
        typedef CUresult CUDAAPI tcuDriverGetVersion(int *driverVersion);
        tcuDriverGetVersion* cuDriverGetVersion;
        typedef CUresult CUDAAPI tcuDeviceGetName(char *name, int len, CUdevice dev);
        tcuDeviceGetName* cuDeviceGetName;
        typedef CUresult CUDAAPI tcuDeviceComputeCapability(int *major, int *minor, CUdevice dev);
        tcuDeviceComputeCapability* cuDeviceComputeCapability;
        typedef CUresult CUDAAPI tcuDeviceGetAttribute(int *pi, CUdevice_attribute attrib, CUdevice dev);
        tcuDeviceGetAttribute* cuDeviceGetAttribute;
        typedef CUresult CUDAAPI tcuvidCtxLockCreate(CUvideoctxlock *pLock, CUcontext ctx);
        tcuvidCtxLockCreate* cuvidCtxLockCreate;
        typedef CUresult CUDAAPI tcuvidCtxLockDestroy(CUvideoctxlock lck);
        tcuvidCtxLockDestroy* cuvidCtxLockDestroy;
        typedef CUresult CUDAAPI tcuvidCtxLock(CUvideoctxlock lck, unsigned int reserved_flags);
        tcuvidCtxLock* cuvidCtxLock;
        typedef CUresult CUDAAPI tcuvidCtxUnlock(CUvideoctxlock lck, unsigned int reserved_flags);
        tcuvidCtxUnlock* cuvidCtxUnlock;
        typedef CUresult CUDAAPI tcuCtxSynchronize();
        tcuCtxSynchronize* cuCtxSynchronize;
        typedef CUresult CUDAAPI tcuvidCreateVideoParser(CUvideoparser *pObj, CUVIDPARSERPARAMS *pParams);
        tcuvidCreateVideoParser* cuvidCreateVideoParser;
        typedef CUresult CUDAAPI tcuvidParseVideoData(CUvideoparser obj, CUVIDSOURCEDATAPACKET *pPacket);
        tcuvidParseVideoData* cuvidParseVideoData;
        typedef CUresult CUDAAPI tcuvidDestroyVideoParser(CUvideoparser obj);
        tcuvidDestroyVideoParser* cuvidDestroyVideoParser;
        typedef CUresult CUDAAPI tcuvidCreateDecoder(CUvideodecoder *phDecoder, CUVIDDECODECREATEINFO *pdci);
        tcuvidCreateDecoder* cuvidCreateDecoder;
        typedef CUresult CUDAAPI tcuvidDestroyDecoder(CUvideodecoder hDecoder);
        tcuvidDestroyDecoder* cuvidDestroyDecoder;
        typedef CUresult CUDAAPI tcuvidDecodePicture(CUvideodecoder hDecoder, CUVIDPICPARAMS *pPicParams);
        tcuvidDecodePicture* cuvidDecodePicture;
        typedef CUresult CUDAAPI tcuvidMapVideoFrame(CUvideodecoder hDecoder, int nPicIdx, unsigned int *pDevPtr, unsigned int *pPitch, CUVIDPROCPARAMS *pVPP);
        tcuvidMapVideoFrame* cuvidMapVideoFrame;
        typedef CUresult CUDAAPI tcuvidUnmapVideoFrame(CUvideodecoder hDecoder, unsigned int DevPtr);
        tcuvidUnmapVideoFrame* cuvidUnmapVideoFrame;
    } api_t;
    api_t api;
#endif // !NV_CONFIG(DLLAPI_CUDA) && !defined(CUDA_LINK)
    bool loaded;
};

cuda_api::cuda_api()
    : ctx(new context())
{
}

cuda_api::~cuda_api()
{
    delete ctx;
}

bool cuda_api::isLoaded() const
{
    return ctx->loaded;
}

#if !NV_CONFIG(DLLAPI_CUDA) && !defined(CUDA_LINK)
////////////////////////////////////////////////////
/// CUDA functions
////////////////////////////////////////////////////
CUresult cuda_api::cuInit(unsigned int Flags)
{
    if (!ctx->api.cuInit)
        ctx->api.cuInit = (context::api_t::tcuInit*)ctx->cuda_dll.resolve("cuInit");
    assert(ctx->api.cuInit);
    return ctx->api.cuInit(Flags);
}

CUresult cuda_api::cuCtxCreate(CUcontext *pctx, unsigned int flags, CUdevice dev )
{
    if (!ctx->api.cuCtxCreate)
        ctx->api.cuCtxCreate = (context::api_t::tcuCtxCreate*)ctx->cuda_dll.resolve("cuCtxCreate");
    assert(ctx->api.cuCtxCreate);
    return ctx->api.cuCtxCreate(pctx, flags, dev);
}

CUresult cuda_api::cuCtxDestroy(CUcontext cuctx)
{
    if (!ctx->api.cuCtxDestroy)
        ctx->api.cuCtxDestroy = (context::api_t::tcuCtxDestroy*)this->ctx->cuda_dll.resolve("cuCtxDestroy");
    assert(ctx->api.cuCtxDestroy);
    return ctx->api.cuCtxDestroy(cuctx);
}

CUresult cuda_api::cuCtxPushCurrent(CUcontext cuctx)
{
    if (!ctx->api.cuCtxPushCurrent)
        ctx->api.cuCtxPushCurrent = (context::api_t::tcuCtxPushCurrent*)this->ctx->cuda_dll.resolve("cuCtxPushCurrent");
    assert(ctx->api.cuCtxPushCurrent);
    return ctx->api.cuCtxPushCurrent(cuctx);
}

CUresult cuda_api::cuCtxPopCurrent(CUcontext *pctx)
{
    if (!ctx->api.cuCtxPopCurrent)
        ctx->api.cuCtxPopCurrent = (context::api_t::tcuCtxPopCurrent*)this->ctx->cuda_dll.resolve("cuCtxPopCurrent");
    assert(ctx->api.cuCtxPopCurrent);
    return ctx->api.cuCtxPopCurrent(pctx);
}

CUresult cuda_api::cuMemAllocHost(void **pp, unsigned int bytesize)
{
    if(!ctx->api.cuMemAllocHost)
        ctx->api.cuMemAllocHost = (context::api_t::tcuMemAllocHost*)ctx->cuda_dll.resolve("cuMemAllocHost");
    assert(ctx->api.cuMemAllocHost);
    return ctx->api.cuMemAllocHost(pp, bytesize);
}

CUresult cuda_api::cuMemFreeHost(void *p)
{
    if (!ctx->api.cuMemFreeHost)
        ctx->api.cuMemFreeHost = (context::api_t::tcuMemFreeHost*)ctx->cuda_dll.resolve("cuMemFreeHost");
    assert(ctx->api.cuMemFreeHost);
    return ctx->api.cuMemFreeHost(p);
}

CUresult cuda_api::cuMemcpyDtoH(void *dstHost, CUdeviceptr srcDevice, unsigned int ByteCount )
{
    if (!ctx->api.cuMemcpyDtoH)
        ctx->api.cuMemcpyDtoH = (context::api_t::tcuMemcpyDtoH*)ctx->cuda_dll.resolve("cuMemcpyDtoH");
    assert(ctx->api.cuMemcpyDtoH);
    return ctx->api.cuMemcpyDtoH(dstHost, srcDevice, ByteCount);
}

CUresult cuda_api::cuMemcpyDtoHAsync(void *dstHost, CUdeviceptr srcDevice, unsigned int ByteCount, CUstream hStream)
{
    if (!ctx->api.cuMemcpyDtoHAsync)
        ctx->api.cuMemcpyDtoHAsync = (context::api_t::tcuMemcpyDtoHAsync*)ctx->cuda_dll.resolve("cuMemcpyDtoHAsync");
    assert(ctx->api.cuMemcpyDtoHAsync);
    return ctx->api.cuMemcpyDtoHAsync(dstHost, srcDevice, ByteCount, hStream);
}

CUresult cuda_api::cuStreamCreate(CUstream *phStream, unsigned int Flags)
{
    if (!ctx->api.cuStreamCreate)
        ctx->api.cuStreamCreate = (context::api_t::tcuStreamCreate*)ctx->cuda_dll.resolve("cuStreamCreate");
    assert(ctx->api.cuStreamCreate);
    return ctx->api.cuStreamCreate(phStream, Flags);
}

CUresult cuda_api::cuStreamDestroy(CUstream hStream)
{
    if (!ctx->api.cuStreamDestroy)
        ctx->api.cuStreamDestroy = (context::api_t::tcuStreamDestroy*)ctx->cuda_dll.resolve("cuStreamDestroy");
    assert(ctx->api.cuStreamDestroy);
    return ctx->api.cuStreamDestroy(hStream);
}

CUresult cuda_api::cuStreamQuery(CUstream hStream)
{
    if (!ctx->api.cuStreamQuery)
        ctx->api.cuStreamQuery = (context::api_t::tcuStreamQuery*)ctx->cuda_dll.resolve("cuStreamQuery");
    assert(ctx->api.cuStreamQuery);
    return ctx->api.cuStreamQuery(hStream);
}

CUresult cuda_api::cuDeviceGetCount(int *count)
{
    if (!ctx->api.cuDeviceGetCount)
        ctx->api.cuDeviceGetCount = (context::api_t::tcuDeviceGetCount*)ctx->cuda_dll.resolve("cuDeviceGetCount");
    assert(ctx->api.cuDeviceGetCount);
    return ctx->api.cuDeviceGetCount(count);
}

CUresult cuda_api::cuDriverGetVersion(int *driverVersion)
{
    if (!ctx->api.cuDriverGetVersion)
        ctx->api.cuDriverGetVersion = (context::api_t::tcuDriverGetVersion*)ctx->cuda_dll.resolve("cuDriverGetVersion");
    assert(ctx->api.cuDriverGetVersion);
    return ctx->api.cuDriverGetVersion(driverVersion);
}

CUresult cuda_api::cuDeviceGetName(char *name, int len, CUdevice dev)
{
    if (!ctx->api.cuDeviceGetName)
        ctx->api.cuDeviceGetName = (context::api_t::tcuDeviceGetName*)ctx->cuda_dll.resolve("cuDeviceGetName");
    assert(ctx->api.cuDeviceGetName);
    return ctx->api.cuDeviceGetName(name, len, dev);
}

CUresult cuda_api::cuDeviceComputeCapability(int *major, int *minor, CUdevice dev)
{
    if (!ctx->api.cuDeviceComputeCapability)
        ctx->api.cuDeviceComputeCapability = (context::api_t::tcuDeviceComputeCapability*)ctx->cuda_dll.resolve("cuDeviceComputeCapability");
    assert(ctx->api.cuDeviceComputeCapability);
    return ctx->api.cuDeviceComputeCapability(major, minor, dev);
}

CUresult cuda_api::cuDeviceGetAttribute(int *pi, CUdevice_attribute attrib, CUdevice dev)
{
    if (!ctx->api.cuDeviceGetAttribute)
        ctx->api.cuDeviceGetAttribute = (context::api_t::tcuDeviceGetAttribute*)ctx->cuda_dll.resolve("cuDeviceGetAttribute");
    assert(ctx->api.cuDeviceGetAttribute);
    return ctx->api.cuDeviceGetAttribute(pi, attrib, dev);
}


////////////////////////////////////////////////////
/// D3D Interop
////////////////////////////////////////////////////
//CUresult cuda_api::cuD3D9CtxCreate( CUcontext *pCtx, CUdevice *pCudaDevice, unsigned int Flags, IDirect3DDevice9 *pD3DDevice );

////////////////////////////////////////////////////
/// CUVID functions
////////////////////////////////////////////////////
CUresult cuda_api::cuvidCtxLockCreate(CUvideoctxlock *pLock, CUcontext cuctx)
{
    if (!ctx->api.cuvidCtxLockCreate)
        ctx->api.cuvidCtxLockCreate = (context::api_t::tcuvidCtxLockCreate*)this->ctx->cuvid_dll.resolve("cuvidCtxLockCreate");
    assert(ctx->api.cuvidCtxLockCreate);
    return ctx->api.cuvidCtxLockCreate(pLock, cuctx);
}

CUresult cuda_api::cuvidCtxLockDestroy(CUvideoctxlock lck)
{
    if (!ctx->api.cuvidCtxLockDestroy)
        ctx->api.cuvidCtxLockDestroy = (context::api_t::tcuvidCtxLockDestroy*)ctx->cuvid_dll.resolve("cuvidCtxLockDestroy");
    assert(ctx->api.cuvidCtxLockDestroy);
    return ctx->api.cuvidCtxLockDestroy(lck);
}

CUresult cuda_api::cuvidCtxLock(CUvideoctxlock lck, unsigned int reserved_flags)
{
    if (!ctx->api.cuvidCtxLock)
        ctx->api.cuvidCtxLock = (context::api_t::tcuvidCtxLock*)ctx->cuvid_dll.resolve("cuvidCtxLock");
    assert(ctx->api.cuvidCtxLock);
    return ctx->api.cuvidCtxLock(lck, reserved_flags);
}

CUresult cuda_api::cuvidCtxUnlock(CUvideoctxlock lck, unsigned int reserved_flags)
{
    if (!ctx->api.cuvidCtxUnlock)
        ctx->api.cuvidCtxUnlock = (context::api_t::tcuvidCtxUnlock*)ctx->cuvid_dll.resolve("cuvidCtxUnlock");
    assert(ctx->api.cuvidCtxUnlock);
    return ctx->api.cuvidCtxUnlock(lck, reserved_flags);
}

CUresult cuda_api::cuCtxSynchronize()
{
    if (!ctx->api.cuCtxSynchronize)
        ctx->api.cuCtxSynchronize = (context::api_t::tcuCtxSynchronize*)ctx->cuda_dll.resolve("cuCtxSynchronize");
    assert(ctx->api.cuCtxSynchronize);
    return ctx->api.cuCtxSynchronize();
}

CUresult cuda_api::cuvidCreateVideoParser(CUvideoparser *pObj, CUVIDPARSERPARAMS *pParams)
{
    if (!ctx->api.cuvidCreateVideoParser)
        ctx->api.cuvidCreateVideoParser = (context::api_t::tcuvidCreateVideoParser*)ctx->cuvid_dll.resolve("cuvidCreateVideoParser");
    assert(ctx->api.cuvidCreateVideoParser);
    return ctx->api.cuvidCreateVideoParser(pObj, pParams);
}

CUresult cuda_api::cuvidParseVideoData(CUvideoparser obj, CUVIDSOURCEDATAPACKET *pPacket)
{
    if (!ctx->api.cuvidParseVideoData)
        ctx->api.cuvidParseVideoData = (context::api_t::tcuvidParseVideoData*)ctx->cuvid_dll.resolve("cuvidParseVideoData");
    assert(ctx->api.cuvidParseVideoData);
    return ctx->api.cuvidParseVideoData(obj, pPacket);
}

CUresult cuda_api::cuvidDestroyVideoParser(CUvideoparser obj)
{
    if (!ctx->api.cuvidDestroyVideoParser)
        ctx->api.cuvidDestroyVideoParser = (context::api_t::tcuvidDestroyVideoParser*)ctx->cuvid_dll.resolve("cuvidDestroyVideoParser");
    assert(ctx->api.cuvidDestroyVideoParser);
    return ctx->api.cuvidDestroyVideoParser(obj);
}

// Create/Destroy the decoder object
CUresult cuda_api::cuvidCreateDecoder(CUvideodecoder *phDecoder, CUVIDDECODECREATEINFO *pdci)
{
    if (!ctx->api.cuvidCreateDecoder)
        ctx->api.cuvidCreateDecoder = (context::api_t::tcuvidCreateDecoder*)ctx->cuvid_dll.resolve("cuvidCreateDecoder");
    assert(ctx->api.cuvidCreateDecoder);
    return ctx->api.cuvidCreateDecoder(phDecoder, pdci);
}

CUresult cuda_api::cuvidDestroyDecoder(CUvideodecoder hDecoder)
{
    if (!ctx->api.cuvidDestroyDecoder)
        ctx->api.cuvidDestroyDecoder = (context::api_t::tcuvidDestroyDecoder*)ctx->cuvid_dll.resolve("cuvidDestroyDecoder");
    assert(ctx->api.cuvidDestroyDecoder);
    return ctx->api.cuvidDestroyDecoder(hDecoder);
}

// Decode a single picture (field or frame)
CUresult cuda_api::cuvidDecodePicture(CUvideodecoder hDecoder, CUVIDPICPARAMS *pPicParams)
{
    if (!ctx->api.cuvidDecodePicture)
        ctx->api.cuvidDecodePicture = (context::api_t::tcuvidDecodePicture*)   ctx->cuvid_dll.resolve("cuvidDecodePicture");
    assert(ctx->api.cuvidDecodePicture);
    return ctx->api.cuvidDecodePicture(hDecoder, pPicParams);
}

// Post-process and map a video frame for use in cuda
CUresult cuda_api::cuvidMapVideoFrame(CUvideodecoder hDecoder, int nPicIdx, unsigned int *pDevPtr, unsigned int *pPitch, CUVIDPROCPARAMS *pVPP)
{
    if (!ctx->api.cuvidMapVideoFrame)
        ctx->api.cuvidMapVideoFrame = (context::api_t::tcuvidMapVideoFrame*)ctx->cuvid_dll.resolve("cuvidMapVideoFrame");
    assert(ctx->api.cuvidMapVideoFrame);
    return ctx->api.cuvidMapVideoFrame(hDecoder, nPicIdx, pDevPtr, pPitch, pVPP);
}

// Unmap a previously mapped video frame
CUresult cuda_api::cuvidUnmapVideoFrame(CUvideodecoder hDecoder, unsigned int DevPtr)
{
    if (!ctx->api.cuvidUnmapVideoFrame)
        ctx->api.cuvidUnmapVideoFrame = (context::api_t::tcuvidUnmapVideoFrame*)ctx->cuvid_dll.resolve("cuvidUnmapVideoFrame");
    assert(ctx->api.cuvidUnmapVideoFrame);
    return ctx->api.cuvidUnmapVideoFrame(hDecoder, DevPtr);
}

#endif // !NV_CONFIG(DLLAPI_CUDA) && !defined(CUDA_LINK)
int cuda_api::GetMaxGflopsGraphicsDeviceId() {
    CUdevice current_device = 0, max_perf_device = 0;
    int device_count     = 0, sm_per_multiproc = 0;
    int max_compute_perf = 0, best_SM_arch     = 0;
    int major = 0, minor = 0, multiProcessorCount, clockRate;
    int bTCC = 0, version;
    char deviceName[256];

    cuDeviceGetCount(&device_count);
    if (device_count <= 0)
        return -1;

    cuDriverGetVersion(&version);
    // Find the best major SM Architecture GPU device that are graphics devices
    while (current_device < device_count) {
        cuDeviceGetName(deviceName, 256, current_device);
        cuDeviceComputeCapability(&major, &minor, current_device);
        if (version >= 3020) {
            cuDeviceGetAttribute(&bTCC, CU_DEVICE_ATTRIBUTE_TCC_DRIVER, current_device);
        } else {
            // Assume a Tesla GPU is running in TCC if we are running CUDA 3.1
            if (deviceName[0] == 'T')
                bTCC = 1;
        }
        if (!bTCC) {
            if (major > 0 && major < 9999) {
                best_SM_arch = std::max(best_SM_arch, major);
            }
        }
        current_device++;
    }
    // Find the best CUDA capable GPU device
    current_device = 0;
    while (current_device < device_count) {
        cuDeviceGetAttribute(&multiProcessorCount, CU_DEVICE_ATTRIBUTE_MULTIPROCESSOR_COUNT, current_device);
        cuDeviceGetAttribute(&clockRate, CU_DEVICE_ATTRIBUTE_CLOCK_RATE, current_device);
        cuDeviceComputeCapability(&major, &minor, current_device);
        if (version >= 3020) {
            cuDeviceGetAttribute(&bTCC, CU_DEVICE_ATTRIBUTE_TCC_DRIVER, current_device);
        } else {
            // Assume a Tesla GPU is running in TCC if we are running CUDA 3.1
            if (deviceName[0] == 'T')
                bTCC = 1;
        }
        if (major == 9999 && minor == 9999) {
            sm_per_multiproc = 1;
        } else {
            sm_per_multiproc = _ConvertSMVer2Cores(major, minor);
        }
        // If this is a Tesla based GPU and SM 2.0, and TCC is disabled, this is a contendor
        if (!bTCC) {// Is this GPU running the TCC driver?  If so we pass on this
            int compute_perf = multiProcessorCount * sm_per_multiproc * clockRate;
            printf("%s @%d compute_perf=%d max_compute_perf=%d\n", __FUNCTION__, __LINE__, compute_perf, max_compute_perf);
            if (compute_perf > max_compute_perf) {
                // If we find GPU with SM major > 2, search only these
                if (best_SM_arch > 2) {
                    printf("%s @%d best_SM_arch=%d\n", __FUNCTION__, __LINE__, best_SM_arch);
                    // If our device = dest_SM_arch, then we pick this one
                    if (major == best_SM_arch) {
                        max_compute_perf = compute_perf;
                        max_perf_device = current_device;
                    }
                } else {
                    max_compute_perf = compute_perf;
                    max_perf_device = current_device;
                }
            }
            cuDeviceGetName(deviceName, 256, current_device);
            printf("CUDA Device: %s, Compute: %d.%d, CUDA Cores: %d, Clock: %d MHz\n", deviceName, major, minor, multiProcessorCount * sm_per_multiproc, clockRate / 1000);
        }
        ++current_device;
    }
    return max_perf_device;
}
