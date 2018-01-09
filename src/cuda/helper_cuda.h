/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2018 Wang Bin <wbsecg1@gmail.com>

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
///from nv's helper_cuda.h

#include <stdio.h>

#include "dllapi/nv_inc.h"
#if NV_CONFIG(DLLAPI_CUDA)
using namespace dllapi::cuda;
#endif /* NV_CONFIG(DLLAPI_CUDA)*/


#ifdef __cuda_cuda_h__

// CUDA Driver API errors
inline const char *_cudaGetErrorEnum(CUresult error)
{
    switch (error) {
        case CUDA_SUCCESS: return "CUDA_SUCCESS";
        case CUDA_ERROR_INVALID_VALUE: return "CUDA_ERROR_INVALID_VALUE";
        case CUDA_ERROR_OUT_OF_MEMORY: return "CUDA_ERROR_OUT_OF_MEMORY";
        case CUDA_ERROR_NOT_INITIALIZED: return "CUDA_ERROR_NOT_INITIALIZED";
        case CUDA_ERROR_DEINITIALIZED: return "CUDA_ERROR_DEINITIALIZED";
        case CUDA_ERROR_PROFILER_DISABLED: return "CUDA_ERROR_PROFILER_DISABLED";
        case CUDA_ERROR_PROFILER_NOT_INITIALIZED: return "CUDA_ERROR_PROFILER_NOT_INITIALIZED";
        case CUDA_ERROR_PROFILER_ALREADY_STARTED: return "CUDA_ERROR_PROFILER_ALREADY_STARTED";
        case CUDA_ERROR_PROFILER_ALREADY_STOPPED: return "CUDA_ERROR_PROFILER_ALREADY_STOPPED";
        case CUDA_ERROR_NO_DEVICE: return "CUDA_ERROR_NO_DEVICE";
        case CUDA_ERROR_INVALID_DEVICE: return "CUDA_ERROR_INVALID_DEVICE";
        case CUDA_ERROR_INVALID_IMAGE: return "CUDA_ERROR_INVALID_IMAGE";
        case CUDA_ERROR_INVALID_CONTEXT: return "CUDA_ERROR_INVALID_CONTEXT";
        case CUDA_ERROR_CONTEXT_ALREADY_CURRENT: return "CUDA_ERROR_CONTEXT_ALREADY_CURRENT";
        case CUDA_ERROR_MAP_FAILED: return "CUDA_ERROR_MAP_FAILED";
        case CUDA_ERROR_UNMAP_FAILED: return "CUDA_ERROR_UNMAP_FAILED";
        case CUDA_ERROR_ARRAY_IS_MAPPED: return "CUDA_ERROR_ARRAY_IS_MAPPED";
        case CUDA_ERROR_ALREADY_MAPPED: return "CUDA_ERROR_ALREADY_MAPPED";
        case CUDA_ERROR_NO_BINARY_FOR_GPU: return "CUDA_ERROR_NO_BINARY_FOR_GPU";
        case CUDA_ERROR_ALREADY_ACQUIRED: return "CUDA_ERROR_ALREADY_ACQUIRED";
        case CUDA_ERROR_NOT_MAPPED: return "CUDA_ERROR_NOT_MAPPED";
        case CUDA_ERROR_NOT_MAPPED_AS_ARRAY: return "CUDA_ERROR_NOT_MAPPED_AS_ARRAY";
        case CUDA_ERROR_NOT_MAPPED_AS_POINTER: return "CUDA_ERROR_NOT_MAPPED_AS_POINTER";
        case CUDA_ERROR_ECC_UNCORRECTABLE: return "CUDA_ERROR_ECC_UNCORRECTABLE";
        case CUDA_ERROR_UNSUPPORTED_LIMIT: return "CUDA_ERROR_UNSUPPORTED_LIMIT";
        case CUDA_ERROR_CONTEXT_ALREADY_IN_USE: return "CUDA_ERROR_CONTEXT_ALREADY_IN_USE";
        case CUDA_ERROR_INVALID_SOURCE: return "CUDA_ERROR_INVALID_SOURCE";
        case CUDA_ERROR_FILE_NOT_FOUND: return "CUDA_ERROR_FILE_NOT_FOUND";
        case CUDA_ERROR_SHARED_OBJECT_SYMBOL_NOT_FOUND: return "CUDA_ERROR_SHARED_OBJECT_SYMBOL_NOT_FOUND";
        case CUDA_ERROR_SHARED_OBJECT_INIT_FAILED: return "CUDA_ERROR_SHARED_OBJECT_INIT_FAILED";
        case CUDA_ERROR_OPERATING_SYSTEM: return "CUDA_ERROR_OPERATING_SYSTEM";
        case CUDA_ERROR_INVALID_HANDLE: return "CUDA_ERROR_INVALID_HANDLE";
        case CUDA_ERROR_NOT_FOUND: return "CUDA_ERROR_NOT_FOUND";
        case CUDA_ERROR_NOT_READY: return "CUDA_ERROR_NOT_READY";
        case CUDA_ERROR_LAUNCH_OUT_OF_RESOURCES: return "CUDA_ERROR_LAUNCH_OUT_OF_RESOURCES";
        case CUDA_ERROR_LAUNCH_TIMEOUT: return "CUDA_ERROR_LAUNCH_TIMEOUT";
        case CUDA_ERROR_LAUNCH_INCOMPATIBLE_TEXTURING: return "CUDA_ERROR_LAUNCH_INCOMPATIBLE_TEXTURING";
        case CUDA_ERROR_PEER_ACCESS_ALREADY_ENABLED: return "CUDA_ERROR_PEER_ACCESS_ALREADY_ENABLED";
        case CUDA_ERROR_PEER_ACCESS_NOT_ENABLED: return "CUDA_ERROR_PEER_ACCESS_NOT_ENABLED";
        case CUDA_ERROR_PRIMARY_CONTEXT_ACTIVE: return "CUDA_ERROR_PRIMARY_CONTEXT_ACTIVE";
        case CUDA_ERROR_CONTEXT_IS_DESTROYED: return "CUDA_ERROR_CONTEXT_IS_DESTROYED";
        case CUDA_ERROR_LAUNCH_FAILED: return "CUDA_ERROR_LAUNCH_FAILED";
        case CUDA_ERROR_UNKNOWN: return "CUDA_ERROR_UNKNOWN";
        default:
            return "CUDA_ERROR_UNKNOWN";
    }
    return "<unknown>";
}
#endif //__cuda_cuda_h__


//from helper_cuda
// Beginning of GPU Architecture definitions
inline int _ConvertSMVer2Cores(int major, int minor)
{
    // Defines for GPU Architecture types (using the SM version to determine the # of cores per SM
    typedef struct
    {
        int SM; // 0xMm (hexidecimal notation), M = SM Major version, and m = SM minor version
        int Cores;
    } sSMtoCores;

    sSMtoCores nGpuArchCoresPerSM[] =
    {
        { 0x10,  8 }, // Tesla Generation (SM 1.0) G80 class
        { 0x11,  8 }, // Tesla Generation (SM 1.1) G8x class
        { 0x12,  8 }, // Tesla Generation (SM 1.2) G9x class
        { 0x13,  8 }, // Tesla Generation (SM 1.3) GT200 class
        { 0x20, 32 }, // Fermi Generation (SM 2.0) GF100 class
        { 0x21, 48 }, // Fermi Generation (SM 2.1) GF10x class
        { 0x30, 192}, // Kepler Generation (SM 3.0) GK10x class
        { 0x32, 192}, // Kepler Generation (SM 3.2) GK10x class
        { 0x35, 192}, // Kepler Generation (SM 3.5) GK11x class
        { 0x37, 192}, // Kepler Generation (SM 3.7) GK21x class
        { 0x50, 128}, // Maxwell Generation (SM 5.0) GM10x class
        { 0x52, 128}, // Maxwell Generation (SM 5.2) GM20x class //enc: h264 yuv444p, hevc (libavcodec/nvenc.c)
        { 0x53, 128}, // Maxwell Generation (SM 5.3) GM20x class
        { 0x60, 64 }, // Pascal Generation (SM 6.0) GP100 class
        { 0x61, 128}, // Pascal Generation (SM 6.1) GP10x class
        { 0x62, 128}, // Pascal Generation (SM 6.2) GP10x class
        { 0x70, 64 }, // Volta Generation (SM 7.0) GV100 class
        {   -1, -1 }
    };
    int index = 0;
    while (nGpuArchCoresPerSM[index].SM != -1) {
        if (nGpuArchCoresPerSM[index].SM == ((major << 4) + minor)) {
            return nGpuArchCoresPerSM[index].Cores;
        }
        ++index;
    }
    // If we don't find the values, we default use the previous one to run properly
    printf("MapSMtoCores for SM %d.%d is undefined.  Default to use %d Cores/SM\n", major, minor, nGpuArchCoresPerSM[index-1].Cores);
    return nGpuArchCoresPerSM[index - 1].Cores;
}

// end of GPU Architecture definitions
