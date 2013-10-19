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
//#include <QtCore/QMetaObject>

#include <QtAV/AVDemuxer.h>
#include <QtAV/AudioFormat.h>
#include <QtAV/AudioResampler.h>
#include <QtAV/AudioThread.h>
#include <QtAV/Packet.h>
#include <QtAV/AudioDecoder.h>
#include <QtAV/VideoRenderer.h>
#include <QtAV/OutputSet.h>
#include <QtAV/AVClock.h>
#include <QtAV/QtAV_Compat.h>
#include <QtAV/VideoCapture.h>
#include <QtAV/VideoDecoderTypes.h>
#include <QtAV/WidgetRenderer.h>
#include <QtAV/VideoThread.h>
#include <QtAV/AVDemuxThread.h>
#include <QtAV/EventFilter.h>
#include <QtAV/VideoOutputEventFilter.h>
#include <QtAV/VideoCapture.h>
#include <QtAV/AudioOutput.h>
#if QTAV_HAVE(OPENAL)
#include <QtAV/AOOpenAL.h>
#endif //QTAV_HAVE(OPENAL)
#if QTAV_HAVE(PORTAUDIO)
#include <QtAV/AOPortAudio.h>
#endif //QTAV_HAVE(PORTAUDIO)
#include <QtAV/FilterManager.h>

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
    formatCtx = 0;
    aCodecCtx = 0;
    vCodecCtx = 0;
    start_pos = 0;
    mpVOSet = new OutputSet(this);
    mpAOSet = new OutputSet(this);
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
    //reenter stop() and reset start_pos. will not emit stopped() again because demuxer thread is the last finished thread(i.e. after avthread.finished())
    //use direct connection otherwise replay may stop immediatly because slot stop() is called after play()
    //FIXME: stop() may be called twice because of demuxer thread may emit finished() before a avthread.finished()
    connect(demuxer_thread, SIGNAL(finished()), this, SLOT(stop()), Qt::DirectConnection);

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

void AVPlayer::addVideoRenderer(VideoRenderer *renderer)
{
    if (!renderer) {
        qWarning("add a null renderer!");
        return;
    }
    renderer->setStatistics(&mStatistics);
    QObject *voo = renderer->widget();
    if (voo) {
        //TODO: how to delete filter if no parent?
        //the filtering object must be in the same thread as this object.
        if (renderer->widget())
            voo->installEventFilter(new VideoOutputEventFilter(renderer));
    }
    mpVOSet->addOutput(renderer);
}

void AVPlayer::removeVideoRenderer(VideoRenderer *renderer)
{
    mpVOSet->removeOutput(renderer);
}

void AVPlayer::clearVideoRenderers()
{
    mpVOSet->clearOutputs();
}

//TODO: check components compatiblity(also when change the filter chain)
void AVPlayer::setRenderer(VideoRenderer *r)
{
    qDebug(">>>>>>>>>>%s", __FUNCTION__);
#if V1_2
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
#else
    VideoRenderer *vo = renderer();
    if (vo && r) {
        VideoRenderer::OutAspectRatioMode oar = vo->outAspectRatioMode();
        //r->resizeRenderer(vo->rendererSize());
        r->setOutAspectRatioMode(oar);
        if (oar == VideoRenderer::CustomAspectRation) {
            r->setOutAspectRatio(vo->outAspectRatio());
        }
    }
    if (r) {
        r->resizeRenderer(r->rendererSize()); //IMPORTANT: the swscaler will resize
    }
    r->setStatistics(&mStatistics);
    clearVideoRenderers();
    addVideoRenderer(r);

#endif //V1_2
}

VideoRenderer *AVPlayer::renderer()
{
#if V1_2
    return _renderer;
#else
    //QList assert empty in debug mode
    if (mpVOSet->outputs().isEmpty())
	return 0;
    return static_cast<VideoRenderer*>(mpVOSet->outputs().last());
#endif //V1_2
}

QList<VideoRenderer*> AVPlayer::videoOutputs()
{
    QList<VideoRenderer*> vos;
    vos.reserve(mpVOSet->outputs().size());
    foreach (AVOutput *out, mpVOSet->outputs()) {
        vos.append(static_cast<VideoRenderer*>(out));
    }
    return vos;
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
            thread->unlock(); //??why here?
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
    }
    event_filter = obj; //the default event filter's parent is this, so AVPlayer will try to delete event_filter
    if (obj) {
        qApp->installEventFilter(event_filter);
    } else {
        //new created renderers keep the event filter
        QList<VideoRenderer*> vos = videoOutputs();
        if (!vos.isEmpty()) {
            foreach (VideoRenderer *vo, vos) {
                vo->enableDefaultEventFilter(false);
            }
        }
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

bool AVPlayer::installAudioFilter(Filter *filter)
{
    if (!FilterManager::instance().registerAudioFilter(filter, this)) {
        return false;
    }
    if (!audio_thread)
        return false; //install later when avthread created
    return audio_thread->installFilter(filter);
}

bool AVPlayer::installVideoFilter(Filter *filter)
{
    if (!FilterManager::instance().registerVideoFilter(filter, this)) {
        return false;
    }
    if (!video_thread)
        return false; //install later when avthread created
    return video_thread->installFilter(filter);
}

bool AVPlayer::uninstallFilter(Filter *filter)
{
    if (!FilterManager::instance().unregisterFilter(filter)) {
        qWarning("unregister filter %p failed", filter);
        return false;
    }
    /*
     * TODO: send FilterUninstallTask(this, filter){this.mFilters.remove} to
     * active player's AVThread
     *
     */
    class UninstallFilterTask : public QRunnable {
    public:
        UninstallFilterTask(AVThread *thread, Filter *filter):
            QRunnable()
          , mpThread(thread)
          , mpFilter(filter)
        {
            setAutoDelete(true);
        }

        virtual void run() {
            mpThread->uninstallFilter(mpFilter, false);
            //QMetaObject::invokeMethod(FilterManager::instance(), "onUninstallInTargetDone", Qt::AutoConnection, Q_ARG(Filter*, filter));
            FilterManager::instance().emitOnUninstallInTargetDone(mpFilter);
        }
    private:
        AVThread *mpThread;
        Filter *mpFilter;
    };
    AVThread *avthread = video_thread;
    if (!avthread || !avthread->filters().contains(filter)) {
        avthread = audio_thread;
    }
    if (!avthread || !avthread->filters().contains(filter)) {
        return false;
    }
    avthread->scheduleTask(new UninstallFilterTask(avthread, filter));
    return true;
}


/*
 * loaded state is the state of current setted file.
 * For replaying, we can avoid load a seekable file again.
 * For playing a new file, load() is required.
 */
void AVPlayer::setFile(const QString &path)
{
    demuxer.setAutoResetStream(!this->path.isEmpty() && this->path != path);
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

bool AVPlayer::load(const QString &path, bool reload)
{
    setFile(path);
    return load(reload);
}

bool AVPlayer::load(bool reload)
{
    loaded = false;
    if (path.isEmpty()) {
        qDebug("No file to play...");
        return loaded;
    }
    qDebug("loading: %s ...", path.toUtf8().constData());
    if (reload || !demuxer.isLoaded(path)) {
        if (!demuxer.loadFile(path)) {
            return loaded;
        }
    } else {
        demuxer.prepareStreams();
    }
    loaded = true;
    formatCtx = demuxer.formatContext();
    aCodecCtx = demuxer.audioCodecContext();
    vCodecCtx = demuxer.videoCodecContext();

    if (masterClock()->isClockAuto()) {
        qDebug("auto select clock: audio > external");
        if (!aCodecCtx) {
            qWarning("No audio found or audio not supported. Using ExternalClock");
            masterClock()->setClockType(AVClock::ExternalClock);
        } else {
            qDebug("Using AudioClock");
            masterClock()->setClockType(AVClock::AudioClock);
        }
    }
    if (start_pos <= 0)
        start_pos = duration() > 0 ? startPosition()/duration() : 0;
    if (start_pos > 0)
        demuxer.seek(start_pos); //just use demuxer.startTime()/duration()?
    if (!setupAudioThread()) {
        demuxer_thread->setAudioThread(0); //set 0 before delete. ptr is used in demux thread when set 0
        if (audio_thread) {
            qDebug("release audio thread.");
            delete audio_thread;
            audio_thread = 0;//shared ptr?
        }
    }
    if (!setupVideoThread()) {
        demuxer_thread->setVideoThread(0); //set 0 before delete. ptr is used in demux thread when set 0
        if (video_thread) {
            qDebug("release video thread.");
            delete video_thread;
            video_thread = 0;//shared ptr?
        }
    }
    if (!audio_thread && !video_thread) {
        qWarning("load failed");
        return false;
    }
    return loaded;
}

qreal AVPlayer::duration() const
{
    if (!formatCtx)
        return 0;
    if (formatCtx->duration == AV_NOPTS_VALUE)
        return 0;
    return qreal(demuxer.duration())/qreal(AV_TIME_BASE); //AVFrameContext.duration time base: AV_TIME_BASE
}

qreal AVPlayer::startPosition() const
{
    if (!formatCtx) //not opened
        return 0;
    if (formatCtx->start_time == AV_NOPTS_VALUE)
        return 0;
    return qreal(formatCtx->start_time)/qreal(AV_TIME_BASE);
}

qreal AVPlayer::position() const
{
    return clock->value();
}

bool AVPlayer::setAudioStream(int n, bool now)
{
    if (!demuxer.setStreamIndex(AVDemuxer::AudioStream, n)) {
        qWarning("set video stream to %d failed", n);
        return false;
    }
    loaded = false;
    start_pos = -1;
    if (!now)
        return true;
    play();
    return isPlaying();
}

bool AVPlayer::setVideoStream(int n, bool now)
{
    if (!demuxer.setStreamIndex(AVDemuxer::VideoStream, n)) {
        qWarning("set video stream to %d failed", n);
        return false;
    }
    loaded = false;
    start_pos = -1;
    if (!now)
        return true;
    play();
    return isPlaying();
}

bool AVPlayer::setSubtitleStream(int n, bool now)
{
    return false;
}

int AVPlayer::currentAudioStream() const
{
    return demuxer.audioStreams().indexOf(demuxer.audioStream());
}

int AVPlayer::currentVideoStream() const
{
    return demuxer.videoStreams().indexOf(demuxer.videoStream());
}

int AVPlayer::currentSubtitleStream() const
{
    return demuxer.subtitleStreams().indexOf(demuxer.subtitleStream());
}

int AVPlayer::audioStreamCount() const
{
    return demuxer.audioStreams().size();
}

int AVPlayer::videoStreamCount() const
{
    return demuxer.videoStreams().size();
}

int AVPlayer::subtitleStreamCount() const
{
    return demuxer.subtitleStreams().size();
}

//FIXME: why no demuxer will not get an eof if replaying by seek(0)?
void AVPlayer::play()
{
    //FIXME: bad delay after play from here
    bool start_last =  start_pos == -1;
    if (isPlaying()) {
        if (start_last) {
            clock->pause(true); //external clock
            start_pos = duration() > 0 ? -position()/duration() : 0;
            qDebug("start pos is current position: %f", start_pos);
        } else {
            start_pos = 0;
            qDebug("start pos is stream start time");
        }
        qreal last_pos = start_pos;
        stop();
        if (last_pos < 0)
            start_pos = -last_pos;
    }
    /*
     * avoid load mutiple times when replaying the same seekable file
     * TODO: force load unseekable stream? avio.seekable. currently you
     * must setFile() agian to reload an unseekable stream
     */
    //TODO: no eof if replay by seek(0)
    if (!isLoaded()) { //if (!isLoaded() && !load())
        if (!load(false)) {
            mStatistics.reset();
            return;
        } else {
            initStatistics();
        }
    } else {
#if EOF_ISSUE_SOLVED
        qDebug("seek(%f)", start_pos);
        demuxer.seek(start_pos); //FIXME: now assume it is seekable. for unseekable, setFile() again
#else
        if (!load(true)) {
            mStatistics.reset();
            return;
        } else {
            initStatistics();
        }
#endif //EOF_ISSUE_SOLVED
    }

    if (aCodecCtx && audio_thread) {
        qDebug("Starting audio thread...");
        audio_thread->start();
    }
    if (vCodecCtx && video_thread) {
        qDebug("Starting video thread...");
        video_thread->start();
    }
    demuxer_thread->start();
    //blockSignals(false);
    Q_ASSERT(clock != 0);
    if (start_last) {
        clock->pause(false); //external clock
    } else {
        clock->reset();
    }
    emit started(); //we called stop(), so must emit started()
}

void AVPlayer::stop()
{
    qDebug("AVPlayer::stop");
    start_pos = duration() > 0 ? startPosition()/duration() : 0;
    if (!isPlaying()) {
        qDebug("Not playing~");
        return;
    }
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
    if (audio_dec && audio_dec->isOpen())
        audio_dec->close();
    if (video_dec && video_dec->isOpen())
        video_dec->close();

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
     if (!demuxer_thread->isRunning())
        return;
    demuxer_thread->seek(pos);
}

void AVPlayer::seekForward()
{
    if (!demuxer_thread->isRunning())
        return;
    demuxer_thread->seekForward();
    qDebug("seek %f%%", clock->value()/duration()*100.0);
}

void AVPlayer::seekBackward()
{
     if (!demuxer_thread->isRunning())
        return;
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
    mStatistics.bit_rate = formatCtx->bit_rate;
    mStatistics.format = formatCtx->iformat->name;
    //AV_TIME_BASE_Q: msvc error C2143
    mStatistics.start_time = QTime(0, 0, 0).addMSecs(int((qreal)formatCtx->start_time/(qreal)AV_TIME_BASE*1000.0));
    mStatistics.duration = QTime(0, 0, 0).addMSecs(int((qreal)formatCtx->duration/(qreal)AV_TIME_BASE*1000.0));
    struct common_statistics_t {
        int stream_idx;
        AVCodecContext *ctx;
        Statistics::Common *st;
        const char *name;
    } common_statistics[] = {
        { demuxer.videoStream(), vCodecCtx, &mStatistics.video, "video"},
        { demuxer.audioStream(), aCodecCtx, &mStatistics.audio, "audio"},
        { 0, 0, 0, 0}
    };
    for (int i = 0; common_statistics[i].name; ++i) {
        common_statistics_t cs = common_statistics[i];
        if (cs.stream_idx < 0)
            continue;
        AVStream *stream = formatCtx->streams[cs.stream_idx];
        qDebug("stream: %d, duration=%lld (%d ms==%f), time_base=%f", cs.stream_idx, stream->duration, int(qreal(stream->duration)*av_q2d(stream->time_base)*1000.0)
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
        char cl[128]; //
        av_get_channel_layout_string(cl, sizeof(cl), -1, aCodecCtx->channel_layout); //TODO: ff version
        mStatistics.audio_only.channel_layout = cl;
        mStatistics.audio_only.sample_fmt = av_get_sample_fmt_name(aCodecCtx->sample_fmt);
        mStatistics.audio_only.frame_number = aCodecCtx->frame_number;
        mStatistics.audio_only.frame_size = aCodecCtx->frame_size;
        mStatistics.audio_only.sample_rate = aCodecCtx->sample_rate;
    }
    if (demuxer.videoStream() >= 0) {
        mStatistics.video.frames = formatCtx->streams[demuxer.videoStream()]->nb_frames;
        mStatistics.video_only.coded_height = vCodecCtx->coded_height;
        mStatistics.video_only.coded_width = vCodecCtx->coded_width;
        mStatistics.video_only.gop_size = vCodecCtx->gop_size;
        mStatistics.video_only.pix_fmt = av_get_pix_fmt_name(vCodecCtx->pix_fmt);
        mStatistics.video_only.height = vCodecCtx->height;
        mStatistics.video_only.width = vCodecCtx->width;
    }
}

bool AVPlayer::setupAudioThread()
{
    if (!aCodecCtx) {
        return false;
    }
    qDebug("has audio");
    if (!audio_dec) {
        audio_dec = new AudioDecoder();
    } else {
        if (!audio_dec->close()) {
            return false;
        }
    }
    audio_dec->setCodecContext(aCodecCtx);
    if (!audio_dec->open()) {
        return false;
    }
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
        // 5, 6, 7 may not play
        if (aCodecCtx->channels > 2)
            _audio->audioFormat().setChannelLayoutFFmpeg(AV_CH_LAYOUT_STEREO);
        else
            _audio->audioFormat().setChannels(aCodecCtx->channels);
        if (!_audio->open()) {
            //return; //audio not ready
        }
    }
    if (_audio)
        audio_dec->resampler()->setOutAudioFormat(_audio->audioFormat());
    audio_dec->resampler()->inAudioFormat().setSampleFormatFFmpeg(aCodecCtx->sample_fmt);
    audio_dec->resampler()->inAudioFormat().setSampleRate(aCodecCtx->sample_rate);
    audio_dec->resampler()->inAudioFormat().setChannels(aCodecCtx->channels);
    audio_dec->resampler()->inAudioFormat().setChannelLayoutFFmpeg(aCodecCtx->channel_layout);
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
        QList<Filter*> filters = FilterManager::instance().audioFilters(this);
        //TODO: isEmpty()==false but size() == 0 in debug mode, it's a Qt bug? we can not just foreach without check empty in debug mode
        if (filters.size() > 0) {
            foreach (Filter *filter, filters) {
                audio_thread->installFilter(filter);
            }
        }
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
    return true;
}

bool AVPlayer::setupVideoThread()
{
    if (!vCodecCtx) {
        return false;
    }
    if (!video_dec) {
        video_dec = VideoDecoderFactory::create(VideoDecoderId_FFmpeg);
    } else {
        if (!video_dec->close()) {
            return false;
        }
    }
    video_dec->setCodecContext(vCodecCtx);
    video_dec->prepare();
    if (!video_dec->open()) {
        return false;
    }
    if (!video_thread) {
        video_thread = new VideoThread(this);
        video_thread->setClock(clock);
        video_thread->setDecoder(video_dec);
        video_thread->setStatistics(&mStatistics);
        video_thread->setVideoCapture(video_capture);
        video_thread->setOutputSet(mpVOSet);
        demuxer_thread->setVideoThread(video_thread);
        //reconnect if disconnected
        connect(video_thread, SIGNAL(finished()), this, SIGNAL(stopped()), Qt::DirectConnection);
        QList<Filter*> filters = FilterManager::instance().videoFilters(this);
        if (filters.size() > 0) {
            foreach (Filter *filter, filters) {
                video_thread->installFilter(filter);
            }
        }
    }
#if V1_2
    setRenderer(_renderer);
#endif
    int queue_min = 0.61803*qMax<qreal>(24.0, mStatistics.video.fps);
    int queue_max = int(1.61803*(qreal)queue_min); //about 1 second
    video_thread->packetQueue()->setThreshold(queue_min);
    video_thread->packetQueue()->setCapacity(queue_max);
    return true;
}

void AVPlayer::setupAVThread(AVThread *&thread, AVCodecContext *ctx)
{
    Q_UNUSED(thread);
    Q_UNUSED(ctx);
}

} //namespace QtAV
