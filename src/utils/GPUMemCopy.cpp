/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2015 Wang Bin <wbsecg1@gmail.com>

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

#include "GPUMemCopy.h"
#include "QtAV/QtAV_Global.h"
#include <algorithm>
#include <stdlib.h> //posix_memalign osx
#ifdef __MINGW32__
#include <malloc.h> //__mingw_aligned_malloc
#endif
extern "C" {
#include <libavutil/cpu.h>
}

/* Branch prediction */
#ifdef __GNUC__
#   define likely(p)   __builtin_expect(!!(p), 1)
#   define unlikely(p) __builtin_expect(!!(p), 0)
#else
#   define likely(p)   (!!(p))
#   define unlikely(p) (!!(p))
#endif

// read qsimd_p.h
#define UINT unsigned int
void CopyFrame_SSE2(void *pSrc, void *pDest, void *pCacheBlock, UINT width, UINT height, UINT pitch);
void CopyFrame_SSE4(void *pSrc, void *pDest, void *pCacheBlock, UINT width, UINT height, UINT pitch);

namespace QtAV {

#if QTAV_HAVE(SSE2) //FIXME
// from vlc_common.h begin
#ifdef __MINGW32__
# define Memalign(align, size) (__mingw_aligned_malloc(size, align))
# define Free(base)            (__mingw_aligned_free(base))
#elif defined(_MSC_VER)
# define Memalign(align, size) (_aligned_malloc(size, align))
# define Free(base)            (_aligned_free(base))
#elif defined(__APPLE__) && !defined(MAC_OS_X_VERSION_10_6)
static inline void *Memalign(size_t align, size_t size)
{
    long diff;
    void *ptr;

    ptr = malloc(size+align);
    if(!ptr)
        return ptr;
    diff = ((-(long)ptr - 1)&(align-1)) + 1;
    ptr  = (char*)ptr + diff;
    ((char*)ptr)[-1]= diff;
    return ptr;
}

static void Free(void *ptr)
{
    if (ptr)
        free((char*)ptr - ((char*)ptr)[-1]);
}
#else
static inline void *Memalign(size_t align, size_t size)
{
    void *base;
    if (unlikely(posix_memalign(&base, align, size)))
        base = NULL;
    return base;
}
# define Free(base) free(base)
#endif
#endif //QTAV_HAVE(SSE2)

// from vlc_common.h end

// from https://software.intel.com/en-us/articles/copying-accelerated-video-decode-frame-buffers
/*
 * 1. Fill a 4K byte cached (WB) memory buffer from the USWC video frame
 * 2. Copy the 4K byte cache contents to the destination WB frame
 * 3. Repeat steps 1 and 2 until the whole frame buffer has been copied.
 *
 * _mm_store_si128 and _mm_load_si128 intrinsics will compile to the MOVDQA instruction, _mm_stream_load_si128 and _mm_stream_si128 intrinsics compile to the MOVNTDQA and MOVNTDQ instructions
 *
 *  using the same pitch (which is assumed to be a multiple of 64 bytes), and expecting 64 byte alignment of every row of the source, cached 4K buffer and destination buffers.
 * The MOVNTDQA streaming load instruction and the MOVNTDQ streaming store instruction require at least 16 byte alignment in their memory addresses.
 */
//  CopyFrame()
//
//  COPIES VIDEO FRAMES FROM USWC MEMORY TO WB SYSTEM MEMORY VIA CACHED BUFFER
//    ASSUMES PITCH IS A MULTIPLE OF 64B CACHE LINE SIZE, WIDTH MAY NOT BE


bool detect_sse4() {
    static bool is_sse4 = !!(av_get_cpu_flags() & AV_CPU_FLAG_SSE4);
    return is_sse4;
}
bool detect_sse2() {
    static bool is_sse2 = !!(av_get_cpu_flags() & AV_CPU_FLAG_SSE2);
    return is_sse2;
}

bool GPUMemCopy::isAvailable()
{
#if QTAV_HAVE(SSE4_1)
    if (detect_sse4())
        return true;
#endif
#if QTAV_HAVE(SSE2)
    if (detect_sse2())
        return true;
#endif
    return false;
}

GPUMemCopy::GPUMemCopy()
    : mInitialized(false)
{
#if QTAV_HAVE(SSE2)
    mCache.buffer = 0;
    mCache.size = 0;
#endif
}

GPUMemCopy::~GPUMemCopy()
{
    cleanCache();
}

bool GPUMemCopy::isReady() const
{
    return mInitialized && GPUMemCopy::isAvailable();
}

#define CACHED_BUFFER_SIZE 4096
bool GPUMemCopy::initCache(unsigned width)
{
    mInitialized = false;
#if QTAV_HAVE(SSE2)
    mCache.size = std::max<size_t>((width + 0x0f) & ~ 0x0f, CACHED_BUFFER_SIZE);
    mCache.buffer = (unsigned char*)Memalign(16, mCache.size);
    mInitialized = !!mCache.buffer;
    return mInitialized;
#else
    Q_UNUSED(width);
#endif
    return false;
}


void GPUMemCopy::cleanCache()
{
    mInitialized = false;
#if QTAV_HAVE(SSE2)
    if (mCache.buffer) {
        Free(mCache.buffer);
    }
    mCache.buffer = 0;
    mCache.size = 0;
#endif
}

void GPUMemCopy::copyFrame(void *pSrc, void *pDest, unsigned width, unsigned height, unsigned pitch)
{
#if QTAV_HAVE(SSE4_1)
    if (detect_sse4())
        CopyFrame_SSE4(pSrc, pDest, mCache.buffer, width, height, pitch);
#elif QTAV_HAVE(SSE2)
    if (detect_sse2())
        CopyFrame_SSE2(pSrc, pDest, mCache.buffer, width, height, pitch);
#else
    Q_UNUSED(pSrc);
    Q_UNUSED(pDest);
    Q_UNUSED(width);
    Q_UNUSED(height);
    Q_UNUSED(pitch);
#endif
}
} //namespace QtAV
