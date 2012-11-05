/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012 Wang Bin <wbsecg1@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include <QtAV/AVPlayer.h>

#include <qevent.h>
#include <qpainter.h>
#include <QApplication>

#include <QtAV/AVDemuxer.h>
#include <private/VideoRenderer_p.h>
#include <QtAV/AudioOutput.h>
#include <QtAV/AudioThread.h>
#include <QtAV/Packet.h>
#include <QtAV/AudioDecoder.h>
#include <QtAV/VideoRenderer.h>
#include <QtAV/AVClock.h>
#include <QtAV/VideoDecoder.h>
#include <QtAV/WidgetRenderer.h>
#include <QtAV/VideoThread.h>
#include <QtAV/AVDemuxThread.h>

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#ifdef __cplusplus
}
#endif //__cplusplus

namespace QtAV {

AVPlayer::AVPlayer(QObject *parent) :
    QObject(parent),renderer(0),audio(0)
{
    avTimerId = -1;
    clock = new AVClock(AVClock::AudioClock);
    audio = new AudioOutput();
    audio_dec = new AudioDecoder();
    audio_thread = new AudioThread(this);
    audio_thread->setClock(clock);
    //audio_thread->setPacketQueue(&audio_queue);
    audio_thread->setDecoder(audio_dec);
    audio_thread->setOutput(audio);

    video_dec = new VideoDecoder();

    video_thread = new VideoThread(this);
    video_thread->setClock(clock);
    video_thread->setDecoder(video_dec);

    demuxer_thread = new AVDemuxThread(this);
    demuxer_thread->setDemuxer(&demuxer);
    demuxer_thread->setAudioThread(audio_thread);
    demuxer_thread->setVideoThread(video_thread);
}

AVPlayer::~AVPlayer()
{
    demuxer_thread->stop();
    video_thread->stop();
    audio_thread->stop();
    if (avTimerId > 0)
        killTimer(avTimerId);

    if (audio) {
        delete audio;
        audio = 0;
    }
    if (audio_dec) {
        delete audio_dec;
        audio_dec = 0;
    }
    if (video_dec) {
        delete video_dec;
        video_dec = 0;
    }

}

void AVPlayer::setRenderer(VideoRenderer *r)
{
    if (renderer) {
        //delete renderer; //Do not own the ptr
    }
    renderer = r;
    video_thread->setOutput(renderer);
    renderer->bindDecoder(video_dec);
    renderer->resizeVideo(renderer->videoSize()); //IMPORTANT: the swscaler will resize
}

void AVPlayer::resizeVideo(const QSize &size)
{
    video_dec->resizeVideo(size);
}

//TODO: when is the end
bool AVPlayer::play(const QString& path)
{
    demuxer_thread->stop();
    video_thread->stop();
    audio_thread->stop();
    if (!path.isEmpty())
        filename = path;
    if (avTimerId > 0)
        killTimer(avTimerId);
    if (!demuxer.loadFile(filename)) {
		return false;
	}
    demuxer.dump();

    formatCtx = demuxer.formatContext();
    vCodecCtx = demuxer.videoCodecContext();
    aCodecCtx = demuxer.audioCodecContext();
    audio->setSampleRate(aCodecCtx->sample_rate);
    audio->setChannels(aCodecCtx->channels);
    audio->open();
    int videoStream = demuxer.videoStream();
	//audio
	//if (videoStream < 0)
	//	return false;
    AVStream *m_v_stream = formatCtx->streams[videoStream];
    qDebug("[AVFormatContext::duration = %lld]", demuxer.duration());
    qDebug("[AVStream::start_time = %lld]", m_v_stream->start_time);
    qDebug("[AVCodecContext::time_base = %d, %d, %.2f %.2f]", vCodecCtx->time_base.num, vCodecCtx->time_base.den
            ,1.0 * vCodecCtx->time_base.num / vCodecCtx->time_base.den
            ,1.0 / (1.0 * vCodecCtx->time_base.num / vCodecCtx->time_base.den));
	qDebug("[AVStream::avg_frame_rate = %d, %d, %.2f]", m_v_stream->avg_frame_rate.num, m_v_stream->avg_frame_rate.den
			,1.0 * m_v_stream->avg_frame_rate.num / m_v_stream->avg_frame_rate.den);
	qDebug("[AVStream::r_frame_rate = %d, %d, %.2f]", m_v_stream->r_frame_rate.num, m_v_stream->r_frame_rate.den
			,1.0 * m_v_stream->r_frame_rate.num / m_v_stream->r_frame_rate.den);
	qDebug("[AVStream::time_base = %d, %d, %.2f]", m_v_stream->time_base.num, m_v_stream->time_base.den
			,1.0 * m_v_stream->time_base.num / m_v_stream->time_base.den);

    m_drop_count = 0;

    audio_dec->setCodecContext(aCodecCtx);
    audio_thread->start(QThread::HighestPriority);

    video_dec->setCodecContext(vCodecCtx);
    video_thread->start();

    demuxer_thread->start();

#if 0
    avTimerId = startTimer(1000/demuxer.frameRate());
#endif
    return true;
}

//TODO: what if no audio stream?
void AVPlayer::timerEvent(QTimerEvent* e)
{
	if (e->timerId() != avTimerId)
		return;
    AVPacket packet;
    int videoStream = demuxer.videoStream();
    int audioStream = demuxer.audioStream();
    while (av_read_frame(formatCtx, &packet) >=0 ) {
        Packet pkt;
        pkt.data = QByteArray((const char*)packet.data, packet.size);
        pkt.duration = packet.duration;
        if (packet.dts != AV_NOPTS_VALUE) //has B-frames
            pkt.pts = packet.dts;
        else if (packet.pts != AV_NOPTS_VALUE)
            pkt.pts = packet.pts;
        else
            pkt.pts = 0;
        AVStream *stream = formatCtx->streams[packet.stream_index];
        pkt.pts *= av_q2d(stream->time_base);

        if (stream->codec->codec_type == AVMEDIA_TYPE_SUBTITLE
                && (packet.flags & AV_PKT_FLAG_KEY)
                &&  packet.convergence_duration != AV_NOPTS_VALUE)
            pkt.duration = packet.convergence_duration * av_q2d(stream->time_base);
        else if (packet.duration > 0)
            pkt.duration = packet.duration * av_q2d(stream->time_base);
        else
            pkt.duration = 0;



        if (packet.stream_index == audioStream) {
            audio_thread->packetQueue()->enqueue(pkt);
            audio_thread->wakeAll();
            av_free_packet(&packet); //TODO: why is needed for static var?
        } else if (packet.stream_index == videoStream) {
            if (video_dec->decode(QByteArray((char*)packet.data, packet.size)))
                renderer->write(video_dec->data());
            break;
        } else { //subtitle
            av_free_packet(&packet);
            continue;
        }
    }
}

} //namespace QtAV
