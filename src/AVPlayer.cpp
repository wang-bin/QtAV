/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2017 Wang Bin <wbsecg1@gmail.com>

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
#include "QtAV/MediaIO.h"
#include "QtAV/VideoRenderer.h"
#include "QtAV/AVClock.h"
#include "QtAV/VideoCapture.h"
#include "QtAV/VideoCapture.h"
#include "filter/FilterManager.h"
#include "output/OutputSet.h"
#include "AudioThread.h"
#include "VideoThread.h"
#include "AVDemuxThread.h"
#include "QtAV/private/AVCompat.h"
#include "utils/internal.h"
#include "utils/Logger.h"
extern "C" {
#include <libavutil/mathematics.h>
}

#define EOF_ISSUE_SOLVED 0
namespace QtAV {
namespace {
static const struct RegisterMetaTypes {
    inline RegisterMetaTypes() {
        qRegisterMetaType<QtAV::AVPlayer::State>(); // required by invoke() parameters
    }
} _registerMetaTypes;
} //namespace
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
    connect(&d->demuxer, SIGNAL(mediaStatusChanged(QtAV::MediaStatus)), this, SLOT(updateMediaStatus(QtAV::MediaStatus)), Qt::DirectConnection);
    connect(&d->demuxer, SIGNAL(loaded()), this, SIGNAL(loaded()));
    connect(&d->demuxer, SIGNAL(seekableChanged()), this, SIGNAL(seekableChanged()));
    d->read_thread = new AVDemuxThread(this);
    d->read_thread->setDemuxer(&d->demuxer);
    //direct connection can not sure slot order?
    connect(d->read_thread, SIGNAL(finished()), this, SLOT(stopFromDemuxerThread()), Qt::DirectConnection);
    connect(d->read_thread, SIGNAL(requestClockPause(bool)), masterClock(), SLOT(pause(bool)), Qt::DirectConnection);
    connect(d->read_thread, SIGNAL(mediaStatusChanged(QtAV::MediaStatus)), this, SLOT(updateMediaStatus(QtAV::MediaStatus)));
    connect(d->read_thread, SIGNAL(bufferProgressChanged(qreal)), this, SIGNAL(bufferProgressChanged(qreal)));
    connect(d->read_thread, SIGNAL(seekFinished(qint64)), this, SLOT(onSeekFinished(qint64)), Qt::DirectConnection);
    connect(d->read_thread, SIGNAL(stepFinished()), this, SLOT(onStepFinished()), Qt::DirectConnection);
    connect(d->read_thread, SIGNAL(internalSubtitlePacketRead(int, QtAV::Packet)), this, SIGNAL(internalSubtitlePacketRead(int, QtAV::Packet)), Qt::DirectConnection);
    d->vcapture = new VideoCapture(this);
}

AVPlayer::~AVPlayer()
{
    stop();
    QMutexLocker lock(&d->load_mutex);
    Q_UNUSED(lock);
    // if not uninstall here, player's qobject children filters will call uninstallFilter too late that player is almost be destroyed
    QList<Filter*> filters(FilterManager::instance().videoFilters(this));
    foreach (Filter* f, filters) {
        uninstallFilter(reinterpret_cast<VideoFilter*>(f));
    }
    filters = FilterManager::instance().audioFilters(this);
    foreach (Filter* f, filters) {
        uninstallFilter(reinterpret_cast<AudioFilter*>(f));
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

AudioOutput* AVPlayer::audio()
{
    return d->ao;
}

void AVPlayer::setSpeed(qreal speed)
{
    if (speed == d->speed)
        return;
    setFrameRate(0); // will set clock to default
    d->speed = speed;
    //TODO: check clock type?
    if (d->ao && d->ao->isAvailable()) {
        qDebug("set speed %.2f", d->speed);
        d->ao->setSpeed(d->speed);
    }
    masterClock()->setSpeed(d->speed);
    Q_EMIT speedChanged(d->speed);
}

qreal AVPlayer::speed() const
{
    return d->speed;
}

void AVPlayer::setInterruptTimeout(qint64 ms)
{
    if (ms < 0LL)
        ms = -1LL;
    if (d->interrupt_timeout == ms)
        return;
    d->interrupt_timeout = ms;
    Q_EMIT interruptTimeoutChanged();
    d->demuxer.setInterruptTimeout(ms);
}

qint64 AVPlayer::interruptTimeout() const
{
    return d->interrupt_timeout;
}

void AVPlayer::setInterruptOnTimeout(bool value)
{
    if (isInterruptOnTimeout() == value)
        return;
    d->demuxer.setInterruptOnTimeout(value);
    Q_EMIT interruptOnTimeoutChanged();
}

bool AVPlayer::isInterruptOnTimeout() const
{
    return d->demuxer.isInterruptOnTimeout();
}

void AVPlayer::setFrameRate(qreal value)
{
    d->force_fps = value;
    // clock set here will be reset in playInternal()
    // also we can't change user's setting of ClockType and autoClock here if force frame rate is disabled.
    if (!isPlaying())
        return;
    d->applyFrameRate();
}

qreal AVPlayer::forcedFrameRate() const
{
    return d->force_fps;
}

const Statistics& AVPlayer::statistics() const
{
    return d->statistics;
}

bool AVPlayer::installFilter(AudioFilter *filter, int index)
{
    if (!FilterManager::instance().registerAudioFilter((Filter*)filter, this, index))
        return false;
    if (!d->athread)
        return false; //install later when avthread created
    return d->athread->installFilter((Filter*)filter, index);
}

bool AVPlayer::installFilter(VideoFilter *filter, int index)
{
    if (!FilterManager::instance().registerVideoFilter((Filter*)filter, this, index))
        return false;
    if (!d->vthread)
        return false; //install later when avthread created
    return d->vthread->installFilter((Filter*)filter, index);
}

bool AVPlayer::uninstallFilter(AudioFilter *filter)
{
    FilterManager::instance().unregisterAudioFilter(filter, this);
    AVThread *avthread = d->athread;
    if (!avthread)
        return false;
    if (!avthread->filters().contains(filter))
        return false;
    return avthread->uninstallFilter(filter, true);
}

bool AVPlayer::uninstallFilter(VideoFilter *filter)
{
    FilterManager::instance().unregisterVideoFilter(filter, this);
    AVThread *avthread = d->vthread;
    if (!avthread)
        return false;
    if (!avthread->filters().contains(filter))
        return false;
    return avthread->uninstallFilter(filter, true);
}

QList<Filter*> AVPlayer::audioFilters() const
{
    return FilterManager::instance().audioFilters((AVPlayer*)this);
}

QList<Filter*> AVPlayer::videoFilters() const
{
    return FilterManager::instance().videoFilters((AVPlayer*)this);
}

void AVPlayer::setPriority(const QVector<VideoDecoderId> &ids)
{
    d->vc_ids = ids;
    if (!isPlaying())
        return;
    // TODO: add an option to apply immediatly?
    if (!d->vthread || !d->vthread->isRunning()) {
        qint64 pos = position();
        d->setupVideoThread(this);
        if (d->vdec) {
            d->vthread->start();
            setPosition(pos);
        }
        return;
    }
#ifndef ASYNC_DECODER_OPEN
    class ChangeDecoderTask : public QRunnable {
        AVPlayer* player;
    public:
        ChangeDecoderTask(AVPlayer *p) : player(p) {}
        void run() Q_DECL_OVERRIDE {
            player->d->tryApplyDecoderPriority(player);
        }
    };
    d->vthread->scheduleTask(new ChangeDecoderTask(this));
#else
    // maybe better experience
    class NewDecoderTask : public QRunnable {
        AVPlayer *player;
    public:
        NewDecoderTask(AVPlayer *p) : player(p) {}
        void run() Q_DECL_OVERRIDE {
            VideoDecoder *vd = NULL;
            AVCodecContext *avctx = player->d->demuxer.videoCodecContext();
            foreach(VideoDecoderId vid, player->d->vc_ids) {
                qDebug("**********trying video decoder: %s...", VideoDecoderFactory::name(vid).c_str());
                vd = VideoDecoder::create(vid);
                if (!vd)
                    continue;
                vd->setCodecContext(avctx); // It's fine because AVDecoder copy the avctx properties
                vd->setOptions(player->d->vc_opt);
                if (vd->open()) {
                    qDebug("**************Video decoder found:%p", vd);
                    break;
                }
                delete vd;
                vd = 0;
            }
            if (!vd) {
                Q_EMIT player->error(AVError(AVError::VideoCodecNotFound));
                return;
            }
            if (vd->id() == player->d->vdec->id()) {
                qDebug("Video decoder does not change");
                delete vd;
                return;
            }
            class ApplyNewDecoderTask : public QRunnable  {
                AVPlayer *player;
                VideoDecoder *dec;
            public:
                ApplyNewDecoderTask(AVPlayer *p, VideoDecoder *d) : player(p), dec(d) {}
                void run() Q_DECL_OVERRIDE {
                    qint64 pos = player->position();
                    VideoThread *vthread = player->d->vthread;
                    vthread->packetQueue()->clear();
                    vthread->setDecoder(dec);
                    // MUST delete decoder after video thread set the decoder to ensure the deleted vdec will not be used in vthread!
                    if (player->d->vdec)
                        delete player->d->vdec;
                    player->d->vdec = dec;
                    QObject::connect(player->d->vdec, SIGNAL(error(QtAV::AVError)), player, SIGNAL(error(QtAV::AVError)));
                    player->d->initVideoStatistics(player->d->demuxer.videoStream());
                    // If no seek, drop packets until a key frame packet is found. But we may drop too many packets, and also a/v sync is a problem.
                    player->setPosition(pos);
                }
            };
            player->d->vthread->scheduleTask(new ApplyNewDecoderTask(player, vd));
        }
    };
    QThreadPool::globalInstance()->start(new NewDecoderTask(this));
#endif
}

template<typename ID, typename T>
static QVector<ID> idsFromNames(const QStringList& names) {
    QVector<ID> decs;
    if (!names.isEmpty()) {
        decs.reserve(names.size());
        foreach (const QString& name, names) {
            if (name.isEmpty())
                continue;
            ID id = T::id(name.toLatin1().constData());
            if (id == 0)
                continue;
            decs.append(id);
        }
    }
    return decs;
}

void AVPlayer::setVideoDecoderPriority(const QStringList &names)
{
    setPriority(idsFromNames<VideoDecoderId, VideoDecoder>(names));
}

template<typename ID, typename T>
static QStringList idsToNames(QVector<ID> ids) {
    QStringList decs;
    if (!ids.isEmpty()) {
        decs.reserve(ids.size());
        foreach (ID id, ids) {
            decs.append(QString::fromLatin1(T::name(id)));
        }
    }
    return decs;
}

QStringList AVPlayer::videoDecoderPriority() const
{
    return idsToNames<VideoDecoderId, VideoDecoder>(d->vc_ids);
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
    const QVariant p(dict.contains(QStringLiteral("priority")));
    if (p.type() == QVariant::StringList) {
        setVideoDecoderPriority(p.toStringList());
        d->vc_opt.remove(QStringLiteral("priority"));
    }
}

QVariantHash AVPlayer::optionsForVideoCodec() const
{
    return d->vc_opt;
}

void AVPlayer::setMediaEndAction(MediaEndAction value)
{
    if (d->end_action == value)
        return;
    d->end_action = value;
    Q_EMIT mediaEndActionChanged(value);
    d->read_thread->setMediaEndAction(value);
}

MediaEndAction AVPlayer::mediaEndAction() const
{
    return d->end_action;
}

/*
 * loaded state is the state of current setted file.
 * For replaying, we can avoid load a seekable file again.
 * For playing a new file, load() is required.
 */
void AVPlayer::setFile(const QString &path)
{
    // file() is used somewhere else. ensure it is correct
    QString p(path);
    // QFile does not support "file:"
    if (p.startsWith(QLatin1String("file:")))
        p = Internal::Path::toLocal(p);
    d->reset_state = d->current_source.type() != QVariant::String || d->current_source.toString() != p;
    d->current_source = p;
    // TODO: d->reset_state = d->demuxer2.setMedia(path);
    if (d->reset_state) {
        d->audio_track = d->video_track = d->subtitle_track = 0;
        Q_EMIT sourceChanged();
        //Q_EMIT error(AVError(AVError::NoError));
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
        } else { // MediaIO
            d->reset_state = true;
        }
    }
    d->loaded = false;
    d->current_source = QVariant::fromValue(device);
    if (d->reset_state) {
        d->audio_track = d->video_track = d->subtitle_track = 0;
        Q_EMIT sourceChanged();
    }
}

void AVPlayer::setInput(MediaIO *in)
{
    // TODO: d->reset_state = d->demuxer2.setMedia(in);
    if (d->current_source.type() == QVariant::String) {
        d->reset_state = true;
    } else {
        if (d->current_source.canConvert<QIODevice*>()) {
            d->reset_state = true;
        } else { // MediaIO
            d->reset_state = d->current_source.value<QtAV::MediaIO*>() != in;
        }
    }
    d->loaded = false;
    d->current_source = QVariant::fromValue<QtAV::MediaIO*>(in);
    if (d->reset_state) {
        d->audio_track = d->video_track = d->subtitle_track = 0;
        Q_EMIT sourceChanged();
    }
}

MediaIO* AVPlayer::input() const
{
    if (d->current_source.type() == QVariant::String)
        return 0;
    if (!d->current_source.canConvert<QtAV::MediaIO*>())
        return 0;
    return d->current_source.value<QtAV::MediaIO*>();
}

VideoCapture* AVPlayer::videoCapture() const
{
    return d->vcapture;
}

void AVPlayer::play(const QString& path)
{
    setFile(path);
    play();
}

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
    if (!isPlaying())
        return;
    if (isPaused() == p)
        return;

    if (!p) {
        if (d->was_stepping) {
            d->was_stepping = false;
            // If was stepping, skip our position a little bit behind us.
            //  This fixes an issue with the audio timer
            seek(position() - 100);
        }
    }

    audio()->pause(p);
    //pause thread. check pause state?
    d->read_thread->pause(p);
    if (d->athread)
        d->athread->pause(p);
    if (d->vthread)
        d->vthread->pause(p);
    d->clock->pause(p);

    d->state = p ? PausedState : PlayingState;
    Q_EMIT stateChanged(d->state);
    Q_EMIT paused(p);
}

bool AVPlayer::isPaused() const
{
    return (d->read_thread && d->read_thread->isPaused())
            || (d->athread && d->athread->isPaused())
            || (d->vthread && d->vthread->isPaused());
}

MediaStatus AVPlayer::mediaStatus() const
{
    return d->status;
}

void AVPlayer::setAutoLoad(bool value)
{
    if (d->auto_load == value)
        return;
    d->auto_load = value;
    Q_EMIT autoLoadChanged();
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
    Q_EMIT asyncLoadChanged();
}

bool AVPlayer::isAsyncLoad() const
{
    return d->async_load;
}

bool AVPlayer::isLoaded() const
{
    return d->loaded;
}

void AVPlayer::loadInternal()
{
    QMutexLocker lock(&d->load_mutex);
    Q_UNUSED(lock);
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
        } else { // MediaIO
            d->demuxer.setMedia(d->current_source.value<QtAV::MediaIO*>());
        }
    }
    d->loaded = d->demuxer.load();
    d->status = d->demuxer.mediaStatus();
    if (!d->loaded) {
        d->statistics.reset();
        qWarning("Load failed!");
        d->audio_tracks = d->getTracksInfo(&d->demuxer, AVDemuxer::AudioStream);
        Q_EMIT internalAudioTracksChanged(d->audio_tracks);
        d->video_tracks = d->getTracksInfo(&d->demuxer, AVDemuxer::VideoStream);
        Q_EMIT internalVideoTracksChanged(d->video_tracks);
        d->subtitle_tracks = d->getTracksInfo(&d->demuxer, AVDemuxer::SubtitleStream);
        Q_EMIT internalSubtitleTracksChanged(d->subtitle_tracks);
        return;
    }
    d->subtitle_tracks = d->getTracksInfo(&d->demuxer, AVDemuxer::SubtitleStream);
    Q_EMIT internalSubtitleTracksChanged(d->subtitle_tracks);
    d->applySubtitleStream(d->subtitle_track, this);
    d->audio_tracks = d->getTracksInfo(&d->demuxer, AVDemuxer::AudioStream);
    Q_EMIT internalAudioTracksChanged(d->audio_tracks);
    d->video_tracks = d->getTracksInfo(&d->demuxer, AVDemuxer::VideoStream);
    Q_EMIT internalVideoTracksChanged(d->video_tracks);
    Q_EMIT durationChanged(duration());
    Q_EMIT chaptersChanged(chapters());
    // setup parameters from loaded media
    d->media_start_pts = d->demuxer.startTime();
    // TODO: what about other proctols? some vob duration() == 0
    if (duration() > 0)
        d->media_end = mediaStartPosition() + duration();
    else
        d->media_end = kInvalidPosition;
    d->start_position_norm = normalizedPosition(d->start_position);
    d->stop_position_norm = normalizedPosition(d->stop_position);
    int interval = qAbs(d->notify_interval);
    d->initStatistics();
    if (interval != qAbs(d->notify_interval))
        Q_EMIT notifyIntervalChanged();
}

void AVPlayer::unload()
{
    if (!isLoaded())
        return;
    QMutexLocker lock(&d->load_mutex);
    Q_UNUSED(lock);
    d->loaded = false;
    d->demuxer.setInterruptStatus(-1);

    if (d->adec) { // FIXME: crash if audio external=>internal then replay
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
    Q_EMIT chaptersChanged(0);
    Q_EMIT durationChanged(0LL); // for ui, slider is invalid. use stopped instead, and remove this signal here?
    // ??
    d->audio_tracks = d->getTracksInfo(&d->demuxer, AVDemuxer::AudioStream);
    Q_EMIT internalAudioTracksChanged(d->audio_tracks);
    d->video_tracks = d->getTracksInfo(&d->demuxer, AVDemuxer::VideoStream);
    Q_EMIT internalVideoTracksChanged(d->video_tracks);
}

void AVPlayer::setRelativeTimeMode(bool value)
{
    if (d->relative_time_mode == value)
        return;
    d->relative_time_mode = value;
    Q_EMIT relativeTimeModeChanged();
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


qint64 AVPlayer::normalizedPosition(qint64 pos)
{
    if (!isLoaded())
        return pos;
    qint64 p0 = mediaStartPosition();
    qint64 p1 = mediaStopPosition();
    if (relativeTimeMode()) {
        p0 = 0;
        if (p1 != kInvalidPosition)
            p1 -= p0; //duration
    }
    if (pos < 0) {
        if (p1 == kInvalidPosition)
            pos = kInvalidPosition;
        else
            pos += p1;
    }
    return qMax(qMin(pos, p1), p0);
}

qint64 AVPlayer::startPosition() const
{
    return d->start_position;
}

void AVPlayer::setStartPosition(qint64 pos)
{
    d->start_position = pos;
    d->start_position_norm = normalizedPosition(pos);
    Q_EMIT startPositionChanged(d->start_position);
}

qint64 AVPlayer::stopPosition() const
{
    return d->stop_position;
}

void AVPlayer::setStopPosition(qint64 pos)
{
    d->stop_position = pos;
    d->stop_position_norm = normalizedPosition(pos);
    Q_EMIT stopPositionChanged(d->stop_position);
}

void AVPlayer::setTimeRange(qint64 start, qint64 stop)
{
    if (start > stop) {
        qWarning("Invalid time range");
        return;
    }
    setStopPosition(stop);
    setStartPosition(start);
}

bool AVPlayer::isSeekable() const
{
    return d->demuxer.isSeekable();
}

qint64 AVPlayer::position() const
{
    // TODO: videoTime()?
    const qint64 pts = d->clock->value()*1000.0;
    if (relativeTimeMode())
        return pts - absoluteMediaStartPosition();
    return pts;
}

qint64 AVPlayer::displayPosition() const
{
    // Return a cached value if there are seek tasks
    if (d->seeking || d->read_thread->hasSeekTasks() || (d->read_thread->buffer() && d->read_thread->buffer()->isBuffering())) {
        return d->last_known_good_pts = d->read_thread->lastSeekPos();
    }

    // TODO: videoTime()?
    qint64 pts = d->clock->videoTime()*1000.0;

    // If we are stepping around, we want the lastSeekPos.
    /// But if we're just paused by the user... we want another value.
    if (d->was_stepping) {
        pts = d->read_thread->lastSeekPos();
    }
    if (pts < 0) {
        return d->last_known_good_pts;
    }
    d->last_known_good_pts = pts;

    return pts;
}

void AVPlayer::setPosition(qint64 position)
{
    // FIXME: strange things happen if seek out of eof
    if (position > d->stop_position_norm)
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
    d->read_thread->seek(position,pos_pts, seekType());

    Q_EMIT positionChanged(position); //emit relative position
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
    Q_EMIT repeatChanged(d->repeat_max);
}

bool AVPlayer::setExternalAudio(const QString &file)
{
    // TODO: update statistics
    int stream = currentAudioStream();
    if (!isLoaded() && stream < 0)
        stream = 0;
    return setAudioStream(file, stream);
}

QString AVPlayer::externalAudio() const
{
    return d->external_audio;
}

const QVariantList& AVPlayer::externalAudioTracks() const
{
    return d->external_audio_tracks;
}

const QVariantList &AVPlayer::internalAudioTracks() const
{
    return d->audio_tracks;
}

const QVariantList &AVPlayer::internalVideoTracks() const
{
    return d->video_tracks;
}

bool AVPlayer::setAudioStream(const QString &file, int n)
{
    QString path(file);
    // QFile does not support "file:"
    if (path.startsWith(QLatin1String("file:")))
        path = Internal::Path::toLocal(path);
    if (d->audio_track == n && d->external_audio == path)
        return true;
    const bool audio_changed = d->audio_demuxer.fileName() != path;
    if (path.isEmpty()) {
        if (isLoaded()) {
            if (n >= d->demuxer.audioStreams().size()) {
                qWarning("Invalid audio stream number %d/%d", n, d->demuxer.audioStreams().size()-1);
                return false;
            }
        }
        if (audio_changed) {
            d->external_audio_tracks = QVariantList();
            Q_EMIT externalAudioTracksChanged(d->external_audio_tracks);
        }
    } else {
        if (!audio_changed && d->audio_demuxer.isLoaded()) {
            if (n >= d->audio_demuxer.audioStreams().size()) {
                qWarning("Invalid external audio stream number %d/%d", n, d->audio_demuxer.audioStreams().size()-1);
                return false;
            }
        }
    }
    d->audio_track = n;
    d->external_audio = path;
    d->audio_demuxer.setMedia(d->external_audio);
    struct scoped_pause {
        scoped_pause() : was_paused(false), player(0) {}
        void set(bool old, AVPlayer* p) {
            was_paused = old;
            player = p;
            if (player)
                player->pause(true);
        }
        ~scoped_pause() {
            if (player && !was_paused) {
                player->pause(false);
            }
        }
        bool was_paused;
        AVPlayer* player;
    };
    scoped_pause sp;
    if (!isPlaying()) {
        qDebug("set audio track when not playing");
        goto update_demuxer;
    }
    // pause demuxer, clear queues, set demuxer stream, set decoder, set ao, resume
    sp.set(isPaused(), this); //before read_thread->pause(true, true)
    if (!d->external_audio.isEmpty())
        d->read_thread->pause(true, true); // wait to safe set ademuxer

update_demuxer:
     if (!d->external_audio.isEmpty()) {
        if (audio_changed || !d->audio_demuxer.isLoaded()) {
            if (!d->audio_demuxer.load()) {
                qWarning("Failed to load audio track %d@%s", d->audio_track, d->external_audio.toUtf8().constData());
                d->external_audio_tracks = QVariantList();
                Q_EMIT externalAudioTracksChanged(d->external_audio_tracks);
                return false;
            }
            d->external_audio_tracks = d->getTracksInfo(&d->audio_demuxer, AVDemuxer::AudioStream);
            Q_EMIT externalAudioTracksChanged(d->external_audio_tracks);
            d->read_thread->setAudioDemuxer(&d->audio_demuxer);
        }
    }
    if (!isPlaying()) {
        if (d->external_audio.isEmpty()) {
            if (audio_changed) {
                d->read_thread->setAudioDemuxer(0);
                d->audio_demuxer.unload();
            }
        }
        return true;
    }

    if (!d->setupAudioThread(this)) { // adec will be deleted. so audio_demuxer must unload later
        stop();
        return false;
    }

    if (d->external_audio.isEmpty()) {
        if (audio_changed) {
            d->read_thread->setAudioDemuxer(0);
            d->audio_demuxer.unload();
        }
    } else {
        d->audio_demuxer.seek(position());
    }
    return true;
}

bool AVPlayer::setAudioStream(int n)
{
    return setAudioStream(externalAudio(), n);
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
    d->demuxer.setStreamIndex(AVDemuxer::VideoStream, n);
//    if (!isPlaying())
//        return true;
//    // pause demuxer, clear queues, set demuxer stream, set decoder, set ao, resume
//    bool p = isPaused();
//    //int bv = bufferValue();
//    setBufferMode(BufferTime);
//    pause(true);
//    if (!d->setupVideoThread(this)) {
//        stop();
//        return false;
//    }
//    if (!p) pause(false);
//    //QTimer::singleShot(10000, this, SLOT(setBufferValue(bv)));
    return true;
}

const QVariantList& AVPlayer::internalSubtitleTracks() const
{
    return d->subtitle_tracks;
}

bool AVPlayer::setSubtitleStream(int n)
{
    if (d->subtitle_track == n)
        return true;
    d->subtitle_track = n;
    Q_EMIT subtitleStreamChanged(n);
    if (!d->demuxer.isLoaded())
        return true;
    return d->applySubtitleStream(n, this);
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

AVPlayer::State AVPlayer::state() const
{
    return d->state;
}

void AVPlayer::setState(State value)
{
    if (d->state == value)
        return;
    if (value == StoppedState) {
        stop();
        return;
    }
    if (value == PausedState) {
        pause(true);
        return;
    }
    // value == PlayingState
    if (d->state == StoppedState) {
        play();
        return;
    }
    if (d->state == PausedState) {
        pause(false);
        return;
    }

}

bool AVPlayer::load()
{
    if (!d->current_source.isValid()) {
        qDebug("Invalid media source. No file or IODevice was set.");
        return false;
    }
    if (!d->checkSourceChange() && (mediaStatus() == QtAV::LoadingMedia || mediaStatus() == LoadedMedia))
        return true;
    if (isLoaded()) {
        // release codec ctx. if not loaded, they are released by avformat. TODO: always let avformat release them?
        if (d->adec)
            d->adec->setCodecContext(0);
        if (d->vdec)
            d->vdec->setCodecContext(0);
    }
    d->loaded = false;
    d->status = LoadingMedia;
    if (!isAsyncLoad()) {
        loadInternal();
        return d->loaded;
    }

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

void AVPlayer::play()
{
    //FIXME: bad delay after play from here
    if (isPlaying()) {
        qDebug("play() when playing");
        if (!d->checkSourceChange())
            return;
        stop();
    }
    if (!load()) {
        qWarning("load error");
        return;
    }
    if (isLoaded()) { // !asyncLoad() is here because load() returned true
        playInternal();
        return;
    }
    connect(this, SIGNAL(loaded()), this, SLOT(playInternal()));
}

void AVPlayer::playInternal()
{
    {
    QMutexLocker lock(&d->load_mutex);
    Q_UNUSED(lock);
    if (!d->demuxer.isLoaded())
        return;
    d->start_position_norm = normalizedPosition(d->start_position);
    d->stop_position_norm = normalizedPosition(d->stop_position);
    // FIXME: if call play() frequently playInternal may not be called if disconnect here
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
    }
    // setup clock before avthread.start() becuase avthreads use clock. after avthreads setup because of ao check
    masterClock()->reset();
    // TODO: add isVideo() or hasVideo()?
    if (masterClock()->isClockAuto()) {
        qDebug("auto select clock: audio > external");
        if (!d->demuxer.audioCodecContext() || !d->ao || !d->ao->isOpen() || !d->athread) {
            masterClock()->setClockType(AVClock::ExternalClock);
            qDebug("No audio found or audio not supported. Using ExternalClock.");
        } else {
            qDebug("Using AudioClock");
            masterClock()->setClockType(AVClock::AudioClock);
        }
    }
    masterClock()->setInitialValue((double)absoluteMediaStartPosition()/1000.0);
    // from previous play()
    if (d->demuxer.audioCodecContext() && d->athread) {
        qDebug("Starting audio thread...");
        d->athread->start();
    }
    if (d->demuxer.videoCodecContext() && d->vthread) {
        qDebug("Starting video thread...");
        d->vthread->start();
    }

    if (d->demuxer.audioCodecContext() && d->athread)
        d->athread->waitForStarted();
    if (d->demuxer.videoCodecContext() && d->vthread)
        d->vthread->waitForStarted();

    d->read_thread->setMediaEndAction(mediaEndAction());
    d->read_thread->start();

    /// demux thread not started, seek tasks will be cleared
    d->read_thread->waitForStarted();
    if (d->timer_id < 0) {
        //d->timer_id = startNotifyTimer(); //may fail if not in this thread
        QMetaObject::invokeMethod(this, "startNotifyTimer", Qt::AutoConnection);
    }
    d->state = PlayingState;
    if (d->repeat_current < 0)
        d->repeat_current = 0;
    } //end lock scoped here to avoid dead lock if connect started() to a slot that call unload()/play()
    if (d->start_position_norm > 0) {
        if (relativeTimeMode())
            setPosition(qint64((d->start_position_norm + absoluteMediaStartPosition())));
        else
            setPosition((qint64)(d->start_position_norm));
    }
    
    d->was_stepping = false;

    Q_EMIT stateChanged(PlayingState);
    Q_EMIT started(); //we called stop(), so must emit started()
}

void AVPlayer::stopFromDemuxerThread()
{
    qDebug("demuxer thread emit finished. repeat: %d/%d", currentRepeat(), repeat());
    d->seeking = false;
    if (currentRepeat() < 0 || (currentRepeat() >= repeat() && repeat() >= 0)) {
        qreal stop_pts = masterClock()->videoTime();
        if (stop_pts <= 0)
            stop_pts = masterClock()->value();
        masterClock()->reset();
        QMetaObject::invokeMethod(this, "stopNotifyTimer");
        // vars not set by user can be reset
        d->repeat_current = -1;
        d->start_position_norm = 0;
        d->stop_position_norm = kInvalidPosition; // already stopped. so not 0 but invalid. 0 can stop the playback in timerEvent
        d->media_end = kInvalidPosition;
        qDebug("avplayer emit stopped()");
        d->state = StoppedState;
        QMetaObject::invokeMethod(this, "stateChanged", Q_ARG(QtAV::AVPlayer::State, d->state));
        QMetaObject::invokeMethod(this, "stopped");
        QMetaObject::invokeMethod(this, "stoppedAt", Q_ARG(qint64, qint64(stop_pts*1000.0)));
        //Q_EMIT stateChanged(d->state);
        //Q_EMIT stopped();
        //Q_EMIT stoppedAt(stop_pts*1000.0);

        /*
         * currently preload is not supported. so always unload. Then some properties will be reset, e.g. duration()
         */
        unload(); //TODO: invoke?
    } else {
        d->repeat_current++;
        QMetaObject::invokeMethod(this, "play"); //ensure play() is called from player thread
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
    d->demuxer.setInterruptStatus(-1);
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
    Q_EMIT notifyIntervalChanged();
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
    if (d->speed != 1.0) {
        //TODO: check clock type?
        if (d->ao && d->ao->isAvailable()) {
            d->ao->setSpeed(d->speed);
        }
        masterClock()->setSpeed(d->speed);
    } else {
        d->applyFrameRate();
    }
}

void AVPlayer::updateMediaStatus(QtAV::MediaStatus status)
{
    if (status == d->status)
        return;
    d->status = status;
    Q_EMIT mediaStatusChanged(d->status);
}

void AVPlayer::onSeekFinished(qint64 value)
{
    d->seeking = false;
    Q_EMIT seekFinished(value);
    //d->clock->updateValue(value/1000.0);
    if (relativeTimeMode())
        Q_EMIT positionChanged(value - absoluteMediaStartPosition());
    else
        Q_EMIT positionChanged(value);
}

void AVPlayer::onStepFinished()
{
    Q_EMIT stepFinished();
}

void AVPlayer::tryClearVideoRenderers()
{
    if (!d->vthread) {
        qWarning("internal error");
        return;
    }
    if (!(mediaEndAction() & MediaEndAction_KeepDisplay)) {
        d->vthread->clearRenderers();
    }
}

void AVPlayer::seekChapter(int incr)
{
    if (!chapters())
        return;

    qint64 pos = masterClock()->value() * AV_TIME_BASE;
    int i = 0;

    AVFormatContext *ic = d->demuxer.formatContext();

    AVRational av_time_base_q;
    av_time_base_q.num = 1;
    av_time_base_q.den = AV_TIME_BASE;

    /* find the current chapter */
    for (i = 0; i < chapters(); ++i) {
        AVChapter *ch = ic->chapters[i];
        if (av_compare_ts(pos, av_time_base_q, ch->start, ch->time_base) < 0) {
            --i;
            break;
        }
    }

    i += incr;
    //i = FFMAX(i, 0);
    if (i <= 0)
        i = 0;
    if (i >= chapters())
        return;

    //av_log(NULL, AV_LOG_VERBOSE, "Seeking to chapter %d.\n", i);
    qDebug() << QString::fromLatin1("Seeking to chapter : ") << QString::number(i);
    setPosition(av_rescale_q(ic->chapters[i]->start, ic->chapters[i]->time_base,
                             av_time_base_q) / 1000);
}

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
        // vars not set by user can be reset
        d->start_position_norm = 0;
        d->stop_position_norm = 0; // 0 can stop play in timerEvent
        d->media_end = kInvalidPosition;
    } else { //called by player
        stopNotifyTimer();
    }
    d->seeking = false;
    d->reset_state = true;
    d->repeat_current = -1;
    if (!isPlaying()) {
        qDebug("Not playing~");
        if (mediaStatus() == LoadingMedia || mediaStatus() == LoadedMedia) {
            qDebug("loading media: %d", mediaStatus() == LoadingMedia);
            d->demuxer.setInterruptStatus(-1);
        }
        return;
    }
    while (d->read_thread->isRunning()) {
        qDebug("stopping demuxer thread...");
        d->read_thread->stop();
        d->read_thread->wait(500);
        // interrupt to quit av_read_frame quickly.
        d->demuxer.setInterruptStatus(-1);
    }
    qDebug("all audio/video threads stopped... state: %d", d->state);
}

void AVPlayer::timerEvent(QTimerEvent *te)
{
    if (te->timerId() == d->timer_id) {
        // killTimer() should be in the same thread as object. kill here?
        if (isPaused()) {
            //return; //ensure positionChanged emitted for stepForward()
        }
        // active only when playing
        const qint64 t = position();
        if (d->stop_position_norm == kInvalidPosition) { // or check d->stop_position_norm < 0
            // not seekable. network stream
            Q_EMIT positionChanged(t);
            return;
        }
        if (t < d->start_position_norm) {
            //qDebug("position %lld < startPosition %lld", t, d->start_position_norm);
            // or set clock initial value to get correct t
            if (d->start_position_norm != mediaStartPosition()) {
                setPosition(d->start_position_norm);
                return;
            }
        }
        if (t <= d->stop_position_norm) {
            if (!d->seeking) {
                Q_EMIT positionChanged(t);
            }
            return;
        }
        // atEnd() supports dynamic changed duration. but we can not break A-B repeat mode, so check stoppos and mediastoppos
        if ((!d->demuxer.atEnd() || d->read_thread->isRunning()) && stopPosition() >= mediaStopPosition()) {
            if (!d->seeking) {
                Q_EMIT positionChanged(t);
            }
            return;
        }
        // TODO: remove. kill timer in an event;
        if (d->stop_position_norm == 0) { //stop() by user in other thread, state is already reset
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
        if (d->stop_position_norm == mediaStopPosition() || !isSeekable()) {
            // if not seekable, how it can start to play at specified position?
            qDebug("normalized stopPosition() == mediaStopPosition() or !seekable. d->repeat_current=%d", d->repeat_current);
            d->reset_state = false;
            stop(); // repeat after all threads stopped
        } else {
            d->repeat_current++;
            qDebug("noramlized stopPosition() != mediaStopPosition() and seekable. d->repeat_current=%d", d->repeat_current);
            setPosition(d->start_position_norm);
        }
    }
}

//FIXME: If not playing, it will just play but not play one frame.
void AVPlayer::stepForward()
{
    // pause clock
    pause(true); // must pause AVDemuxThread (set user_paused true)
    d->was_stepping = true;
    d->read_thread->stepForward();
}

void AVPlayer::stepBackward()
{
    pause(true);
    d->was_stepping = true;
    d->read_thread->stepBackward();
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

void AVPlayer::seekNextChapter()
{
    if (chapters() <= 1)
        return;
    seekChapter(1);
}

void AVPlayer::seekPreviousChapter()
{
    if (chapters() <= 1)
        return;
    seekChapter(-1);
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

qreal AVPlayer::bufferSpeed() const
{
    const PacketBuffer* buf = d->read_thread->buffer();
    return buf ? buf->bufferSpeedInBytes() : 0;
}

qint64 AVPlayer::buffered() const
{
    const PacketBuffer* buf = d->read_thread->buffer();
    return buf ? buf->buffered() : 0LL;
}

void AVPlayer::setBufferMode(BufferMode mode)
{
    d->buffer_mode = mode;
}

BufferMode AVPlayer::bufferMode() const
{
    return d->buffer_mode;
}

void AVPlayer::setBufferValue(qint64 value)
{
    if (d->buffer_value == value)
        return;
    d->buffer_value = value;
    d->updateBufferValue();
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
    Q_EMIT brightnessChanged(d->brightness);
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
    Q_EMIT contrastChanged(d->contrast);
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

unsigned int AVPlayer::chapters() const
{
    return d->demuxer.formatContext()->nb_chapters;
}

void AVPlayer::setSaturation(int val)
{
    if (d->saturation == val)
        return;
    d->saturation = val;
    Q_EMIT saturationChanged(d->saturation);
    if (d->vthread) {
        d->vthread->setSaturation(val);
    }
}

} //namespace QtAV
