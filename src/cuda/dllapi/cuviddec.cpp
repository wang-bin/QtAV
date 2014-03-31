#define __CUVID_INTERNAL //avoid replaced bt 64 api

#include "cuviddec.h"

#include "dllapi_p.h"
#include "dllapi.h"

using namespace DllAPI;
//namespace dllapi {
//namespace cuvid {

DEFINE_DLL_INSTANCE_N("cuviddec", "nvcuvid", NULL)

DEFINE_DLLAPI_ARG(2, CUresult, cuvidCreateDecoder, CUvideodecoder *, CUVIDDECODECREATEINFO *)
DEFINE_DLLAPI_ARG(1, CUresult, cuvidDestroyDecoder, CUvideodecoder)
DEFINE_DLLAPI_ARG(2, CUresult, cuvidDecodePicture, CUvideodecoder, CUVIDPICPARAMS *)
DEFINE_DLLAPI_ARG(5, CUresult, cuvidMapVideoFrame, CUvideodecoder, int, unsigned int *, unsigned int *, CUVIDPROCPARAMS *)
DEFINE_DLLAPI_ARG(5, CUresult, cuvidMapVideoFrame64, CUvideodecoder, int, unsigned long long *, unsigned int *, CUVIDPROCPARAMS *)
DEFINE_DLLAPI_ARG(2, CUresult, cuvidUnmapVideoFrame, CUvideodecoder, unsigned int)
DEFINE_DLLAPI_ARG(2, CUresult, cuvidUnmapVideoFrame64, CUvideodecoder, unsigned long long)
DEFINE_DLLAPI_ARG(3, CUresult, cuvidGetVideoFrameSurface, CUvideodecoder, int, void **)
DEFINE_DLLAPI_ARG(2, CUresult, cuvidCtxLockCreate, CUvideoctxlock *, CUcontext)
DEFINE_DLLAPI_ARG(1, CUresult, cuvidCtxLockDestroy, CUvideoctxlock)
DEFINE_DLLAPI_ARG(2, CUresult, cuvidCtxLock, CUvideoctxlock, unsigned int)
DEFINE_DLLAPI_ARG(2, CUresult, cuvidCtxUnlock, CUvideoctxlock, unsigned int)

//} //namespace cuvid
//} //namespace dllapi
