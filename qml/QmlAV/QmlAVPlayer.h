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
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QObject* videoOut READ videoOut WRITE setVideoOut NOTIFY videoOutChanged)
public:
    explicit QmlAVPlayer(QObject *parent = 0);
    QUrl source() const;
    void setSource(const QUrl& url);
    QObject* videoOut();
    void setVideoOut(QObject* out);
    Q_INVOKABLE void play();
signals:
    void sourceChanged();
    void videoOutChanged();

private:
    Q_DISABLE_COPY(QmlAVPlayer)
    QtAV::AVPlayer *mpPlayer;
    QQuickItemRenderer *mpVideoOut;
    QUrl mSource;
};

#endif // QTAV_QMLAVPLAYER_H
