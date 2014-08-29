#include "QmlAV/QuickSubtitle.h"
#include "QmlAV/QmlAVPlayer.h"
#include <QtCore/QFileInfo>
#include <QtCore/QDir>

using namespace QtAV;

QuickSubtitle::QuickSubtitle(QObject *parent) :
    Subtitle(parent)
  , m_player(0)
{
    QmlAVPlayer *p = qobject_cast<QmlAVPlayer*>(parent);
    if (p)
        setPlayer(p);
}

void QuickSubtitle::setPlayer(QObject *player)
{
    QmlAVPlayer *p = qobject_cast<QmlAVPlayer*>(player);
    if (m_player == p)
        return;
    if (m_player) {
        disconnect(m_player, SIGNAL(sourceChanged()), this, SLOT(onPlayerSourceChanged()));
    }
    m_player = p;
    connect(m_player, SIGNAL(sourceChanged()), SLOT(onPlayerSourceChanged()));
    connect(m_player, SIGNAL(positionChanged()), SLOT(onPlayerPositionChanged()));
}

QObject* QuickSubtitle::player()
{
    return m_player;
}

void QuickSubtitle::onPlayerSourceChanged()
{
    QmlAVPlayer *p = qobject_cast<QmlAVPlayer*>(sender());
    if (!p)
        return;
    QString path = p->source().toString();
    //qDebug() << "path :" << path << " scheme: " << p->source().scheme();
    path.remove(p->source().scheme() + "://");
    QString name = QFileInfo(path).completeBaseName();
    path = QFileInfo(path).dir().absoluteFilePath(name);
    qDebug() << "open subtitle:" << path;
    setFileName(path);
    load();
}

void QuickSubtitle::onPlayerPositionChanged()
{
    QmlAVPlayer *p = qobject_cast<QmlAVPlayer*>(sender());
    if (!p)
        return;
    setTimestamp(qreal(p->position())/1000.0);
}
