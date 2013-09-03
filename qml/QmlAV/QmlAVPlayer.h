#ifndef QTAV_QMLAVPLAYER_H
#define QTAV_QMLAVPLAYER_H

#include <QObject>
#include <QtQml/qqmlparserstatus.h>
#include <QmlAV/QQuickItemRenderer.h>

namespace QtAV {
class AVPlayer;
}
using namespace QtAV;
class QmlAVPlayer : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QQuickItemRenderer *videoOut READ videoOut WRITE setVideoOut NOTIFY videoOutChanged)
    Q_INTERFACES(QQmlParserStatus)
public:
    explicit QmlAVPlayer(QObject *parent = 0);

    void classBegin();
    void componentComplete();

    QUrl source() const;
    void setSource(const QUrl& url);
    QQuickItemRenderer* videoOut();
    void setVideoOut(QQuickItemRenderer* out);
signals:
    void sourceChanged();
    void videoOutChanged();

public slots:
    void play();

private:
    Q_DISABLE_COPY(QmlAVPlayer)
    QtAV::AVPlayer *mpPlayer;
    QQuickItemRenderer *mpVideoOut;
    QUrl mSource;
};

#endif // QTAV_QMLAVPLAYER_H
