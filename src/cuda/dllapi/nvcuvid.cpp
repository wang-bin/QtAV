#include "nvcuvid.h"
#include "dllapi_p.h"
#include "dllapi.h"

using namespace DllAPI;
//namespace dllapi {
//namespace cuvid {

DEFINE_DLL_INSTANCE_N("nvcuvid", "nvcuvid", NULL)

DEFINE_DLLAPI_ARG(3, CUresult, cuvidCreateVideoSource, CUvideosource *, const char *, CUVIDSOURCEPARAMS *)
DEFINE_DLLAPI_ARG(3, CUresult, cuvidCreateVideoSourceW, CUvideosource *, const int *, CUVIDSOURCEPARAMS *)
DEFINE_DLLAPI_ARG(1, CUresult, cuvidDestroyVideoSource, CUvideosource)
DEFINE_DLLAPI_ARG(2, CUresult, cuvidSetVideoSourceState, CUvideosource, cudaVideoState)
DEFINE_DLLAPI_ARG(1, cudaVideoState, cuvidGetVideoSourceState, CUvideosource)
DEFINE_DLLAPI_ARG(3, CUresult, cuvidGetSourceVideoFormat, CUvideosource, CUVIDEOFORMAT *, unsigned int)
DEFINE_DLLAPI_ARG(3, CUresult, cuvidGetSourceAudioFormat, CUvideosource, CUAUDIOFORMAT *, unsigned int)
DEFINE_DLLAPI_ARG(2, CUresult, cuvidCreateVideoParser, CUvideoparser *, CUVIDPARSERPARAMS *)
DEFINE_DLLAPI_ARG(2, CUresult, cuvidParseVideoData, CUvideoparser, CUVIDSOURCEDATAPACKET *)
DEFINE_DLLAPI_ARG(1, CUresult, cuvidDestroyVideoParser, CUvideoparser)

//} //namespace cuvid
//} //namespace dllapi
