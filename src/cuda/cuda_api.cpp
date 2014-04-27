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

class cuda_api::context {
public:
    context()
        : loaded(true) {
#if !NV_CONFIG(DLLAPI_CUDA) && !defined(CUDA_LINK)
        loaded = false;
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
    typedef CUresult CUDAAPI tcuInit(unsigned int);
    static tcuInit* fp_tcuInit = (tcuInit*)ctx->cuda_dll.resolve("cuInit");
    assert(fp_tcuInit);
    return fp_tcuInit(Flags);
}

CUresult cuda_api::cuCtxCreate(CUcontext *pctx, unsigned int flags, CUdevice dev )
{
    typedef CUresult CUDAAPI tcuCtxCreate(CUcontext *, unsigned int, CUdevice);
    static tcuCtxCreate* fp_tcuCtxCreate = (tcuCtxCreate*)ctx->cuda_dll.resolve("cuCtxCreate");
    assert(fp_tcuCtxCreate);
    return fp_tcuCtxCreate(pctx, flags, dev);
}

CUresult cuda_api::cuCtxDestroy(CUcontext ctx)
{
    typedef CUresult CUDAAPI tcuCtxDestroy(CUcontext);
    static tcuCtxDestroy* fp_tcuCtxDestroy = (tcuCtxDestroy*)this->ctx->cuda_dll.resolve("cuCtxDestroy");
    assert(fp_tcuCtxDestroy);
    return fp_tcuCtxDestroy(ctx);
}

CUresult cuda_api::cuCtxPushCurrent(CUcontext ctx)
{
    typedef CUresult CUDAAPI tcuCtxPushCurrent(CUcontext);
    static tcuCtxPushCurrent* fp_tcuCtxPushCurrent = (tcuCtxPushCurrent*)this->ctx->cuda_dll.resolve("cuCtxPushCurrent");
    assert(fp_tcuCtxPushCurrent);
    return fp_tcuCtxPushCurrent(ctx);
}

CUresult cuda_api::cuCtxPopCurrent(CUcontext *pctx)
{
    typedef CUresult CUDAAPI tcuCtxPopCurrent(CUcontext *);
    static tcuCtxPopCurrent* fp_tcuCtxPopCurrent = (tcuCtxPopCurrent*)this->ctx->cuda_dll.resolve("cuCtxPopCurrent");
    assert(fp_tcuCtxPopCurrent);
    return fp_tcuCtxPopCurrent(pctx);
}

CUresult cuda_api::cuMemAllocHost(void **pp, unsigned int bytesize)
{
    typedef CUresult CUDAAPI tcuMemAllocHost(void **pp, unsigned int bytesize);
    static tcuMemAllocHost* fp_tcuMemAllocHost = (tcuMemAllocHost*)ctx->cuda_dll.resolve("cuMemAllocHost");
    assert(fp_tcuMemAllocHost);
    return fp_tcuMemAllocHost(pp, bytesize);
}

CUresult cuda_api::cuMemFreeHost(void *p)
{
    typedef CUresult CUDAAPI tcuMemFreeHost(void *p);
    static tcuMemFreeHost* fp_tcuMemFreeHost = (tcuMemFreeHost*)ctx->cuda_dll.resolve("cuMemFreeHost");
    assert(fp_tcuMemFreeHost);
    return fp_tcuMemFreeHost(p);
}

CUresult cuda_api::cuMemcpyDtoH(void *dstHost, CUdeviceptr srcDevice, unsigned int ByteCount )
{
    typedef CUresult CUDAAPI tcuMemcpyDtoH(void *dstHost, CUdeviceptr srcDevice, unsigned int ByteCount );
    static tcuMemcpyDtoH* fp_tcuMemcpyDtoH = (tcuMemcpyDtoH*)ctx->cuda_dll.resolve("cuMemcpyDtoH");
    assert(fp_tcuMemcpyDtoH);
    return fp_tcuMemcpyDtoH(dstHost, srcDevice, ByteCount);
}

CUresult cuda_api::cuMemcpyDtoHAsync(void *dstHost, CUdeviceptr srcDevice, unsigned int ByteCount, CUstream hStream)
{
    typedef CUresult CUDAAPI tcuMemcpyDtoHAsync(void *dstHost, CUdeviceptr srcDevice, unsigned int ByteCount, CUstream hStream);
    static tcuMemcpyDtoHAsync* fp_tcuMemcpyDtoHAsync = (tcuMemcpyDtoHAsync*)ctx->cuda_dll.resolve("cuMemcpyDtoHAsync");
    assert(fp_tcuMemcpyDtoHAsync);
    return fp_tcuMemcpyDtoHAsync(dstHost, srcDevice, ByteCount, hStream);
}

CUresult cuda_api::cuStreamCreate(CUstream *phStream, unsigned int Flags)
{
    typedef CUresult CUDAAPI tcuStreamCreate(CUstream *phStream, unsigned int Flags);
    static tcuStreamCreate* fp_cuStreamCreate = (tcuStreamCreate*)ctx->cuda_dll.resolve("cuStreamCreate");
    assert(fp_cuStreamCreate);
    return fp_cuStreamCreate(phStream, Flags);
}

CUresult cuda_api::cuStreamDestroy(CUstream hStream)
{
    typedef CUresult CUDAAPI tcuStreamDestroy(CUstream hStream);
    static tcuStreamDestroy* fp_tcuStreamDestroy = (tcuStreamDestroy*)ctx->cuda_dll.resolve("cuStreamDestroy");
    assert(fp_tcuStreamDestroy);
    return fp_tcuStreamDestroy(hStream);
}

CUresult cuda_api::cuStreamQuery(CUstream hStream)
{
    typedef CUresult CUDAAPI tcuStreamQuery(CUstream hStream);
    static tcuStreamQuery* fp_tcuStreamQuery = (tcuStreamQuery*)ctx->cuda_dll.resolve("cuStreamQuery");
    assert(fp_tcuStreamQuery);
    return fp_tcuStreamQuery(hStream);
}

CUresult cuda_api::cuDeviceGetCount(int *count)
{
    typedef CUresult CUDAAPI tcuDeviceGetCount(int *count);
    static tcuDeviceGetCount* fp_tcuDeviceGetCount = (tcuDeviceGetCount*)ctx->cuda_dll.resolve("cuDeviceGetCount");
    assert(fp_tcuDeviceGetCount);
    return fp_tcuDeviceGetCount(count);
}

CUresult cuda_api::cuDriverGetVersion(int *driverVersion)
{
    typedef CUresult CUDAAPI tcuDriverGetVersion(int *driverVersion);
    static tcuDriverGetVersion* fp_tcuDriverGetVersion = (tcuDriverGetVersion*)ctx->cuda_dll.resolve("cuDriverGetVersion");
    assert(fp_tcuDriverGetVersion);
    return fp_tcuDriverGetVersion(driverVersion);
}

CUresult cuda_api::cuDeviceGetName(char *name, int len, CUdevice dev)
{
    typedef CUresult CUDAAPI tcuDeviceGetName(char *name, int len, CUdevice dev);
    static tcuDeviceGetName* fp_tcuDeviceGetName = (tcuDeviceGetName*)ctx->cuda_dll.resolve("cuDeviceGetName");
    assert(fp_tcuDeviceGetName);
    return fp_tcuDeviceGetName(name, len, dev);
}

CUresult cuda_api::cuDeviceComputeCapability(int *major, int *minor, CUdevice dev)
{
    typedef CUresult CUDAAPI tcuDeviceComputeCapability(int *major, int *minor, CUdevice dev);
    static tcuDeviceComputeCapability* fp_tcuDeviceComputeCapability = (tcuDeviceComputeCapability*)ctx->cuda_dll.resolve("cuDeviceComputeCapability");
    assert(fp_tcuDeviceComputeCapability);
    return fp_tcuDeviceComputeCapability(major, minor, dev);
}

CUresult cuda_api::cuDeviceGetAttribute(int *pi, CUdevice_attribute attrib, CUdevice dev)
{
    typedef CUresult CUDAAPI tcuDeviceGetAttribute(int *pi, CUdevice_attribute attrib, CUdevice dev);
    static tcuDeviceGetAttribute* fp_tcuDeviceGetAttribute = (tcuDeviceGetAttribute*)ctx->cuda_dll.resolve("cuDeviceGetAttribute");
    assert(fp_tcuDeviceGetAttribute);
    return fp_tcuDeviceGetAttribute(pi, attrib, dev);
}


////////////////////////////////////////////////////
/// D3D Interop
////////////////////////////////////////////////////
//CUresult cuda_api::cuD3D9CtxCreate( CUcontext *pCtx, CUdevice *pCudaDevice, unsigned int Flags, IDirect3DDevice9 *pD3DDevice );

////////////////////////////////////////////////////
/// CUVID functions
////////////////////////////////////////////////////
CUresult cuda_api::cuvidCtxLockCreate(CUvideoctxlock *pLock, CUcontext ctx)
{
    typedef CUresult CUDAAPI tcuvidCtxLockCreate(CUvideoctxlock *pLock, CUcontext ctx);
    static tcuvidCtxLockCreate* fp_tcuvidCtxLockCreate = (tcuvidCtxLockCreate*)this->ctx->cuvid_dll.resolve("cuvidCtxLockCreate");
    assert(fp_tcuvidCtxLockCreate);
    return fp_tcuvidCtxLockCreate(pLock, ctx);
}

CUresult cuda_api::cuvidCtxLockDestroy(CUvideoctxlock lck)
{
    typedef CUresult CUDAAPI tcuvidCtxLockDestroy(CUvideoctxlock lck);
    static tcuvidCtxLockDestroy* fp_tcuvidCtxLockDestroy = (tcuvidCtxLockDestroy*)ctx->cuvid_dll.resolve("cuvidCtxLockDestroy");
    assert(fp_tcuvidCtxLockDestroy);
    return fp_tcuvidCtxLockDestroy(lck);
}

CUresult cuda_api::cuvidCtxLock(CUvideoctxlock lck, unsigned int reserved_flags)
{
    typedef CUresult CUDAAPI tcuvidCtxLock(CUvideoctxlock lck, unsigned int reserved_flags);
    static tcuvidCtxLock* fp_tcuvidCtxLock = (tcuvidCtxLock*)ctx->cuvid_dll.resolve("cuvidCtxLock");
    assert(fp_tcuvidCtxLock);
    return fp_tcuvidCtxLock(lck, reserved_flags);
}

CUresult cuda_api::cuvidCtxUnlock(CUvideoctxlock lck, unsigned int reserved_flags)
{
    typedef CUresult CUDAAPI tcuvidCtxUnlock(CUvideoctxlock lck, unsigned int reserved_flags);
    static tcuvidCtxUnlock* fp_tcuvidCtxUnlock = (tcuvidCtxUnlock*)ctx->cuvid_dll.resolve("cuvidCtxUnlock");
    assert(fp_tcuvidCtxUnlock);
    return fp_tcuvidCtxUnlock(lck, reserved_flags);
}

CUresult cuda_api::cuCtxSynchronize()
{
    typedef CUresult CUDAAPI tcuCtxSynchronize();
    static tcuCtxSynchronize* fp_tcuCtxSynchronize = (tcuCtxSynchronize*)ctx->cuda_dll.resolve("cuCtxSynchronize");
    assert(fp_tcuCtxSynchronize);
    return fp_tcuCtxSynchronize();
}

CUresult cuda_api::cuvidCreateVideoParser(CUvideoparser *pObj, CUVIDPARSERPARAMS *pParams)
{
    typedef CUresult CUDAAPI tcuvidCreateVideoParser(CUvideoparser *pObj, CUVIDPARSERPARAMS *pParams);
    static tcuvidCreateVideoParser* fp_tcuvidCreateVideoParser = (tcuvidCreateVideoParser*)ctx->cuvid_dll.resolve("cuvidCreateVideoParser");
    assert(fp_tcuvidCreateVideoParser);
    return fp_tcuvidCreateVideoParser(pObj, pParams);
}

CUresult cuda_api::cuvidParseVideoData(CUvideoparser obj, CUVIDSOURCEDATAPACKET *pPacket)
{
    typedef CUresult CUDAAPI tcuvidParseVideoData(CUvideoparser obj, CUVIDSOURCEDATAPACKET *pPacket);
    static tcuvidParseVideoData* fp_tcuvidParseVideoData = (tcuvidParseVideoData*)ctx->cuvid_dll.resolve("cuvidParseVideoData");
    assert(fp_tcuvidParseVideoData);
    return fp_tcuvidParseVideoData(obj, pPacket);
}

CUresult cuda_api::cuvidDestroyVideoParser(CUvideoparser obj)
{
    typedef CUresult CUDAAPI tcuvidDestroyVideoParser(CUvideoparser obj);
    static tcuvidDestroyVideoParser* fp_tcuvidDestroyVideoParser = (tcuvidDestroyVideoParser*)ctx->cuvid_dll.resolve("cuvidDestroyVideoParser");
    assert(fp_tcuvidDestroyVideoParser);
    return fp_tcuvidDestroyVideoParser(obj);
}

// Create/Destroy the decoder object
CUresult cuda_api::cuvidCreateDecoder(CUvideodecoder *phDecoder, CUVIDDECODECREATEINFO *pdci)
{
    typedef CUresult CUDAAPI tcuvidCreateDecoder(CUvideodecoder *phDecoder, CUVIDDECODECREATEINFO *pdci);
    static tcuvidCreateDecoder* fp_tcuvidCreateDecoder = (tcuvidCreateDecoder*)ctx->cuvid_dll.resolve("cuvidCreateDecoder");
    assert(fp_tcuvidCreateDecoder);
    return fp_tcuvidCreateDecoder(phDecoder, pdci);
}

CUresult cuda_api::cuvidDestroyDecoder(CUvideodecoder hDecoder)
{
    typedef CUresult CUDAAPI tcuvidDestroyDecoder(CUvideodecoder hDecoder);
    static tcuvidDestroyDecoder* fp_tcuvidDestroyDecoder = (tcuvidDestroyDecoder*)ctx->cuvid_dll.resolve("cuvidDestroyDecoder");
    assert(fp_tcuvidDestroyDecoder);
    return fp_tcuvidDestroyDecoder(hDecoder);
}

// Decode a single picture (field or frame)
CUresult cuda_api::cuvidDecodePicture(CUvideodecoder hDecoder, CUVIDPICPARAMS *pPicParams)
{
    typedef CUresult CUDAAPI tcuvidDecodePicture(CUvideodecoder hDecoder, CUVIDPICPARAMS *pPicParams);
    static tcuvidDecodePicture* fp_tcuvidDecodePicture = (tcuvidDecodePicture*)   ctx->cuvid_dll.resolve("cuvidDecodePicture");
    assert(fp_tcuvidDecodePicture);
    return fp_tcuvidDecodePicture(hDecoder, pPicParams);
}

// Post-process and map a video frame for use in cuda
CUresult cuda_api::cuvidMapVideoFrame(CUvideodecoder hDecoder, int nPicIdx, unsigned int *pDevPtr, unsigned int *pPitch, CUVIDPROCPARAMS *pVPP)
{
    typedef CUresult CUDAAPI tcuvidMapVideoFrame(CUvideodecoder hDecoder, int nPicIdx, unsigned int *pDevPtr, unsigned int *pPitch, CUVIDPROCPARAMS *pVPP);
    static tcuvidMapVideoFrame* fp_tcuvidMapVideoFrame = (tcuvidMapVideoFrame*)ctx->cuvid_dll.resolve("cuvidMapVideoFrame");
    assert(fp_tcuvidMapVideoFrame);
    return fp_tcuvidMapVideoFrame(hDecoder, nPicIdx, pDevPtr, pPitch, pVPP);
}

// Unmap a previously mapped video frame
CUresult cuda_api::cuvidUnmapVideoFrame(CUvideodecoder hDecoder, unsigned int DevPtr)
{
    typedef CUresult CUDAAPI tcuvidUnmapVideoFrame(CUvideodecoder hDecoder, unsigned int DevPtr);
    static tcuvidUnmapVideoFrame* fp_tcuvidUnmapVideoFrame = (tcuvidUnmapVideoFrame*)ctx->cuvid_dll.resolve("cuvidUnmapVideoFrame");
    assert(fp_tcuvidUnmapVideoFrame);
    return fp_tcuvidUnmapVideoFrame(hDecoder, DevPtr);
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
