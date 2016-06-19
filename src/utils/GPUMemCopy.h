/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

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

#ifndef GPUMemCopy_H
#define GPUMemCopy_H

#include <stddef.h>

namespace QtAV {

class GPUMemCopy
{
public:
    static bool isAvailable();
    GPUMemCopy();
    ~GPUMemCopy();
    // available and initialized
    bool isReady() const;
    bool initCache(unsigned int width);
    void cleanCache();
    void copyFrame(void *pSrc, void *pDest, unsigned int width, unsigned int height, unsigned int pitch);
    //memcpy
private:
    bool mInitialized;
    typedef struct {
        unsigned char* buffer;
        size_t size;
    } cache_t;
    cache_t mCache;
};

void* gpu_memcpy(void* dst, const void* src, size_t size);

} //namespace QtAV

#endif // GPUMemCopy_H
