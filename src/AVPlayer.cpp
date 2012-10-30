#include <QtAV/AVPlayer.h>

#include <qevent.h>
#include <qpainter.h>
#include <QApplication>

#include <QtAV/AVDemuxer.h>
#include <private/VideoRenderer_p.h>
#include <QtAV/AudioOutput.h>
#include <QtAV/AudioThread.h>
#include <QtAV/QAVPacket.h>
#include <QtAV/AudioDecoder.h>
#include <QtAV/VideoRenderer.h>
#include <QtAV/AVClock.h>
#include <QtAV/VideoDecoder.h>
#include <QtAV/WidgetRenderer.h>

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

	video_thread = 0;
}

AVPlayer::~AVPlayer()
{
    audio_thread->stop();
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
        delete renderer;
    }
    renderer = r;
    connect((WidgetRenderer*)renderer, SIGNAL(sizeChanged(QSize)), SLOT(resizeVideo(QSize)));
}

void AVPlayer::resizeVideo(const QSize &size)
{
    video_dec->resizeVideo(size);
}

//TODO: when is the end
bool AVPlayer::play(const QString& path)
{
    if (!path.isEmpty())
        filename = path;
    if (avTimerId > 0)
        killTimer(avTimerId);
    if (!avinfo.loadFile(filename)) {
		return false;
	}
	avinfo.dump();

	formatCtx = avinfo.formatContext();
    vCodecCtx = avinfo.videoCodecContext();
    aCodecCtx = avinfo.audioCodecContext();
    audio->setSampleRate(aCodecCtx->sample_rate);
    audio->setChannels(aCodecCtx->channels);
    audio->open();
	int videoStream = avinfo.videoStream();
	//audio
	//if (videoStream < 0)
	//	return false;

    AVStream *m_v_stream = formatCtx->streams[videoStream];
	qDebug("[AVFormatContext::duration = %lld]", avinfo.duration());
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
    audio_thread->setDecoder(audio_dec);
    audio_thread->start(QThread::HighestPriority);

    video_dec->setCodecContext(vCodecCtx);

    avTimerId = startTimer(1000/avinfo.frameRate());

    return true;
}

//TODO: what if no audio stream?
void AVPlayer::timerEvent(QTimerEvent* e)
{
	if (e->timerId() != avTimerId)
		return;
	static AVPacket packet;
	static int videoStream = avinfo.videoStream();
    static int audioStream = avinfo.audioStream();
    while (av_read_frame(formatCtx, &packet) >=0 ) {
        QAVPacket pkt;
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

}
