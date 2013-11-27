#include "QtAV/VideoDecoderTypes.h"
#include "QtAV/FactoryDefine.h"

namespace QtAV {

VideoDecoderId VideoDecoderId_FFmpeg = 0;
VideoDecoderId VideoDecoderId_CUDA = 1;
VideoDecoderId VideoDecoderId_DXVA = 2;

QVector<VideoDecoderId> GetRegistedVideoDecoderIds()
{
    return QVector<VideoDecoderId>::fromStdVector(VideoDecoderFactory::registeredIds());
}

} //namespace QtAV
