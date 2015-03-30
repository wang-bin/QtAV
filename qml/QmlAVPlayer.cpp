/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013-2015 Wang Bin <wbsecg1@gmail.com>

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

#include "QmlAV/QmlAVPlayer.h"
#include <QtAV/AVPlayer.h>
#include <QtAV/AudioOutput.h>

template<typename ID, typename Factory>
static QStringList idsToNames(QVector<ID> ids) {
    QStringList decs;
    foreach (ID id, ids) {
        decs.append(Factory::name(id).c_str());
    }
    return decs;
}

template<typename ID, typename Factory>
static QVector<ID> idsFromNames(const QStringList& names) {
    QVector<ID> decs;
    foreach (const QString& name, names) {
        if (name.isEmpty())
            continue;
        ID id = Factory::id(name.toStdString(), false);
        if (id == 0)
            continue;
        decs.append(id);
    }
    return decs;
}

static inline QStringList VideoDecodersToNames(QVector<QtAV::VideoDecoderId> ids) {
    return idsToNames<QtAV::VideoDecoderId, VideoDecoderFactory>(ids);
}

static inline QVector<VideoDecoderId> VideoDecodersFromNames(const QStringList& names) {
    return idsFromNames<QtAV::VideoDecoderId, VideoDecoderFactory>(names);
}

QmlAVPlayer::QmlAVPlayer(QObject *parent) :
    QObject(parent)
  , m_complete(false)
  , m_mute(false)
  , mAutoPlay(false)
  , mAutoLoad(false)
  , mHasAudio(false)
  , mHasVideo(false)
  , m_fastSeek(false)
  , mLoopCount(1)
  , mPlaybackRate(1.0)
  , mVolume(1.0)
  , mPlaybackState(StoppedState)
  , mError(NoError)
  , mpPlayer(0)
  , mChannelLayout(ChannelLayoutAuto)
  , m_timeout(30000)
{
}

void QmlAVPlayer::classBegin()
{
    mpPlayer = new AVPlayer(this);
    connect(mpPlayer, SIGNAL(mediaStatusChanged(QtAV::MediaStatus)), SLOT(_q_statusChanged()));
    connect(mpPlayer, SIGNAL(error(QtAV::AVError)), SLOT(_q_error(QtAV::AVError)));
    connect(mpPlayer, SIGNAL(paused(bool)), SLOT(_q_paused(bool)));
    connect(mpPlayer, SIGNAL(started()), SLOT(_q_started()));
    connect(mpPlayer, SIGNAL(stopped()), SLOT(_q_stopped()));
    connect(mpPlayer, SIGNAL(positionChanged(qint64)), SIGNAL(positionChanged()));
    connect(mpPlayer, SIGNAL(seekableChanged()), SIGNAL(seekableChanged()));
    connect(mpPlayer, SIGNAL(bufferProgressChanged(qreal)), SIGNAL(bufferProgressChanged()));
    connect(this, SIGNAL(channelLayoutChanged()), SLOT(applyChannelLayout()));

    mVideoCodecs << "FFmpeg";

    m_metaData.reset(new MediaMetaData());

    emit mediaObjectChanged();
}

void QmlAVPlayer::componentComplete()
{
    // TODO: set player parameters
    if (mSource.isValid() && (mAutoLoad || mAutoPlay)) {
        mpPlayer->setFile(QUrl::fromPercentEncoding(mSource.toEncoded()));
    }

    m_complete = true;

    if (mAutoPlay) {
        if (!mSource.isValid()) {
            stop();
        } else {
            play();
        }
    }
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
    mpPlayer->setFile(QUrl::fromPercentEncoding(mSource.toEncoded()));
    emit sourceChanged(); //TODO: emit only when player loaded a new source

    if (mHasAudio) {
        mHasAudio = false;
        emit hasAudioChanged();
    }
    if (mHasVideo) {
        mHasVideo = false;
        emit hasVideoChanged();
    }

    // TODO: in componentComplete()?
    if (m_complete && (mAutoLoad || mAutoPlay)) {
        mError = NoError;
        mErrorString = tr("No error");
        emit error(mError, mErrorString);
        emit errorChanged();
        stop();
        //mpPlayer->load(); //QtAV internal bug: load() or play() results in reload
        if (mAutoPlay) {
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
    emit autoLoadChanged();
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
    emit autoPlayChanged();
}

MediaMetaData* QmlAVPlayer::metaData() const
{
    return m_metaData.data();
}

QObject* QmlAVPlayer::mediaObject() const
{
    return mpPlayer;
}

QStringList QmlAVPlayer::videoCodecs() const
{
    return VideoDecodersToNames(QtAV::GetRegistedVideoDecoderIds());
}

void QmlAVPlayer::setVideoCodecPriority(const QStringList &p)
{
    if (!mpPlayer) {
        qWarning("player not ready");
        return;
    }
    mVideoCodecs = p;
    mpPlayer->setPriority(VideoDecodersFromNames(p));
    emit videoCodecPriorityChanged();
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
    emit videoCodecOptionsChanged();
    // player maybe not ready
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
    { QmlAVPlayer::Stero, AudioFormat::ChannelLayout_Stero },
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
    emit channelLayoutChanged();
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
    emit timeoutChanged();
    if (mpPlayer)
        mpPlayer->setInterruptTimeout(m_timeout);
}

int QmlAVPlayer::timeout() const
{
    return m_timeout;
}

QStringList QmlAVPlayer::videoCodecPriority() const
{
    return mVideoCodecs;
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
    emit loopCountChanged();
}

qreal QmlAVPlayer::volume() const
{
    return mVolume;
}

// mVolume, m_mute are required by qml properties. player.audio()->setXXX is not enought because player maybe not created
void QmlAVPlayer::setVolume(qreal volume)
{
    if (mVolume < 0) {
        qWarning("volume must > 0");
        return;
    }
    if (mVolume == volume)
        return;
    mVolume = volume;
    emit volumeChanged();
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
    emit mutedChanged();
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

bool QmlAVPlayer::isFastSeek() const
{
    return m_fastSeek;
}

void QmlAVPlayer::setFastSeek(bool value)
{
    if (m_fastSeek == value)
        return;
    m_fastSeek = value;
    emit fastSeekChanged();
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
    mPlaybackState = playbackState;
    switch (mPlaybackState) {
    case PlayingState:
        if (mpPlayer->isPaused()) {
            mpPlayer->pause(false);
        } else {
            mpPlayer->setInterruptTimeout(m_timeout);
            mpPlayer->setRepeat(mLoopCount - 1);
            if (!vcodec_opt.isEmpty()) {
                QVariantHash vcopt;
                for (QVariantMap::const_iterator cit = vcodec_opt.cbegin(); cit != vcodec_opt.cend(); ++cit) {
                    vcopt[cit.key()] = cit.value();
                }
                if (!vcopt.isEmpty())
                    mpPlayer->setOptionsForVideoCodec(vcopt);
            }
            mpPlayer->play();
        }
        break;
    case PausedState:
        mpPlayer->pause(true);
        break;
    case StoppedState:
        mpPlayer->stop();
        mpPlayer->unload();
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
    emit playbackRateChanged();
}

AVPlayer* QmlAVPlayer::player()
{
    return mpPlayer;
}

void QmlAVPlayer::play(const QUrl &url)
{
    if (mSource == url)
        return;
    setSource(url);
    play();
}

void QmlAVPlayer::play()
{
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

void QmlAVPlayer::nextFrame()
{
    if (!mpPlayer)
        return;
    mpPlayer->playNextFrame();
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
    emit error(mError, mErrorString);
    emit errorChanged();
}

void QmlAVPlayer::_q_statusChanged()
{
    emit statusChanged();
}

void QmlAVPlayer::_q_paused(bool p)
{
    if (p) {
        mPlaybackState = PausedState;
        emit paused();
    } else {
        mPlaybackState = PlayingState;
        emit playing();
    }
    emit playbackStateChanged();
}

void QmlAVPlayer::_q_started()
{
    mPlaybackState = PlayingState;
    applyChannelLayout();
    // applyChannelLayout() first because it may reopen audio device
    applyVolume(); //sender is AVPlayer

    mpPlayer->setMute(isMuted());
    mpPlayer->setSpeed(playbackRate());
    // TODO: in load()?
    m_metaData->setValuesFromStatistics(mpPlayer->statistics());
    if (!mHasAudio) {
        mHasAudio = mpPlayer->audioStreamCount() > 0;
        if (mHasAudio)
            emit hasAudioChanged();
    }
    if (!mHasVideo) {
        mHasVideo = mpPlayer->videoStreamCount() > 0;
        if (mHasVideo)
            emit hasVideoChanged();
    }
    emit playing();
    emit playbackStateChanged();
    if (mHasAudio) {
        connect(this, SIGNAL(volumeChanged()), SLOT(applyVolume()));
        connect(this, SIGNAL(mutedChanged()), SLOT(applyVolume()));
        // direct connection to ensure volume() in slots is correct
        connect(mpPlayer->audio(), SIGNAL(volumeChanged(qreal)), SLOT(applyVolume()), Qt::DirectConnection);
        connect(mpPlayer->audio(), SIGNAL(muteChanged(bool)), SLOT(applyVolume()), Qt::DirectConnection);
    } else {
        disconnect(this, SLOT(applyVolume()));
    }
}

void QmlAVPlayer::_q_stopped()
{
    mPlaybackState = StoppedState;
    emit stopped();
    emit playbackStateChanged();
}

void QmlAVPlayer::applyVolume()
{
    AudioOutput *ao = mpPlayer->audio();
    if (!ao || !ao->isAvailable())
        return;
    if (!sender() || qobject_cast<AudioOutput*>(sender()) != ao) {
        if (!qFuzzyCompare(volume() + 1.0, ao->volume() + 1.0)) {
            ao->setVolume(volume());
        }
        if (isMuted() != ao->isMute()) {
            ao->setMute(isMuted());
        }
        return;
    }
    // from ao.reportVolume()
    if (!qFuzzyCompare(mVolume + 1.0, ao->volume() + 1.0)) {
        mVolume = ao->volume();
        Q_EMIT volumeChanged();
    }
    if (m_mute != ao->isMute()) {
        m_mute = ao->isMute();
        Q_EMIT mutedChanged();
    }
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
