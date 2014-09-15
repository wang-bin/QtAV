#include "SurfaceInterop_CUDA_GL.h"
#include "cuda_gl_interop.h"
namespace QtAV {
namespace cuda {

//cuda<5.0: cuGLCtxCreate to get max perf

SurfaceInterop_CUDA_GL::SurfaceInterop_CUDA_GL()
{
    memset(tex_res, 0, sizeof(tex_res));
}

SurfaceInterop_CUDA_GL::~SurfaceInterop_CUDA_GL()
{
    for (int i = 0; i < 2; ++i) {
        if (tex_res[i].res)
            cuGraphicsUnregisterResource(tex_res[i].res);
    }
}

bool SurfaceInterop_CUDA_GL::registerTextureResource(GLuint tex, int plane)
{
    TextureRes &t_r(tex_res[plane]);
    if (t_r.tex == tex)
        return true;
    // register only once
    if (t_r.tex) {
        checkCudaErrors(cuGraphicsUnregisterResource(t_r.res));
    }
    checkCudaErrors(cuGraphicsGLRegisterImage(&t_r.res, tex, GL_TEXTURE_2D, CU_GRAPHICS_REGISTER_FLAGS_NONE)); //WO?
    t_r.tex = tex;
    return true;
}

void* SurfaceInterop_CUDA_GL::map(SurfaceType type, const VideoFormat& fmt, void* handle, int plane)
{
    if (type != GLTextureSurface)
        return 0;
    Q_ASSERT(plane <= 1); //NV12
    Q_ASSERT(handle); //
    if (!registerTextureResource(*((GLuint*)handle), plane))
        return 0;
            //cuGraphicsSubResourceGetMappedPointer(CUdevicePtr) //>=0302
    CUarray array;
    CUresult r = cuGraphicsSubResourceGetMappedArray(&array, tex_res[plane].res, 0/*arrayIndex*/, 0 /*mipLevel*/);
    if (r != CUDA_SUCCESS)
        return 0;
    CUDA_MEMCPY2D cp2d;
    memset(&cp2d, 0, sizeof(cp2d));

    r = cuMemcpy2D(&cp2d);
    if (r != CUDA_SUCCESS)
        return 0;
    //cuGraphicsUnmapResources(1, tex_res[plane].res, 0);
    return handle;
}

void SurfaceInterop_CUDA_GL::unmap(void *handle)
{
    CUgraphicsResource *res = 0;
    for (int i = 0; i < 2; ++i) {
        if (tex_res[i].tex == *((GLuint*)handle)) {
            res = &tex_res[i].res;
            break;
        }
    }
    if (!res)
        return;
    cuGraphicsUnmapResources(1, res, 0);
}

} //namespace cuda
} //namespace QtAV


