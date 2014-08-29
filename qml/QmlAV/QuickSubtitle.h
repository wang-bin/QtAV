#ifndef QTAV_QML_QUICKSUBTITLE_H
#define QTAV_QML_QUICKSUBTITLE_H

#include <QtAV/Subtitle.h>

class QmlAVPlayer;
class QuickSubtitle : public QtAV::Subtitle
{
    Q_OBJECT
    Q_PROPERTY(QObject* player READ player WRITE setPlayer)
public:
    explicit QuickSubtitle(QObject *parent = 0);
    void setPlayer(QObject* player);
    QObject* player();
private slots:
    void onPlayerSourceChanged();
    void onPlayerPositionChanged();
private:
    QmlAVPlayer *m_player;
};

#endif // QTAV_QML_QUICKSUBTITLE_H
