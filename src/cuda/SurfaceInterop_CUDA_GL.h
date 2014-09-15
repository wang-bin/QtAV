#ifndef QTAV_SURFACEINTEROP_CUDA_GL_H
#define QTAV_SURFACEINTEROP_CUDA_GL_H

#include <QtCore/QMap>
#include <QtCore/QSharedPointer>
#include "helper_cuda.h"
#include "cuda_api.h"
#include "QtAV/SurfaceInterop.h"

namespace QtAV {
namespace cuda {

class SurfaceInterop_CUDA_GL : public VideoSurfaceInterop, public cuda_api
{
public:
    SurfaceInterop_CUDA_GL();
    ~SurfaceInterop_CUDA_GL();

    bool registerTextureResource(GLuint tex, int plane);
    virtual void* map(SurfaceType type, const VideoFormat& fmt, void* handle, int plane);
    virtual void unmap(void *handle);

private:
    typedef struct {
        GLuint tex;
        CUgraphicsResource res;
    } TextureRes;
    TextureRes tex_res[2];
    // CUDA_MEMCPY2D cp2d;
    CUdeviceptr src_dev;
};
typedef QSharedPointer<SurfaceInterop_CUDA_GL> SurfaceInterop_CUDA_GL_Ptr;

} //namespace cuda
} //namespace QtAV
#endif // QTAV_SURFACEINTEROP_CUDA_GL_H
