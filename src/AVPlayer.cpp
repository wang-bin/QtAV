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
#include <QEvent>

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
    qApp->installEventFilter(this);
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
    if (avTimerId > 0)
        killTimer(avTimerId);

    stop();
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

bool AVPlayer::play(const QString& path)
{
    filename = path;
    play();
    return true;//isPlaying();
}

//TODO: when is the end
void AVPlayer::play()
{
    stop();
    qDebug("all stoped");
    if (avTimerId > 0)
        killTimer(avTimerId);
    qDebug("loading: %s ...", qPrintable(filename));
    if (!demuxer.loadFile(filename)) {
        return;
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
}

void AVPlayer::stop()
{
    if (demuxer_thread->isRunning()) {
        demuxer_thread->stop();
        demuxer_thread->terminate(); //may waiting. We need it stop immediately
        //wait for finish then we can safely set the vars, e.g. a/v decoders
        demuxer_thread->wait();
    }
    if (audio_thread->isRunning()) {
        audio_thread->stop();
        audio_thread->terminate();
        audio_thread->wait();
    }
    if (video_thread->isRunning()) {
        video_thread->stop();
        video_thread->terminate();
        video_thread->wait();
    }
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

bool AVPlayer::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched);
    //TODO: if not send to QWidget based class, return false; instanceOf()?
    QEvent::Type type = event->type();
    switch (type) {
    case QEvent::MouseButtonPress:
        static_cast<QMouseEvent*>(event)->button();
        //TODO: wheel to control volume etc.
        return false;
        break;
    case QEvent::KeyPress: {
        int key = static_cast<QKeyEvent*>(event)->key();
        switch (key) {
        case Qt::Key_P:
            play();
            break;
        case Qt::Key_S:
            stop();
            break;
        case Qt::Key_F: {
            QWidget *w = qApp->activeWindow();
            if (!w)
                return false;
            if (w->isFullScreen())
                w->showNormal();
            else
                w->showFullScreen();
        }
            break;
        case Qt::Key_Up:
            audio->setVolume(audio->volume() + 0.1);
            qDebug("vol = %.1f", audio->volume());
            break;
        case Qt::Key_Down:
            audio->setVolume(audio->volume() - 0.1);
            qDebug("vol = %.1f", audio->volume());
            break;
        case Qt::Key_M:
            audio->setMute(!audio->isMute());
            break;
        case Qt::Key_T: {
            QWidget *w = qApp->activeWindow();
            if (!w)
                return false;
            Qt::WindowFlags wf = w->windowFlags();
            if (wf & Qt::WindowStaysOnTopHint) {
                qDebug("Window  notstay on top");
                w->setWindowFlags(wf & ~Qt::WindowStaysOnTopHint);
            } else {
                qDebug("Window stay on top");
                w->setWindowFlags(wf | Qt::WindowStaysOnTopHint);
            }
            //call setParent() when changing the flags, causing the widget to be hidden
            w->show();
        }
            break;
        default:
            return false;
        }
        break;
    }
    default:
        return false;
    }
    return true;
}

} //namespace QtAV
