#ifndef QTAV_QMLAVPLAYER_H
#define QTAV_QMLAVPLAYER_H

#include <QmlAV/Export.h>
#include <QtCore/QObject>
#include <QmlAV/QQuickItemRenderer.h>

namespace QtAV {
class AVPlayer;
}
using namespace QtAV;
class QMLAV_EXPORT QmlAVPlayer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool mute READ mute WRITE setMute NOTIFY muteChanged)
    Q_PROPERTY(PlaybackState playbackState READ playbackState NOTIFY playbackStateChanged)
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QObject* videoOut READ videoOut WRITE setVideoOut NOTIFY videoOutChanged)
    Q_ENUMS(PlaybackState)
public:
    enum PlaybackState {
        PlayingState,
        PausedState,
        StoppedState
    };

    explicit QmlAVPlayer(QObject *parent = 0);
    QUrl source() const;
    void setSource(const QUrl& url);
    QObject* videoOut();
    void setVideoOut(QObject* out);
    bool mute() const;
    void setMute(bool m);
    PlaybackState playbackState() const;
    void setPlaybackState(PlaybackState playbackState);
    Q_INVOKABLE void play(const QUrl& url);

public Q_SLOTS:
    void play();
    void pause();
    void resume();
    void togglePause();
    void stop();
    void nextFrame();
    void seek(qreal position);
    void seekForward();
    void seekBackward();

Q_SIGNALS:
    void muteChanged();
    void sourceChanged();
    void videoOutChanged();
    void playbackStateChanged();
    void paused();
    void stopped();
    void playing();
private Q_SLOTS:
    void _q_started();
    void _q_stopped();
    void _q_paused(bool);

private:
    Q_DISABLE_COPY(QmlAVPlayer)
    PlaybackState mPlaybackState;
    QtAV::AVPlayer *mpPlayer;
    QQuickItemRenderer *mpVideoOut;
    QUrl mSource;
};

#endif // QTAV_QMLAVPLAYER_H
