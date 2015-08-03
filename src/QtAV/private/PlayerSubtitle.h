/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2014-2015 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_PLAYERSUBTITLE_H
#define QTAV_PLAYERSUBTITLE_H

#include <QtCore/QObject>
#include <QtCore/QVariant>
#include <QtCore/QVector>
#include <QtAV/QtAV_Global.h>
#include <QtAV/Packet.h>

namespace QtAV {

class AVPlayer;
class Subtitle;
/*!
 * \brief The PlayerSubtitle class
 * Bind Subtitle to AVPlayer. Used by SubtitleFilter and QuickSubtitle.
 */
class Q_AV_PRIVATE_EXPORT PlayerSubtitle : public QObject
{
    Q_OBJECT
public:
    PlayerSubtitle(QObject *parent = 0);
    void setPlayer(AVPlayer* player);
    Subtitle* subtitle();
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
    // TODO: enable changed & autoload=> load
    void setAutoLoad(bool value);
    bool autoLoad() const;
Q_SIGNALS:
    void autoLoadChanged(bool value);
    void fileChanged();
public Q_SLOTS:
    void onEnabledChanged(bool value);
private Q_SLOTS:
    void onPlayerSourceChanged();
    void onPlayerPositionChanged();
    void onPlayerStart();
    void tryReload();
    void tryReloadInternalSub();
    void updateInternalSubtitleTracks(const QVariantList& tracks);
    void processInternalSubtitlePacket(int track, const QtAV::Packet& packet);
    void processInternalSubtitleHeader(const QByteArray &codec, const QByteArray& data); //TODO: remove
private:
    void connectSignals();
    void disconnectSignals();
    void tryReload(int flag); //1: internal, 2: external, 3: internal+external
private:
    bool m_auto;
    bool m_enabled; // TODO: m_enable_external
    AVPlayer *m_player;
    Subtitle *m_sub;
    QString m_file;
    QVariantList m_tracks;
    QVector<Packet> m_current_pkt;
};

}
#endif // QTAV_PLAYERSUBTITLE_H
