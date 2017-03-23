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
#if defined(__SSE__) || defined(_M_IX86) || defined(_M_X64) // gcc, clang defines __SSE__, vc does not
// for mingw gcc
#include <smmintrin.h> //stream load
#include <stdint.h> //intptr_t
#include <string.h>
#define STREAM_LOAD_SI128(x) _mm_stream_load_si128(x)
namespace sse4 { //avoid name conflict
#define INC_FROM_NAMESPACE
#include "CopyFrame_SSE2.cpp"
} //namespace sse4

//QT_FUNCTION_TARGET("sse4.1")
void CopyFrame_SSE4(void *pSrc, void *pDest, void *pCacheBlock, UINT width, UINT height, UINT pitch)
{
    sse4::CopyFrame_SSE2(pSrc, pDest, pCacheBlock, width, height, pitch);
}

void *memcpy_sse4(void* dst, const void* src, size_t size)
{
    return sse4::memcpy_sse2(dst, src, size);
}
#endif
