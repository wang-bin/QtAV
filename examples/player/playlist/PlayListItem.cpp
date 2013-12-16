/******************************************************************************
    PlayListItem.cpp: description
    Copyright (C) 2013 Wang Bin <wbsecg1@gmail.com>

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

    Alternatively, this file may be used under the terms of the GNU
    General Public License version 3.0 as published by the Free Software
    Foundation and appearing in the file LICENSE.GPL included in the
    packaging of this file.  Please review the following information to
    ensure the GNU General Public License version 3.0 requirements will be
    met: http://www.gnu.org/copyleft/gpl.html.
******************************************************************************/


#include "PlayListItem.h"
#include <QtCore/QTime>

QDataStream& operator>> (QDataStream& s, PlayListItem& p)
{
    int stars;
    qint64 duration, last_time;
    QString url, title;
    s >> url >> title >> duration >> last_time >> stars;
    p.setTitle(title);
    p.setUrl(url);
    p.setStars(stars);
    p.setDuration(duration);
    p.setLastTime(last_time);
    return s;
}

QDataStream& operator<< (QDataStream& s, const PlayListItem& p)
{
    s << p.url() << p.title() << p.duration() << p.lastTime() << p.stars();
    return s;
}

PlayListItem::PlayListItem()
    : mStars(0)
    , mLastTime(0)
    , mDuration(0)
{
}

void PlayListItem::setTitle(const QString &title)
{
    mTitle = title;
}

QString PlayListItem::title() const
{
    return mTitle;
}

void PlayListItem::setUrl(const QString &url)
{
    mUrl = url;
}

QString PlayListItem::url() const
{
    return mUrl;
}

void PlayListItem::setStars(int s)
{
    mStars = s;
}

int PlayListItem::stars() const
{
    return mStars;
}

void PlayListItem::setLastTime(qint64 ms)
{
    mLastTime = ms;
    mLastTimeS = QTime(0, 0, 0, 0).addMSecs(ms).toString("HH:mm:ss");
}

qint64 PlayListItem::lastTime() const
{
    return mLastTime;
}

QString PlayListItem::lastTimeString() const
{
    return mLastTimeS;
}

void PlayListItem::setDuration(qint64 ms)
{
    mDuration = ms;
    mDurationS = QTime(0, 0, 0, 0).addMSecs(ms).toString("HH:mm:ss");
}

qint64 PlayListItem::duration() const
{
    return mDuration;
}

QString PlayListItem::durationString() const
{
    return mDurationS;
}
