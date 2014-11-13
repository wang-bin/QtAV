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

#include "AVPlayerPrivate.h"

#include <limits>

#include <QtCore/QCoreApplication>
#include <QtCore/QEvent>
#include <QtCore/QDir>
#include <QtCore/QIODevice>
#include <QtCore/QThreadPool>
#if QTAV_HAVE(WIDGETS)
#include <QWidget>
#endif //QTAV_HAVE(WIDGETS)
#include "QtAV/AVDemuxer.h"
#include "QtAV/Packet.h"
#include "QtAV/AudioDecoder.h"
#include "QtAV/VideoRenderer.h"
#include "QtAV/AVClock.h"
#include "QtAV/VideoCapture.h"
#include "QtAV/VideoDecoderTypes.h"
#include "QtAV/VideoCapture.h"
#include "QtAV/AudioOutputTypes.h"
#include "filter/FilterManager.h"
#include "output/OutputSet.h"
#include "output/video/VideoOutputEventFilter.h"
#include "AudioThread.h"
#include "VideoThread.h"
#include "AVDemuxThread.h"
#include "QtAV/private/AVCompat.h"
#include "utils/Logger.h"

#define EOF_ISSUE_SOLVED 0
namespace QtAV {

static const int kPosistionCheckMS = 500;
static const qint64 kSeekMS = 10000;

AVPlayer::AVPlayer(QObject *parent) :
    QObject(parent)
  , d(new Private())
{
    d->vos = new OutputSet(this);
    d->aos = new OutputSet(this);
    /*
     * call stop() before the window(d->vo) closed to stop the waitcondition
     * If close the d->vo widget, the the d->vo may destroy before waking up.
     */
    connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(aboutToQuitApp()));
    //d->clock->setClockType(AVClock::ExternalClock);
    connect(&d->demuxer, SIGNAL(started()), d->clock, SLOT(start()));
    connect(&d->demuxer, SIGNAL(error(QtAV::AVError)), this, SIGNAL(error(QtAV::AVError)));
    connect(&d->demuxer, SIGNAL(mediaStatusChanged(QtAV::MediaStatus)), this, SIGNAL(mediaStatusChanged(QtAV::MediaStatus)));
    connect(&d->demuxer, SIGNAL(loaded()), this, SIGNAL(loaded()));
    d->read_thread = new AVDemuxThread(this);
    d->read_thread->setDemuxer(&d->demuxer);
    //direct connection can not sure slot order?
    connect(d->read_thread, SIGNAL(finished()), this, SLOT(stopFromDemuxerThread()));
    connect(d->read_thread, SIGNAL(requestClockPause(bool)), masterClock(), SLOT(pause(bool)), Qt::DirectConnection);

    d->vcapture = new VideoCapture(this);
}

AVPlayer::~AVPlayer()
{
    stop();
    QList<Filter*> filters(FilterManager::instance().videoFilters(this));
    foreach (Filter *f, filters) {
        uninstallFilter(f);
    }
}

AVClock* AVPlayer::masterClock()
{
    return d->clock;
}

void AVPlayer::addVideoRenderer(VideoRenderer *renderer)
{
    if (!renderer) {
        qWarning("add a null renderer!");
        return;
    }
    renderer->setStatistics(&d->statistics);
#if QTAV_HAVE(WIDGETS)
    QObject *voo = renderer->widget();
    if (voo) {
        //TODO: how to delete filter if no parent?
        //the filtering object must be in the same thread as this object.
        if (renderer->widget())
            voo->installEventFilter(new VideoOutputEventFilter(renderer));
    }
#endif //QTAV_HAVE(WIDGETS)
    d->vos->addOutput(renderer);
}

void AVPlayer::removeVideoRenderer(VideoRenderer *renderer)
{
    d->vos->removeOutput(renderer);
}

void AVPlayer::clearVideoRenderers()
{
    d->vos->clearOutputs();
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
    clearVideoRenderers();
    if (!r)
        return;
    r->resizeRenderer(r->rendererSize()); //IMPORTANT: the swscaler will resize
    r->setStatistics(&d->statistics);
    addVideoRenderer(r);
}

VideoRenderer *AVPlayer::renderer()
{
    //QList assert empty in debug mode
    if (!d->vos || d->vos->outputs().isEmpty())
        return 0;
    return static_cast<VideoRenderer*>(d->vos->outputs().last());
}

QList<VideoRenderer*> AVPlayer::videoOutputs()
{
    if (!d->vos)
        return QList<VideoRenderer*>();
    QList<VideoRenderer*> vos;
    vos.reserve(d->vos->outputs().size());
    foreach (AVOutput *out, d->vos->outputs()) {
        vos.append(static_cast<VideoRenderer*>(out));
    }
    return vos;
}

void AVPlayer::setAudioOutput(AudioOutput* ao)
{
    d->setAVOutput(d->ao, ao, d->athread);
}

AudioOutput* AVPlayer::audio()
{
    return d->ao;
}

void AVPlayer::enableAudio(bool enable)
{
    d->ao_enabled = enable;
}

void AVPlayer::disableAudio(bool disable)
{
    d->ao_enabled = !disable;
}

void AVPlayer::setMute(bool mute)
{
    if (d->ao)
        d->ao->setMute(mute);
}

bool AVPlayer::isMute() const
{
    return !d->ao || d->ao->isMute();
}

void AVPlayer::setSpeed(qreal speed)
{
    if (speed == d->speed)
        return;
    d->speed = speed;
    //TODO: check clock type?
    if (d->ao && d->ao->isAvailable()) {
        qDebug("set speed %.2f", d->speed);
        d->ao->setSpeed(d->speed);
    }
    masterClock()->setSpeed(d->speed);
    emit speedChanged(d->speed);
}

qreal AVPlayer::speed() const
{
    return d->speed;
}

Statistics& AVPlayer::statistics()
{
    return d->statistics;
}

const Statistics& AVPlayer::statistics() const
{
    return d->statistics;
}

bool AVPlayer::installAudioFilter(Filter *filter)
{
    if (!FilterManager::instance().registerAudioFilter(filter, this)) {
        return false;
    }
    if (!d->athread)
        return false; //install later when avthread created
    return d->athread->installFilter(filter);
}

bool AVPlayer::installVideoFilter(Filter *filter)
{
    if (!FilterManager::instance().registerVideoFilter(filter, this)) {
        return false;
    }
    if (!d->vthread)
        return false; //install later when avthread created
    return d->vthread->installFilter(filter);
}

bool AVPlayer::uninstallFilter(Filter *filter)
{
    if (!FilterManager::instance().unregisterFilter(filter)) {
        qWarning("unregister filter %p failed", filter);
        //return false;
    }
    AVThread *avthread = d->vthread;
    if (!avthread || !avthread->filters().contains(filter)) {
        avthread = d->athread;
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
    d->vc_ids = ids;
}

void AVPlayer::setOptionsForFormat(const QVariantHash &dict)
{
    d->demuxer.setOptions(dict);
}

QVariantHash AVPlayer::optionsForFormat() const
{
    return d->demuxer.options();
}

void AVPlayer::setOptionsForAudioCodec(const QVariantHash &dict)
{
    d->ac_opt = dict;
}

QVariantHash AVPlayer::optionsForAudioCodec() const
{
    return d->ac_opt;
}

void AVPlayer::setOptionsForVideoCodec(const QVariantHash &dict)
{
    d->vc_opt = dict;
}

QVariantHash AVPlayer::optionsForVideoCodec() const
{
    return d->vc_opt;
}

/*
 * loaded state is the state of current setted file.
 * For replaying, we can avoid load a seekable file again.
 * For playing a new file, load() is required.
 */
void AVPlayer::setFile(const QString &path)
{
    d->reset_state = d->current_source.type() != QVariant::String || d->current_source.toString() != path;
    d->current_source = path;
    if (d->reset_state) {
        emit sourceChanged();
        //emit error(AVError(AVError::NoError));
    }
    d->reset_state = !path.isEmpty() && d->reset_state;
    d->demuxer.setAutoResetStream(d->reset_state);
    // TODO: use absoluteFilePath?
    d->loaded = false; //
}

QString AVPlayer::file() const
{
    if (d->current_source.type() == QVariant::String)
        return d->current_source.toString();
    return QString();
}

void AVPlayer::setIODevice(QIODevice* device)
{
    if (!device)
        return;

    if (device->isSequential()) {
        qDebug("No support for sequential devices.");
        return;
    }
    d->demuxer.setAutoResetStream(d->reset_state);
    // FIXME: not compare QVariant::String
    d->reset_state = d->current_source.type() == QVariant::String || d->current_source.value<QIODevice*>() != device;
    d->current_source = QVariant::fromValue(device);
    d->loaded = false;
    if (d->reset_state)
        emit sourceChanged();
}

VideoCapture* AVPlayer::videoCapture()
{
    return d->vcapture;
}

bool AVPlayer::captureVideo()
{
    if (!d->vcapture || !d->vthread)
        return false;
    if (isPaused()) {
        QString cap_name = QFileInfo(file()).completeBaseName();
        d->vcapture->setCaptureName(cap_name + "_" + QString::number(masterClock()->value(), 'f', 3));
        d->vcapture->start();
        return true;
    }
    d->vcapture->request();
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
    return (d->read_thread &&d->read_thread->isRunning())
            || (d->athread && d->athread->isRunning())
            || (d->vthread && d->vthread->isRunning());
}

void AVPlayer::togglePause()
{
    pause(!isPaused());
}

void AVPlayer::pause(bool p)
{
    //pause thread. check pause state?
    d->read_thread->pause(p);
    if (d->athread)
        d->athread->pause(p);
    if (d->vthread)
        d->vthread->pause(p);
    d->clock->pause(p);
    emit paused(p);
}

bool AVPlayer::isPaused() const
{
    return (d->read_thread && d->read_thread->isPaused())
            || (d->athread && d->athread->isPaused())
            || (d->vthread && d->vthread->isPaused());
}

MediaStatus AVPlayer::mediaStatus() const
{
    return d->demuxer.mediaStatus();
}

void AVPlayer::setAutoLoad(bool value)
{
    if (d->auto_load == value)
        return;
    d->auto_load = value;
    emit autoLoadChanged();
}

bool AVPlayer::isAutoLoad() const
{
    return d->auto_load;
}

void AVPlayer::setAsyncLoad(bool value)
{
    if (d->async_load == value)
        return;
    d->async_load = value;
    emit asyncLoadChanged();
}

bool AVPlayer::isAsyncLoad() const
{
    return d->async_load;
}

bool AVPlayer::isLoaded() const
{
    return d->loaded;
}

bool AVPlayer::load(const QString &path, bool reload)
{
    setFile(path);
    return load(reload);
}

bool AVPlayer::load(bool reload)
{
    d->loaded = false;
    if (!d->current_source.isValid()) {
        qDebug("Invalid media source. No file or IODevice was set.");
        return false;
    }
    // release codec ctx
    if (d->adec) {
        d->adec->setCodecContext(0);
    }
    if (d->vdec) {
        d->vdec->setCodecContext(0);
    }
    qDebug() << "Loading " << d->current_source << " ...";
    if (reload || !d->demuxer.isLoaded(d->current_source.toString())) {
        if (isAsyncLoad()) {
            class LoadWorker : public QRunnable {
            public:
                LoadWorker(AVPlayer *player) : m_player(player) {}
                virtual void run() {
                    if (!m_player)
                        return;
                    m_player->loadInternal();
                }
            private:
                AVPlayer* m_player;
            };
            // TODO: thread pool has a max thread limit
            QThreadPool::globalInstance()->start(new LoadWorker(this));
            return true;
        }
        loadInternal();
    } else {
        d->demuxer.prepareStreams();
        d->loaded = true;
    }
    return d->loaded;
}

void AVPlayer::loadInternal()
{
    // release codec ctx
    if (d->adec) {
        d->adec->setCodecContext(0);
    }
    if (d->vdec) {
        d->vdec->setCodecContext(0);
    }
    //close decoders here to make sure open and close in the same thread
    if (d->adec && d->adec->isOpen()) {
        d->adec->close();
    }
    if (d->vdec && d->vdec->isOpen()) {
        d->vdec->close();
    }
    if (d->current_source.type() == QVariant::String) {
        d->loaded = d->demuxer.loadFile(d->current_source.toString());
    } else {
        d->loaded = d->demuxer.load(d->current_source.value<QIODevice*>());
    }
    if (!d->loaded)
        d->statistics.reset(); //else: can not init here because d->fmt_ctx is not ready
}

qreal AVPlayer::durationF() const
{
    return double(d->demuxer.durationUs())/double(AV_TIME_BASE); //AVFrameContext.duration time base: AV_TIME_BASE
}

qint64 AVPlayer::duration() const
{
    return d->demuxer.duration();
}

qint64 AVPlayer::mediaStartPosition() const
{
    // check stopposition?
    if (d->demuxer.startTime() >= mediaStopPosition())
        return 0;
    return d->demuxer.startTime();
}

qint64 AVPlayer::mediaStopPosition() const
{
    return d->media_end;
}

qreal AVPlayer::mediaStartPositionF() const
{
    return double(d->demuxer.startTimeUs())/double(AV_TIME_BASE);
}

qint64 AVPlayer::startPosition() const
{
    return d->start_position;
}

void AVPlayer::setStartPosition(qint64 pos)
{
    if (pos > stopPosition() && stopPosition() > 0) {
        qWarning("start position too large (%lld > %lld). ignore", pos, stopPosition());
        return;
    }
    d->start_position = pos;
    if (d->start_position < 0)
        d->start_position += mediaStopPosition();
    if (d->start_position < 0)
        d->start_position = 0;
    emit startPositionChanged(d->start_position);
}

qint64 AVPlayer::stopPosition() const
{
    return d->stop_position;
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
    d->stop_position = pos;
    if (d->stop_position < 0)
        d->stop_position += mediaStopPosition();
    if (d->stop_position < 0)
        d->stop_position = mediaStopPosition();
    if (d->stop_position < startPosition()) {
        qWarning("stop postion %lld < start position %lld. ignore", d->stop_position, startPosition());
        return;
    }
    emit stopPositionChanged(d->stop_position);
}

qreal AVPlayer::positionF() const
{
    return d->clock->value();
}

qint64 AVPlayer::position() const
{
    return d->clock->value()*1000.0; //TODO: avoid *1000.0
}

void AVPlayer::setPosition(qint64 position)
{
    if (!isPlaying())
        return;
    if (position < 0)
        position += mediaStopPosition();
    d->seeking = true;
    d->seek_target = position;
    qreal s = (qreal)position/1000.0;
    // TODO: check flag accurate seek
    if (d->athread) {
        d->athread->skipRenderUntil(s);
    }
    if (d->vthread) {
        d->vthread->skipRenderUntil(s);
    }
    masterClock()->updateValue(double(position)/1000.0); //what is duration == 0
    masterClock()->updateExternalClock(position); //in msec. ignore usec part using t/1000
    d->read_thread->seek(position);

    emit positionChanged(position);
}

int AVPlayer::repeat() const
{
    return d->repeat_max;
}

int AVPlayer::currentRepeat() const
{
    return d->repeat_current;
}

// TODO: reset current_repeat?
void AVPlayer::setRepeat(int max)
{
    d->repeat_max = max;
    if (d->repeat_max < 0)
        d->repeat_max = std::numeric_limits<int>::max();
    emit repeatChanged(d->repeat_max);
}


bool AVPlayer::setAudioStream(int n, bool now)
{
    if (!d->demuxer.setStreamIndex(AVDemuxer::AudioStream, n)) {
        qWarning("set video stream to %d failed", n);
        return false;
    }
    d->loaded = false;
    d->last_position = -1;
    d->demuxer.setAutoResetStream(false);
    if (!now)
        return true;
    play();
    return true;
}

bool AVPlayer::setVideoStream(int n, bool now)
{
    if (!d->demuxer.setStreamIndex(AVDemuxer::VideoStream, n)) {
        qWarning("set video stream to %d failed", n);
        return false;
    }
    d->loaded = false;
    d->last_position = -1;
    d->demuxer.setAutoResetStream(false);
    if (!now)
        return true;
    play();
    return true;
}

bool AVPlayer::setSubtitleStream(int n, bool now)
{
    Q_UNUSED(n);
    Q_UNUSED(now);
    d->demuxer.setAutoResetStream(false);
    return false;
}

int AVPlayer::currentAudioStream() const
{
    return d->demuxer.audioStreams().indexOf(d->demuxer.audioStream());
}

int AVPlayer::currentVideoStream() const
{
    return d->demuxer.videoStreams().indexOf(d->demuxer.videoStream());
}

int AVPlayer::currentSubtitleStream() const
{
    return d->demuxer.subtitleStreams().indexOf(d->demuxer.subtitleStream());
}

int AVPlayer::audioStreamCount() const
{
    return d->demuxer.audioStreams().size();
}

int AVPlayer::videoStreamCount() const
{
    return d->demuxer.videoStreams().size();
}

int AVPlayer::subtitleStreamCount() const
{
    return d->demuxer.subtitleStreams().size();
}

//FIXME: why no d->demuxer will not get an eof if replaying by seek(0)?
void AVPlayer::play()
{
    //FIXME: bad delay after play from here
    bool start_last = d->last_position == -1;
    if (isPlaying()) {
        if (start_last) {
            d->clock->pause(true); //external clock
            d->last_position = mediaStopPosition() != kInvalidPosition ? -position() : 0;
            d->reset_state = false; //set a new stream number should not reset other states
            qDebug("start pos is current position: %lld", d->last_position);
        } else {
            d->last_position = 0;
            qDebug("start pos is stream start time");
        }
        qint64 last_pos = d->last_position;
        stop();
        // wait here to ensure stopped() and startted() is in correct order
        if (d->read_thread->isRunning()) {
            d->read_thread->wait(500);
        }
        if (last_pos < 0)
            d->last_position = -last_pos;
    }
    if (isAsyncLoad()) {
        connect(this, SIGNAL(loaded()), this, SLOT(playInternal()));
        load(true);
        return;
    }
    loadInternal();
    playInternal();
    return;
    /*
     * avoid load mutiple times when replaying the same seekable file
     * TODO: force load unseekable stream? avio.seekable. currently you
     * must setFile() agian to reload an unseekable stream
     */
    //TODO: no eof if replay by seek(0)
#if EOF_ISSUE_SOLVED
    bool force_reload = position() > 0; //eof issue now
    if (!isLoaded() || force_reload) { //if (!isLoaded() && !load())
        if (!load(force_reload)) {
            d->statistics.reset();
            return;
        } else {
            d->initStatistics(this);
        }
    } else {
        qDebug("seek(%f)", d->last_position);
        d->demuxer.seek(d->last_position); //FIXME: now assume it is seekable. for unseekable, setFile() again
#else
        if (!load(true)) {
            d->statistics.reset();
            return;
        }
        d->initStatistics(this);

#endif //EOF_ISSUE_SOLVED
#if EOF_ISSUE_SOLVED
    }
#endif //EOF_ISSUE_SOLVED    
}

void AVPlayer::playInternal()
{
    disconnect(this, SIGNAL(loaded()), this, SLOT(playInternal()));

    d->fmt_ctx = d->demuxer.formatContext();

    // TODO: what about other proctols? some vob duration() == 0
    if (duration() > 0)
        d->media_end = mediaStartPosition() + duration();
    else
        d->media_end = kInvalidPosition;
    // if file changed or stop() called by user, d->stop_position changes.
    if (stopPosition() > mediaStopPosition() || stopPosition() == 0) {
        d->stop_position = mediaStopPosition();
    }

    d->initStatistics(this);

    if (!d->setupAudioThread(this)) {
        d->read_thread->setAudioThread(0); //set 0 before delete. ptr is used in demux thread when set 0
        if (d->athread) {
            qDebug("release audio thread.");
            delete d->athread;
            d->athread = 0;//shared ptr?
        }
    }
    if (!d->setupVideoThread(this)) {
        d->read_thread->setVideoThread(0); //set 0 before delete. ptr is used in demux thread when set 0
        if (d->vthread) {
            qDebug("release video thread.");
            delete d->vthread;
            d->vthread = 0;//shared ptr?
        }
    }
    if (!d->athread && !d->vthread) {
        d->loaded = false;
        qWarning("load failed");
        return;
        //return false;
    }


    // from previous play()
    if (d->demuxer.audioCodecContext() && d->athread) {
        qDebug("Starting audio thread...");
        d->athread->start();
        d->athread->waitForReady();
    }
    if (d->demuxer.videoCodecContext() && d->vthread) {
        qDebug("Starting video thread...");
        d->vthread->start();
        d->vthread->waitForReady();
    }
    if (startPosition() > 0 && startPosition() < mediaStopPosition() && d->last_position <= 0) {
        d->demuxer.seek((qint64)startPosition());
    }
    d->read_thread->start();
    if (d->last_position > 0) {//start_last) {
        masterClock()->pause(false); //external clock
    } else {
        masterClock()->reset();
    }
    if (masterClock()->isClockAuto()) {
        qDebug("auto select clock: audio > external");
        if (!d->demuxer.audioCodecContext() || !d->ao) {
            qWarning("No audio found or audio not supported. Using ExternalClock");
            masterClock()->setClockType(AVClock::ExternalClock);
            masterClock()->setInitialValue(mediaStartPositionF());
        } else {
            qDebug("Using AudioClock");
            masterClock()->setClockType(AVClock::AudioClock);
            //masterClock()->setInitialValue(0);
        }
    }

    if (d->timer_id < 0) {
        //d->timer_id = startTimer(kPosistionCheckMS); //may fail if not in this thread
        QMetaObject::invokeMethod(this, "startNotifyTimer", Qt::AutoConnection);
    }
// ffplay does not seek to stream's start position. usually it's 0, maybe < 1. seeking will result in a non-key frame position and it's bad.
    //if (d->last_position <= 0)
    //    d->last_position = mediaStartPosition();
    if (d->last_position > 0)
        setPosition(d->last_position); //just use d->demuxer.startTime()/duration()?

    emit started(); //we called stop(), so must emit started()
}

void AVPlayer::stopFromDemuxerThread()
{
    d->reset_state = false;
    qDebug("demuxer thread emit finished. avplayer emit stopped()");
    if (d->read_thread->isRunning()) {
        qWarning("XXXXXXXXX demuxer thread is running........");
        return;
    }
    emit stopped();
}

void AVPlayer::aboutToQuitApp()
{
    d->reset_state = true;
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
    d->timer_id = startTimer(kPosistionCheckMS);
}

void AVPlayer::stopNotifyTimer()
{
    killTimer(d->timer_id);
    d->timer_id = -1;
}

// TODO: doc about when the state will be reset
void AVPlayer::stop()
{
    // check d->timer_id, <0 return?
    if (d->reset_state) {
        /*
         * must kill in main thread! If called from user, may be not in main thread.
         * then timer goes on, and player find that d->stop_position reached(d->stop_position is already
         * 0 after user call stop),
         * stop() is called again by player and reset state. but this call is later than demuxer stop.
         * so if user call play() immediatly, may be stopped by AVPlayer
         */
        // TODO: invokeMethod "stopNotifyTimer"
        if (d->timer_id >= 0) {
            qDebug("timer: %d, current thread: %p, player thread: %p", d->timer_id, QThread::currentThread(), thread());
            if (QThread::currentThread() == thread()) { //called by user in the same thread as player
                killTimer(d->timer_id);
                d->timer_id = -1;
            } else {
                //TODO: post event.
            }
        }
        d->start_position = 0;
        d->stop_position = 0; // 0 can stop play in timerEvent
        d->media_end = kInvalidPosition;
        d->repeat_current = d->repeat_max = 0;
    } else { //called by player
        if (d->timer_id >= 0) {
            killTimer(d->timer_id);
            d->timer_id = -1;
        }
    }
    d->reset_state = true;

    d->last_position = mediaStopPosition() != kInvalidPosition ? startPosition() : 0;
    if (!isPlaying()) {
        qDebug("Not playing~");
        return;
    }

    while (d->read_thread->isRunning()) {
        qDebug("stopping demuxer thread...");
        d->read_thread->stop();
        d->read_thread->wait(500);
    }
    qDebug("all audio/video threads  stopped...");
}

void AVPlayer::timerEvent(QTimerEvent *te)
{
    if (te->timerId() == d->timer_id) {
        // killTimer() should be in the same thread as object. kill here?
        if (isPaused()) {
            return;
        }
        // active only when playing
        const qint64 t = d->clock->value()*1000.0;
        if (stopPosition() == kInvalidPosition) { // or check stopPosition() < 0
            // not seekable. network stream
            emit positionChanged(t);
            return;
        }
        if (d->seeking && t >= d->seek_target + 1000) {
            d->seeking = false;
            d->seek_target = 0;
        }
        if (t <= stopPosition()) {
            if (!d->seeking) {
                emit positionChanged(t);
            }
            return;
        }
        // TODO: remove. kill timer in an event;
        if (stopPosition() == 0) { //stop() by user in other thread, state is already reset
            d->reset_state = false;
            qDebug("stopPosition() == 0, stop");
            stop();
        }
        // t < d->start_position is ok. d->repeat_max<0 means repeat forever
        if (currentRepeat() >= repeat() && repeat() >= 0) {
            d->reset_state = true; // true is default, can remove here
            qDebug("stopPosition() reached and no repeat: %d", repeat());
            stop();
            return;
        }
        d->repeat_current++;
        // FIXME: now stop instead of seek if reach media's end. otherwise will not get eof again
        if (stopPosition() == mediaStopPosition() || !d->demuxer.isSeekable()) {
            // if not seekable, how it can start to play at specified position?
            qDebug("stopPosition() == mediaStopPosition() or !seekable. d->repeat_current=%d", d->repeat_current);
            d->reset_state = false;
            stop();
            d->last_position = startPosition(); // for seeking to startPosition(). already set in stop()
            play();
        } else {
            qDebug("stopPosition() != mediaStopPosition() and seekable. d->repeat_current=%d", d->repeat_current);
            setPosition(startPosition());
        }
    }
}

//FIXME: If not playing, it will just play but not play one frame.
void AVPlayer::playNextFrame()
{
    // pause clock
    pause(true); // must pause AVDemuxThread (set user_paused true)
    d->read_thread->nextFrame();
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
    d->clock->updateExternalClock(msecs);
}

int AVPlayer::brightness() const
{
    return d->brightness;
}

void AVPlayer::setBrightness(int val)
{
    if (d->brightness == val)
        return;
    d->brightness = val;
    emit brightnessChanged(d->brightness);
    if (d->vthread) {
        d->vthread->setBrightness(val);
    }
}

int AVPlayer::contrast() const
{
    return d->contrast;
}

void AVPlayer::setContrast(int val)
{
    if (d->contrast == val)
        return;
    d->contrast = val;
    emit contrastChanged(d->contrast);
    if (d->vthread) {
        d->vthread->setContrast(val);
    }
}

int AVPlayer::hue() const
{
    return 0;
}

void AVPlayer::setHue(int val)
{
    Q_UNUSED(val);
}

int AVPlayer::saturation() const
{
    return d->saturation;
}

void AVPlayer::setSaturation(int val)
{
    if (d->saturation == val)
        return;
    d->saturation = val;
    emit saturationChanged(d->saturation);
    if (d->vthread) {
        d->vthread->setSaturation(val);
    }
}

} //namespace QtAV
