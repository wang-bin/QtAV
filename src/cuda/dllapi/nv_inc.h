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

#ifndef NV_INC_H
#define NV_INC_H

#undef NV_CONFIG
#define NV_CONFIG(FEATURE) (defined QTAV_HAVE_##FEATURE && QTAV_HAVE_##FEATURE)

#if NV_CONFIG(DLLAPI_CUDA)
namespace dllapi {
namespace cuda {
#endif /*NV_CONFIG(DLLAPI_CUDA)*/
#if defined(__cplusplus)
//extern "C" {
#endif /* __cplusplus */

#include "cuda.h"
#include "nvcuvid.h"

#if defined(__cplusplus)
//}
#endif /* __cplusplus */
#if NV_CONFIG(DLLAPI_CUDA)
} //namespace cuda
} //namespace dllapi
#endif /*NV_CONFIG(DLLAPI_CUDA)*/

#endif /* NV_INC_H*/
