/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2014)

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

#include "QtAV/private/PlayerSubtitle.h"
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include "QtAV/AVPlayer.h"
#include "QtAV/Subtitle.h"
#include "utils/internal.h"
#include "utils/Logger.h"

namespace QtAV {

// /xx/oo/a.01.mov => /xx/oo/a.01. native dir separator => /
/*!
 * \brief getSubtitleBasePath
 * \param fullPath path of video or without extension
 * \return absolute path without extension and file://
 */
static QString getSubtitleBasePath(const QString fullPath)
{
    QString path(QDir::fromNativeSeparators(fullPath));
    //path.remove(p->source().scheme() + "://");
    // QString name = QFileInfo(path).completeBaseName();
    // why QFileInfo(path).dir() starts with qml app dir?
    QString name(path);
    int lastSep = path.lastIndexOf(QLatin1Char('/'));
    if (lastSep >= 0) {
        name = name.mid(lastSep + 1);
        path = path.left(lastSep + 1); // endsWidth "/"
    }
    int lastDot = name.lastIndexOf(QLatin1Char('.')); // not path.lastIndexof("."): xxx.oo/xx
    if (lastDot > 0)
        name = name.left(lastDot);
    if (path.startsWith(QLatin1String("file:"))) // can skip convertion here. Subtitle class also convert the path
        path = Internal::Path::toLocal(path);
    path.append(name);
    return path;
}

PlayerSubtitle::PlayerSubtitle(QObject *parent)
    : QObject(parent)
    , m_auto(true)
    , m_enabled(true)
    , m_player(0)
    , m_sub(new Subtitle(this))
{
}

Subtitle* PlayerSubtitle::subtitle()
{
    return m_sub;
}

void PlayerSubtitle::setPlayer(AVPlayer *player)
{
    if (m_player == player)
        return;
    if (m_player) {
        disconnectSignals();
    }
    m_player = player;
    if (!m_player)
        return;
    connectSignals();
}

void PlayerSubtitle::setFile(const QString &file)
{
    if (m_file != file)
        Q_EMIT fileChanged();
    // always load
    // file was set but now playing with fuzzy match. if file is set again to the same value, subtitle must load that file
    m_file = file;
    if (!m_enabled)
        return;
    m_sub->setFileName(file);
    m_sub->setFuzzyMatch(false);
    m_sub->loadAsync();
}

QString PlayerSubtitle::file() const
{
    return m_file;
}
void PlayerSubtitle::setAutoLoad(bool value)
{
    if (m_auto == value)
        return;
    m_auto = value;
    Q_EMIT autoLoadChanged(value);
}

bool PlayerSubtitle::autoLoad() const
{
    return m_auto;
}

void PlayerSubtitle::onPlayerSourceChanged()
{
    if (!m_auto) {
        m_sub->setFileName(QString());
        return;
    }
    if (!m_enabled)
        return;
    AVPlayer *p = qobject_cast<AVPlayer*>(sender());
    if (!p)
        return;
    m_sub->setFileName(getSubtitleBasePath(p->file()));
    m_sub->setFuzzyMatch(true);
    m_sub->loadAsync();
}

void PlayerSubtitle::onPlayerPositionChanged()
{
    AVPlayer *p = qobject_cast<AVPlayer*>(sender());
    if (!p)
        return;
    m_sub->setTimestamp(qreal(p->position())/1000.0);
}

void PlayerSubtitle::onPlayerStart()
{
    if (!m_enabled)
        return;
    // priority: user file > auto load file > embedded
    if (!m_file.isEmpty()) {
        if (m_file == m_sub->fileName())
            return;
        m_sub->setFileName(m_file);
        m_sub->setFuzzyMatch(false);
        m_sub->loadAsync();
        return;
    }
    if (autoLoad() && !m_sub->fileName().isEmpty())
        return; //already loaded in onPlayerSourceChanged()
    // try embedded subtitles
    const int n = m_player->currentSubtitleStream();
    if (n < 0 || m_tracks.isEmpty() || m_tracks.size() <= n) {
        m_sub->processHeader(QByteArray(), QByteArray()); // reset
        return;
    }
    QVariantMap track = m_tracks[n].toMap();
    QByteArray codec(track.value(QStringLiteral("codec")).toByteArray());
    QByteArray data(track.value(QStringLiteral("extra")).toByteArray());
    m_sub->processHeader(codec, data);
    return;
}

void PlayerSubtitle::onEnabledChanged(bool value)
{
    m_enabled = value;
    if (!m_enabled) {
        disconnectSignals();
        return;
    }
    connectSignals();
    // priority: user file > auto load file > embedded
    if (!m_file.isEmpty()) {
        if (m_sub->fileName() == m_file && m_sub->isLoaded())
            return;
        m_sub->setFileName(m_file);
        m_sub->setFuzzyMatch(false);
        m_sub->loadAsync();
    }
    if (!m_player)
        return;
    if (!autoLoad()) // fallback to internal subtitles
        return;
    m_sub->setFileName(getSubtitleBasePath(m_player->file()));
    m_sub->setFuzzyMatch(true);
    m_sub->loadAsync();
    return;
}

void PlayerSubtitle::tryReload()
{
    tryReload(3);
}

void PlayerSubtitle::tryReloadInternalSub()
{
    tryReload(1);
}

void PlayerSubtitle::tryReload(int flag)
{
    if (!m_enabled)
        return;
    if (!m_player->isPlaying())
        return;
    const int kReloadExternal = 1<<1;
    if (flag & kReloadExternal) {
        //engine or charset changed
        m_sub->processHeader(QByteArray(), QByteArray()); // reset
        m_sub->loadAsync();
        return;
    }
    // kReloadExternal is not set, kReloadInternal is unknown
    //fallback to external sub if no valid internal sub track is set
    const int n = m_player->currentSubtitleStream();
    if (n < 0 || m_tracks.isEmpty() || m_tracks.size() <= n) {
        m_sub->processHeader(QByteArray(), QByteArray()); // reset, null processor
        m_sub->loadAsync();
        return;
    }
    // try internal subtitles
    QVariantMap track = m_tracks[n].toMap();
    QByteArray codec(track.value(QStringLiteral("codec")).toByteArray());
    QByteArray data(track.value(QStringLiteral("extra")).toByteArray());
    m_sub->processHeader(codec, data);
    Packet pkt(m_current_pkt[n]);
    if (pkt.isValid()) {
        processInternalSubtitlePacket(n, pkt);
    }
}

void  PlayerSubtitle::updateInternalSubtitleTracks(const QVariantList &tracks)
{
    m_tracks = tracks;
    m_current_pkt.resize(tracks.size());
}

void PlayerSubtitle::processInternalSubtitlePacket(int track, const QtAV::Packet &packet)
{
    m_sub->processLine(packet.data, packet.pts, packet.duration);
    m_current_pkt[track] = packet;
}

void PlayerSubtitle::processInternalSubtitleHeader(const QByteArray& codec, const QByteArray &data)
{
    m_sub->processHeader(codec, data);
}

void PlayerSubtitle::connectSignals()
{
    if (!m_player)
        return;
    connect(m_player, SIGNAL(sourceChanged()), this, SLOT(onPlayerSourceChanged()));
    connect(m_player, SIGNAL(positionChanged(qint64)), this, SLOT(onPlayerPositionChanged()));
    connect(m_player, SIGNAL(started()), this, SLOT(onPlayerStart()));
    connect(m_player, SIGNAL(internalSubtitlePacketRead(int,QtAV::Packet)), this, SLOT(processInternalSubtitlePacket(int,QtAV::Packet)));
    connect(m_player, SIGNAL(internalSubtitleHeaderRead(QByteArray,QByteArray)), this, SLOT(processInternalSubtitleHeader(QByteArray,QByteArray)));
    connect(m_player, SIGNAL(internalSubtitleTracksChanged(QVariantList)), this, SLOT(updateInternalSubtitleTracks(QVariantList)));
    // try to reload internal subtitle track. if failed and external subtitle is enabled, fallback to external
    connect(m_player, SIGNAL(subtitleStreamChanged(int)), this, SLOT(tryReloadInternalSub()));
    connect(m_sub, SIGNAL(codecChanged()), this, SLOT(tryReload()));
    connect(m_sub, SIGNAL(enginesChanged()), this, SLOT(tryReload()));
}

void PlayerSubtitle::disconnectSignals()
{
    if (!m_player)
        return;
    disconnect(m_player, SIGNAL(sourceChanged()), this, SLOT(onPlayerSourceChanged()));
    disconnect(m_player, SIGNAL(positionChanged(qint64)), this, SLOT(onPlayerPositionChanged()));
    disconnect(m_player, SIGNAL(started()), this, SLOT(onPlayerStart()));
    disconnect(m_player, SIGNAL(internalSubtitlePacketRead(int,QtAV::Packet)), this, SLOT(processInternalSubtitlePacket(int,QtAV::Packet)));
    disconnect(m_player, SIGNAL(internalSubtitleHeaderRead(QByteArray,QByteArray)), this, SLOT(processInternalSubtitleHeader(QByteArray,QByteArray)));
    disconnect(m_player, SIGNAL(internalSubtitleTracksChanged(QVariantList)), this, SLOT(updateInternalSubtitleTracks(QVariantList)));
    disconnect(m_sub, SIGNAL(codecChanged()), this, SLOT(tryReload()));
    disconnect(m_sub, SIGNAL(enginesChanged()), this, SLOT(tryReload()));
}

} //namespace QtAV
