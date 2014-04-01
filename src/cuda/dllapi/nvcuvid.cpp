#include "nvcuvid.h"
#include "dllapi_p.h"
#include "dllapi.h"

using namespace dllapi;
//namespace dllapi {
//namespace cuvid {

DEFINE_DLL_INSTANCE_N("nvcuvid", "nvcuvid", NULL)

DEFINE_DLLAPI_M_ARG(3, CUresult, CUDAAPI, cuvidCreateVideoSource, CUvideosource *, const char *, CUVIDSOURCEPARAMS *)
DEFINE_DLLAPI_M_ARG(3, CUresult, CUDAAPI, cuvidCreateVideoSourceW, CUvideosource *, const int *, CUVIDSOURCEPARAMS *)
DEFINE_DLLAPI_M_ARG(1, CUresult, CUDAAPI, cuvidDestroyVideoSource, CUvideosource)
DEFINE_DLLAPI_M_ARG(2, CUresult, CUDAAPI, cuvidSetVideoSourceState, CUvideosource, cudaVideoState)
DEFINE_DLLAPI_M_ARG(1, cudaVideoState, CUDAAPI, cuvidGetVideoSourceState, CUvideosource)
DEFINE_DLLAPI_M_ARG(3, CUresult, CUDAAPI, cuvidGetSourceVideoFormat, CUvideosource, CUVIDEOFORMAT *, unsigned int)
DEFINE_DLLAPI_M_ARG(3, CUresult, CUDAAPI, cuvidGetSourceAudioFormat, CUvideosource, CUAUDIOFORMAT *, unsigned int)
DEFINE_DLLAPI_M_ARG(2, CUresult, CUDAAPI, cuvidCreateVideoParser, CUvideoparser *, CUVIDPARSERPARAMS *)
DEFINE_DLLAPI_M_ARG(2, CUresult, CUDAAPI, cuvidParseVideoData, CUvideoparser, CUVIDSOURCEDATAPACKET *)
DEFINE_DLLAPI_M_ARG(1, CUresult, CUDAAPI, cuvidDestroyVideoParser, CUvideoparser)

//} //namespace cuvid
//} //namespace dllapi
