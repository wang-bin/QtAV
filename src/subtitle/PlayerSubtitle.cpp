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

#include "QtAV/private/PlayerSubtitle.h"
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include "QtAV/AVPlayer.h"
#include "QtAV/Subtitle.h"
#include "utils/Logger.h"

namespace QtAV {

extern QString getLocalPath(const QString& fullPath);

// /xx/oo/a.01.mov => /xx/oo/a.01.
/*!
 * \brief getSubtitleBasePath
 * \param fullPath path of video or without extension
 * \return absolute path without extension and file://
 */
static QString getSubtitleBasePath(const QString fullPath)
{
    QString path(fullPath);
    //path.remove(p->source().scheme() + "://");
    // QString name = QFileInfo(path).completeBaseName();
    // why QFileInfo(path).dir() starts with qml app dir?
    QString name(path);
    int lastSep = path.lastIndexOf(QChar('/'));
    if (lastSep >= 0) {
        name = name.mid(lastSep + 1);
        path = path.left(lastSep + 1); // endsWidth "/"
    }
    int lastDot = name.lastIndexOf(QChar('.')); // not path.lastIndexof("."): xxx.oo/xx
    if (lastDot > 0)
        name = name.left(lastDot);
    if (path.startsWith("file:")) // can skip convertion here. Subtitle class also convert the path
        path = getLocalPath(path);
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
    // always load
    m_file = file;
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
    emit autoLoadChanged(value);
}

bool PlayerSubtitle::autoLoad() const
{
    return m_auto;
}

void PlayerSubtitle::onPlayerSourceChanged()
{
    m_file = QString();
    if (!m_auto) {
        return;
    }
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
    if (!autoLoad()) {
        if (m_file == m_sub->fileName())
            return;
        m_sub->setFileName(m_file);
        m_sub->setFuzzyMatch(false);
        m_sub->loadAsync();
        return;
    }
    if (m_file != m_sub->fileName())
        return;
    if (!m_player)
        return;
    // autoLoad was false then reload then true then reload
    // previous loaded is user selected subtitle
    m_sub->setFileName(getSubtitleBasePath(m_player->file()));
    m_sub->setFuzzyMatch(true);
    m_sub->loadAsync();
    return;
}

void PlayerSubtitle::onEnableChanged(bool value)
{
    m_enabled = value;
    if (value) {
        if (m_player) {
            connectSignals();
        }
        if (autoLoad()) {
            if (!m_player)
                return;
            m_sub->setFileName(getSubtitleBasePath(m_player->file()));
            m_sub->setFuzzyMatch(true);
            m_sub->loadAsync();
        } else {
            m_sub->setFileName(m_file);
            m_sub->setFuzzyMatch(false);
            m_sub->loadAsync();
        }
    } else {
        if (m_player) {
            disconnectSignals();
        }
    }
}

void PlayerSubtitle::tryReload()
{
    if (!m_enabled)
        return;
    if (!m_player)
        return;
    if (!m_player->isPlaying())
        return;
    m_sub->loadAsync();
}

void PlayerSubtitle::connectSignals()
{
    connect(m_player, SIGNAL(sourceChanged()), this, SLOT(onPlayerSourceChanged()));
    connect(m_player, SIGNAL(positionChanged(qint64)), this, SLOT(onPlayerPositionChanged()));
    connect(m_player, SIGNAL(started()), this, SLOT(onPlayerStart()));
    connect(m_sub, SIGNAL(codecChanged()), this, SLOT(tryReload()));
    connect(m_sub, SIGNAL(enginesChanged()), this, SLOT(tryReload()));
}

void PlayerSubtitle::disconnectSignals()
{
    disconnect(m_player, SIGNAL(sourceChanged()), this, SLOT(onPlayerSourceChanged()));
    disconnect(m_player, SIGNAL(positionChanged(qint64)), this, SLOT(onPlayerPositionChanged()));
    disconnect(m_player, SIGNAL(started()), this, SLOT(onPlayerStart()));
    disconnect(m_sub, SIGNAL(codecChanged()), this, SLOT(tryReload()));
    disconnect(m_sub, SIGNAL(enginesChanged()), this, SLOT(tryReload()));
}

} //namespace QtAV
