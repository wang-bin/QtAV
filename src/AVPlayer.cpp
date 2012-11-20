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
#include <QFileDialog>

#include <QtAV/AVDemuxer.h>
#include <QtAV/AOPortAudio.h>
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
    QObject(parent),renderer(0),capture_dir("capture"),audio(0)
  ,event_target(0)
{
    qDebug("QtAV %s\nCopyright (C) 2012 Wang Bin <wbsecg1@gmail.com>"
           "\nDistributed under GPLv3 or later"
           , QTAV_VERSION_STR_LONG);
    qApp->installEventFilter(this);
    /*
     * call stop() before the window(renderer) closed to stop the waitcondition
     * If close the renderer widget, the the renderer may destroy before waking up.
     */
    connect(qApp, SIGNAL(aboutToQuit()), SLOT(stop()));
    avTimerId = -1;
    clock = new AVClock(AVClock::AudioClock);
	demuxer.setClock(clock);
    audio = new AOPortAudio();
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
		if (isPlaying())
			stop();
		//delete renderer; //Do not own the ptr
    }
    renderer = r;
    video_thread->setOutput(renderer);
    renderer->bindDecoder(video_dec);
    renderer->resizeVideo(renderer->videoSize()); //IMPORTANT: the swscaler will resize
}

void AVPlayer::setMute(bool mute)
{
    audio->setMute(mute);
}

bool AVPlayer::isMute() const
{
    return audio->isMute();
}

void AVPlayer::resizeVideo(const QSize &size)
{
    video_dec->resizeVideo(size);
}

void AVPlayer::setFile(const QString &path)
{
    this->path = path;
    //qApp->activeWindow()->setWindowTitle(path); //crash on linux
}

QString AVPlayer::file() const
{
    return path;
}

void AVPlayer::setCaptureName(const QString &name)
{
    capture_name = name;
}

void AVPlayer::setCaptureSaveDir(const QString &dir)
{
    capture_dir = dir;
}
//TODO: capture in another thread
bool AVPlayer::capture()
{
    double pts = clock->videoPts();
    QImage cap = renderer->currentFrameImage();
    if (cap.isNull()) {
        qWarning("Null image");
        return false;
    }
    if (!QDir(capture_dir).exists()) {
        if (!QDir().mkpath(capture_dir)) {
            qWarning("Failed to create capture dir [%s]", qPrintable(capture_dir));
            return false;
        }
    }
    QString cap_path = capture_dir + "/";
    if (capture_name.isEmpty())
        cap_path += QFileInfo(path).completeBaseName();
    else
        cap_path += capture_name;
    cap_path += "_" + QString::number(pts, 'f', 3) + ".jpg";
    qDebug("Saving capture to [%s]", qPrintable(cap_path));
    if (!cap.save(cap_path, "jpg")) {
        qWarning("Save capture failed");
        return false;
    }
    return true;
}

bool AVPlayer::play(const QString& path)
{
    setFile(path);
    play();
    return true;//isPlaying();
}

bool AVPlayer::isPlaying() const
{
	return demuxer_thread->isRunning() || audio_thread->isRunning() || video_thread->isRunning();
}

void AVPlayer::pause(bool p)
{
    if (audio)
        audio->pause(p);
    if (renderer)
        renderer->pause(p);
}

bool AVPlayer::isPaused() const
{
    bool p = false;
    if (audio)
        p |= audio->isPaused();
    if (renderer)
        p |= renderer->isPaused();
    return p;
}

//TODO: when is the end
void AVPlayer::play()
{
    if (path.isEmpty()) {
		qDebug("No file to play...");
		return;
	}
	if (isPlaying())
		stop();
    if (avTimerId > 0)
        killTimer(avTimerId);
    qDebug("loading: %s ...", qPrintable(path));
    if (!demuxer.loadFile(path)) {
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
        //demuxer_thread->terminate(); //Terminate() causes the wait condition destroyed without waking up
        //wait for finish then we can safely set the vars, e.g. a/v decoders
        demuxer_thread->wait();
    }
    if (audio_thread->isRunning()) {
        audio_thread->stop();
        //audio_thread->terminate();
        audio_thread->wait();
    }
    if (video_thread->isRunning()) {
        video_thread->stop();
        //video_thread->terminate();
        video_thread->wait();
    }
}

void AVPlayer::seekForward()
{
    demuxer_thread->seekForward();
}

void AVPlayer::seekBackward()
{
    demuxer_thread->seekBackward();
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
            audio_thread->packetQueue()->put(pkt);
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
        //avoid receive an event multiple times
        if (!event_target)
            event_target = watched;
        if (event_target != watched)
            return false;
        int key = static_cast<QKeyEvent*>(event)->key();
        switch (key) {
        case Qt::Key_C: //capture
            capture();
            break;
        case Qt::Key_N: //check playing?
            pause(false);
            pause(true);
            break;
        case Qt::Key_P:
            play();
            break;
        case Qt::Key_S:
            stop(); //check playing?
            break;
        case Qt::Key_Space: //check playing?
            qDebug("isPaused = %d", isPaused());
            pause(!isPaused());
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
        case Qt::Key_O: {
            QString file = QFileDialog::getOpenFileName(0, tr("Open a video"));
            if (!file.isEmpty())
                play(file);
        }
            break;
        case Qt::Key_Left:
            qDebug("<-");
            seekBackward();
            break;
        case Qt::Key_Right:
            qDebug("->");
            seekForward();
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
                qDebug("Window not stays on top");
                w->setWindowFlags(wf & ~Qt::WindowStaysOnTopHint);
            } else {
                qDebug("Window stays on top");
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
    return false; //for text input
}

} //namespace QtAV
