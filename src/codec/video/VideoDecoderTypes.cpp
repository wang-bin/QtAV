#include "QtAV/VideoDecoderTypes.h"
#include "QtAV/FactoryDefine.h"
#include "QtAV/private/mkid.h"

namespace QtAV {

VideoDecoderId VideoDecoderId_FFmpeg = mkid32base36_6<'F', 'F', 'm', 'p', 'e', 'g'>::value;
VideoDecoderId VideoDecoderId_CUDA = mkid32base36_4<'C', 'U', 'D', 'A'>::value;
VideoDecoderId VideoDecoderId_DXVA = mkid32base36_4<'D', 'X', 'V', 'A'>::value;
VideoDecoderId VideoDecoderId_VAAPI = mkid32base36_5<'V', 'A', 'A', 'P', 'I'>::value;
VideoDecoderId VideoDecoderId_Cedarv = mkid32base36_6<'C', 'e', 'd', 'a', 'r', 'V'>::value;
VideoDecoderId VideoDecoderId_VDA = mkid32base36_3<'V', 'D', 'A'>::value;

QVector<VideoDecoderId> GetRegistedVideoDecoderIds()
{
    return QVector<VideoDecoderId>::fromStdVector(VideoDecoderFactory::registeredIds());
}

} //namespace QtAV
