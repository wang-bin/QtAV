/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2013 Wang Bin <wbsecg1@gmail.com>

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
#include <QtAV/QtAV_Compat.h>
#include <QtAV/VideoCapture.h>
#include <QtAV/VideoDecoder.h>
#include <QtAV/WidgetRenderer.h>
#include <QtAV/VideoThread.h>
#include <QtAV/AVDemuxThread.h>
#include <QtAV/EventFilter.h>
#include <QtAV/VideoCapture.h>
#include <QtAV/AudioOutput.h>
#if HAVE_OPENAL
#include <QtAV/AOOpenAL.h>
#endif //HAVE_OPENAL
#if HAVE_PORTAUDIO
#include <QtAV/AOPortAudio.h>
#endif //HAVE_PORTAUDIO

namespace QtAV {

AVPlayer::AVPlayer(QObject *parent) :
    QObject(parent),loaded(false),capture_dir("capture"),_renderer(0),_audio(0)
  , event_filter(0),video_capture(0)
{
    qDebug("%s", aboutQtAV().toUtf8().constData());
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
    if (_audio) {
        _audio->setStatistics(&mStatistics);
    }
    audio_dec = new AudioDecoder();
    audio_thread = new AudioThread(this);
    audio_thread->setClock(clock);
    //audio_thread->setPacketQueue(&audio_queue);
    audio_thread->setDecoder(audio_dec);
    audio_thread->setOutput(_audio);
    audio_thread->setStatistics(&mStatistics);

    video_dec = new VideoDecoder();

    video_thread = new VideoThread(this);
    video_thread->setClock(clock);
    video_thread->setDecoder(video_dec);
    video_thread->setStatistics(&mStatistics);

    demuxer_thread = new AVDemuxThread(this);
    demuxer_thread->setDemuxer(&demuxer);
    demuxer_thread->setAudioThread(audio_thread);
    demuxer_thread->setVideoThread(video_thread);

    setPlayerEventFilter(new EventFilter(this));
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
        _renderer->setStatistics(&mStatistics);
        if (isPlaying())
            stop();
        //delete _renderer; //Do not own the ptr
        _renderer->resizeRenderer(_renderer->rendererSize()); //IMPORTANT: the swscaler will resize
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

//setPlayerEventFilter(0) will remove the previous event filter
void AVPlayer::setPlayerEventFilter(QObject *obj)
{
    if (event_filter) {
        qApp->removeEventFilter(event_filter);
        delete event_filter;
        event_filter = 0; //the default event filter's parent is this, so AVPlayer will try to delete event_filter
    }
    if (obj) {
        event_filter = obj;
        qApp->installEventFilter(event_filter);
    }
}

Statistics& AVPlayer::statistics()
{
    return mStatistics;
}

const Statistics& AVPlayer::statistics() const
{
    return mStatistics;
}
//TODO: remove?
void AVPlayer::resizeRenderer(const QSize &size)
{
    _renderer->resizeRenderer(size); //TODO: deprecate
    //video_dec->resizeVideoFrame(size);
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
    //TODO: init statistics
    return loaded;
}

qreal AVPlayer::duration() const
{
    return qreal(demuxer.duration())/qreal(AV_TIME_BASE); //AVFrameContext.duration time base: AV_TIME_BASE
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
    //FIXME: seek(0) for audio without video crashes, why?
    if (!isLoaded() || !vCodecCtx) { //if (!isLoaded() && !load())
        if (!load()) {
            mStatistics.reset();
            return;
        } else {
            initStatistics();
        }
    } else {
        qDebug("seek(0)");
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
    demuxer_thread->pause(false);
    clock->pause(false);
    audio_thread->nextAndPause();
    video_thread->nextAndPause();
    clock->pause(true);
    demuxer_thread->pause(true);
}

void AVPlayer::seek(qreal pos)
{
    demuxer_thread->seek(pos);
}

void AVPlayer::seekForward()
{
    demuxer_thread->seekForward();
    qDebug("seek %f%%", clock->value()/duration()*100.0);
}

void AVPlayer::seekBackward()
{
    demuxer_thread->seekBackward();
    qDebug("seek %f%%", clock->value()/duration()*100.0);
}

void AVPlayer::updateClock(qint64 msecs)
{
    clock->updateExternalClock(msecs);
}


void AVPlayer::initStatistics()
{
    mStatistics.reset();
    mStatistics.url = path;
    mStatistics.start_time = QTime(0, 0, 0).addMSecs(int((qreal)formatCtx->start_time*(qreal)av_q2d(AV_TIME_BASE_Q)*1000.0));
    mStatistics.duration = QTime(0, 0, 0).addMSecs(int((qreal)formatCtx->duration*(qreal)av_q2d(AV_TIME_BASE_Q)*1000.0));
    AVStream *stream = formatCtx->streams[demuxer.audioStream()];
    qDebug("duration=%lld (%d ms==%f), time_base=%f", stream->duration, int(qreal(stream->duration)*av_q2d(stream->time_base)*1000.0)
           , duration(), av_q2d(stream->time_base));
    //mStatistics.audio.format =
    mStatistics.audio.codec = aCodecCtx->codec->name;
    mStatistics.audio.codec_long = aCodecCtx->codec->long_name;
    mStatistics.audio.total_time = QTime(0, 0, 0).addMSecs(int(qreal(stream->duration)*av_q2d(stream->time_base)*1000.0));
    mStatistics.audio.start_time = QTime(0, 0, 0).addMSecs(int(qreal(stream->start_time)*av_q2d(stream->time_base)*1000.0));
    mStatistics.audio.bit_rate = aCodecCtx->bit_rate; //formatCtx
    mStatistics.audio.avg_frame_rate = av_q2d(stream->avg_frame_rate);
    mStatistics.audio.frames = stream->nb_frames;
    //mStatistics.audio.size =

    stream = formatCtx->streams[demuxer.videoStream()];
    //mStatistics.audio.format =
    mStatistics.video.codec = vCodecCtx->codec->name;
    mStatistics.video.codec_long = vCodecCtx->codec->long_name;
    qDebug("duration=%lld (%d ms==%f), time_base=%f", stream->duration, int(qreal(stream->duration)*av_q2d(stream->time_base)*1000.0)
           , duration(), av_q2d(stream->time_base));
    mStatistics.video.total_time = QTime(0, 0, 0).addMSecs(int(qreal(stream->duration)*av_q2d(stream->time_base)*1000.0));
    mStatistics.video.start_time = QTime(0, 0, 0).addMSecs(int(qreal(stream->start_time)*av_q2d(stream->time_base)*1000.0));
    mStatistics.video.bit_rate = vCodecCtx->bit_rate; //formatCtx
    mStatistics.video.avg_frame_rate = av_q2d(stream->avg_frame_rate);
    mStatistics.video.frames = stream->nb_frames;
    //mStatistics.audio.size =

    mStatistics.audio_only.block_align = aCodecCtx->block_align;
    mStatistics.audio_only.channels = aCodecCtx->channels;
    mStatistics.audio_only.frame_number = aCodecCtx->frame_number;
    mStatistics.audio_only.frame_size = aCodecCtx->frame_size;
    mStatistics.audio_only.sample_rate = aCodecCtx->sample_rate;

    mStatistics.video_only.coded_height = vCodecCtx->coded_height;
    mStatistics.video_only.coded_width = vCodecCtx->coded_width;
    mStatistics.video_only.gop_size = vCodecCtx->gop_size;
    mStatistics.video_only.height = vCodecCtx->height;
    mStatistics.video_only.width = vCodecCtx->width;
}

} //namespace QtAV
