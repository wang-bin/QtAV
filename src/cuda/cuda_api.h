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


#ifndef CUDA_API_H
#define CUDA_API_H

#undef NV_CONFIG
#define NV_CONFIG(FEATURE) (defined QTAV_HAVE_##FEATURE && QTAV_HAVE_##FEATURE)

// high version will define cuXXX macro, so functions here will be not they look like
#if !NV_CONFIG(DLLAPI_CUDA) && !defined(CUDA_LINK)
#define CUDA_FORCE_API_VERSION 3010
#endif
#include "dllapi/nv_inc.h"

#ifdef __DRIVER_TYPES_H__
#ifndef DEVICE_RESET
#define DEVICE_RESET cudaDeviceReset();
#endif //DEVICE_RESET
#else
#ifndef DEVICE_RESET
#define DEVICE_RESET
#endif //DEVICE_RESET
#endif //__DRIVER_TYPES_H__

#define checkCudaErrors(val) \
    if (!check ( (val), #val, __FILE__, __LINE__ )) \
        return false;

class cuda_api {
public:
    cuda_api();
    virtual ~cuda_api();
    bool isLoaded() const;
#if !NV_CONFIG(DLLAPI_CUDA) && !defined(CUDA_LINK)
    ////////////////////////////////////////////////////
    /// CUDA functions
    ////////////////////////////////////////////////////
    CUresult cuInit(unsigned int Flags);
    CUresult cuCtxCreate(CUcontext *pctx, unsigned int flags, CUdevice dev );
    CUresult cuCtxDestroy(CUcontext cuctx );
    CUresult cuCtxPushCurrent(CUcontext cuctx );
    CUresult cuCtxPopCurrent( CUcontext *pctx );
    CUresult cuCtxSynchronize();
    CUresult cuMemAllocHost(void **pp, unsigned int bytesize);
    CUresult cuMemFreeHost(void *p);
    CUresult cuMemcpyDtoH (void *dstHost, CUdeviceptr srcDevice, unsigned int ByteCount );
    CUresult cuMemcpyDtoHAsync(void *dstHost, CUdeviceptr srcDevice, unsigned int ByteCount, CUstream hStream);
    CUresult cuStreamCreate(CUstream *phStream, unsigned int Flags);
    CUresult cuStreamDestroy(CUstream hStream);
    CUresult cuStreamQuery(CUstream hStream);
    CUresult cuDeviceGetCount(int *count);
    CUresult cuDriverGetVersion(int *driverVersion);
    CUresult cuDeviceGetName(char *name, int len, CUdevice dev);
    CUresult cuDeviceComputeCapability(int *major, int *minor, CUdevice dev);
    CUresult cuDeviceGetAttribute(int *pi, CUdevice_attribute attrib, CUdevice dev);

    ////////////////////////////////////////////////////
    /// D3D Interop
    ////////////////////////////////////////////////////
    //CUresult cuD3D9CtxCreate(CUcontext *pCtx, CUdevice *pCudaDevice, unsigned int Flags, IDirect3DDevice9 *pD3DDevice );

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

    // Post-process and map a video frame for use in cuda
    CUresult cuvidMapVideoFrame(CUvideodecoder hDecoder, int nPicIdx, unsigned int *pDevPtr, unsigned int *pPitch, CUVIDPROCPARAMS *pVPP);
    // Unmap a previously mapped video frame
    CUresult cuvidUnmapVideoFrame(CUvideodecoder hDecoder, unsigned int DevPtr);


    class AutoCtxLock
    {
    private:
        CUvideoctxlock m_lock;
        cuda_api *m_api;
    public:
        AutoCtxLock(cuda_api *api, CUvideoctxlock lck) {
            m_api = api;
            m_lock=lck;
            m_api->cuvidCtxLock(m_lock, 0);
        }
        ~AutoCtxLock() { m_api->cuvidCtxUnlock(m_lock, 0); }
    };

#endif // !NV_CONFIG(DLLAPI_CUDA) && !defined(CUDA_LINK)

    int GetMaxGflopsGraphicsDeviceId();

    template< typename T >
    bool check(T result, char const *const func, const char *const file, int const line)
    {
        if (result != CUDA_SUCCESS) {
            qWarning("CUDA error at %s:%d code=%d(%s) \"%s\"",
                    file, line, static_cast<unsigned int>(result), _cudaGetErrorEnum(result), func);
            DEVICE_RESET
            // Make sure we call CUDA Device Reset before exiting
            //exit(EXIT_FAILURE);
        }
        return result == CUDA_SUCCESS;
    }

private:
    class context;
    context *ctx;
};


#endif // CUDA_API_H
