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
#include <QmlAV/MediaMetaData.h>
#include <QtAV/AVError.h>
#include <QtAV/CommonTypes.h>

/*!
 *  Qt.Multimedia like api
 * MISSING:
 * bufferProgress, error, errorString, metaData
 * NOT COMPLETE:
 * seekable
 */
namespace QtAV {
class AVPlayer;
}
using namespace QtAV;
class QMLAV_EXPORT QmlAVPlayer : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(Error error READ error NOTIFY errorChanged)
    Q_PROPERTY(int duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(int position READ position NOTIFY positionChanged)
    Q_PROPERTY(bool muted READ isMuted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(bool hasAudio READ hasAudio NOTIFY hasAudioChanged)
    Q_PROPERTY(bool hasVideo READ hasVideo NOTIFY hasVideoChanged)
    Q_PROPERTY(PlaybackState playbackState READ playbackState NOTIFY playbackStateChanged)
    Q_PROPERTY(bool autoPlay READ autoPlay WRITE setAutoPlay NOTIFY autoPlayChanged)
    Q_PROPERTY(bool autoLoad READ isAutoLoad WRITE setAutoLoad NOTIFY autoLoadChanged)
    Q_PROPERTY(qreal playbackRate READ playbackRate WRITE setPlaybackRate NOTIFY playbackRateChanged)
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(int loops READ loopCount WRITE setLoopCount NOTIFY loopCountChanged)
    Q_PROPERTY(bool seekable READ isSeekable NOTIFY seekableChanged)
    Q_PROPERTY(MediaMetaData *metaData READ metaData CONSTANT)
    Q_PROPERTY(QObject *mediaObject READ mediaObject)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorChanged)
    Q_ENUMS(Loop)
    Q_ENUMS(PlaybackState)
    Q_ENUMS(Status)
    Q_ENUMS(Error)
    Q_ENUMS(ChannelLayout)
    // not supported by QtMultimedia
    Q_PROPERTY(ChannelLayout channelLayout READ channelLayout WRITE setChannelLayout NOTIFY channelLayoutChanged)
    Q_PROPERTY(QStringList videoCodecs READ videoCodecs)
    Q_PROPERTY(QStringList videoCodecPriority READ videoCodecPriority WRITE setVideoCodecPriority NOTIFY videoCodecPriorityChanged)
    Q_PROPERTY(QVariantMap videoCodecOptions READ videoCodecOptions WRITE setVideoCodecOptions NOTIFY videoCodecOptionsChanged)
public:
    enum Loop { Infinite = -1 };
    enum PlaybackState {
        StoppedState,
        PlayingState,
        PausedState
    };
    enum Status {
        UnknownMediaStatus = QtAV::UnknownMediaStatus, // e.g. user status after interrupt
        NoMedia = QtAV::NoMedia,
        LoadingMedia = QtAV::LoadingMedia, // when source is set
        LoadedMedia = QtAV::LoadedMedia, // if auto load and source is set. player is stopped state
        StalledMedia = QtAV::StalledMedia,
        BufferingMedia = QtAV::BufferingMedia,
        BufferedMedia = QtAV::BufferedMedia, // when playing
        EndOfMedia = QtAV::EndOfMedia,
        InvalidMedia = QtAV::InvalidMedia
    };
    enum Error {
        NoError,
        ResourceError,
        FormatError,
        NetworkError,
        AccessDenied,
        ServiceMissing
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
    //derived
    virtual void classBegin();
    virtual void componentComplete();

    // add QtAV::AVPlayer::isAudioAvailable()?
    bool hasAudio() const;
    bool hasVideo() const;

    QUrl source() const;
    void setSource(const QUrl& url);

    // 0,1: play once. Loop.Infinite: forever.
    // >1: play loopCount() - 1 times. different from Qt
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

    Status status() const;
    Error error() const;
    QString errorString() const;
    PlaybackState playbackState() const;
    void setPlaybackState(PlaybackState playbackState);
    qreal playbackRate() const;
    void setPlaybackRate(qreal s);
    Q_INVOKABLE void play(const QUrl& url);
    AVPlayer *player();

    bool isAutoLoad() const;
    void setAutoLoad(bool autoLoad);

    bool autoPlay() const;
    void setAutoPlay(bool autoplay);

    MediaMetaData *metaData() const;
    QObject *mediaObject() const;

    // "FFmpeg", "CUDA", "DXVA", "VAAPI" etc
    QStringList videoCodecs() const;
    QStringList videoCodecPriority() const;
    void setVideoCodecPriority(const QStringList& p);
    QVariantMap videoCodecOptions() const;
    void setVideoCodecOptions(const QVariantMap& value);

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
    void playbackRateChanged();
    void paused();
    void stopped();
    void playing();
    void seekableChanged();
    void videoCodecPriorityChanged();
    void videoCodecOptionsChanged();
    void channelLayoutChanged();

    void errorChanged();
    void error(Error error, const QString &errorString);
    void statusChanged();
    void mediaObjectChanged();

private Q_SLOTS:
    // connect to signals from player
    void _q_error(const QtAV::AVError& e);
    void _q_statusChanged();
    void _q_started();
    void _q_stopped();
    void _q_paused(bool);

private Q_SLOTS:
    void applyVolume();
    void applyChannelLayout();

private:
    Q_DISABLE_COPY(QmlAVPlayer)

    bool m_complete;
    bool m_mute;
    bool mAutoPlay;
    bool mAutoLoad;
    bool mHasAudio, mHasVideo;
    int mLoopCount;
    qreal mPlaybackRate;
    qreal mVolume;
    PlaybackState mPlaybackState;
    Error mError;
    QString mErrorString;
    QtAV::MediaStatus m_status;
    QtAV::AVPlayer *mpPlayer;
    QUrl mSource;
    QStringList mVideoCodecs;
    ChannelLayout mChannelLayout;

    QScopedPointer<MediaMetaData> m_metaData;
    QVariantMap vcodec_opt;
};

#endif // QTAV_QML_AVPLAYER_H
