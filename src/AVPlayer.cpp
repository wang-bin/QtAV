/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2013 Wang Bin <wbsecg1@gmail.com>

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
#include <QtCore/QDir>

#include <QtAV/AVDemuxer.h>
#include <QtAV/AudioThread.h>
#include <QtAV/Packet.h>
#include <QtAV/AudioDecoder.h>
#include <QtAV/VideoRenderer.h>
#include <QtAV/AVClock.h>
#include <QtAV/VideoCapture.h>
#include <QtAV/VideoDecoder.h>
#include <QtAV/WidgetRenderer.h>
#include <QtAV/VideoThread.h>
#include <QtAV/AVDemuxThread.h>
#include <QtAV/EventFilter.h>
#include <QtAV/VideoCapture.h>
#if HAVE_OPENAL
#include <QtAV/AOOpenAL.h>
#endif //HAVE_OPENAL
#if HAVE_PORTAUDIO
#include <QtAV/AOPortAudio.h>
#endif //HAVE_PORTAUDIO
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
    QObject(parent),loaded(false),capture_dir("capture"),_renderer(0),_audio(0)
  ,event_filter(0),video_capture(0)
{
    qDebug("QtAV %s\nCopyright (C) 2012 Wang Bin <wbsecg1@gmail.com>"
           "\nDistributed under GPLv3 or later"
           "\nShanghai University, China"
           , QTAV_VERSION_STR_LONG);
    /*
     * call stop() before the window(_renderer) closed to stop the waitcondition
     * If close the _renderer widget, the the _renderer may destroy before waking up.
     */
    connect(qApp, SIGNAL(aboutToQuit()), SLOT(stop()));
    clock = new AVClock(AVClock::AudioClock);
    //clock->setClockType(AVClock::ExternalClock);
	demuxer.setClock(clock);
    connect(&demuxer, SIGNAL(started()), clock, SLOT(start()));
#if HAVE_OPENAL
    _audio = new AOOpenAL();
#elif HAVE_PORTAUDIO
    _audio = new AOPortAudio();
#endif
    audio_dec = new AudioDecoder();
    audio_thread = new AudioThread(this);
    audio_thread->setClock(clock);
    //audio_thread->setPacketQueue(&audio_queue);
    audio_thread->setDecoder(audio_dec);
    audio_thread->setOutput(_audio);

    video_dec = new VideoDecoder();

    video_thread = new VideoThread(this);
    video_thread->setClock(clock);
    video_thread->setDecoder(video_dec);

    demuxer_thread = new AVDemuxThread(this);
    demuxer_thread->setDemuxer(&demuxer);
    demuxer_thread->setAudioThread(audio_thread);
    demuxer_thread->setVideoThread(video_thread);

    event_filter = new EventFilter(this);

    setVideoCapture(new VideoCapture());
}

AVPlayer::~AVPlayer()
{
    stop();
    if (_audio) {
        delete _audio;
        _audio = 0;
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

AVClock* AVPlayer::masterClock()
{
    return clock;
}

VideoRenderer* AVPlayer::setRenderer(VideoRenderer *r)
{
    VideoRenderer *old = _renderer;
    _renderer = r;
    video_thread->setOutput(_renderer);
    if (_renderer) {
		if (isPlaying())
			stop();
        //delete _renderer; //Do not own the ptr
        _renderer->registerEventFilter(event_filter);
        _renderer->resizeVideo(_renderer->videoSize()); //IMPORTANT: the swscaler will resize
    }
    return old;
}

VideoRenderer *AVPlayer::renderer()
{
    return _renderer;
}

AudioOutput* AVPlayer::audio()
{
    return _audio;
}

void AVPlayer::setMute(bool mute)
{
    if (_audio)
        _audio->setMute(mute);
}

bool AVPlayer::isMute() const
{
    return !_audio || _audio->isMute();
}

//TODO: remove?
void AVPlayer::resizeVideo(const QSize &size)
{
    _renderer->resizeVideo(size); //TODO: deprecate
    //video_dec->resizeVideo(size);
}
/*
 * loaded state is the state of current setted file.
 * For replaying, we can avoid load a seekable file again.
 * For playing a new file, load() is required.
 */
void AVPlayer::setFile(const QString &path)
{
    this->path = path;
    loaded = false; //
    //qApp->activeWindow()->setWindowTitle(path); //crash on linux
}

QString AVPlayer::file() const
{
    return path;
}

VideoCapture* AVPlayer::setVideoCapture(VideoCapture *cap)
{
    VideoCapture *old = video_capture;
    video_capture = cap;
    video_thread->setVideoCapture(video_capture);
    return old;
}

VideoCapture* AVPlayer::videoCapture()
{
    return video_capture;
}

void AVPlayer::setCaptureName(const QString &name)
{
    capture_name = name;
}

void AVPlayer::setCaptureSaveDir(const QString &dir)
{
    capture_dir = dir;
}

bool AVPlayer::captureVideo()
{
    if (!video_capture)
        return false;
    bool pause_old = isPaused();
    if (!video_capture->isAsync())
        pause(true);
    video_capture->setCaptureDir(capture_dir);
    QString cap_name(capture_name);
    if (cap_name.isEmpty())
        cap_name = QFileInfo(path).completeBaseName();
    //FIXME: pts is not correct because of multi-thread
    double pts = video_thread->currentPts();
    cap_name += "_" + QString::number(pts, 'f', 3);
    video_capture->setCaptureName(cap_name);
    qDebug("request capture: %s", qPrintable(cap_name));
    video_capture->request();
    if (!video_capture->isAsync())
        pause(pause_old);
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
    //pause thread. check pause state?
    demuxer_thread->pause(p);
    audio_thread->pause(p);
    video_thread->pause(p);
    clock->pause(p);
#if 0
    /*Pause output. all threads using those outputs will be paused. If a output is not paused
     *, then other players' avthread can use it.
     */
    if (_audio)
        _audio->pause(p);
    if (_renderer)
        _renderer->pause(p);
#endif
}

bool AVPlayer::isPaused() const
{
    return demuxer_thread->isPaused() | audio_thread->isPaused() | video_thread->isPaused();
#if 0
    bool p = false;
    if (_audio)
        p |= _audio->isPaused();
    if (_renderer)
        p |= _renderer->isPaused();
    return p;
#endif
}

bool AVPlayer::isLoaded() const
{
    return loaded;
}

bool AVPlayer::load(const QString &path)
{
    setFile(path);
    return load();
}

bool AVPlayer::load()
{
    loaded = false;
    if (path.isEmpty()) {
        qDebug("No file to play...");
        return loaded;
    }
    qDebug("loading: %s ...", path.toUtf8().constData());
    if (!demuxer.loadFile(path)) {
        return loaded;
    }
    loaded = true;
    demuxer.dump();
    formatCtx = demuxer.formatContext();
    aCodecCtx = demuxer.audioCodecContext();
    vCodecCtx = demuxer.videoCodecContext();
    if (_audio && aCodecCtx) {
        _audio->setSampleRate(aCodecCtx->sample_rate);
        _audio->setChannels(aCodecCtx->channels);
        if (!_audio->open()) {
            //return; //audio not ready
        }
    }
    audio_dec->setCodecContext(aCodecCtx);
    video_dec->setCodecContext(vCodecCtx);
    return loaded;
}

//FIXME: why no demuxer will not get an eof if replaying by seek(0)?
void AVPlayer::play()
{
    if (isPlaying())
        stop();
    /*
     * avoid load mutiple times when replaying the same seekable file
     * TODO: force load unseekable stream? avio.seekable. currently you
     * must setFile() agian to reload an unseekable stream
     */
    if (!isLoaded()) { //if (!isLoaded() && !load())
        if (!load())
            return;
    } else {
        demuxer.seek(0); //FIXME: now assume it is seekable. for unseekable, setFile() again
    }
    Q_ASSERT(clock != 0);
    clock->reset();

    if (aCodecCtx) {
        qDebug("Starting audio thread...");
        audio_thread->start(QThread::HighestPriority);
    }
    if (vCodecCtx) {
        qDebug("Starting video thread...");
        video_thread->start();
    }
    demuxer_thread->start();
    emit started();
}

void AVPlayer::stop()
{
    if (demuxer_thread->isRunning()) {
        qDebug("stop d");
        demuxer_thread->stop();
        //wait for finish then we can safely set the vars, e.g. a/v decoders
        if (!demuxer_thread->wait()) {
            qWarning("Timeout waiting for demux thread stopped. Terminate it.");
            demuxer_thread->terminate(); //Terminate() causes the wait condition destroyed without waking up
        }
    }
    if (audio_thread->isRunning()) {
        qDebug("stop a");
        audio_thread->stop();
        if (!audio_thread->wait(1000)) {
            qWarning("Timeout waiting for audio thread stopped. Terminate it.");
            audio_thread->terminate();
        }
    }
    if (video_thread->isRunning()) {
        qDebug("stopv");
        video_thread->stop();
        if (!video_thread->wait(1000)) {
            qWarning("Timeout waiting for video thread stopped. Terminate it.");
            video_thread->terminate(); ///if time out
        }
    }
    emit stopped();
}
//FIXME: If not playing, it will just play but not play one frame.
void AVPlayer::playNextFrame()
{
    if (!isPlaying()) {
        play();
    }
    pause(false);
    pause(true);
}

void AVPlayer::seek(qreal pos)
{
    demuxer_thread->seek(pos);
}

void AVPlayer::seekForward()
{
    demuxer_thread->seekForward();
}

void AVPlayer::seekBackward()
{
    demuxer_thread->seekBackward();
}

void AVPlayer::updateClock(qint64 msecs)
{
    clock->updateExternalClock(msecs);
}

} //namespace QtAV
