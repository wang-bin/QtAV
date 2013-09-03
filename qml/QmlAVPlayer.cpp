#include "QmlAVPlayer.h"
#include <QtAV/AVPlayer.h>

QmlAVPlayer::QmlAVPlayer(QObject *parent) :
    QObject(parent)
  , mpPlayer(0)
  , mpVideoOut(0)
{
    mpPlayer = new AVPlayer(this);
    mpPlayer->setPlayerEventFilter(0);
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


void QmlAVPlayer::play()
{
    if (!mpVideoOut) {
        qWarning("No video output!");
        return;
    }
    mpPlayer->play();
}
