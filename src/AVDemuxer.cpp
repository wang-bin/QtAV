#include <QtAV/AVDemuxer.h>
#ifdef __cplusplus
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}
#endif //__cplusplus

#include <QtAV/QAVPacket.h>

namespace QtAV {
AVDemuxer::AVDemuxer()
    :formatCtx(0)
{
}


bool AVDemuxer::readPacket(QAVPacket *packet, int stream)
{
    AVPacket pkt;
    int ret = av_read_frame(formatCtx, &pkt);

    if (ret == AVERROR(EAGAIN))
        return true;
    else if (ret)
        return false;
 /*   idx = pkt.stream_index;
    if (idx >= _streams.count())
    {
        loadStreams();
        if ( idx >= _streams.count() )
            return false;
    }
*/
    packet->data = QByteArray((const char *)pkt.data, pkt.size);
#if 0
    AVStream *&stream = ( AVStream *& )_streams[ idx ];

//TODO: seek by byte?
    if (pkt.dts == AV_NOPTS_VALUE && pkt.pts != AV_NOPTS_VALUE)
        packet->pts = pkt.pts;
    else if (pkt.dts != AV_NOPTS_VALUE)
        packet->pts = pkt.dts;
    if (packet->pts < 0.)
        packet->pts = 0.;
    packet->pts *= av_q2d(stream->time_base);
    if (packet->pts && packet->pts - start_time >= 0.0 )
        packet->pts -= start_time;

    if (stream->codec->codec_type == AVMEDIA_TYPE_SUBTITLE
            && ( pkt.flags & AV_PKT_FLAG_KEY )
            &&  pkt.convergence_duration != AV_NOPTS_VALUE)
        packet->duration = pkt.convergence_duration * av_q2d(stream->time_base);
    else if (pkt.duration > 0)
        packet->duration = pkt.duration * av_q2d(stream->time_base);
    else
        packet->duration = 0.;
#endif
    return true;
}

}
