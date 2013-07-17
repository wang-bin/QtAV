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
#include <QtAV/AudioFormat.h>
#include <QtAV/AudioResampler.h>
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
#if QTAV_HAVE(OPENAL)
#include <QtAV/AOOpenAL.h>
#endif //QTAV_HAVE(OPENAL)
#if QTAV_HAVE(PORTAUDIO)
#include <QtAV/AOPortAudio.h>
#endif //QTAV_HAVE(PORTAUDIO)

namespace QtAV {

AVPlayer::AVPlayer(QObject *parent) :
    QObject(parent)
  , loaded(false)
  , _renderer(0)
  , _audio(0)
  , audio_dec(0)
  , video_dec(0)
  , audio_thread(0)
  , video_thread(0)
  , event_filter(0)
  , video_capture(0)
  , mSpeed(1.0)
  , ao_enable(true)
{
    qDebug("%s", aboutQtAV_PlainText().toUtf8().constData());
    /*
     * call stop() before the window(_renderer) closed to stop the waitcondition
     * If close the _renderer widget, the the _renderer may destroy before waking up.
     */
    connect(qApp, SIGNAL(aboutToQuit()), SLOT(stop()));
    clock = new AVClock(AVClock::AudioClock);
    //clock->setClockType(AVClock::ExternalClock);
	demuxer.setClock(clock);
    connect(&demuxer, SIGNAL(started()), clock, SLOT(start()));

    demuxer_thread = new AVDemuxThread(this);
    demuxer_thread->setDemuxer(&demuxer);

    setPlayerEventFilter(new EventFilter(this));
    video_capture = new VideoCapture(this);
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

//TODO: check components compatiblity(also when change the filter chain)
void AVPlayer::setRenderer(VideoRenderer *r)
{
    qDebug(">>>>>>>>>>%s", __FUNCTION__);
    if (_renderer && r) {
        VideoRenderer::OutAspectRatioMode oar = _renderer->outAspectRatioMode();
        r->setOutAspectRatioMode(oar);
        if (oar == VideoRenderer::CustomAspectRation) {
            r->setOutAspectRatio(_renderer->outAspectRatio());
        }
    }
    setAVOutput(_renderer, r, video_thread);
    if (_renderer) {
        qDebug("resizeRenderer after setRenderer");
        _renderer->resizeRenderer(_renderer->rendererSize()); //IMPORTANT: the swscaler will resize
    }
}

VideoRenderer *AVPlayer::renderer()
{
    return _renderer;
}

void AVPlayer::setAudioOutput(AudioOutput* ao)
{
    qDebug(">>>>>>>>>>%s", __FUNCTION__);
    setAVOutput(_audio, ao, audio_thread);
}

template<class Out>
void AVPlayer::setAVOutput(Out *&pOut, Out *pNew, AVThread *thread)
{
    Out *old = pOut;
    bool delete_old = false;
    if (pOut == pNew) {
        qDebug("output not changed: %p", pOut);
        if (thread && thread->output() == pNew) {//avthread already set that output
            qDebug("avthread already set that output");
            return;
        }
    } else {
        pOut = pNew;
        delete_old = true;
    }
    if (!thread) {
        qDebug("avthread not ready. can not set output.");
        //no avthread, we can delete it safely
        //AVOutput must be allocated in heap. Just like QObject's children.
        if (delete_old) {
            delete old;
            old = 0;
        }
        return;
    }
    //FIXME: what if isPaused()==false but pause(true) in another thread?
    bool need_lock = isPlaying() && !thread->isPaused();
    if (need_lock)
        thread->lock();
    qDebug("set AVThread output");
    thread->setOutput(pOut);
    if (pOut) {
        pOut->setStatistics(&mStatistics);
        if (need_lock)
            thread->unlock();
    }
    //now the old avoutput is not used by avthread, we can delete it safely
    //AVOutput must be allocated in heap. Just like QObject's children.
    if (delete_old) {
        delete old;
        old = 0;
    }
}

AudioOutput* AVPlayer::audio()
{
    return _audio;
}

void AVPlayer::enableAudio(bool enable)
{
    ao_enable = enable;
}

void AVPlayer::disableAudio(bool disable)
{
    ao_enable = !disable;
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

void AVPlayer::setSpeed(qreal speed)
{
    if (speed == mSpeed)
        return;
    mSpeed = speed;
    //TODO: check clock type?
    if (_audio && _audio->isAvailable()) {
        qDebug("set speed %.2f", mSpeed);
        _audio->setSpeed(mSpeed);
    } else {
        masterClock()->setSpeed(mSpeed);
    }
    emit speedChanged(mSpeed);
}

qreal AVPlayer::speed() const
{
    return mSpeed;
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

VideoCapture* AVPlayer::videoCapture()
{
    return video_capture;
}

bool AVPlayer::captureVideo()
{
    if (!video_capture || !video_thread)
        return false;
    bool pause_old = isPaused();
    if (!video_capture->isAsync())
        pause(true);
    QString cap_name = QFileInfo(path).completeBaseName();
    //FIXME: pts is not correct because of multi-thread
    double pts = video_thread->currentPts();
    video_capture->setCaptureName(cap_name + "_" + QString::number(pts, 'f', 3));
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
    return (demuxer_thread &&demuxer_thread->isRunning())
            || (audio_thread && audio_thread->isRunning())
            || (video_thread && video_thread->isRunning());
}

void AVPlayer::togglePause()
{
    pause(!isPaused());
}

void AVPlayer::pause(bool p)
{
    //pause thread. check pause state?
    demuxer_thread->pause(p);
    if (audio_thread)
        audio_thread->pause(p);
    if (video_thread)
        video_thread->pause(p);
    clock->pause(p);
    emit paused(p);
}

bool AVPlayer::isPaused() const
{
    return (demuxer_thread && demuxer_thread->isPaused())
            || (audio_thread && audio_thread->isPaused())
            || (video_thread && video_thread->isPaused());
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
    setupAudioThread();
    setupVideoThread();
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
    //TODO: no eof if replay by seek(0)
    if (true || !isLoaded() || !vCodecCtx) { //if (!isLoaded() && !load())
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

    if (vCodecCtx && video_thread) {
        qDebug("Starting video thread...");
        video_thread->start();
    }
    if (aCodecCtx && audio_thread) {
        qDebug("Starting audio thread...");
        audio_thread->start();
    }
    demuxer_thread->start();
    //blockSignals(false);
    emit started();
}

void AVPlayer::stop()
{
    if (!isPlaying())
        return;
    qDebug("AVPlayer::stop");        
    //blockSignals(true); //TODO: move emit stopped() before it. or connect avthread.finished() to tryEmitStop() {if (!called_by_stop) emit}
    struct avthreads_t {
        AVThread *thread;
        const char *name;
    } threads[] = {
        { audio_thread, "audio thread" },
        { video_thread, "video thread" },
        { 0, 0 }
    };
    for (int i = 0; threads[i].name; ++i) {
        AVThread *thread = threads[i].thread;
        if (!thread)
            continue;
        if (thread->isRunning()) {
            qDebug("stopping %s...", threads[i].name);
            //avoid emit stopped multiple times. AVThread.stopped() connects to AVDemuxThread.stopped().
            thread->blockSignals(true);
            thread->stop();
            if (!thread->wait(1000)) {
                qWarning("Timeout waiting for %s stopped. Terminate it.", threads[i].name);
                thread->terminate();
            }
            thread->blockSignals(false);
        }
    }
    //stop demux thread after avthread is better. otherwise demux thread may be terminated when waiting for avthread ?
    if (demuxer_thread->isRunning()) {
        qDebug("stopping demux thread...");
        demuxer_thread->stop();
        //wait for finish then we can safely set the vars, e.g. a/v decoders
        if (!demuxer_thread->wait(1000)) {
            qWarning("Timeout waiting for demux thread stopped. Terminate it.");
            demuxer_thread->terminate(); //Terminate() causes the wait condition destroyed without waking up
        }
    }
    qDebug("all threads [a|v|d] stopped...");
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
    if (audio_thread)
        audio_thread->nextAndPause();
    if (video_thread)
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

//TODO: av_guess_frame_rate in latest ffmpeg
void AVPlayer::initStatistics()
{
    mStatistics.reset();
    mStatistics.url = path;
    //AV_TIME_BASE_Q: msvc error C2143
    mStatistics.start_time = QTime(0, 0, 0).addMSecs(int((qreal)formatCtx->start_time/(qreal)AV_TIME_BASE*1000.0));
    mStatistics.duration = QTime(0, 0, 0).addMSecs(int((qreal)formatCtx->duration/(qreal)AV_TIME_BASE*1000.0));
    struct common_statistics_t {
        int stream_idx;
        AVCodecContext *ctx;
        Statistics::Common *st;
        const char *name;
    } common_statistics[] = {
        { demuxer.audioStream(), aCodecCtx, &mStatistics.audio, "audio"},
        { demuxer.videoStream(), vCodecCtx, &mStatistics.video, "video"},
        { 0, 0, 0, 0}
    };
    for (int i = 0; common_statistics[i].name; ++i) {
        common_statistics_t cs = common_statistics[i];
        if (cs.stream_idx < 0)
            continue;
        AVStream *stream = formatCtx->streams[cs.stream_idx];
        qDebug("stream: %p, duration=%lld (%d ms==%f), time_base=%f", stream, stream->duration, int(qreal(stream->duration)*av_q2d(stream->time_base)*1000.0)
               , duration(), av_q2d(stream->time_base));
        cs.st->available = true;
        cs.st->codec = cs.ctx->codec->name;
        cs.st->codec_long = cs.ctx->codec->long_name;
        cs.st->total_time = QTime(0, 0, 0).addMSecs(int(qreal(stream->duration)*av_q2d(stream->time_base)*1000.0));
        cs.st->start_time = QTime(0, 0, 0).addMSecs(int(qreal(stream->start_time)*av_q2d(stream->time_base)*1000.0));
        //FIXME: which 1 should we choose? avg_frame_rate may be nan, r_frame_rate may be wrong(guessed value)
        qDebug("codec: %s(%s)", qPrintable(cs.st->codec), qPrintable(cs.st->codec_long));
        if (stream->avg_frame_rate.num) //avg_frame_rate.num,den may be 0
            cs.st->fps_guess = av_q2d(stream->avg_frame_rate);
        else
            cs.st->fps_guess = av_q2d(stream->r_frame_rate);
        cs.st->fps = cs.st->fps_guess;
        cs.st->bit_rate = cs.ctx->bit_rate; //formatCtx
        cs.st->avg_frame_rate = av_q2d(stream->avg_frame_rate);
        cs.st->frames = stream->nb_frames;
        //qDebug("time: %f~%f, nb_frames=%lld", cs.st->start_time, cs.st->total_time, stream->nb_frames); //why crash on mac? av_q2d({0,0})?
        qDebug("%s fps: r_frame_rate=%f avg_frame_rate=%f", cs.name, av_q2d(stream->r_frame_rate), av_q2d(stream->avg_frame_rate));
    }
    if (demuxer.audioStream() >= 0) {
        mStatistics.audio_only.block_align = aCodecCtx->block_align;
        mStatistics.audio_only.channels = aCodecCtx->channels;
        mStatistics.audio_only.frame_number = aCodecCtx->frame_number;
        mStatistics.audio_only.frame_size = aCodecCtx->frame_size;
        mStatistics.audio_only.sample_rate = aCodecCtx->sample_rate;
    }
    if (demuxer.videoStream() >= 0) {
        mStatistics.video.frames = formatCtx->streams[demuxer.videoStream()]->nb_frames;
        mStatistics.video_only.coded_height = vCodecCtx->coded_height;
        mStatistics.video_only.coded_width = vCodecCtx->coded_width;
        mStatistics.video_only.gop_size = vCodecCtx->gop_size;
        mStatistics.video_only.height = vCodecCtx->height;
        mStatistics.video_only.width = vCodecCtx->width;
    }
}

void AVPlayer::setupAudioThread()
{
    if (aCodecCtx) {
        qDebug("has audio");
        //TODO: setAudioOutput() like vo
        if (!_audio && ao_enable) {
            qDebug("new audio output");
#if QTAV_HAVE(OPENAL)
            _audio = new AOOpenAL();
#elif QTAV_HAVE(PORTAUDIO)
            _audio = new AOPortAudio();
#endif
        }
        if (!_audio) {
            masterClock()->setClockType(AVClock::ExternalClock);
            //return;
        } else {
            _audio->audioFormat().setSampleFormat(AudioFormat::SampleFormat_Float);
            _audio->audioFormat().setSampleRate(aCodecCtx->sample_rate);
            _audio->audioFormat().setChannels(aCodecCtx->channels);
            if (!_audio->open()) {
                //return; //audio not ready
            }
        }
        if (!audio_dec) {
            audio_dec = new AudioDecoder();
        }
        qDebug("setCodecContext");
        audio_dec->setCodecContext(aCodecCtx);
        if (_audio)
            audio_dec->resampler()->setOutAudioFormat(_audio->audioFormat());
        audio_dec->prepare();
        if (!audio_thread) {
            qDebug("new audio thread");
            audio_thread = new AudioThread(this);
            audio_thread->setClock(clock);
            audio_thread->setDecoder(audio_dec);
            audio_thread->setStatistics(&mStatistics);
            qDebug("demux thread setAudioThread");
            demuxer_thread->setAudioThread(audio_thread);
            //reconnect if disconnected
        }
        setAudioOutput(_audio);
        /*if has video, connect video's only to avoid emitting stopped() mulltiple times
         *otherwise connect audio's. but if video exists previously and now is null.
         *audio thread will not change. so connection should be here
         */
        if (!vCodecCtx) {
            disconnect(audio_thread, SIGNAL(finished()), this, SIGNAL(stopped()));
            connect(audio_thread, SIGNAL(finished()), this, SIGNAL(stopped()), Qt::DirectConnection);
        }
        int queue_min = 0.61803*qMax<qreal>(24.0, mStatistics.video.fps);
        int queue_max = int(1.61803*(qreal)queue_min); //about 1 second
        audio_thread->packetQueue()->setThreshold(queue_min);
        audio_thread->packetQueue()->setCapacity(queue_max);
    } else {
        //set 0 before delete because demux thread will use the address
        //TODO: use avthread** ?
        demuxer_thread->setAudioThread(0);
        if (audio_thread) {
            qDebug("release audio thread.");
            delete audio_thread;
            audio_thread = 0; //shared ptr?
        }
        if (audio_dec) {
            delete audio_dec;
            audio_dec = 0;
        }
        //DO NOT delete AVOutput. it is setted by user
    }
}

void AVPlayer::setupVideoThread()
{
    if (vCodecCtx) {
        if (!video_dec) {
            video_dec = new VideoDecoder();
        }
        video_dec->setCodecContext(vCodecCtx);
        video_dec->prepare();
        if (!video_thread) {
            video_thread = new VideoThread(this);
            video_thread->setClock(clock);
            video_thread->setDecoder(video_dec);
            video_thread->setStatistics(&mStatistics);
            video_thread->setVideoCapture(video_capture);
            demuxer_thread->setVideoThread(video_thread);
            //reconnect if disconnected
            connect(video_thread, SIGNAL(finished()), this, SIGNAL(stopped()), Qt::DirectConnection);
        }
        setRenderer(_renderer);
        int queue_min = 0.61803*qMax<qreal>(24.0, mStatistics.video.fps);
        int queue_max = int(1.61803*(qreal)queue_min); //about 1 second
        video_thread->packetQueue()->setThreshold(queue_min);
        video_thread->packetQueue()->setCapacity(queue_max);
    } else {
        demuxer_thread->setVideoThread(0); //set 0 before delete. ptr is used in demux thread when set 0
        if (video_thread) {
            qDebug("release video thread.");
            delete video_thread;
            video_thread = 0;//shared ptr?
        }
        if (video_dec) { //TODO: should the decoder managed by avthread?
            delete video_dec;
            video_dec = 0;
        }
        //DO NOT delete AVOutput. it is setted by user
    }
}

void AVPlayer::setupAVThread(AVThread *&thread, AVCodecContext *ctx)
{
}

} //namespace QtAV
