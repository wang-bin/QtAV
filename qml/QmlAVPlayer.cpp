#include "QmlAVPlayer.h"
#include <QtAV/AVPlayer.h>

QmlAVPlayer::QmlAVPlayer(QObject *parent) :
    QObject(parent)
  , mpPlayer(0)
  , mpVideoOut(0)
{
}

void QmlAVPlayer::classBegin()
{
    mpPlayer = new AVPlayer(this);
}

void QmlAVPlayer::componentComplete()
{
    mpPlayer->stop();
}

QUrl QmlAVPlayer::source() const
{
    return mSource;
}

void QmlAVPlayer::setSource(const QUrl &url)
{
    mSource = url;
}

QQuickItemRenderer* QmlAVPlayer::videoOut()
{
    return mpVideoOut;
}

void QmlAVPlayer::setVideoOut(QQuickItemRenderer *out)
{
    mpVideoOut = out;
}


void QmlAVPlayer::play()
{
    if (!mpVideoOut) {
        qWarning("No video output!");
        return;
    }
    mpPlayer->setRenderer(mpVideoOut);
    mpPlayer->play(mSource.toString());
}
