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

#include "QmlAV/QuickSubtitle.h"
#include "QmlAV/QmlAVPlayer.h"
#include <QtAV/private/PlayerSubtitle.h>

using namespace QtAV;

QuickSubtitle::QuickSubtitle(QObject *parent) :
    QObject(parent)
  , SubtitleAPIProxy(this)
  , m_enable(true)
  , m_player(0)
  , m_player_sub(new PlayerSubtitle(this))
{
    QmlAVPlayer *p = qobject_cast<QmlAVPlayer*>(parent);
    if (p)
        setPlayer(p);

    setSubtitle(m_player_sub->subtitle()); //for proxy
    connect(this, SIGNAL(enableChanged(bool)), m_player_sub, SLOT(onEnableChanged(bool))); //////
    connect(this, SIGNAL(codecChanged()), m_player_sub->subtitle(), SLOT(loadAsync()));
    connect(m_player_sub, SIGNAL(autoLoadChanged(bool)), this, SIGNAL(autoLoadChanged(bool)));
}

QString QuickSubtitle::getText() const
{
    return m_player_sub->subtitle()->getText();
}

void QuickSubtitle::setPlayer(QObject *player)
{
    QmlAVPlayer *p = qobject_cast<QmlAVPlayer*>(player);
    if (m_player == p)
        return;
    m_player = p;
    // TODO: check AVPlayer null?
    m_player_sub->setPlayer(p->player());
}

QObject* QuickSubtitle::player()
{
    return m_player;
}

void QuickSubtitle::setEnabled(bool value)
{
    if (m_enable == value)
        return;
    m_enable = value;
    emit enableChanged(value);
}

bool QuickSubtitle::isEnabled() const
{
    return m_enable;
}

void QuickSubtitle::setFile(const QString &file)
{
    m_player_sub->setFile(file);
}

QString QuickSubtitle::file() const
{
    return m_player_sub->file();
}

void QuickSubtitle::setAutoLoad(bool value)
{
    m_player_sub->setAutoLoad(value);
}

bool QuickSubtitle::autoLoad() const
{
    return m_player_sub->autoLoad();
}

