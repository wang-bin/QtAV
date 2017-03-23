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
#ifndef INC_FROM_NAMESPACE
#include <stdint.h> //intptr_t
#include <string.h>
#include <emmintrin.h>
#endif

// from https://software.intel.com/en-us/articles/copying-accelerated-video-decode-frame-buffers
// modified by wang-bin to support unaligned src/dest and sse2
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
//
//  COPIES VIDEO FRAMES FROM USWC MEMORY TO WB SYSTEM MEMORY VIA CACHED BUFFER
//    ASSUMES PITCH IS A MULTIPLE OF 64B CACHE LINE SIZE, WIDTH MAY NOT BE
#ifndef STREAM_LOAD_SI128
#define STREAM_LOAD_SI128(x) _mm_load_si128(x)
#endif //STREAM_LOAD_SI128

#define CACHED_BUFFER_SIZE 4096
#define UINT unsigned int

// copy plane
//QT_FUNCTION_TARGET("sse2")
void CopyFrame_SSE2(void *pSrc, void *pDest, void *pCacheBlock, UINT width, UINT height, UINT pitch)
{
    //assert(((intptr_t)pCacheBlock & 0x0f) == 0 && (dst_pitch & 0x0f) == 0);
    __m128i		x0, x1, x2, x3;
    __m128i		*pCache;
    UINT		x, y, yLoad, yStore;

    UINT rowsPerBlock = CACHED_BUFFER_SIZE / pitch;
    const UINT width64 = (width + 63) & ~0x03f;
    const UINT extraPitch = (pitch - width64) / 16;

    __m128i *pLoad  = (__m128i*)pSrc;
    __m128i *pStore = (__m128i*)pDest;

    const bool src_unaligned = !!((intptr_t)pSrc & 0x0f);
    const bool dst_unaligned = !!((intptr_t)pDest & 0x0f);
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
                x0 = STREAM_LOAD_SI128(pLoad + 0);
                x1 = STREAM_LOAD_SI128(pLoad + 1);
                x2 = STREAM_LOAD_SI128(pLoad + 2);
                x3 = STREAM_LOAD_SI128(pLoad + 3);
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
}

//Taken from the QuickSync decoder by Eric Gur
// a memcpy style function that copied data very fast from a GPU tiled memory (write back)
// Performance tip: page offset (12 lsb) of both addresses should be different
//  optimally use a 2K offset between them.
void *memcpy_sse2(void* dst, const void* src, size_t size)
{
    static const size_t kRegsInLoop = sizeof(size_t) * 2; // 8 or 16

    if (!dst || !src)
        return NULL;

    // If memory is not aligned, use memcpy
    // TODO: only check dst aligned
    const bool isAligned = !(((size_t)(src) | (size_t)(dst)) & 0x0F);
    if (!isAligned)
        return memcpy(dst, src, size);

    __m128i xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;
#ifdef __x86_64__
    __m128i xmm8, xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15;
#endif

    size_t reminder = size & (kRegsInLoop * sizeof(xmm0) - 1); // Copy 128 or 256 bytes every loop
    size_t end = 0;

    __m128i* pTrg = (__m128i*)dst;
    __m128i* pTrgEnd = pTrg + ((size - reminder) >> 4);
    __m128i* pSrc = (__m128i*)src;

    // Make sure source is synced - doesn't hurt if not needed.
    _mm_sfence();

    while (pTrg < pTrgEnd) {
        // _mm_stream_load_si128 emits the Streaming SIMD Extensions 4 (SSE4.1) instruction MOVNTDQA
        // Fastest method for copying GPU RAM. Available since Penryn (45nm Core 2 Duo/Quad)
        xmm0  = STREAM_LOAD_SI128(pSrc);
        xmm1  = STREAM_LOAD_SI128(pSrc + 1);
        xmm2  = STREAM_LOAD_SI128(pSrc + 2);
        xmm3  = STREAM_LOAD_SI128(pSrc + 3);
        xmm4  = STREAM_LOAD_SI128(pSrc + 4);
        xmm5  = STREAM_LOAD_SI128(pSrc + 5);
        xmm6  = STREAM_LOAD_SI128(pSrc + 6);
        xmm7  = STREAM_LOAD_SI128(pSrc + 7);
#ifdef __x86_64__ // Use all 16 xmm registers
        xmm8  = STREAM_LOAD_SI128(pSrc + 8);
        xmm9  = STREAM_LOAD_SI128(pSrc + 9);
        xmm10 = STREAM_LOAD_SI128(pSrc + 10);
        xmm11 = STREAM_LOAD_SI128(pSrc + 11);
        xmm12 = STREAM_LOAD_SI128(pSrc + 12);
        xmm13 = STREAM_LOAD_SI128(pSrc + 13);
        xmm14 = STREAM_LOAD_SI128(pSrc + 14);
        xmm15 = STREAM_LOAD_SI128(pSrc + 15);
#endif
        pSrc += kRegsInLoop;
        // _mm_store_si128 emit the SSE2 intruction MOVDQA (aligned store)
        // TODO: why not _mm_stream_si128? it works
        _mm_store_si128(pTrg     , xmm0);
        _mm_store_si128(pTrg +  1, xmm1);
        _mm_store_si128(pTrg +  2, xmm2);
        _mm_store_si128(pTrg +  3, xmm3);
        _mm_store_si128(pTrg +  4, xmm4);
        _mm_store_si128(pTrg +  5, xmm5);
        _mm_store_si128(pTrg +  6, xmm6);
        _mm_store_si128(pTrg +  7, xmm7);
#ifdef __x86_64__ // Use all 16 xmm registers
        _mm_store_si128(pTrg +  8, xmm8);
        _mm_store_si128(pTrg +  9, xmm9);
        _mm_store_si128(pTrg + 10, xmm10);
        _mm_store_si128(pTrg + 11, xmm11);
        _mm_store_si128(pTrg + 12, xmm12);
        _mm_store_si128(pTrg + 13, xmm13);
        _mm_store_si128(pTrg + 14, xmm14);
        _mm_store_si128(pTrg + 15, xmm15);
#endif
        pTrg += kRegsInLoop;
    }

    // Copy in 16 byte steps
    if (reminder >= 16) {
        size = reminder;
        reminder = size & 15;
        end = size >> 4;
        for (size_t i = 0; i < end; ++i)
            pTrg[i] = STREAM_LOAD_SI128(pSrc + i);
    }

    // Copy last bytes - shouldn't happen as strides are modulu 16
    if (reminder) {
        __m128i temp = STREAM_LOAD_SI128(pSrc + end);

        char* ps = (char*)(&temp);
        char* pt = (char*)(pTrg + end);

        for (size_t i = 0; i < reminder; ++i)
            pt[i] = ps[i];
    }

    return dst;
}
#endif
