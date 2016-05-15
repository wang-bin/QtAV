/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

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

#ifndef CUDA_API_H
#define CUDA_API_H

#include "dllapi/nv_inc.h"

#define CUDA_ENSURE(f, ...) CUDA_CHECK(f, return __VA_ARGS__;) //call cuda_api.cuGetErrorXXX
#define CUDA_WARN(f) CUDA_CHECK(f) //call cuda_api.cuGetErrorXXX
#define CUDA_ENSURE2(f, ...) CUDA_CHECK2(f, return __VA_ARGS__;)
#define CUDA_WARN2(f) CUDA_CHECK2(f)

#if CUDA_VERSION < 7050
#if CUDA_VERSION < 6050
#define cudaVideoCodec_HEVC cudaVideoCodec(cudaVideoCodec_H264_MVC + 1)
#endif //6050
#define cudaVideoCodec_VP8 cudaVideoCodec(cudaVideoCodec_HEVC+1)
#define cudaVideoCodec_VP9 cudaVideoCodec(cudaVideoCodec_VP8+1)
#endif //7050
struct IDirect3DDevice9;
struct IDirect3DResource9;
// TODO: cuda_drvapi_dylink.c/h

class cuda_api {
public:
    cuda_api();
    virtual ~cuda_api();
    bool isLoaded() const;
#if !NV_CONFIG(DLLAPI_CUDA) && !defined(CUDA_LINK)
    typedef unsigned int GLuint;
    typedef unsigned int GLenum;
    ////////////////////////////////////////////////////
    /// CUDA functions
    ////////////////////////////////////////////////////
    CUresult cuGetErrorName(CUresult error, const char **pStr); // since 6.0. fallback to _cudaGetErrorEnum defined in helper_cuda.h if symbol not found
    CUresult cuGetErrorString(CUresult error, const char **pStr); // since 6.0. fallback to a empty string if symbol not found
    CUresult cuInit(unsigned int Flags);
    CUresult cuCtxGetApiVersion(CUcontext pctx, unsigned int *version);
    CUresult cuCtxCreate(CUcontext *pctx, unsigned int flags, CUdevice dev);
    CUresult cuCtxDestroy(CUcontext cuctx);
    CUresult cuCtxPushCurrent(CUcontext cuctx);
    CUresult cuCtxPopCurrent( CUcontext *pctx);
    CUresult cuCtxGetCurrent(CUcontext *pctx);
    CUresult cuCtxSynchronize();
    CUresult cuMemAllocHost(void **pp, size_t bytesize);
    CUresult cuMemFreeHost(void *p);
    CUresult cuMemcpyDtoH(void *dstHost, CUdeviceptr srcDevice, size_t ByteCount); //TODO: size_t in new version
    CUresult cuMemcpyDtoHAsync(void *dstHost, CUdeviceptr srcDevice, size_t ByteCount, CUstream hStream); //TODO: size_t in new version
    CUresult cuMemcpy2D(const CUDA_MEMCPY2D *pCopy);
    CUresult cuMemcpy2DAsync(const CUDA_MEMCPY2D *pCopy, CUstream hStream);
    CUresult cuStreamCreate(CUstream *phStream, unsigned int Flags); //TODO: size_t in new version
    CUresult cuStreamDestroy(CUstream hStream);
    CUresult cuStreamQuery(CUstream hStream);
    CUresult cuStreamSynchronize(CUstream hStream);
    CUresult cuDeviceGetCount(int *count);
    CUresult cuDriverGetVersion(int *driverVersion);
    CUresult cuDeviceGetName(char *name, int len, CUdevice dev);
    CUresult cuDeviceComputeCapability(int *major, int *minor, CUdevice dev);
    CUresult cuDeviceGetAttribute(int *pi, CUdevice_attribute attrib, CUdevice dev);

    CUresult cuGLCtxCreate(CUcontext *pCtx, unsigned int Flags, CUdevice device);
    CUresult cuGraphicsGLRegisterImage(CUgraphicsResource *pCudaResource, GLuint image, GLenum target, unsigned int Flags);
    CUresult cuGraphicsUnregisterResource(CUgraphicsResource resource);
    CUresult cuGraphicsMapResources(unsigned int count, CUgraphicsResource *resources, CUstream hStream);
    CUresult cuGraphicsSubResourceGetMappedArray(CUarray *pArray, CUgraphicsResource resource, unsigned int arrayIndex, unsigned int mipLevel);
    CUresult cuGraphicsUnmapResources(unsigned int count, CUgraphicsResource *resources, CUstream hStream);

    CUresult cuD3D9CtxCreate(CUcontext *pCtx, CUdevice *pCudaDevice, unsigned int Flags, IDirect3DDevice9 *pD3DDevice);
    CUresult cuGraphicsD3D9RegisterResource(CUgraphicsResource *pCudaResource, IDirect3DResource9 *pD3DResource, unsigned int Flags);

    ////////////////////////////////////////////////////
    /// CUVID functions
    ////////////////////////////////////////////////////
    CUresult cuvidCtxLockCreate(CUvideoctxlock *pLock, CUcontext cuctx);
    CUresult cuvidCtxLockDestroy(CUvideoctxlock lck);
    CUresult cuvidCtxLock(CUvideoctxlock lck, unsigned int reserved_flags);
    CUresult cuvidCtxUnlock(CUvideoctxlock lck, unsigned int reserved_flags);

    CUresult cuvidCreateVideoParser(CUvideoparser *pObj, CUVIDPARSERPARAMS *pParams);
    CUresult cuvidParseVideoData(CUvideoparser obj, CUVIDSOURCEDATAPACKET *pPacket);
    CUresult cuvidDestroyVideoParser(CUvideoparser obj);

    // Create/Destroy the decoder object
    CUresult cuvidCreateDecoder(CUvideodecoder *phDecoder, CUVIDDECODECREATEINFO *pdci);
    CUresult cuvidDestroyDecoder(CUvideodecoder hDecoder);

    // Decode a single picture (field or frame)
    CUresult cuvidDecodePicture(CUvideodecoder hDecoder, CUVIDPICPARAMS *pPicParams);

    // Post-process and map a video frame for use in cuda.
    // unsigned long long* (CUdevicePtr 64) for x64+cuda3.2, otherwise unsigned int* (CUdevicePtr 32)
    CUresult cuvidMapVideoFrame(CUvideodecoder hDecoder, int nPicIdx, CUdeviceptr *pDevPtr, unsigned int *pPitch, CUVIDPROCPARAMS *pVPP);
    // Unmap a previously mapped video frame
    CUresult cuvidUnmapVideoFrame(CUvideodecoder hDecoder, CUdeviceptr DevPtr);

#endif // !NV_CONFIG(DLLAPI_CUDA) && !defined(CUDA_LINK)
    // This function returns the best Graphics GPU based on performance
    int GetMaxGflopsGraphicsDeviceId();
private:
    class context;
    context *ctx;
};

#define CUDA_CHECK(f, ...) \
    do { \
        CUresult cuR = f; \
        if (cuR != CUDA_SUCCESS) { \
            const char* errName = NULL; \
            const char* errDetail = NULL; \
            cuGetErrorName(cuR, &errName); \
            cuGetErrorString(cuR, &errDetail); \
            qWarning("CUDA error %s@%d. " #f ": %d %s - %s", __FILE__, __LINE__, cuR, errName, errDetail); \
            __VA_ARGS__ \
        } \
    } while (0)

#define CUDA_CHECK2(f, ...) \
    do { \
        CUresult cuR = f; \
        if (cuR != CUDA_SUCCESS) { \
            qWarning("CUDA error %s@%d. " #f ": %d %s", __FILE__, __LINE__, cuR, _cudaGetErrorEnum(cuR)); \
            __VA_ARGS__ \
        } \
    } while (0)


// TODO: error check
class AutoCtxLock {
    cuda_api *m_api;
    CUvideoctxlock m_lock;
public:
    AutoCtxLock(cuda_api *api, CUvideoctxlock lck) : m_api(api), m_lock(lck) {
        m_api->cuvidCtxLock(m_lock, 0);
    }
    ~AutoCtxLock() { m_api->cuvidCtxUnlock(m_lock, 0); }
};

class CUVIDAutoUnmapper {
    cuda_api *api;
    CUvideodecoder dec;
    CUdeviceptr devptr;
public:
    CUVIDAutoUnmapper(cuda_api *a, CUvideodecoder d, CUdeviceptr p) : api(a), dec(d), devptr(p) {}
    ~CUVIDAutoUnmapper() {
        api->cuvidUnmapVideoFrame(dec, devptr);
    }
};

#endif // CUDA_API_H
