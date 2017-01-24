/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2017 Wang Bin <wbsecg1@gmail.com>

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

#ifndef NV_INC_H
#define NV_INC_H

#undef NV_CONFIG
#define NV_CONFIG(FEATURE) (defined QTAV_HAVE_##FEATURE && QTAV_HAVE_##FEATURE)

// TODO: remove DLLAPI_CUDA code
// high version will define cuXXX macro, so functions here will be not they look like
#if !NV_CONFIG(DLLAPI_CUDA) && !defined(CUDA_LINK)
//#define CUDA_FORCE_API_VERSION 3010 //enable this line to build with old cuda APIs
///#define __CUDA_API_VERSION_INTERNAL
#endif

#if NV_CONFIG(DLLAPI_CUDA)
namespace dllapi {
namespace cuda {
#endif /*NV_CONFIG(DLLAPI_CUDA)*/
#if defined(__cplusplus)
//extern "C" {
#endif /* __cplusplus */

#ifdef HAVE_CUDA_H
#include "cuda.h"
#include "nvcuvid.h"
#else
#include "dynlink_cuda.h"
#include "dynlink_nvcuvid.h"
#endif
// __CUDA_API_VERSION is undefined in cuda.h
#ifdef CUDA_FORCE_API_VERSION
#define __CUDA_API_VERSION CUDA_FORCE_API_VERSION
#else
#ifndef __CUDA_API_VERSION
#define __CUDA_API_VERSION 4000
#endif
#endif
#if defined(__cplusplus)
//}
#endif /* __cplusplus */
#if NV_CONFIG(DLLAPI_CUDA)
} //namespace cuda
} //namespace dllapi
#endif /*NV_CONFIG(DLLAPI_CUDA)*/
#endif /* NV_INC_H*/
