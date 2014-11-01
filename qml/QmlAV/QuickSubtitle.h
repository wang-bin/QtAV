/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2014 Wang Bin <wbsecg1@gmail.com>
    theoribeiro <theo@fictix.com.br>

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

#ifndef QTAV_QML_QUICKSUBTITLE_H
#define QTAV_QML_QUICKSUBTITLE_H

#include <QmlAV/Export.h>
#include <QtAV/Subtitle.h>
#include <QtCore/QMutexLocker>

class QMLAV_EXPORT QuickSubtitleObserver {
public:
    virtual void update(const QImage& image, const QRect& r, int width, int height) = 0;
};

namespace QtAV {
class PlayerSubtitle;
}
class QmlAVPlayer;
/*!
 * \brief The QuickSubtitle class
 * high level Subtitle processor for QML. No rendering.
 */
class QMLAV_EXPORT QuickSubtitle : public QObject, public QtAV::SubtitleAPIProxy
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enableChanged)
    Q_PROPERTY(QObject* player READ player WRITE setPlayer)
    // proxy api
    Q_PROPERTY(QByteArray codec READ codec WRITE setCodec NOTIFY codecChanged)
    Q_PROPERTY(QStringList engines READ engines WRITE setEngines NOTIFY enginesChanged)
    Q_PROPERTY(QString engine READ engine NOTIFY engineChanged)
    Q_PROPERTY(bool fuzzyMatch READ fuzzyMatch WRITE setFuzzyMatch NOTIFY fuzzyMatchChanged)
    //Q_PROPERTY(QString fileName READ fileName WRITE setFileName NOTIFY fileNameChanged)
    Q_PROPERTY(QStringList dirs READ dirs WRITE setDirs NOTIFY dirsChanged)
    Q_PROPERTY(QStringList suffixes READ suffixes WRITE setSuffixes NOTIFY suffixesChanged)
    Q_PROPERTY(QStringList supportedSuffixes READ supportedSuffixes NOTIFY supportedSuffixesChanged)
    Q_PROPERTY(bool canRender READ canRender NOTIFY canRenderChanged)
    //PlayerSubtitle api
    Q_PROPERTY(bool autoLoad READ autoLoad WRITE setAutoLoad NOTIFY autoLoadChanged)
    Q_PROPERTY(QString file READ file WRITE setFile)
    //
    Q_PROPERTY(QString text READ getText)
public:
    explicit QuickSubtitle(QObject *parent = 0);
    Q_INVOKABLE QString getText() const;
    // observer is only for ass image subtitle
    void addObserver(QuickSubtitleObserver* ob);
    void removeObserver(QuickSubtitleObserver* ob);
    // 0: notify all
    void notifyObservers(const QImage& image, const QRect& r, int width, int height, QuickSubtitleObserver* ob = 0);
    /*!
     * \brief setPlayer
     * if player is set, subtitle will automatically loaded if playing file changed.
     * \param player
     */
    void setPlayer(QObject* player);
    QObject* player();

    // TODO: enableRenderImage + enabled
    void setEnabled(bool value = true); //AVComponent.enabled
    bool isEnabled() const;
    /*!
     * \brief setFile
     * load user selected subtitle. autoLoad must be false.
     * if replay the same video, subtitle does not change
     * if play a new video, you have to set subtitle again
     */
    void setFile(const QString& file);
    QString file() const;
    /*!
     * \brief autoLoad
     * auto find a suitable subtitle.
     * if false, load the user selected subtile in setFile() (empty if start a new video)
     * \return
     */
    bool autoLoad() const;
public Q_SLOTS:
    // TODO: enable changed & autoload=> load
    void setAutoLoad(bool value);
Q_SIGNALS:
    void canRenderChanged();
    void loaded(const QString& path);
    void enableChanged(bool);
    void autoLoadChanged(bool value);

    void codecChanged();
    void enginesChanged();
    void fuzzyMatchChanged();
    void contentChanged();
    //void fileNameChanged();
    void dirsChanged();
    void suffixesChanged();
    void supportedSuffixesChanged();
    void engineChanged();

private:
    bool m_enable;
    QmlAVPlayer *m_player;
    QtAV::PlayerSubtitle *m_player_sub;

    class Filter;
    Filter *m_filter;
    QMutex m_mutex;
    QList<QuickSubtitleObserver*> m_observers;
};

#endif // QTAV_QML_QUICKSUBTITLE_H
