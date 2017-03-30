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

#include "GPUMemCopy.h"
#include "QtAV/QtAV_Global.h"
#include <string.h> //memcpy
#include <algorithm>
extern "C" {
#include <libavutil/cpu.h>
}

#ifndef Q_PROCESSOR_X86 // qt4
#if defined(__SSE__) || defined(_M_IX86) || defined(_M_X64)
#define Q_PROCESSOR_X86
#endif
#endif
// read qsimd_p.h
#define UINT unsigned int
void CopyFrame_SSE2(void *pSrc, void *pDest, void *pCacheBlock, UINT width, UINT height, UINT pitch);
void CopyFrame_SSE4(void *pSrc, void *pDest, void *pCacheBlock, UINT width, UINT height, UINT pitch);

void *memcpy_sse2(void* dst, const void* src, size_t size);
void *memcpy_sse4(void* dst, const void* src, size_t size);

namespace QtAV {

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
#if QTAV_HAVE(SSE4_1) && defined(Q_PROCESSOR_X86)
    if (detect_sse4())
        return true;
#endif
#if QTAV_HAVE(SSE2) && defined(Q_PROCESSOR_X86)
    if (detect_sse2())
        return true;
#endif
    return false;
}

GPUMemCopy::GPUMemCopy()
    : mInitialized(false)
{
#if QTAV_HAVE(SSE2) && defined(Q_PROCESSOR_X86)
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
#if QTAV_HAVE(SSE2) && defined(Q_PROCESSOR_X86)
    mCache.size = std::max<size_t>((width + 0x0f) & ~ 0x0f, CACHED_BUFFER_SIZE);
    mCache.buffer = (unsigned char*)qMallocAligned(mCache.size, 16);
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
#if QTAV_HAVE(SSE2) && defined(Q_PROCESSOR_X86)
    if (mCache.buffer) {
        qFreeAligned(mCache.buffer);
    }
    mCache.buffer = 0;
    mCache.size = 0;
#endif
}

void GPUMemCopy::copyFrame(void *pSrc, void *pDest, unsigned width, unsigned height, unsigned pitch)
{
#if QTAV_HAVE(SSE4_1) && defined(Q_PROCESSOR_X86)
    if (detect_sse4())
        CopyFrame_SSE4(pSrc, pDest, mCache.buffer, width, height, pitch);
#elif QTAV_HAVE(SSE2) && defined(Q_PROCESSOR_X86)
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

void* gpu_memcpy(void *dst, const void *src, size_t size)
{
#if QTAV_HAVE(SSE4_1) && defined(Q_PROCESSOR_X86)
    if (detect_sse4())
        return memcpy_sse4(dst, src, size);
#elif QTAV_HAVE(SSE2) && defined(Q_PROCESSOR_X86)
    if (detect_sse2())
        return memcpy_sse2(dst, src, size);
#endif
    return memcpy(dst, src, size);
}
} //namespace QtAV
