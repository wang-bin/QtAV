/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2018 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
******************************************************************************/

#include "QtAV/VideoDecoder.h"
#include "QtAV/private/AVDecoder_p.h"
#include "QtAV/private/factory.h"
#include "QtAV/private/mkid.h"
#include "utils/Logger.h"

namespace QtAV {
FACTORY_DEFINE(VideoDecoder)

VideoDecoderId VideoDecoderId_FFmpeg = mkid::id32base36_6<'F', 'F', 'm', 'p', 'e', 'g'>::value;
VideoDecoderId VideoDecoderId_CUDA = mkid::id32base36_4<'C', 'U', 'D', 'A'>::value;
VideoDecoderId VideoDecoderId_DXVA = mkid::id32base36_4<'D', 'X', 'V', 'A'>::value;
VideoDecoderId VideoDecoderId_D3D11 = mkid::id32base36_5<'D','3','D','1','1'>::value;
VideoDecoderId VideoDecoderId_VAAPI = mkid::id32base36_5<'V', 'A', 'A', 'P', 'I'>::value;
VideoDecoderId VideoDecoderId_Cedarv = mkid::id32base36_6<'C', 'e', 'd', 'a', 'r', 'V'>::value;
VideoDecoderId VideoDecoderId_VDA = mkid::id32base36_3<'V', 'D', 'A'>::value;
VideoDecoderId VideoDecoderId_VideoToolbox = mkid::id32base36_5<'V', 'T', 'B', 'o', 'x'>::value;
VideoDecoderId VideoDecoderId_MediaCodec = mkid::id32base36_4<'F','F','M','C'>::value;
VideoDecoderId VideoDecoderId_MMAL = mkid::id32base36_6<'F','F','M', 'M','A', 'L'>::value;
VideoDecoderId VideoDecoderId_QSV = mkid::id32base36_5<'F','F','Q','S', 'V'>::value;
VideoDecoderId VideoDecoderId_CrystalHD = mkid::id32base36_5<'F','F','C','H', 'D'>::value;

static void VideoDecoder_RegisterAll()
{
    static bool called = false;
    if (called)
        return;
    called = true;
    // factory.h does not check whether an id is registered
    if (VideoDecoder::name(VideoDecoderId_FFmpeg)) //registered on load
        return;
    extern bool RegisterVideoDecoderFFmpeg_Man();
    RegisterVideoDecoderFFmpeg_Man();
#if QTAV_HAVE(DXVA)
    extern bool RegisterVideoDecoderDXVA_Man();
    RegisterVideoDecoderDXVA_Man();
#endif //QTAV_HAVE(DXVA)
#if QTAV_HAVE(D3D11VA)
    extern bool RegisterVideoDecoderD3D11_Man();
    RegisterVideoDecoderD3D11_Man();
#endif //QTAV_HAVE(DXVA)
#if QTAV_HAVE(CUDA)
    extern bool RegisterVideoDecoderCUDA_Man();
    RegisterVideoDecoderCUDA_Man();
#endif //QTAV_HAVE(CUDA)
#if QTAV_HAVE(VAAPI)
    extern bool RegisterVideoDecoderVAAPI_Man();
    RegisterVideoDecoderVAAPI_Man();
#endif //QTAV_HAVE(VAAPI)
#if QTAV_HAVE(VIDEOTOOLBOX)
    extern bool RegisterVideoDecoderVideoToolbox_Man();
    RegisterVideoDecoderVideoToolbox_Man();
#endif //QTAV_HAVE(VIDEOTOOLBOX)
#if QTAV_HAVE(VDA)
    extern bool RegisterVideoDecoderVDA_Man();
    RegisterVideoDecoderVDA_Man();
#endif //QTAV_HAVE(VDA)
#if QTAV_HAVE(CEDARV)
    extern bool RegisterVideoDecoderCedarv_Man();
    RegisterVideoDecoderCedarv_Man();
#endif //QTAV_HAVE(CEDARV)
}
// TODO: called in ::create()/next() etc. to ensure registered?
QVector<VideoDecoderId> VideoDecoder::registered()
{
    VideoDecoder_RegisterAll();
    return QVector<VideoDecoderId>::fromStdVector(VideoDecoderFactory::Instance().registeredIds());
}

QStringList VideoDecoder::supportedCodecs()
{
    static QStringList codecs;
    if (!codecs.isEmpty())
        return codecs;
    const AVCodec* c = NULL;
#if AVCODEC_STATIC_REGISTER
    void* it = NULL;
    while ((c = av_codec_iterate(&it))) {
#else
    avcodec_register_all();
    while ((c = av_codec_next(c))) {
#endif
        if (!av_codec_is_decoder(c) || c->type != AVMEDIA_TYPE_VIDEO)
            continue;
        codecs.append(QString::fromLatin1(c->name));
    }
    return codecs;
}

VideoDecoder::VideoDecoder(VideoDecoderPrivate &d):
    AVDecoder(d)
{
}

QString VideoDecoder::name() const
{
    return QLatin1String(VideoDecoder::name(id()));
}
} //namespace QtAV
