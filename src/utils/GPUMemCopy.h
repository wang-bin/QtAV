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

    bool initCache(unsigned int width);
    void cleanCache();
    void copyFrame(void *pSrc, void *pDest, unsigned int width, unsigned int height, unsigned int pitch);
    //memcpy
private:
    typedef struct {
        unsigned char* buffer;
        size_t size;
    } cache_t;
    cache_t mCache;
};

} //namespace QtAV

#endif // GPUMemCopy_H
