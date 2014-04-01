#define __CUVID_INTERNAL //avoid replaced bt 64 api

#include "cuviddec.h"

#include "dllapi_p.h"
#include "dllapi.h"

using namespace dllapi;
namespace dllapi {
namespace cuviddec {

//DEFINE_DLL_INSTANCE_N("cuviddec", "nvcuvid", NULL)
static char* cuvid_names[] = { "nvcuvid", NULL };
DEFINE_DLL_INSTANCE_V("cuviddec", cuvid_names)

DEFINE_DLLAPI_M_ARG(2, CUresult, CUDAAPI, cuvidCreateDecoder, CUvideodecoder *, CUVIDDECODECREATEINFO *)
DEFINE_DLLAPI_M_ARG(1, CUresult, CUDAAPI, cuvidDestroyDecoder, CUvideodecoder)
DEFINE_DLLAPI_M_ARG(2, CUresult, CUDAAPI, cuvidDecodePicture, CUvideodecoder, CUVIDPICPARAMS *)
DEFINE_DLLAPI_M_ARG(5, CUresult, CUDAAPI, cuvidMapVideoFrame, CUvideodecoder, int, unsigned int *, unsigned int *, CUVIDPROCPARAMS *)
DEFINE_DLLAPI_M_ARG(5, CUresult, CUDAAPI, cuvidMapVideoFrame64, CUvideodecoder, int, unsigned long long *, unsigned int *, CUVIDPROCPARAMS *)
DEFINE_DLLAPI_M_ARG(2, CUresult, CUDAAPI, cuvidUnmapVideoFrame, CUvideodecoder, unsigned int)
DEFINE_DLLAPI_M_ARG(2, CUresult, CUDAAPI, cuvidUnmapVideoFrame64, CUvideodecoder, unsigned long long)
DEFINE_DLLAPI_M_ARG(3, CUresult, CUDAAPI, cuvidGetVideoFrameSurface, CUvideodecoder, int, void **)
DEFINE_DLLAPI_M_ARG(2, CUresult, CUDAAPI, cuvidCtxLockCreate, CUvideoctxlock *, CUcontext)
DEFINE_DLLAPI_M_ARG(1, CUresult, CUDAAPI, cuvidCtxLockDestroy, CUvideoctxlock)
DEFINE_DLLAPI_M_ARG(2, CUresult, CUDAAPI, cuvidCtxLock, CUvideoctxlock, unsigned int)
DEFINE_DLLAPI_M_ARG(2, CUresult, CUDAAPI, cuvidCtxUnlock, CUvideoctxlock, unsigned int)

} //namespace cuviddec
} //namespace dllapi
