/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013 Wang Bin <wbsecg1@gmail.com>

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

#include "QmlAVPlayer.h"
#include <QtAV/AVPlayer.h>
#include <QtAV/AudioOutput.h>

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

qreal QmlAVPlayer::volume() const
{
    AudioOutput *ao = mpPlayer->audio();
    if (ao && ao->isAvailable()) {
        return ao->volume();
    }
    return 0;
}

void QmlAVPlayer::setVolume(qreal volume)
{
    AudioOutput *ao = mpPlayer->audio();
    if (ao && ao->isAvailable() && ao->volume() != volume) {
        ao->setVolume(volume);
        emit volumeChanged();
    }
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

int QmlAVPlayer::duration() const
{
    return mpPlayer->duration();
}

int QmlAVPlayer::position() const
{
    return mpPlayer->masterClock()->value();
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

qreal QmlAVPlayer::speed() const
{
    return mpPlayer->speed();
}

void QmlAVPlayer::setSpeed(qreal s)
{
    if (mpPlayer->speed() == s)
        return;
    mpPlayer->setSpeed(s);
    emit speedChanged();
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
