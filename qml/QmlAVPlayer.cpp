/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2017 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2013)

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

#include "QmlAV/QmlAVPlayer.h"
#include <QtAV/AVPlayer.h>
#include <QtAV/AudioOutput.h>
#include <QtAV/VideoCapture.h>

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

static inline QStringList VideoDecodersToNames(QVector<QtAV::VideoDecoderId> ids) {
    return idsToNames<QtAV::VideoDecoderId, VideoDecoder>(ids);
}

QmlAVPlayer::QmlAVPlayer(QObject *parent) :
    QObject(parent)
  , mUseWallclockAsTimestamps(false)
  , m_complete(false)
  , m_mute(false)
  , mAutoPlay(false)
  , mAutoLoad(false)
  , mHasAudio(false)
  , mHasVideo(false)
  , m_fastSeek(false)
  , m_loading(false)
  , mLoopCount(1)
  , mStart(0)
  , mStop(PositionMax)
  , mPlaybackRate(1.0)
  , mVolume(1.0)
  , mPlaybackState(StoppedState)
  , mError(NoError)
  , mpPlayer(0)
  , mChannelLayout(ChannelLayoutAuto)
  , m_timeout(30000)
  , m_abort_timeout(true)
  , m_audio_track(0)
  , m_video_track(0)
  , m_sub_track(0)
  , m_ao(AudioOutput::backendsAvailable())
{
    classBegin();
}

void QmlAVPlayer::classBegin()
{
    if (mpPlayer)
        return;
    mpPlayer = new AVPlayer(this);
    connect(mpPlayer, SIGNAL(internalSubtitleTracksChanged(QVariantList)), SIGNAL(internalSubtitleTracksChanged()));
    connect(mpPlayer, SIGNAL(internalAudioTracksChanged(QVariantList)), SIGNAL(internalAudioTracksChanged()));
    connect(mpPlayer, SIGNAL(internalVideoTracksChanged(QVariantList)), SIGNAL(internalVideoTracksChanged()));
    connect(mpPlayer, SIGNAL(externalAudioTracksChanged(QVariantList)), SIGNAL(externalAudioTracksChanged()));
    connect(mpPlayer, SIGNAL(durationChanged(qint64)), SIGNAL(durationChanged()));
    connect(mpPlayer, SIGNAL(mediaStatusChanged(QtAV::MediaStatus)), SLOT(_q_statusChanged()));
    connect(mpPlayer, SIGNAL(error(QtAV::AVError)), SLOT(_q_error(QtAV::AVError)));
    connect(mpPlayer, SIGNAL(paused(bool)), SLOT(_q_paused(bool)));
    connect(mpPlayer, SIGNAL(started()), SLOT(_q_started()));
    connect(mpPlayer, SIGNAL(stopped()), SLOT(_q_stopped()));
    connect(mpPlayer, SIGNAL(positionChanged(qint64)), SIGNAL(positionChanged()));
    connect(mpPlayer, SIGNAL(seekableChanged()), SIGNAL(seekableChanged()));
    connect(mpPlayer, SIGNAL(seekFinished(qint64)), this, SIGNAL(seekFinished()), Qt::DirectConnection);
    connect(mpPlayer, SIGNAL(bufferProgressChanged(qreal)), SIGNAL(bufferProgressChanged()));
    connect(this, SIGNAL(channelLayoutChanged()), SLOT(applyChannelLayout()));
    // direct connection to ensure volume() in slots is correct
    connect(mpPlayer->audio(), SIGNAL(volumeChanged(qreal)), SLOT(applyVolume()), Qt::DirectConnection);
    connect(mpPlayer->audio(), SIGNAL(muteChanged(bool)), SLOT(applyVolume()), Qt::DirectConnection);

    mVideoCodecs << QStringLiteral("FFmpeg");

    m_metaData.reset(new MediaMetaData());

    Q_EMIT mediaObjectChanged();
}

void QmlAVPlayer::componentComplete()
{
    // TODO: set player parameters
    if (mSource.isValid() && (mAutoLoad || mAutoPlay)) {
        if (mAutoLoad)
            mpPlayer->load();
        if (mAutoPlay)
            mpPlayer->play();
    }

    m_complete = true;
}

bool QmlAVPlayer::hasAudio() const
{
    if (!m_complete)
        return false;
    return mHasAudio;
}

bool QmlAVPlayer::hasVideo() const
{
    if (!m_complete)
        return false;
    return mHasVideo;
}

QUrl QmlAVPlayer::source() const
{
    return mSource;
}

void QmlAVPlayer::setSource(const QUrl &url)
{
    if (mSource == url)
        return;
    mSource = url;
    if (url.isLocalFile() || url.scheme().isEmpty()
            || url.scheme().startsWith("qrc")
            || url.scheme().startsWith("avdevice")
            // TODO: what about custom io?
            )
        mpPlayer->setFile(QUrl::fromPercentEncoding(url.toEncoded()));
    else
        mpPlayer->setFile(url.toEncoded());
    Q_EMIT sourceChanged(); //TODO: Q_EMIT only when player loaded a new source

    if (mHasAudio) {
        mHasAudio = false;
        Q_EMIT hasAudioChanged();
    }
    if (mHasVideo) {
        mHasVideo = false;
        Q_EMIT hasVideoChanged();
    }

    // TODO: in componentComplete()?
    if (m_complete && (mAutoLoad || mAutoPlay)) {
        mError = NoError;
        mErrorString = tr("No error");
        Q_EMIT error(mError, mErrorString);
        Q_EMIT errorChanged();
        stop(); // TODO: no stop for autoLoad?
        if (mAutoLoad)
            mpPlayer->load();
        if (mAutoPlay) {
            //mPlaybackState is actually changed in slots. But if set to a new source the state may not change before call play()
            mPlaybackState = StoppedState;
            play();
        }
    }
}

bool QmlAVPlayer::isAutoLoad() const
{
    return mAutoLoad;
}

void QmlAVPlayer::setAutoLoad(bool autoLoad)
{
    if (mAutoLoad == autoLoad)
        return;

    mAutoLoad = autoLoad;
    Q_EMIT autoLoadChanged();
}

bool QmlAVPlayer::autoPlay() const
{
    return mAutoPlay;
}

void QmlAVPlayer::setAutoPlay(bool autoplay)
{
    if (mAutoPlay == autoplay)
        return;

    mAutoPlay = autoplay;
    Q_EMIT autoPlayChanged();
}

MediaMetaData* QmlAVPlayer::metaData() const
{
    return m_metaData.data();
}

QObject* QmlAVPlayer::mediaObject() const
{
    return mpPlayer;
}

VideoCapture *QmlAVPlayer::videoCapture() const
{
    return mpPlayer->videoCapture();
}

QStringList QmlAVPlayer::videoCodecs() const
{
    return VideoDecodersToNames(VideoDecoder::registered());
}

void QmlAVPlayer::setVideoCodecPriority(const QStringList &p)
{
    if (mVideoCodecs == p)
        return;
    mVideoCodecs = p;
    Q_EMIT videoCodecPriorityChanged();
    if (!mpPlayer) {
        qWarning("player not ready");
        return;
    }
    QVariantHash vcopt;
    for (QVariantMap::const_iterator cit = vcodec_opt.cbegin(); cit != vcodec_opt.cend(); ++cit) {
        vcopt[cit.key()] = cit.value();
    }
    if (!vcopt.isEmpty())
        mpPlayer->setOptionsForVideoCodec(vcopt);
    mpPlayer->setVideoDecoderPriority(p);
}

QStringList QmlAVPlayer::videoCodecPriority() const
{
    return mVideoCodecs;
}

QVariantMap QmlAVPlayer::videoCodecOptions() const
{
    return vcodec_opt;
}

void QmlAVPlayer::setVideoCodecOptions(const QVariantMap &value)
{
    if (value == vcodec_opt)
        return;
    vcodec_opt = value;
    Q_EMIT videoCodecOptionsChanged();
    if (!mpPlayer) {
        qWarning("player not ready");
        return;
    }
    QVariantHash vcopt;
    for (QVariantMap::const_iterator cit = vcodec_opt.cbegin(); cit != vcodec_opt.cend(); ++cit) {
        vcopt[cit.key()] = cit.value();
    }
    if (!vcopt.isEmpty())
        mpPlayer->setOptionsForVideoCodec(vcopt);
    mpPlayer->setVideoDecoderPriority(videoCodecPriority());
}

QVariantMap QmlAVPlayer::avFormatOptions() const
{
    return avfmt_opt;
}

void QmlAVPlayer::setAVFormatOptions(const QVariantMap &value)
{
    if (value == avfmt_opt)
        return;
    avfmt_opt = value;
    Q_EMIT avFormatOptionsChanged();
    if (!mpPlayer) {
        qWarning("player not ready");
        return;
    }
    QVariantHash avfopt;
    for (QVariantMap::const_iterator cit = avfmt_opt.cbegin(); cit != avfmt_opt.cend(); ++cit) {
        avfopt[cit.key()] = cit.value();
    }
    if (!avfopt.isEmpty())
        mpPlayer->setOptionsForFormat(avfopt);
}

bool QmlAVPlayer::useWallclockAsTimestamps() const
{
    return mUseWallclockAsTimestamps;
}

void QmlAVPlayer::setWallclockAsTimestamps(bool use_wallclock_as_timestamps)
{
    if (mUseWallclockAsTimestamps == use_wallclock_as_timestamps)
        return;

    mUseWallclockAsTimestamps = use_wallclock_as_timestamps;

    QVariantHash opt = mpPlayer->optionsForFormat();

    if (use_wallclock_as_timestamps) {
        opt[QStringLiteral("use_wallclock_as_timestamps")] = 1;
        mpPlayer->setBufferValue(1);
    } else {
        opt.remove(QStringLiteral("use_wallclock_as_timestamps"));
        mpPlayer->setBufferValue(-1);
    }
    mpPlayer->setOptionsForFormat(opt);
    Q_EMIT useWallclockAsTimestampsChanged();
}

static AudioFormat::ChannelLayout toAudioFormatChannelLayout(QmlAVPlayer::ChannelLayout ch)
{
    struct {
        QmlAVPlayer::ChannelLayout ch;
        AudioFormat::ChannelLayout a;
    } map[] = {
    { QmlAVPlayer::Left, AudioFormat::ChannelLayout_Left },
    { QmlAVPlayer::Right, AudioFormat::ChannelLayout_Right },
    { QmlAVPlayer::Mono, AudioFormat::ChannelLayout_Mono },
    { QmlAVPlayer::Stereo, AudioFormat::ChannelLayout_Stereo },
    };
    for (uint i = 0; i < sizeof(map)/sizeof(map[0]); ++i) {
        if (map[i].ch == ch)
            return map[i].a;
    }
    return AudioFormat::ChannelLayout_Unsupported;
}


void QmlAVPlayer::setChannelLayout(ChannelLayout channel)
{
    if (mChannelLayout == channel)
        return;
    mChannelLayout = channel;
    Q_EMIT channelLayoutChanged();
}

QmlAVPlayer::ChannelLayout QmlAVPlayer::channelLayout() const
{
    return mChannelLayout;
}

void QmlAVPlayer::setTimeout(int value)
{
    if (m_timeout == value)
        return;
    m_timeout = value;
    Q_EMIT timeoutChanged();
    if (mpPlayer)
        mpPlayer->setInterruptTimeout(m_timeout);
}

int QmlAVPlayer::timeout() const
{
    return m_timeout;
}

void QmlAVPlayer::setAbortOnTimeout(bool value)
{
    if (m_abort_timeout == value)
        return;
    m_abort_timeout = value;
    Q_EMIT abortOnTimeoutChanged();
    if (mpPlayer)
        mpPlayer->setInterruptOnTimeout(value);
}

bool QmlAVPlayer::abortOnTimeout() const
{
    return m_abort_timeout;
}

int QmlAVPlayer::audioTrack() const
{
    return m_audio_track;
}

void QmlAVPlayer::setAudioTrack(int value)
{
    if (m_audio_track == value)
        return;
    m_audio_track = value;
    Q_EMIT audioTrackChanged();
    if (mpPlayer)
        mpPlayer->setAudioStream(value);
}

int QmlAVPlayer::videoTrack() const
{
    return m_video_track;
}

void QmlAVPlayer::setVideoTrack(int value)
{
    if (m_video_track == value)
        return;
    m_video_track = value;
    Q_EMIT videoTrackChanged();
    if (mpPlayer)
        mpPlayer->setVideoStream(value);
}

int QmlAVPlayer::bufferSize() const
{
    return mpPlayer->bufferValue();
}

void QmlAVPlayer::setBufferSize(int value)
{
    if (mpPlayer->bufferValue() == value)
        return;
    if (mpPlayer) {
        mpPlayer->setBufferValue(value);
        Q_EMIT bufferSizeChanged();
    }
}

QmlAVPlayer::BufferMode QmlAVPlayer::bufferMode() const
{
    return static_cast<BufferMode>(mpPlayer->bufferMode());
}

void QmlAVPlayer::setBufferMode(QmlAVPlayer::BufferMode value)
{
    QtAV::BufferMode mode = static_cast<QtAV::BufferMode>(value);
    if(mpPlayer->bufferMode() == mode)
        return;
    if(mpPlayer) {
        mpPlayer->setBufferMode(mode);
        Q_EMIT bufferModeChanged();
    }
}

qreal QmlAVPlayer::frameRate() const
{
    return mpPlayer->forcedFrameRate();
}

void QmlAVPlayer::setFrameRate(qreal value)
{
    if(mpPlayer) {
        mpPlayer->setFrameRate(value);
        Q_EMIT frameRateChanged();
    }
}

QmlAVPlayer::MediaEndAction QmlAVPlayer::mediaEndAction() const
{
    return  static_cast<MediaEndAction>(int(mpPlayer->mediaEndAction()));
}

void QmlAVPlayer::setMediaEndAction(QmlAVPlayer::MediaEndAction value)
{
    QtAV::MediaEndAction action = static_cast<QtAV::MediaEndAction>(value);
    if (mpPlayer->mediaEndAction() == action)
        return;
    if (mpPlayer) {
        mpPlayer->setMediaEndAction(action);
        Q_EMIT mediaEndActionChanged();
    }
}

QUrl QmlAVPlayer::externalAudio() const
{
    return m_audio;
}

void QmlAVPlayer::setExternalAudio(const QUrl &url)
{
    if (m_audio == url)
        return;
    m_audio = url;
    mpPlayer->setExternalAudio(QUrl::fromPercentEncoding(m_audio.toEncoded()));
    Q_EMIT externalAudioChanged();
}

QVariantList QmlAVPlayer::externalAudioTracks() const
{
    return mpPlayer ? mpPlayer->externalAudioTracks() : QVariantList();
}

QVariantList QmlAVPlayer::internalAudioTracks() const
{
    return mpPlayer ? mpPlayer->internalAudioTracks() : QVariantList();
}

QVariantList QmlAVPlayer::internalVideoTracks() const
{
    return mpPlayer ? mpPlayer->internalVideoTracks() : QVariantList();
}

int QmlAVPlayer::internalSubtitleTrack() const
{
    return m_sub_track;
}

void QmlAVPlayer::setInternalSubtitleTrack(int value)
{
    if (m_sub_track == value)
        return;
    m_sub_track = value;
    Q_EMIT internalSubtitleTrackChanged();
    if (mpPlayer)
        mpPlayer->setSubtitleStream(value);
}

QVariantList QmlAVPlayer::internalSubtitleTracks() const
{
    return mpPlayer ? mpPlayer->internalSubtitleTracks() : QVariantList();
}

QQmlListProperty<QuickAudioFilter> QmlAVPlayer::audioFilters()
{
    return QQmlListProperty<QuickAudioFilter>(this, NULL, af_append, af_count, af_at, af_clear);
}

QQmlListProperty<QuickVideoFilter> QmlAVPlayer::videoFilters()
{
    return QQmlListProperty<QuickVideoFilter>(this, NULL, vf_append, vf_count, vf_at, vf_clear);
}

void QmlAVPlayer::af_append(QQmlListProperty<QuickAudioFilter> *property, QuickAudioFilter *value)
{
    QmlAVPlayer* self = static_cast<QmlAVPlayer*>(property->object);
    self->m_afilters.append(value);
    if (self->mpPlayer)
        self->mpPlayer->installFilter(value);
}

int QmlAVPlayer::af_count(QQmlListProperty<QuickAudioFilter> *property)
{
    QmlAVPlayer* self = static_cast<QmlAVPlayer*>(property->object);
    return self->m_afilters.size();
}

QuickAudioFilter* QmlAVPlayer::af_at(QQmlListProperty<QuickAudioFilter> *property, int index)
{
    QmlAVPlayer* self = static_cast<QmlAVPlayer*>(property->object);
    return self->m_afilters.at(index);
}

void QmlAVPlayer::af_clear(QQmlListProperty<QuickAudioFilter> *property)
{
    QmlAVPlayer* self = static_cast<QmlAVPlayer*>(property->object);
    if (self->mpPlayer) {
        foreach (QuickAudioFilter *f, self->m_afilters) {
            self->mpPlayer->uninstallFilter(f);
        }
    }
    self->m_afilters.clear();
}

void QmlAVPlayer::vf_append(QQmlListProperty<QuickVideoFilter> *property, QuickVideoFilter *value)
{
    QmlAVPlayer* self = static_cast<QmlAVPlayer*>(property->object);
    self->m_vfilters.append(value);
    if (self->mpPlayer)
        self->mpPlayer->installFilter(value);
}

int QmlAVPlayer::vf_count(QQmlListProperty<QuickVideoFilter> *property)
{
    QmlAVPlayer* self = static_cast<QmlAVPlayer*>(property->object);
    return self->m_vfilters.size();
}

QuickVideoFilter* QmlAVPlayer::vf_at(QQmlListProperty<QuickVideoFilter> *property, int index)
{
    QmlAVPlayer* self = static_cast<QmlAVPlayer*>(property->object);
    return self->m_vfilters.at(index);
}

void QmlAVPlayer::vf_clear(QQmlListProperty<QuickVideoFilter> *property)
{
    QmlAVPlayer* self = static_cast<QmlAVPlayer*>(property->object);
    if (self->mpPlayer) {
        foreach (QuickVideoFilter *f, self->m_vfilters) {
            self->mpPlayer->uninstallFilter(f);
        }
    }
    self->m_vfilters.clear();
}

QStringList QmlAVPlayer::audioBackends() const
{
    return m_ao;
}

void QmlAVPlayer::setAudioBackends(const QStringList &value)
{
    if (m_ao == value)
        return;
    m_ao = value;
    Q_EMIT audioBackendsChanged();
}

QStringList QmlAVPlayer::supportedAudioBackends() const
{
    return AudioOutput::backendsAvailable();
}

int QmlAVPlayer::loopCount() const
{
    return mLoopCount;
}

void QmlAVPlayer::setLoopCount(int c)
{
    if (c == 0)
        c = 1;
    else if (c < -1)
        c = -1;
    if (mLoopCount == c) {
        return;
    }
    mLoopCount = c;
    Q_EMIT loopCountChanged();
}

qreal QmlAVPlayer::volume() const
{
    return mVolume;
}

// mVolume, m_mute are required by qml properties. player.audio()->setXXX is not enought because player maybe not created
void QmlAVPlayer::setVolume(qreal value)
{
    if (mVolume < 0) {
        qWarning("volume must > 0");
        return;
    }
    if (qFuzzyCompare(mVolume + 1.0, value + 1.0))
        return;
    mVolume = value;
    Q_EMIT volumeChanged();
    applyVolume();
}

bool QmlAVPlayer::isMuted() const
{
    return m_mute;
}

void QmlAVPlayer::setMuted(bool m)
{
    if (isMuted() == m)
        return;
    m_mute = m;
    Q_EMIT mutedChanged();
    applyVolume();
}

int QmlAVPlayer::duration() const
{
    return mpPlayer ? mpPlayer->duration() : 0;
}

int QmlAVPlayer::position() const
{
    return mpPlayer ? mpPlayer->position() : 0;
}

bool QmlAVPlayer::isSeekable() const
{
    return mpPlayer && mpPlayer->isSeekable();
}

int QmlAVPlayer::startPosition() const
{
    return mStart;
}

void QmlAVPlayer::setStartPosition(int value)
{
    if (mStart == value)
        return;
    mStart = value;
    Q_EMIT startPositionChanged();
    if (mpPlayer) {
        mpPlayer->setStartPosition(mStart);
    }
}

int QmlAVPlayer::stopPosition() const
{
    return mStop;
}

void QmlAVPlayer::setStopPosition(int value)
{
    if (mStop == value)
        return;
    mStop = value;
    Q_EMIT stopPositionChanged();
    if (mpPlayer) {
        if (value == PositionMax)
            mpPlayer->setStopPosition();
        else
            mpPlayer->setStopPosition(value);
    }
}

bool QmlAVPlayer::isFastSeek() const
{
    return m_fastSeek;
}

void QmlAVPlayer::setFastSeek(bool value)
{
    if (m_fastSeek == value)
        return;
    m_fastSeek = value;
    Q_EMIT fastSeekChanged();
}

qreal QmlAVPlayer::bufferProgress() const
{
    if (!mpPlayer)
        return 0;
    return mpPlayer->bufferProgress();
}

QmlAVPlayer::Status QmlAVPlayer::status() const
{
    if (!mpPlayer)
        return NoMedia;
    return (Status)mpPlayer->mediaStatus();
}

QmlAVPlayer::Error QmlAVPlayer::error() const
{
    return mError;
}

QString QmlAVPlayer::errorString() const
{
    return mErrorString;
}

QmlAVPlayer::PlaybackState QmlAVPlayer::playbackState() const
{
    return mPlaybackState;
}

void QmlAVPlayer::setPlaybackState(PlaybackState playbackState)
{
    if (mPlaybackState == playbackState) {
        return;
    }
    if (!m_complete || !mpPlayer)
        return;
    switch (playbackState) {
    case PlayingState:
        if (mpPlayer->isPaused()) {
            mpPlayer->pause(false);
        } else {
            mpPlayer->setInterruptTimeout(m_timeout);
            mpPlayer->setInterruptOnTimeout(m_abort_timeout);
            mpPlayer->setRepeat(mLoopCount - 1);
            mpPlayer->setAudioStream(m_audio_track);
            mpPlayer->setVideoStream(m_video_track); // will check in case of error
            mpPlayer->setSubtitleStream(m_sub_track);
            if (!vcodec_opt.isEmpty()) {
                QVariantHash vcopt;
                for (QVariantMap::const_iterator cit = vcodec_opt.cbegin(); cit != vcodec_opt.cend(); ++cit) {
                    vcopt[cit.key()] = cit.value();
                }
                if (!vcopt.isEmpty())
                    mpPlayer->setOptionsForVideoCodec(vcopt);
            }
            if (!avfmt_opt.isEmpty()) {
                QVariantHash avfopt;
                for (QVariantMap::const_iterator cit = avfmt_opt.cbegin(); cit != avfmt_opt.cend(); ++cit) {
                    avfopt[cit.key()] = cit.value();
                }
                if (!avfopt.isEmpty())
                    mpPlayer->setOptionsForFormat(avfopt);
            }

            mpPlayer->setStartPosition(startPosition());
            if (stopPosition() == PositionMax)
                mpPlayer->setStopPosition();
            else
                mpPlayer->setStopPosition(stopPosition());

            m_loading = true;
            // TODO: change backends is not thread safe now, so change when stopped
            mpPlayer->audio()->setBackends(m_ao);
            mpPlayer->play();
        }
        break;
    case PausedState:
        mpPlayer->pause(true);
        mPlaybackState = PausedState;
        break;
    case StoppedState:
        mpPlayer->stop();
        m_loading = false;
        mPlaybackState = StoppedState;
        break;
    default:
        break;
    }
}

qreal QmlAVPlayer::playbackRate() const
{
    return mPlaybackRate;
}

void QmlAVPlayer::setPlaybackRate(qreal s)
{
    if (playbackRate() == s)
        return;
    mPlaybackRate = s;
    if (mpPlayer)
        mpPlayer->setSpeed(s);
    Q_EMIT playbackRateChanged();
}

AVPlayer* QmlAVPlayer::player()
{
    return mpPlayer;
}

void QmlAVPlayer::play(const QUrl &url)
{
    if (mSource == url && (playbackState() != StoppedState || m_loading))
        return;
    setSource(url);
    if (!autoPlay())
        play();
}

void QmlAVPlayer::play()
{
    // if not autoPlay, maybe a different source was set and play() was not called
    if (isAutoLoad() && (playbackState() == PlayingState || m_loading))
        return;
    setPlaybackState(PlayingState);
}

void QmlAVPlayer::pause()
{
    setPlaybackState(PausedState);
}

void QmlAVPlayer::stop()
{
    setPlaybackState(StoppedState);
}

void QmlAVPlayer::stepForward()
{
    if (!mpPlayer)
        return;
    mpPlayer->stepForward();
}

void QmlAVPlayer::stepBackward()
{
    if (!mpPlayer)
        return;
    return mpPlayer->stepBackward();
}

void QmlAVPlayer::seek(int offset)
{
    if (!mpPlayer)
        return;
    mpPlayer->setSeekType(isFastSeek() ? KeyFrameSeek : AccurateSeek);
    mpPlayer->seek(qint64(offset));
}

void QmlAVPlayer::seekForward()
{
    if (!mpPlayer)
        return;
    mpPlayer->setSeekType(isFastSeek() ? KeyFrameSeek : AccurateSeek);
    mpPlayer->seekForward();
}

void QmlAVPlayer::seekBackward()
{
    if (!mpPlayer)
        return;
    mpPlayer->setSeekType(isFastSeek() ? KeyFrameSeek : AccurateSeek);
    mpPlayer->seekBackward();
}

void QmlAVPlayer::_q_error(const AVError &e)
{
    mError = NoError;
    mErrorString = e.string();
    const AVError::ErrorCode ec = e.error();
    if (ec <= AVError::NoError)
        mError = NoError;
    else if (ec <= AVError::NetworkError)
        mError = NetworkError;
    else if (ec <= AVError::ResourceError)
        mError = ResourceError;
    else if (ec <= AVError::FormatError)
        mError = FormatError;
    else if (ec <= AVError::AccessDenied)
        mError = AccessDenied;
    //else
      //  err = ServiceMissing;
    if (ec != AVError::NoError)
        m_loading = false;
    Q_EMIT error(mError, mErrorString);
    Q_EMIT errorChanged();
}

void QmlAVPlayer::_q_statusChanged()
{
    Q_EMIT statusChanged();
}

void QmlAVPlayer::_q_paused(bool p)
{
    if (p) {
        mPlaybackState = PausedState;
        Q_EMIT paused();
    } else {
        mPlaybackState = PlayingState;
        Q_EMIT playing();
    }
    Q_EMIT playbackStateChanged();
}

void QmlAVPlayer::_q_started()
{
    m_loading = false;
    mPlaybackState = PlayingState;
    applyChannelLayout();
    // applyChannelLayout() first because it may reopen audio device
    applyVolume(); //sender is AVPlayer

    mpPlayer->audio()->setMute(isMuted());
    mpPlayer->setSpeed(playbackRate());
    // TODO: in load()?
    m_metaData->setValuesFromStatistics(mpPlayer->statistics());
    if (!mHasAudio) {
        mHasAudio = !mpPlayer->internalAudioTracks().isEmpty();
        if (mHasAudio)
            Q_EMIT hasAudioChanged();
    }
    if (!mHasVideo) {
        mHasVideo = mpPlayer->videoStreamCount() > 0;
        if (mHasVideo)
            Q_EMIT hasVideoChanged();
    }
    Q_EMIT playing();
    Q_EMIT playbackStateChanged();
}

void QmlAVPlayer::_q_stopped()
{
    mPlaybackState = StoppedState;
    Q_EMIT stopped();
    Q_EMIT playbackStateChanged();
}

void QmlAVPlayer::applyVolume()
{
    AudioOutput *ao = mpPlayer->audio();
    if (!ao || !ao->isAvailable())
        return;
    if (!sender() || qobject_cast<AudioOutput*>(sender()) != ao) {
        ao->setVolume(volume()); // will omit if value is not changed
        ao->setMute(isMuted());
        return;
    }
    // from ao.reportVolume() reportMute()
    setVolume(ao->volume());// will omit if value is not changed
    setMuted(ao->isMute());
}

void QmlAVPlayer::applyChannelLayout()
{
    AudioOutput *ao = mpPlayer->audio();
    if (!ao || !ao->isAvailable())
        return;
    AudioFormat af = ao->audioFormat();
    AudioFormat::ChannelLayout ch = toAudioFormatChannelLayout(channelLayout());
    if (channelLayout() == ChannelLayoutAuto || ch == af.channelLayout())
        return;
    af.setChannelLayout(ch);
    if (!ao->close()) {
        qWarning("close audio failed");
        return;
    }
    ao->setAudioFormat(af);
    if (!ao->open()) {
        qWarning("open audio failed");
        return;
    }
}
