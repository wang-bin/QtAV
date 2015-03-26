/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2015 Wang Bin <wbsecg1@gmail.com>

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
#include <QtCore/QTimer>
#include "QtAV/AVDemuxer.h"
#include "QtAV/Packet.h"
#include "QtAV/AudioDecoder.h"
#include "QtAV/AVInput.h"
#include "QtAV/VideoRenderer.h"
#include "QtAV/AVClock.h"
#include "QtAV/VideoCapture.h"
#include "QtAV/VideoDecoderTypes.h"
#include "QtAV/VideoCapture.h"
#include "filter/FilterManager.h"
#include "output/OutputSet.h"
#include "AudioThread.h"
#include "VideoThread.h"
#include "AVDemuxThread.h"
#include "QtAV/private/AVCompat.h"
#include "utils/Logger.h"

Q_DECLARE_METATYPE(QtAV::AVInput*)
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
Q_DECLARE_METATYPE(QIODevice*)
#endif
#define EOF_ISSUE_SOLVED 0
namespace QtAV {

static const qint64 kSeekMS = 10000;

Q_GLOBAL_STATIC(QThreadPool, loaderThreadPool)

/// Supported input protocols. A static string list
const QStringList& AVPlayer::supportedProtocols()
{
    return AVDemuxer::supportedProtocols();
}

AVPlayer::AVPlayer(QObject *parent) :
    QObject(parent)
  , d(new Private())
{
    d->vos = new OutputSet(this);
    d->aos = new OutputSet(this);
    connect(this, SIGNAL(started()), this, SLOT(onStarted()));
    /*
     * call stop() before the window(d->vo) closed to stop the waitcondition
     * If close the d->vo widget, the the d->vo may destroy before waking up.
     */
    connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(aboutToQuitApp()));
    //d->clock->setClockType(AVClock::ExternalClock);
    connect(&d->demuxer, SIGNAL(started()), masterClock(), SLOT(start()));
    connect(&d->demuxer, SIGNAL(error(QtAV::AVError)), this, SIGNAL(error(QtAV::AVError)));
    connect(&d->demuxer, SIGNAL(mediaStatusChanged(QtAV::MediaStatus)), this, SIGNAL(mediaStatusChanged(QtAV::MediaStatus)));
    connect(&d->demuxer, SIGNAL(loaded()), this, SIGNAL(loaded()));
    connect(&d->demuxer, SIGNAL(seekableChanged()), this, SIGNAL(seekableChanged()));
    d->read_thread = new AVDemuxThread(this);
    d->read_thread->setDemuxer(&d->demuxer);
    //direct connection can not sure slot order?
    connect(d->read_thread, SIGNAL(finished()), this, SLOT(stopFromDemuxerThread()));
    connect(d->read_thread, SIGNAL(requestClockPause(bool)), masterClock(), SLOT(pause(bool)), Qt::DirectConnection);
    connect(d->read_thread, SIGNAL(mediaStatusChanged(QtAV::MediaStatus)), this, SIGNAL(mediaStatusChanged(QtAV::MediaStatus)));
    connect(d->read_thread, SIGNAL(bufferProgressChanged(qreal)), this, SIGNAL(bufferProgressChanged(qreal)));

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
    if (d->mute == mute)
        return;
    d->mute = mute;
    emit muteChanged();
    if (!d->ao)
        return;
    if (d->ao->isMute() == isMute())
        return;
    d->ao->setMute(mute);
}

bool AVPlayer::isMute() const
{
    return d->mute;
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

void AVPlayer::setInterruptTimeout(qint64 ms)
{
    if (d->interrupt_timeout == ms)
        return;
    d->interrupt_timeout = ms;
    emit interruptTimeoutChanged();
    d->demuxer.setInterruptTimeout(ms);
}

qint64 AVPlayer::interruptTimeout() const
{
    return d->interrupt_timeout;
}

void AVPlayer::setFrameRate(qreal value)
{
    d->force_fps = value;
    // clock set here will be reset in playInternal()
    // also we can't change user's setting of ClockType and autoClock here if force frame rate is disabled.
}

qreal AVPlayer::forcedFrameRate() const
{
    return d->force_fps;
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
extern QString getLocalPath(const QString& fullPath);
void AVPlayer::setFile(const QString &path)
{
    // file() is used somewhere else. ensure it is correct
    QString p(path);
    // QFile does not support "file:"
    if (p.startsWith("file:"))
        p = getLocalPath(p);
    d->reset_state = d->current_source.type() != QVariant::String || d->current_source.toString() != p;
    d->current_source = p;
    // TODO: d->reset_state = d->demuxer2.setMedia(path);
    if (d->reset_state) {
        d->audio_track = d->video_track = d->subtitle_track = 0;
        emit sourceChanged();
        //emit error(AVError(AVError::NoError));
    }
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
    // TODO: d->reset_state = d->demuxer2.setMedia(device);
    if (d->current_source.type() == QVariant::String) {
        d->reset_state = true;
    } else {
        if (d->current_source.canConvert<QIODevice*>()) {
            d->reset_state = d->current_source.value<QIODevice*>() != device;
        } else { // AVInput
            d->reset_state = true;
        }
    }
    d->loaded = false;
    d->current_source = QVariant::fromValue(device);
    if (d->reset_state) {
        d->audio_track = d->video_track = d->subtitle_track = 0;
        emit sourceChanged();
    }
}

void AVPlayer::setInput(AVInput *in)
{
    // TODO: d->reset_state = d->demuxer2.setMedia(in);
    if (d->current_source.type() == QVariant::String) {
        d->reset_state = true;
    } else {
        if (d->current_source.canConvert<QIODevice*>()) {
            d->reset_state = true;
        } else { // AVInput
            d->reset_state = d->current_source.value<QtAV::AVInput*>() != in;
        }
    }
    d->loaded = false;
    d->current_source = QVariant::fromValue<QtAV::AVInput*>(in);
    if (d->reset_state) {
        d->audio_track = d->video_track = d->subtitle_track = 0;
        emit sourceChanged();
    }
}

AVInput* AVPlayer::input() const
{
    if (d->current_source.type() == QVariant::String)
        return 0;
    if (!d->current_source.canConvert<QtAV::AVInput*>())
        return 0;
    return d->current_source.value<QtAV::AVInput*>();
}

VideoCapture* AVPlayer::videoCapture()
{
    return d->vcapture;
}

bool AVPlayer::captureVideo()
{
    if (!d->vcapture || !d->vthread)
        return false;
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
    // TODO: call unload if reload?
    if (mediaStatus() == QtAV::LoadingMedia) //async loading
        return true;
    if (isLoaded()) {
        // release codec ctx. if not loaded, they are released by avformat. TODO: always let avformat release them?
        if (d->adec)
            d->adec->setCodecContext(0);
        if (d->vdec)
            d->vdec->setCodecContext(0);
    }
    d->loaded = false;
    if (!d->current_source.isValid()) {
        qDebug("Invalid media source. No file or IODevice was set.");
        return false;
    }
    if (!reload) {
        if (d->current_source.type() == QVariant::String) {
            reload = d->demuxer.fileName() != d->current_source.toString();
        } else {
            if (d->current_source.canConvert<QIODevice*>()) {
                reload = d->demuxer.ioDevice() != d->current_source.value<QIODevice*>();
            } else { // AVInput
                reload = d->demuxer.input() != d->current_source.value<QtAV::AVInput*>();
            }
        }
        reload = reload || !d->demuxer.isLoaded();
    }
    if (reload) {
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
            loaderThreadPool()->start(new LoadWorker(this));
            return true;
        }
        loadInternal();
    } else {
        d->loaded = true;
    }
    return d->loaded;
}

void AVPlayer::loadInternal()
{
    // release codec ctx
    //close decoders here to make sure open and close in the same thread if not async load
    if (isLoaded()) {
        if (d->adec)
            d->adec->setCodecContext(0);
        if (d->vdec)
            d->vdec->setCodecContext(0);
    }
    qDebug() << "Loading " << d->current_source << " ...";
    if (d->current_source.type() == QVariant::String) {
        d->demuxer.setMedia(d->current_source.toString());
    } else {
        if (d->current_source.canConvert<QIODevice*>()) {
            d->demuxer.setMedia(d->current_source.value<QIODevice*>());
        } else { // AVInput
            d->demuxer.setMedia(d->current_source.value<QtAV::AVInput*>());
        }
    }
    d->loaded = d->demuxer.load();
    if (!d->loaded) {
        d->statistics.reset();
        qWarning("Load failed!");
        return;
    }
    // setup parameters from loaded media
    d->fmt_ctx = d->demuxer.formatContext();
    d->media_start_pts = d->demuxer.startTime();
    // TODO: what about other proctols? some vob duration() == 0
    if (duration() > 0)
        d->media_end = mediaStartPosition() + duration();
    else
        d->media_end = kInvalidPosition;
    // if file changed or stop() called by user, d->stop_position changes.
    if (stopPosition() > mediaStopPosition() || stopPosition() <= 0 || stopPosition() == kInvalidPosition) {
        d->stop_position = mediaStopPosition();
    }

    int interval = qAbs(d->notify_interval);
    d->initStatistics();
    if (interval != qAbs(d->notify_interval))
        emit notifyIntervalChanged();
}

void AVPlayer::unload()
{
    // what about threads just start and going to use the decoder(thread not start when stop())
    if (isPlaying()) {
        qWarning("call unload() after stopped() is emitted!");
        return;
    }
    if (mediaStatus() != LoadingMedia) {
        unloadInternal();
        //QTimer::singleShot(0, this, SLOT(unloadInternal()));
        return;
    }
    // maybe it is loaded soon
    connect(&d->demuxer, SIGNAL(loaded()), this, SLOT(unloadInternal()));
    // user interrupt if still loading
    connect(&d->demuxer, SIGNAL(userInterrupted()), this, SLOT(unloadInternal()));
    d->demuxer.setInterruptStatus(1);
}

void AVPlayer::unloadInternal()
{
    disconnect(&d->demuxer, SIGNAL(loaded()), this, SLOT(unloadInternal()));
    disconnect(&d->demuxer, SIGNAL(userInterrupted()), this, SLOT(unloadInternal()));

    d->loaded = false;
    /*
     * FIXME: no a/v/d thread started and in LoadedMedia status when stop(). but now is running (and to using the decoder).
     * then we must wait the threads finished. or use smart ptr for decoders? demuxer is still unsafe
     * change to LoadedMedia until all threads started?
     * will it happen if playInternal() and unloadInternal() in ui thread?(use singleShort())
     */
    if (isPlaying())
        stop();

    if (d->adec) {
        d->adec->setCodecContext(0);
        delete d->adec;
        d->adec = 0;
    }
    if (d->vdec) {
        d->vdec->setCodecContext(0);
        delete d->vdec;
        d->vdec = 0;
    }
    d->demuxer.unload();
}

void AVPlayer::setRelativeTimeMode(bool value)
{
    if (d->relative_time_mode == value)
        return;
    d->relative_time_mode = value;
    emit relativeTimeModeChanged();
}

bool AVPlayer::relativeTimeMode() const
{
    return d->relative_time_mode;
}

qint64 AVPlayer::absoluteMediaStartPosition() const
{
    return d->media_start_pts;
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
    if (relativeTimeMode())
        return 0;
    return d->demuxer.startTime();
}

qint64 AVPlayer::mediaStopPosition() const
{
    if (d->media_end == kInvalidPosition && duration() > 0) {
        // called in stop()
        return mediaStartPosition() + duration();
    }
    return d->media_end;
}

qreal AVPlayer::mediaStartPositionF() const
{
    if (relativeTimeMode())
        return 0;
    return double(d->demuxer.startTimeUs())/double(AV_TIME_BASE);
}

qint64 AVPlayer::startPosition() const
{
    return d->start_position;
}

void AVPlayer::setStartPosition(qint64 pos)
{
    // default stopPosition() is int64 max, so set start position before media loaded is ok
    if (pos > stopPosition() && stopPosition() > 0) {
        qWarning("start position too large (%lld > %lld). ignore", pos, stopPosition());
        return;
    }
    d->start_position = pos;
    if (d->start_position < 0)
        d->start_position += mediaStopPosition();
    if (d->start_position < 0)
        d->start_position = mediaStartPosition();
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

bool AVPlayer::isSeekable() const
{
    return d->demuxer.isSeekable();
}

qreal AVPlayer::positionF() const
{
    return d->clock->value();
}

qint64 AVPlayer::position() const
{
    const qint64 pts = d->clock->value()*1000.0; //TODO: avoid *1000.0
    if (relativeTimeMode())
        return pts - absoluteMediaStartPosition();
    return pts;
}

void AVPlayer::setPosition(qint64 position)
{
    // FIXME: strange things happen if seek out of eof
    if (position > stopPosition())
        return;
    if (!isPlaying())
        return;
    qint64 pos_pts = position;
    if (pos_pts < 0)
        pos_pts = 0;
    // position passed in is relative to the start pts in relative time mode
    if (relativeTimeMode())
        pos_pts += absoluteMediaStartPosition();
    d->seeking = true;
    d->seek_target = position;
    qreal s = (qreal)pos_pts/1000.0;
    if (seekType() == AccurateSeek) {
        if (d->athread) {
            d->athread->skipRenderUntil(s);
        }
        if (d->vthread) {
            d->vthread->skipRenderUntil(s);
        }
    }
    masterClock()->updateValue(double(pos_pts)/1000.0); //what is duration == 0
    masterClock()->updateExternalClock(pos_pts); //in msec. ignore usec part using t/1000
    d->read_thread->seek(pos_pts, seekType());

    emit positionChanged(position); //emit relative position
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


bool AVPlayer::setAudioStream(int n)
{
    if (n < 0)
        return false;
    if (d->audio_track == n)
        return true;
    if (isLoaded()) {
        if (n >= d->demuxer.audioStreams().size())
            return false;
    }
    d->audio_track = n;
    if (!isPlaying())
        return true;
    // pause demuxer, clear queues, set demuxer stream, set decoder, set ao, resume
    bool p = isPaused();
    pause(true);
    if (!d->setupAudioThread(this)) {
        stop();
        return false;
    }
    if (!p)
        pause(false);
    return true;
}

bool AVPlayer::setVideoStream(int n)
{
    if (n < 0)
        return false;
    if (d->video_track == n)
        return true;
    if (isLoaded()) {
        if (n >= d->demuxer.videoStreams().size())
            return false;
    }
    d->video_track = n;
    if (!isPlaying())
        return true;
    // pause demuxer, clear queues, set demuxer stream, set decoder, set ao, resume
    bool p = isPaused();
    pause(true);
    if (!d->setupVideoThread(this)) {
        stop();
        return false;
    }
    if (!p)
        pause(false);
    return true;
}

bool AVPlayer::setSubtitleStream(int n)
{
    Q_UNUSED(n);
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
    if (mediaStatus() == LoadingMedia) {
        // can not use userInterrupted(), userInterrupted() is connected to unloadInternal()
        connect(&d->demuxer, SIGNAL(unloaded()), this, SLOT(loadAndPlay()));
        unload();
    } else {
        unload();
        loadAndPlay();
    }
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
            d->initStatistics();
        }
    } else {
        qDebug("seek(%f)", d->last_position);
        d->demuxer.seek(d->last_position); //FIXME: now assume it is seekable. for unseekable, setFile() again
#else
        if (!load(true))
            return;
#endif //EOF_ISSUE_SOLVED
#if EOF_ISSUE_SOLVED
    }
#endif //EOF_ISSUE_SOLVED    
}

void AVPlayer::loadAndPlay()
{
    disconnect(&d->demuxer, SIGNAL(unloaded()), this, SLOT(loadAndPlay()));
    if (isAsyncLoad()) {
        connect(this, SIGNAL(loaded()), this, SLOT(playInternal()));
        load(true);
        return;
    }
    loadInternal();
    playInternal();
    //QTimer::singleShot(0, this, SLOT(playInternal()));
}

void AVPlayer::playInternal()
{
    disconnect(this, SIGNAL(loaded()), this, SLOT(playInternal()));
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
    // setup clock before avthread.start() becuase avthreads use clock. after avthreads setup because of ao check
    if (d->last_position > 0) {//start_last) {
        masterClock()->pause(false); //external clock
    } else {
        masterClock()->reset();
    }
    // TODO: add isVideo() or hasVideo()?
    if (d->force_fps > 0 && d->demuxer.videoCodecContext() && d->vthread) {
        masterClock()->setClockAuto(false);
        masterClock()->setClockType(AVClock::VideoClock);
        d->vthread->setFrameRate(d->force_fps);
    } else {
        d->vthread->setFrameRate(-1.0);
    }
    if (masterClock()->isClockAuto()) {
        qDebug("auto select clock: audio > external");
        if (!d->demuxer.audioCodecContext() || !d->ao) {
            masterClock()->setClockType(AVClock::ExternalClock);
            qDebug("No audio found or audio not supported. Using ExternalClock.");
        } else {
            qDebug("Using AudioClock");
            masterClock()->setClockType(AVClock::AudioClock);
        }
        masterClock()->setInitialValue((double)absoluteMediaStartPosition()/1000.0);
        qDebug("Clock initial value: %f", masterClock()->value());
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
        if (relativeTimeMode())
            d->demuxer.seek(qint64((startPosition() + absoluteMediaStartPosition())));
        else
            d->demuxer.seek((qint64)(startPosition()));
    }
    d->read_thread->start();

    if (d->timer_id < 0) {
        //d->timer_id = startNotifyTimer(); //may fail if not in this thread
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
    qDebug("demuxer thread emit finished.");
    if (currentRepeat() >= repeat() && repeat() >= 0) {
        masterClock()->reset();
        stopNotifyTimer();
        d->seeking = false;
        d->start_position = 0;
        d->stop_position = kInvalidPosition; // already stopped. so not 0 but invalid. 0 can stop the playback in timerEvent
        d->media_end = kInvalidPosition;
        d->repeat_current = d->repeat_max = 0;
        qDebug("avplayer emit stopped()");
        emit stopped();
    } else {
        qDebug("stopPosition() == mediaStopPosition() or !seekable. repeate: %d/%d", currentRepeat(), repeat());
        d->repeat_current++;
        d->last_position = startPosition(); // for seeking to startPosition() if seekable. already set in stop()
        play();
    }
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
    d->demuxer.setInterruptStatus(true);
    loaderThreadPool()->waitForDone();
}

void AVPlayer::setNotifyInterval(int msec)
{
    if (d->notify_interval == msec)
        return;
    if (d->notify_interval < 0 && msec <= 0)
        return;
    const int old = qAbs(d->notify_interval);
    d->notify_interval = msec;
    d->updateNotifyInterval();
    emit notifyIntervalChanged();
    if (d->timer_id < 0)
        return;
    if (old != qAbs(d->notify_interval)) {
        stopNotifyTimer();
        startNotifyTimer();
    }
}

int AVPlayer::notifyInterval() const
{
    return qAbs(d->notify_interval);
}

void AVPlayer::startNotifyTimer()
{
    d->timer_id = startTimer(qAbs(d->notify_interval));
}

void AVPlayer::stopNotifyTimer()
{
    if (d->timer_id < 0)
        return;
    killTimer(d->timer_id);
    d->timer_id = -1;
}

void AVPlayer::onStarted()
{
    //TODO: check clock type?
    if (d->ao && d->ao->isAvailable()) {
        d->ao->setSpeed(d->speed);
    }
    masterClock()->setSpeed(d->speed);

    if (!d->ao)
        return;
    if (d->ao->isMute() != isMute())
        d->ao->setMute(isMute());
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
                stopNotifyTimer();
            } else {
                //TODO: post event.
            }
        }
        d->start_position = 0;
        d->stop_position = 0; // 0 can stop play in timerEvent
        d->media_end = kInvalidPosition;
        d->repeat_current = d->repeat_max = 0;
    } else { //called by player
        stopNotifyTimer();
    }
    d->seeking = false;
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
        const qint64 t = position();
        if (stopPosition() == kInvalidPosition) { // or check stopPosition() < 0
            // not seekable. network stream
            emit positionChanged(t);
            return;
        }
        // FIXME: totally wrong if seek_target - keyframe_seek > 1000
        if (d->seeking && t >= d->seek_target + 1000) {
            d->seeking = false;
            d->seek_target = 0;
        }
        if (t < startPosition()) {
            //qDebug("position %lld < startPosition %lld", t, startPosition());
            // or set clock initial value to get correct t
            if (startPosition() != mediaStartPosition()) {
                setPosition(startPosition());
                return;
            }
        }
        if (t <= stopPosition()) {
            if (!d->seeking) { // FIXME
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
            qDebug("stopPosition() %lld/%lld reached and no repeat: %d", t, stopPosition(), repeat());
            stop();
            return;
        }
        // FIXME: now stop instead of seek if reach media's end. otherwise will not get eof again
        if (stopPosition() == mediaStopPosition() || !isSeekable()) {
            // if not seekable, how it can start to play at specified position?
            qDebug("stopPosition() == mediaStopPosition() or !seekable. d->repeat_current=%d", d->repeat_current);
            d->reset_state = false;
            stop(); // repeat after all threads stopped
        } else {
            d->repeat_current++;
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

void AVPlayer::setSeekType(SeekType type)
{
    d->seek_type = type;
}

SeekType AVPlayer::seekType() const
{
    return d->seek_type;
}

qreal AVPlayer::bufferProgress() const
{
    const PacketBuffer* buf = d->read_thread->buffer();
    return buf ? buf->bufferProgress() : 0;
}

int AVPlayer::buffered() const
{
    const PacketBuffer* buf = d->read_thread->buffer();
    return buf ? buf->buffered() : 0;
}

void AVPlayer::setBufferMode(BufferMode mode)
{
    d->buffer_mode = mode;
}

BufferMode AVPlayer::bufferMode() const
{
    return d->buffer_mode;
}

void AVPlayer::setBufferValue(int value)
{
    d->buffer_value = value;
}

int AVPlayer::bufferValue() const
{
    return d->buffer_value;
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
