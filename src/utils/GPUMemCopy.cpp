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

#include "GPUMemCopy.h"

#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __MINGW32__
#include <malloc.h> //__mingw_aligned_malloc
#endif
#include <algorithm>

#include "QtAV/private/AVCompat.h"

// for mingw gcc
#if QTAV_HAVE(SSE4_1)
#include <smmintrin.h> //stream load
#endif

/* Branch prediction */
#ifdef __GNUC__
#   define likely(p)   __builtin_expect(!!(p), 1)
#   define unlikely(p) __builtin_expect(!!(p), 0)
#else
#   define likely(p)   (!!(p))
#   define unlikely(p) (!!(p))
#endif

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

#define CACHED_BUFFER_SIZE 4096
typedef unsigned int UINT;
void CopyGPUFrame_SSE4_1(void *pSrc, void *pDest, void * pCacheBlock, UINT width, UINT height, UINT pitch);

bool GPUMemCopy::isAvailable()
{
#ifdef __SSE4_1__
    return true;
#endif
#if QTAV_HAVE(SSE4_1)
    return true;
#endif
    static bool is_sse41 = !!(av_get_cpu_flags() & AV_CPU_FLAG_SSE4);
    return is_sse41;
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

bool GPUMemCopy::initCache(unsigned width)
{
    mInitialized = false;
#if QTAV_HAVE(SSE2)
    mCache.size = std::max<size_t>((width + 0x0f) & ~ 0x0f, CACHED_BUFFER_SIZE);
    mCache.buffer = (unsigned char*)Memalign(16, mCache.size);
    mInitialized = !!mCache.buffer;
    return mInitialized;
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
    CopyGPUFrame_SSE4_1(pSrc, pDest, mCache.buffer, width, height, pitch);
}

void CopyGPUFrame_SSE4_1(void *pSrc, void *pDest, void *pCacheBlock, UINT width, UINT height, UINT pitch)
{
#if QTAV_HAVE(SSE4_1)
    //assert(((intptr_t)pCacheBlock & 0x0f) == 0 && (dst_pitch & 0x0f) == 0);
    __m128i		x0, x1, x2, x3;
    __m128i		*pLoad;
    __m128i		*pStore;
    __m128i		*pCache;
    UINT		x, y, yLoad, yStore;
    UINT		rowsPerBlock;
    UINT		width64;
    UINT		extraPitch;

    rowsPerBlock = CACHED_BUFFER_SIZE / pitch;
    width64 = (width + 63) & ~0x03f;
    extraPitch = (pitch - width64) / 16;

    pLoad  = (__m128i *)pSrc;
    pStore = (__m128i *)pDest;

    const bool src_unaligned = ((intptr_t)pSrc) & 0x0f;
    const bool dst_unaligned = ((intptr_t)pDest & 0x0f);
    //if (src_unaligned || dst_unaligned)
      //  qDebug("===========unaligned: src %d, dst: %d,  extraPitch: %d", src_unaligned, dst_unaligned, extraPitch);
    //  COPY THROUGH 4KB CACHED BUFFER
    for (y = 0; y < height; y += rowsPerBlock) {
        //  ROWS LEFT TO COPY AT END
        if (y + rowsPerBlock > height)
            rowsPerBlock = height - y;

        pCache = (__m128i *)pCacheBlock;

        _mm_mfence();

        // LOAD ROWS OF PITCH WIDTH INTO CACHED BLOCK
        for (yLoad = 0; yLoad < rowsPerBlock; yLoad++) {
            // COPY A ROW, CACHE LINE AT A TIME
            for (x = 0; x < pitch; x +=64) {
                // movntdqa
                x0 = _mm_stream_load_si128(pLoad + 0);
                x1 = _mm_stream_load_si128(pLoad + 1);
                x2 = _mm_stream_load_si128(pLoad + 2);
                x3 = _mm_stream_load_si128(pLoad + 3);

                if (src_unaligned) {
                    // movdqu
                    _mm_storeu_si128(pCache +0, x0);
                    _mm_storeu_si128(pCache +1, x1);
                    _mm_storeu_si128(pCache +2, x2);
                    _mm_storeu_si128(pCache +3, x3);
                } else {
                    // movdqa
                    _mm_store_si128(pCache +0, x0);
                    _mm_store_si128(pCache +1, x1);
                    _mm_store_si128(pCache +2, x2);
                    _mm_store_si128(pCache +3, x3);
                }
                pCache += 4;
                pLoad += 4;
            }
        }

        _mm_mfence();

        pCache = (__m128i *)pCacheBlock;
        // STORE ROWS OF FRAME WIDTH FROM CACHED BLOCK
        for (yStore = 0; yStore < rowsPerBlock; yStore++) {
            // copy a row, cache line at a time
            for (x = 0; x < width64; x += 64) {
                // movdqa
                x0 = _mm_load_si128(pCache);
                x1 = _mm_load_si128(pCache + 1);
                x2 = _mm_load_si128(pCache + 2);
                x3 = _mm_load_si128(pCache + 3);

                if (dst_unaligned) {
                    // movdqu
                    _mm_storeu_si128(pStore,	x0);
                    _mm_storeu_si128(pStore + 1, x1);
                    _mm_storeu_si128(pStore + 2, x2);
                    _mm_storeu_si128(pStore + 3, x3);
                } else {
                    // movntdq
                    _mm_stream_si128(pStore,	x0);
                    _mm_stream_si128(pStore + 1, x1);
                    _mm_stream_si128(pStore + 2, x2);
                    _mm_stream_si128(pStore + 3, x3);
                }
                pCache += 4;
                pStore += 4;
            }
            pCache += extraPitch;
            pStore += extraPitch;
        }
    }
#else
    Q_UNUSED(pSrc);
    Q_UNUSED(pDest);
    Q_UNUSED(pCacheBlock);
    Q_UNUSED(width);
    Q_UNUSED(height);
    Q_UNUSED(pitch);
#endif //QTAV_HAVE(SSE4_1)
}

} //namespace QtAV
