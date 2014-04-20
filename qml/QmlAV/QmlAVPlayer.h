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

#ifndef QTAV_QML_AVPLAYER_H
#define QTAV_QML_AVPLAYER_H

#include <QmlAV/Export.h>
#include <QtCore/QObject>
#include <QmlAV/QQuickItemRenderer.h>

/*!
 *  Not work: autoPlay, autoLoad
 */
namespace QtAV {
class AVPlayer;
}
using namespace QtAV;
class QMLAV_EXPORT QmlAVPlayer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(int duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(int position READ position NOTIFY positionChanged)
    Q_PROPERTY(bool muted READ isMuted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(bool hasAudio READ hasAudio NOTIFY hasAudioChanged)
    Q_PROPERTY(bool hasVideo READ hasVideo NOTIFY hasVideoChanged)
    Q_PROPERTY(PlaybackState playbackState READ playbackState NOTIFY playbackStateChanged)
    Q_PROPERTY(bool autoPlay READ autoPlay WRITE setAutoPlay NOTIFY autoPlayChanged)
    Q_PROPERTY(bool autoLoad READ isAutoLoad WRITE setAutoLoad NOTIFY autoLoadChanged)
    Q_PROPERTY(qreal speed READ speed WRITE setSpeed NOTIFY speedChanged)
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(int loops READ loopCount WRITE setLoopCount NOTIFY loopCountChanged)
    Q_PROPERTY(bool seekable READ isSeekable NOTIFY seekableChanged)

    Q_PROPERTY(ChannelLayout channelLayout READ channelLayout WRITE setChannelLayout NOTIFY channelLayoutChanged)

    Q_ENUMS(Loop)
    Q_ENUMS(PlaybackState)
    Q_ENUMS(ChannelLayout)
    // not supported by QtMultimedia
    Q_PROPERTY(QStringList videoCodecs READ videoCodecs)
    Q_PROPERTY(QStringList videoCodecPriority READ videoCodecPriority WRITE setVideoCodecPriority NOTIFY videoCodecPriorityChanged)
public:
    enum Loop
    {
        Infinite = -1
    };

    enum PlaybackState {
        PlayingState,
        PausedState,
        StoppedState
    };

    // currently supported channels<3.
    enum ChannelLayout {
        ChannelLayoutAuto, //the same as source if channels<=2. otherwise resamples to stero
        Left,
        Right,
        Mono,
        Stero
    };

    explicit QmlAVPlayer(QObject *parent = 0);

    // add QtAV::AVPlayer::isAudioAvailable()?
    bool hasAudio() const;
    bool hasVideo() const;

    QUrl source() const;
    void setSource(const QUrl& url);

    int loopCount() const;
    void setLoopCount(int c);

    QObject* videoOut();
    void setVideoOut(QObject* out);
    qreal volume() const;
    void setVolume(qreal volume);
    bool isMuted() const;
    void setMuted(bool m);
    int duration() const;
    int position() const;
    bool isSeekable() const;
    PlaybackState playbackState() const;
    void setPlaybackState(PlaybackState playbackState);
    qreal speed() const;
    void setSpeed(qreal s);
    Q_INVOKABLE void play(const QUrl& url);
    AVPlayer *player();

    bool isAutoLoad() const;
    void setAutoLoad(bool autoLoad);

    bool autoPlay() const;
    void setAutoPlay(bool autoplay);

    // "FFmpeg", "CUDA", "DXVA", "VAAPI" etc
    QStringList videoCodecs() const;
    QStringList videoCodecPriority() const;
    void setVideoCodecPriority(const QStringList& p);

    void setAudioChannels(int channels);
    int audioChannels() const;
    void setChannelLayout(ChannelLayout channel);
    ChannelLayout channelLayout() const;

public Q_SLOTS:
    void play();
    void pause();
    void stop();
    void nextFrame();
    void seek(int offset);
    void seekForward();
    void seekBackward();

Q_SIGNALS:
    void volumeChanged();
    void mutedChanged();
    // TODO: signal from QtAV::AVPlayer
    void hasAudioChanged();
    void hasVideoChanged();
    void durationChanged();
    void positionChanged();
    void sourceChanged();
    void autoLoadChanged();
    void loopCountChanged();
    void videoOutChanged();
    void playbackStateChanged();
    void autoPlayChanged();
    void speedChanged();
    void paused();
    void stopped();
    void playing();
    void seekableChanged();
    void videoCodecPriorityChanged();
    void channelLayoutChanged();

private Q_SLOTS:
    // connect to signals from player
    void _q_started();
    void _q_stopped();
    void _q_paused(bool);

private:
    Q_DISABLE_COPY(QmlAVPlayer)

    bool mAutoPlay;
    bool mAutoLoad;
    int mLoopCount;
    PlaybackState mPlaybackState;
    QtAV::AVPlayer *mpPlayer;
    QUrl mSource;
    QStringList mVideoCodecs;
    ChannelLayout mChannelLayout;
};

#endif // QTAV_QML_AVPLAYER_H
