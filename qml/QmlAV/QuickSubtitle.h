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
    /*!
     * \brief setPlayer
     * if player is set, subtitle will automatically loaded if playing file changed.
     * \param player
     */
    void setPlayer(QObject* player);
    QObject* player();
private slots:
    void onPlayerSourceChanged();
    void onPlayerPositionChanged();
private:
    QmlAVPlayer *m_player;
};

#endif // QTAV_QML_QUICKSUBTITLE_H
