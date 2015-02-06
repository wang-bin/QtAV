/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2015 Wang Bin <wbsecg1@gmail.com>

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
#include <stdint.h> //intptr_t
#include <emmintrin.h>

#ifndef STREAM_LOAD_SI128
#define STREAM_LOAD_SI128(x) _mm_load_si128(x)
#endif //STREAM_LOAD_SI128

#define CACHED_BUFFER_SIZE 4096
#define UINT unsigned int

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
