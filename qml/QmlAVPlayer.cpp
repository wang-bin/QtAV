#include "QmlAVPlayer.h"
#include <QtAV/AVPlayer.h>

QmlAVPlayer::QmlAVPlayer(QObject *parent) :
    QObject(parent)
  , mpPlayer(0)
  , mpVideoOut(0)
{
    mpPlayer = new AVPlayer(this);
    mpPlayer->setPlayerEventFilter(0);
    connect(mpPlayer, SIGNAL(paused(bool)), SLOT(_q_paused(bool)));
    connect(mpPlayer, SIGNAL(started()), SLOT(_q_started()));
    connect(mpPlayer, SIGNAL(stopped()), SLOT(_q_stopped()));
}

QUrl QmlAVPlayer::source() const
{
    return mSource;
}

void QmlAVPlayer::setSource(const QUrl &url)
{
    mSource = url;
    mpPlayer->setRenderer(mpVideoOut);
    if (mSource.isLocalFile()) {
        mpPlayer->setFile(mSource.toLocalFile());
    } else {
        mpPlayer->setFile(mSource.toString());
    }
}

QObject *QmlAVPlayer::videoOut()
{
    return mpVideoOut;
}

void QmlAVPlayer::setVideoOut(QObject *out)
{
    mpVideoOut = static_cast<QQuickItemRenderer*>(out);
}

bool QmlAVPlayer::mute() const
{
    return mpPlayer->isMute();
}

void QmlAVPlayer::setMute(bool m)
{
    if (mpPlayer->isMute() == m)
        return;
    mpPlayer->setMute(m);
    emit muteChanged();
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
    mPlaybackState = playbackState;
    switch (mPlaybackState) {
    case PlayingState:
        if (mpPlayer->isPaused())
            mpPlayer->pause(false);
        else
            mpPlayer->play();
        break;
    case PausedState:
        mpPlayer->pause(true);
        break;
    case StoppedState:
        mpPlayer->stop();
        break;
    default:
        break;
    }
}

void QmlAVPlayer::play(const QUrl &url)
{
    setSource(url);
    play();
}

void QmlAVPlayer::play()
{
    if (!mpVideoOut) {
        qWarning("No video output!");
    }
    mpPlayer->play();
}

void QmlAVPlayer::pause()
{
    mpPlayer->pause(true);
}

void QmlAVPlayer::resume()
{
    mpPlayer->pause(false);
}

void QmlAVPlayer::togglePause()
{
    mpPlayer->togglePause();
}

void QmlAVPlayer::stop()
{
    mpPlayer->stop();
}

void QmlAVPlayer::nextFrame()
{
    mpPlayer->playNextFrame();
}

void QmlAVPlayer::seek(qreal position)
{
    mpPlayer->seek(position);
}

void QmlAVPlayer::seekForward()
{
    mpPlayer->seekForward();
}

void QmlAVPlayer::seekBackward()
{
    mpPlayer->seekBackward();
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
    emit playing();
    emit playbackStateChanged();
}

void QmlAVPlayer::_q_stopped()
{
    mPlaybackState = StoppedState;
    emit stopped();
    emit playbackStateChanged();
}
