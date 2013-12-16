#include "QtAV/VideoDecoderTypes.h"
#include "QtAV/FactoryDefine.h"

namespace QtAV {

VideoDecoderId VideoDecoderId_FFmpeg = 1;
VideoDecoderId VideoDecoderId_CUDA = 2;
VideoDecoderId VideoDecoderId_DXVA = 3;
VideoDecoderId VideoDecoderId_VAAPI = 4;
VideoDecoderId VideoDecoderId_Cedarv = 5;

QVector<VideoDecoderId> GetRegistedVideoDecoderIds()
{
    return QVector<VideoDecoderId>::fromStdVector(VideoDecoderFactory::registeredIds());
}

} //namespace QtAV
