/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>

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

#include "QtAV/AVPlayer.h"

#include <limits>

#include <QCoreApplication>
#include <QtCore/QEvent>
#include <QtCore/QDir>
#include <QtCore/QIODevice>
#if QTAV_HAVE(WIDGETS)
#include <QWidget>
#endif //QTAV_HAVE(WIDGETS)
#include "QtAV/AVDemuxer.h"
#include "QtAV/AudioFormat.h"
#include "QtAV/AudioResampler.h"
#include "QtAV/AudioThread.h"
#include "QtAV/Packet.h"
#include "QtAV/AudioDecoder.h"
#include "QtAV/VideoRenderer.h"
#include "QtAV/AVClock.h"
#include "QtAV/VideoCapture.h"
#include "QtAV/VideoDecoderTypes.h"
#include "QtAV/VideoThread.h"
#include "QtAV/AVDemuxThread.h"
#include "QtAV/VideoCapture.h"
#include "QtAV/AudioOutputTypes.h"
#include "filter/FilterManager.h"
#include "output/OutputSet.h"
#include "output/video/VideoOutputEventFilter.h"
#include "QtAV/private/AVCompat.h"

namespace QtAV {

static const int kPosistionCheckMS = 500;
static const qint64 kSeekMS = 10000;
static const qint64 kInvalidPosition = std::numeric_limits<qint64>::max();

static bool correct_audio_channels(AVCodecContext *ctx) {
    if (ctx->channels <= 0) {
        if (ctx->channel_layout) {
            ctx->channels = av_get_channel_layout_nb_channels(ctx->channel_layout);
        }
    } else {
        if (!ctx->channel_layout) {
            ctx->channel_layout = av_get_default_channel_layout(ctx->channels);
        }
    }
    return ctx->channel_layout > 0 && ctx->channels > 0;
}

AVPlayer::AVPlayer(QObject *parent) :
    QObject(parent)
  , loaded(false)
  , _renderer(0)
  , _audio(0)
  , audio_dec(0)
  , video_dec(0)
  , audio_thread(0)
  , video_thread(0)
  , video_capture(0)
  , mSpeed(1.0)
  , ao_enable(true)
  , mBrightness(0)
  , mContrast(0)
  , mSaturation(0)
  , mSeeking(false)
  , mSeekTarget(0)
{
    formatCtx = 0;
    last_position = 0;

    m_pIODevice = 0;
    /*
     * must be the same value at the end of stop(), and must be different from value in
     * stopFromDemuxerThread()(which is false), so the initial value must be true
     */
    reset_state = true;
    media_end_pos = kInvalidPosition;
    start_position = 0;
    stop_position = mediaStopPosition();
    repeat_current = repeat_max = 0;
    timer_id = -1;

    mpVOSet = new OutputSet(this);
    mpAOSet = new OutputSet(this);
    qDebug("%s", aboutQtAV_PlainText().toUtf8().constData());
    /*
     * call stop() before the window(_renderer) closed to stop the waitcondition
     * If close the _renderer widget, the the _renderer may destroy before waking up.
     */
    connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(aboutToQuitApp()));
    clock = new AVClock(AVClock::AudioClock);
    //clock->setClockType(AVClock::ExternalClock);
    connect(&demuxer, SIGNAL(started()), clock, SLOT(start()));
    connect(&demuxer, SIGNAL(error(QtAV::AVError)), this, SIGNAL(error(QtAV::AVError)));
    connect(&demuxer, SIGNAL(mediaStatusChanged(QtAV::MediaStatus)), this, SIGNAL(mediaStatusChanged(QtAV::MediaStatus)));
    demuxer_thread = new AVDemuxThread(this);
    demuxer_thread->setDemuxer(&demuxer);
    //use direct connection otherwise replay may stop immediatly because slot stop() is called after play()
    connect(demuxer_thread, SIGNAL(finished()), this, SLOT(stopFromDemuxerThread()), Qt::DirectConnection);

    video_capture = new VideoCapture(this);

    vcodec_ids
#if QTAV_HAVE(DXVA)
            //<< VideoDecoderId_DXVA
#endif //QTAV_HAVE(DXVA)
#if QTAV_HAVE(VAAPI)
            //<< VideoDecoderId_VAAPI
#endif //QTAV_HAVE(VAAPI)
#if QTAV_HAVE(CEDARV)
            << VideoDecoderId_Cedarv
#endif //QTAV_HAVE(CEDARV)
            << VideoDecoderId_FFmpeg;
    audioout_ids
#if QTAV_HAVE(OPENAL)
            << AudioOutputId_OpenAL
#endif
#if QTAV_HAVE(PORTAUDIO)
            << AudioOutputId_PortAudio
#endif
#if QTAV_HAVE(OPENSL)
            << AudioOutputId_OpenSL
#endif
#if QTAV_HAVE(DSound)
            << AudioOutputId_DSound
#endif
              ;
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
    if (mpVOSet) {
        mpVOSet->clearOutputs();
        delete mpVOSet;
        mpVOSet = 0;
    }
    if (mpAOSet) {
        mpAOSet->clearOutputs();
        delete mpAOSet;
        mpAOSet = 0;
    }
    if (video_capture) {
        delete video_capture;
        video_capture = 0;
    }
    if (clock) {
        delete clock;
        clock = 0;
    }
    if (demuxer_thread) {
        delete demuxer_thread;
        demuxer_thread = 0;
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
#if QTAV_HAVE(WIDGETS)
    QObject *voo = renderer->widget();
    if (voo) {
        //TODO: how to delete filter if no parent?
        //the filtering object must be in the same thread as this object.
        if (renderer->widget())
            voo->installEventFilter(new VideoOutputEventFilter(renderer));
    }
#endif //QTAV_HAVE(WIDGETS)
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
}

VideoRenderer *AVPlayer::renderer()
{
    //QList assert empty in debug mode
    if (!mpVOSet || mpVOSet->outputs().isEmpty())
        return 0;
    return static_cast<VideoRenderer*>(mpVOSet->outputs().last());
}

QList<VideoRenderer*> AVPlayer::videoOutputs()
{
    if (!mpVOSet)
        return QList<VideoRenderer*>();
    QList<VideoRenderer*> vos;
    vos.reserve(mpVOSet->outputs().size());
    foreach (AVOutput *out, mpVOSet->outputs()) {
        vos.append(static_cast<VideoRenderer*>(out));
    }
    return vos;
}

void AVPlayer::setAudioOutput(AudioOutput* ao)
{
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
    //bool need_lock = isPlaying() && !thread->isPaused();
    //if (need_lock)
    //    thread->lock();
    qDebug("set AVThread output");
    thread->setOutput(pOut);
    if (pOut) {
        pOut->setStatistics(&mStatistics);
        //if (need_lock)
        //    thread->unlock(); //??why here?
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
    }
    masterClock()->setSpeed(mSpeed);
    emit speedChanged(mSpeed);
}

qreal AVPlayer::speed() const
{
    return mSpeed;
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
        //return false;
    }
    AVThread *avthread = video_thread;
    if (!avthread || !avthread->filters().contains(filter)) {
        avthread = audio_thread;
    }
    if (!avthread || !avthread->filters().contains(filter)) {
        return false;
    }
    avthread->uninstallFilter(filter, true);
    return true;
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
    avthread->scheduleTask(new UninstallFilterTask(avthread, filter));
    return true;
}

void AVPlayer::setPriority(const QVector<VideoDecoderId> &ids)
{
    vcodec_ids = ids;
}

void AVPlayer::setOptionsForFormat(const QVariantHash &dict)
{
    demuxer.setOptions(dict);
}

QVariantHash AVPlayer::optionsForFormat() const
{
    return demuxer.options();
}

void AVPlayer::setOptionsForAudioCodec(const QVariantHash &dict)
{
    audio_codec_opt = dict;
}

QVariantHash AVPlayer::optionsForAudioCodec() const
{
    return audio_codec_opt;
}

void AVPlayer::setOptionsForVideoCodec(const QVariantHash &dict)
{
    video_codec_opt = dict;
}

QVariantHash AVPlayer::optionsForVideoCodec() const
{
    return video_codec_opt;
}

/*
 * loaded state is the state of current setted file.
 * For replaying, we can avoid load a seekable file again.
 * For playing a new file, load() is required.
 */
void AVPlayer::setFile(const QString &path)
{
    reset_state = this->path != path;
    this->path = path;
    if (reset_state)
        emit sourceChanged();
    reset_state = !this->path.isEmpty() && reset_state;
    demuxer.setAutoResetStream(reset_state);
    // TODO: use absoluteFilePath?
    m_pIODevice = 0;
    loaded = false; //
    //qApp->activeWindow()->setWindowTitle(path); //crash on linux
}

QString AVPlayer::file() const
{
    return path;
}

void AVPlayer::setIODevice(QIODevice* device)
{
    if (!device)
        return;

    if (device->isSequential()) {
        qDebug("No support for sequential devices.");
        return;
    }
    demuxer.setAutoResetStream(reset_state);
    reset_state = m_pIODevice != device;
    m_pIODevice = device;
    loaded = false;
    if (!path.isEmpty())
        emit sourceChanged();
    path = QString();
}

VideoCapture* AVPlayer::videoCapture()
{
    return video_capture;
}

bool AVPlayer::captureVideo()
{
    if (!video_capture || !video_thread)
        return false;
    if (isPaused()) {
        QString cap_name = QFileInfo(file()).completeBaseName();
        video_capture->setCaptureName(cap_name + "_" + QString::number(masterClock()->value(), 'f', 3));
        video_capture->start();
        return true;
    }
    video_capture->request();
    return true;
}

bool AVPlayer::play(const QString& path)
{
    setFile(path);
    play();
    return true;//isPlaying();
}

// TODO: check repeat status?
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

MediaStatus AVPlayer::mediaStatus() const
{
    return demuxer.mediaStatus();
}

bool AVPlayer::load(const QString &path, bool reload)
{
    setFile(path);
    return load(reload);
}

bool AVPlayer::load(bool reload)
{
    loaded = false;
    if (path.isEmpty() && !m_pIODevice) {
        qDebug("No file or IODevice was set.");
        return false;
    }
    // release codec ctx
    if (audio_dec) {
        audio_dec->setCodecContext(0);
    }
    if (video_dec) {
        video_dec->setCodecContext(0);
    }
    if (m_pIODevice)
        qDebug("Loading from IODevice...");
    else
        qDebug("loading: %s ...", path.toUtf8().constData());
    if (reload || !demuxer.isLoaded(path)) {
        //close decoders here to make sure open and close in the same thread
        if (audio_dec && audio_dec->isOpen()) {
            audio_dec->close();
        }
        if (video_dec && video_dec->isOpen()) {
            video_dec->close();
        }
        if (!m_pIODevice) {
            if (!demuxer.loadFile(path))
                return false;
        } else {
            if (!demuxer.load(m_pIODevice))
                return false;
        }
    } else {
        demuxer.prepareStreams();
    }
    loaded = true;
    formatCtx = demuxer.formatContext();

    if (masterClock()->isClockAuto()) {
        qDebug("auto select clock: audio > external");
        if (!demuxer.audioCodecContext()) {
            qWarning("No audio found or audio not supported. Using ExternalClock");
            masterClock()->setClockType(AVClock::ExternalClock);
        } else {
            qDebug("Using AudioClock");
            masterClock()->setClockType(AVClock::AudioClock);
        }
    }

    // TODO: what about other proctols? some vob duration() == 0
    if ((path.startsWith("file:") || QFile(path).exists()) && duration() > 0) {
        media_end_pos = mediaStartPosition() + duration();
    } else {
        media_end_pos = kInvalidPosition;
    }
    // if file changed or stop() called by user, stop_position changes.
    if (stopPosition() > mediaStopPosition() || stopPosition() == 0) {
        stop_position = mediaStopPosition();
    }

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

qreal AVPlayer::durationF() const
{
    return double(demuxer.durationUs())/double(AV_TIME_BASE); //AVFrameContext.duration time base: AV_TIME_BASE
}

qint64 AVPlayer::duration() const
{
    return demuxer.duration();
}

qint64 AVPlayer::mediaStartPosition() const
{
    // check stopposition?
    if (demuxer.startTime() >= mediaStopPosition())
        return 0;
    return demuxer.startTime();
}

qint64 AVPlayer::mediaStopPosition() const
{
    return media_end_pos;
}

qreal AVPlayer::mediaStartPositionF() const
{
    return double(demuxer.startTimeUs())/double(AV_TIME_BASE);
}

qint64 AVPlayer::startPosition() const
{
    return start_position;
}

void AVPlayer::setStartPosition(qint64 pos)
{
    if (pos > stopPosition() && stopPosition() > 0) {
        qWarning("start position too large (%lld > %lld). ignore", pos, stopPosition());
        return;
    }
    start_position = pos;
    if (start_position < 0)
        start_position += mediaStopPosition();
    if (start_position < 0)
        start_position = 0;
    emit startPositionChanged(start_position);
}

qint64 AVPlayer::stopPosition() const
{
    return stop_position;
}

void AVPlayer::setStopPosition(qint64 pos)
{
    if (pos == -mediaStopPosition()) {
        return;
    }
    if (pos == 0) {
        pos = mediaStopPosition();
    }
    if (pos > mediaStopPosition()) {
        qWarning("stop position too large (%lld > %lld). ignore", pos, mediaStopPosition());
        return;
    }
    stop_position = pos;
    if (stop_position < 0)
        stop_position += mediaStopPosition();
    if (stop_position < 0)
        stop_position = mediaStopPosition();
    if (stop_position < startPosition()) {
        qWarning("stop postion %lld < start position %lld. ignore", stop_position, startPosition());
        return;
    }
    emit stopPositionChanged(stop_position);
}

qreal AVPlayer::positionF() const
{
    return clock->value();
}

qint64 AVPlayer::position() const
{
    return clock->value()*1000.0; //TODO: avoid *1000.0
}

void AVPlayer::setPosition(qint64 position)
{
    if (!isPlaying())
        return;
    if (position < 0)
        position += mediaStopPosition();
    mSeeking = true;
    mSeekTarget = position;
    qreal s = (qreal)position/1000.0;
    // TODO: check flag accurate seek
    if (audio_thread) {
        audio_thread->skipRenderUntil(s);
    }
    if (video_thread) {
        video_thread->skipRenderUntil(s);
    }
    qDebug("seek to %lld ms (%f%%)", position, double(position)/double(duration())*100.0);
    masterClock()->updateValue(double(position)/1000.0); //what is duration == 0
    masterClock()->updateExternalClock(position); //in msec. ignore usec part using t/1000
    demuxer_thread->seek(position);

    emit positionChanged(position);
}

int AVPlayer::repeat() const
{
    return repeat_max;
}

int AVPlayer::currentRepeat() const
{
    return repeat_current;
}

// TODO: reset current_repeat?
void AVPlayer::setRepeat(int max)
{
    repeat_max = max;
    if (repeat_max < 0)
        repeat_max = std::numeric_limits<int>::max();
    emit repeatChanged(repeat_max);
}


bool AVPlayer::setAudioStream(int n, bool now)
{
    if (!demuxer.setStreamIndex(AVDemuxer::AudioStream, n)) {
        qWarning("set video stream to %d failed", n);
        return false;
    }
    loaded = false;
    last_position = -1;
    demuxer.setAutoResetStream(false);
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
    last_position = -1;
    demuxer.setAutoResetStream(false);
    if (!now)
        return true;
    play();
    return isPlaying();
}

bool AVPlayer::setSubtitleStream(int n, bool now)
{
    Q_UNUSED(n);
    Q_UNUSED(now);
    demuxer.setAutoResetStream(false);
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
    bool start_last = last_position == -1;
    if (isPlaying()) {
        if (start_last) {
            clock->pause(true); //external clock
            last_position = mediaStopPosition() != kInvalidPosition ? -position() : 0;
            reset_state = false; //set a new stream number should not reset other states
            qDebug("start pos is current position: %lld", last_position);
        } else {
            last_position = 0;
            qDebug("start pos is stream start time");
        }
        qint64 last_pos = last_position;
        stop();
        // wait here to ensure stopped() and startted() is in correct order
        if (demuxer_thread->isRunning()) {
            demuxer_thread->wait(500);
        }
        if (last_pos < 0)
            last_position = -last_pos;
    }
    /*
     * avoid load mutiple times when replaying the same seekable file
     * TODO: force load unseekable stream? avio.seekable. currently you
     * must setFile() agian to reload an unseekable stream
     */
    //TODO: no eof if replay by seek(0)
#if EOF_ISSUE_SOLVED
    if (!isLoaded()) { //if (!isLoaded() && !load())
        if (!load(false)) {
            mStatistics.reset();
            return;
        } else {
            initStatistics();
        }
    } else {
        qDebug("seek(%f)", last_position);
        demuxer.seek(last_position); //FIXME: now assume it is seekable. for unseekable, setFile() again
#else
        if (!load(true)) {
            mStatistics.reset();
            return;
        }
        initStatistics();

#endif //EOF_ISSUE_SOLVED
#if EOF_ISSUE_SOLVED
    }
#endif //EOF_ISSUE_SOLVED

    if (demuxer.audioCodecContext() && audio_thread) {
        qDebug("Starting audio thread...");
        audio_thread->start();
        audio_thread->waitForReady();
    }
    if (demuxer.videoCodecContext() && video_thread) {
        qDebug("Starting video thread...");
        video_thread->start();
        video_thread->waitForReady();
    }
    if (startPosition() > 0 && startPosition() < mediaStopPosition() && last_position <= 0) {
        demuxer.seek((qint64)startPosition());
    }
    demuxer_thread->start();
    if (start_last) {
        clock->pause(false); //external clock
    } else {
        clock->reset();
    }
    if (timer_id < 0) {
        //timer_id = startTimer(kPosistionCheckMS); //may fail if not in this thread
        QMetaObject::invokeMethod(this, "startNotifyTimer", Qt::AutoConnection);
    }
// ffplay does not seek to stream's start position. usually it's 0, maybe < 1. seeking will result in a non-key frame position and it's bad.
    //if (last_position <= 0)
    //    last_position = mediaStartPosition();
    if (last_position > 0)
        setPosition(last_position); //just use demuxer.startTime()/duration()?

    emit started(); //we called stop(), so must emit started()
}

void AVPlayer::stopFromDemuxerThread()
{
    reset_state = false;
    qDebug("demuxer thread emit finished. avplayer emit stopped()");
    emit stopped();
}

void AVPlayer::aboutToQuitApp()
{
    reset_state = true;
    stop();
    while (isPlaying()) {
        qApp->processEvents();
        qDebug("about to quit.....");
        pause(false); // may be paused. then aboutToQuitApp will not finish
        stop();
    }
}

void AVPlayer::startNotifyTimer()
{
    timer_id = startTimer(kPosistionCheckMS);
}

void AVPlayer::stopNotifyTimer()
{
    killTimer(timer_id);
    timer_id = -1;
}

// TODO: doc about when the state will be reset
void AVPlayer::stop()
{
    // check timer_id, <0 return?
    if (reset_state) {
        /*
         * must kill in main thread! If called from user, may be not in main thread.
         * then timer goes on, and player find that stop_position reached(stop_position is already
         * 0 after user call stop),
         * stop() is called again by player and reset state. but this call is later than demuxer stop.
         * so if user call play() immediatly, may be stopped by AVPlayer
         */
        // TODO: invokeMethod "stopNotifyTimer"
        if (timer_id >= 0) {
            qDebug("timer: %d, current thread: %p, player thread: %p", timer_id, QThread::currentThread(), thread());
            if (QThread::currentThread() == thread()) { //called by user in the same thread as player
                killTimer(timer_id);
                timer_id = -1;
            } else {
                //TODO: post event.
            }
        }
        start_position = 0;
        stop_position = 0; // 0 can stop play in timerEvent
        media_end_pos = kInvalidPosition;
        repeat_current = repeat_max = 0;
    } else { //called by player
        if (timer_id >= 0) {
            killTimer(timer_id);
            timer_id = -1;
        }
    }
    reset_state = true;

    last_position = mediaStopPosition() != kInvalidPosition ? startPosition() : 0;
    if (!isPlaying()) {
        qDebug("Not playing~");
        return;
    }

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
        qDebug("stopping %s...", threads[i].name);
        thread->stop();
        // use if instead of while? eventloop in play()
        while (thread->isRunning()) {
            thread->wait(500);
            qDebug("stopping %s...", threads[i].name);
        }
    }
    // can not close decoders here since close and open may be in different threads
    qDebug("all audio/video threads  stopped...");
}

void AVPlayer::timerEvent(QTimerEvent *te)
{
    if (te->timerId() == timer_id) {
        // killTimer() should be in the same thread as object. kill here?
        if (isPaused()) {
            return;
        }
        // active only when playing
        const qint64 t = clock->value()*1000.0;
        if (stopPosition() == kInvalidPosition) {
            // not seekable. network stream
            emit positionChanged(t);
            return;
        }
        if (mSeeking && t >= mSeekTarget + 1000) {
            mSeeking = false;
            mSeekTarget = 0;
        }
        if (t <= stopPosition()) {
            if (!mSeeking) {
                emit positionChanged(t);
            }
            return;
        }
        // TODO: remove. kill timer in an event;
        if (stopPosition() == 0) { //stop() by user in other thread, state is already reset
            reset_state = false;
            qDebug("stopPosition() == 0, stop");
            stop();
        }
        // t < start_position is ok. repeat_max<0 means repeat forever
        if (currentRepeat() >= repeat() && repeat() >= 0) {
            reset_state = true; // true is default, can remove here
            qDebug("stopPosition() reached and no repeat: %d", repeat());
            stop();
            return;
        }
        repeat_current++;
        // FIXME: now stop instead of seek if reach media's end. otherwise will not get eof again
        if (stopPosition() == mediaStopPosition() || !demuxer.isSeekable()) {
            // if not seekable, how it can start to play at specified position?
            qDebug("stopPosition() == mediaStopPosition() or !seekable. repeat_current=%d", repeat_current);
            reset_state = false;
            stop();
            last_position = startPosition(); // for seeking to startPosition(). already set in stop()
            play();
        } else {
            qDebug("stopPosition() != mediaStopPosition() and seekable. repeat_current=%d", repeat_current);
            setPosition(startPosition());
        }
    }
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

void AVPlayer::seek(qreal r)
{
    seek(qint64(r*double(duration())));
}

void AVPlayer::seek(qint64 pos)
{
    setPosition(pos);
}

void AVPlayer::seekForward()
{
    seek(position() + kSeekMS);
}

void AVPlayer::seekBackward()
{
    seek(position() - kSeekMS);
}

void AVPlayer::updateClock(qint64 msecs)
{
    clock->updateExternalClock(msecs);
}

int AVPlayer::brightness() const
{
    return mBrightness;
}

void AVPlayer::setBrightness(int val)
{
    if (mBrightness == val)
        return;
    mBrightness = val;
    emit brightnessChanged(mBrightness);
    if (video_thread) {
        video_thread->setBrightness(val);
    }
}

int AVPlayer::contrast() const
{
    return mContrast;
}

void AVPlayer::setContrast(int val)
{
    if (mContrast == val)
        return;
    mContrast = val;
    emit contrastChanged(mContrast);
    if (video_thread) {
        video_thread->setContrast(val);
    }
}

int AVPlayer::hue() const
{
    return 0;
}

void AVPlayer::setHue(int val)
{
    Q_UNUSED(val);
    qWarning("Not implemented");
}

int AVPlayer::saturation() const
{
    return mSaturation;
}

void AVPlayer::setSaturation(int val)
{
    if (mSaturation == val)
        return;
    mSaturation = val;
    emit saturationChanged(mSaturation);
    if (video_thread) {
        video_thread->setSaturation(val);
    }
}

//TODO: av_guess_frame_rate in latest ffmpeg
void AVPlayer::initStatistics()
{
    mStatistics.reset();
    mStatistics.url = path;
    mStatistics.bit_rate = formatCtx->bit_rate;
    mStatistics.format = formatCtx->iformat->name;
    //AV_TIME_BASE_Q: msvc error C2143
    //formatCtx->duration may be AV_NOPTS_VALUE. AVDemuxer.duration deals with this case
    mStatistics.start_time = QTime(0, 0, 0).addMSecs(int(mediaStartPosition()));
    mStatistics.duration = QTime(0, 0, 0).addMSecs((int)duration());
    if (video_dec)
        mStatistics.video.decoder = VideoDecoderFactory::name(video_dec->id()).c_str();
    mStatistics.metadata.clear();
    AVDictionaryEntry *tag = NULL;
    while ((tag = av_dict_get(formatCtx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        mStatistics.metadata.insert(tag->key, tag->value);
    }
    struct common_statistics_t {
        int stream_idx;
        AVCodecContext *ctx;
        Statistics::Common *st;
        const char *name;
    } common_statistics[] = {
        { demuxer.videoStream(), demuxer.videoCodecContext(), &mStatistics.video, "video"},
        { demuxer.audioStream(), demuxer.audioCodecContext(), &mStatistics.audio, "audio"},
        { 0, 0, 0, 0}
    };
    for (int i = 0; common_statistics[i].name; ++i) {
        common_statistics_t cs = common_statistics[i];
        if (cs.stream_idx < 0)
            continue;
        AVStream *stream = formatCtx->streams[cs.stream_idx];
        qDebug("stream: %d, duration=%lld (%lld ms==%lld), time_base=%f", cs.stream_idx, stream->duration, qint64(qreal(stream->duration)*av_q2d(stream->time_base)*1000.0)
               , duration(), av_q2d(stream->time_base));
        cs.st->available = true;
        if (cs.ctx->codec) {
            cs.st->codec = cs.ctx->codec->name;
            cs.st->codec_long = cs.ctx->codec->long_name;
        }
        cs.st->total_time = QTime(0, 0, 0).addMSecs(int(qreal(stream->duration)*av_q2d(stream->time_base)*1000.0));
        cs.st->start_time = QTime(0, 0, 0).addMSecs(int(qreal(stream->start_time)*av_q2d(stream->time_base)*1000.0));
        qDebug("codec: %s(%s)", qPrintable(cs.st->codec), qPrintable(cs.st->codec_long));
        cs.st->bit_rate = cs.ctx->bit_rate; //formatCtx
        cs.st->frames = stream->nb_frames;
        //qDebug("time: %f~%f, nb_frames=%lld", cs.st->start_time, cs.st->total_time, stream->nb_frames); //why crash on mac? av_q2d({0,0})?
        tag = NULL;
        while ((tag = av_dict_get(stream->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
            cs.st->metadata.insert(tag->key, tag->value);
        }
    }
    if (demuxer.audioStream() >= 0) {
        AVCodecContext *aCodecCtx = demuxer.audioCodecContext();
        correct_audio_channels(aCodecCtx);
        mStatistics.audio_only.block_align = aCodecCtx->block_align;
        mStatistics.audio_only.channels = aCodecCtx->channels;
        char cl[128]; //
        // nb_channels -1: will use av_get_channel_layout_nb_channels
        av_get_channel_layout_string(cl, sizeof(cl), aCodecCtx->channels, aCodecCtx->channel_layout); //TODO: ff version
        mStatistics.audio_only.channel_layout = cl;
        mStatistics.audio_only.sample_fmt = av_get_sample_fmt_name(aCodecCtx->sample_fmt);
        mStatistics.audio_only.frame_number = aCodecCtx->frame_number;
        mStatistics.audio_only.frame_size = aCodecCtx->frame_size;
        mStatistics.audio_only.sample_rate = aCodecCtx->sample_rate;
    }
    if (demuxer.videoStream() >= 0) {
        AVCodecContext *vCodecCtx = demuxer.videoCodecContext();
        AVStream *stream = formatCtx->streams[demuxer.videoStream()];
        mStatistics.video.frames = stream->nb_frames;
        //FIXME: which 1 should we choose? avg_frame_rate may be nan, r_frame_rate may be wrong(guessed value)
        // TODO: seems that r_frame_rate will be removed libav > 9.10. Use macro to check version?
        //if (stream->avg_frame_rate.num) //avg_frame_rate.num,den may be 0
            mStatistics.video_only.fps_guess = av_q2d(stream->avg_frame_rate);
        //else
        //    mStatistics.video_only.fps_guess = av_q2d(stream->r_frame_rate);
        mStatistics.video_only.fps = mStatistics.video_only.fps_guess;
        mStatistics.video_only.avg_frame_rate = av_q2d(stream->avg_frame_rate);
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
    AVCodecContext *aCodecCtx = demuxer.audioCodecContext();
    if (!aCodecCtx) {
        return false;
    }
    qDebug("has audio");
    if (!audio_dec) {
        audio_dec = new AudioDecoder();
    }
    audio_dec->setCodecContext(aCodecCtx);
    audio_dec->setOptions(audio_codec_opt);
    if (!audio_dec->open()) {
        return false;
    }
    //TODO: setAudioOutput() like vo
    if (!_audio && ao_enable) {
        foreach (AudioOutputId aoid, audioout_ids) {
            qDebug("trying audio output '%s'", AudioOutputFactory::name(aoid).c_str());
            _audio = AudioOutputFactory::create(aoid);
            if (_audio) {
                qDebug("audio output found.");
                break;
            }
        }
    }
    if (!_audio) {
        // TODO: only when no audio stream or user disable audio stream. running an audio thread without sound is waste resource?
        //masterClock()->setClockType(AVClock::ExternalClock);
        //return;
    } else {
        _audio->close();
        correct_audio_channels(aCodecCtx);
        AudioFormat af;
        af.setSampleRate(aCodecCtx->sample_rate);
        af.setSampleFormatFFmpeg(aCodecCtx->sample_fmt);
        // 5, 6, 7 channels may not play
        if (aCodecCtx->channels > 2)
            af.setChannelLayout(_audio->preferredChannelLayout());
        else
            af.setChannelLayoutFFmpeg(aCodecCtx->channel_layout);
        //af.setChannels(aCodecCtx->channels);
        // FIXME: workaround. planar convertion crash now!
        if (af.isPlanar()) {
            af.setSampleFormat(_audio->preferredSampleFormat());
        }
        if (!_audio->isSupported(af)) {
            if (!_audio->isSupported(af.sampleFormat())) {
                af.setSampleFormat(_audio->preferredSampleFormat());
            }
            if (!_audio->isSupported(af.channelLayout())) {
                af.setChannelLayout(_audio->preferredChannelLayout());
            }
        }
        _audio->setAudioFormat(af);
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
        audio_thread->setOutputSet(mpAOSet);
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
    int queue_min = 0.61803*qMax<qreal>(24.0, mStatistics.video_only.fps_guess);
    int queue_max = int(1.61803*(qreal)queue_min); //about 1 second
    audio_thread->packetQueue()->setThreshold(queue_min);
    audio_thread->packetQueue()->setCapacity(queue_max);
    return true;
}

bool AVPlayer::setupVideoThread()
{
    AVCodecContext *vCodecCtx = demuxer.videoCodecContext();
    if (!vCodecCtx) {
        return false;
    }
    /*
    if (!video_dec) {
        video_dec = VideoDecoderFactory::create(VideoDecoderId_FFmpeg);
    }
    */
    if (video_dec) {
        delete video_dec;
        video_dec = 0;
    }
    foreach(VideoDecoderId vid, vcodec_ids) {
        qDebug("**********trying video decoder: %s...", VideoDecoderFactory::name(vid).c_str());
        VideoDecoder *vd = VideoDecoderFactory::create(vid);
        if (!vd) {
            continue;
        }
        //vd->isAvailable() //TODO: the value is wrong now
        vd->setCodecContext(vCodecCtx);
        vd->setOptions(video_codec_opt);
        if (vd->prepare() && vd->open()) {
            video_dec = vd;
            qDebug("**************Video decoder found");
            break;
        }
        delete vd;
    }
    if (!video_dec) {
        qWarning("No video decoder can be used.");
        return false;
    }

    if (!video_thread) {
        video_thread = new VideoThread(this);
        video_thread->setClock(clock);
        video_thread->setStatistics(&mStatistics);
        video_thread->setVideoCapture(video_capture);
        video_thread->setOutputSet(mpVOSet);
        demuxer_thread->setVideoThread(video_thread);

        QList<Filter*> filters = FilterManager::instance().videoFilters(this);
        if (filters.size() > 0) {
            foreach (Filter *filter, filters) {
                video_thread->installFilter(filter);
            }
        }
    }
    video_thread->setDecoder(video_dec);
    video_thread->setBrightness(mBrightness);
    video_thread->setContrast(mContrast);
    video_thread->setSaturation(mSaturation);
    int queue_min = 0.61803*qMax<qreal>(24.0, mStatistics.video_only.fps_guess);
    int queue_max = int(1.61803*(qreal)queue_min); //about 1 second
    video_thread->packetQueue()->setThreshold(queue_min);
    video_thread->packetQueue()->setCapacity(queue_max);
    return true;
}

} //namespace QtAV
