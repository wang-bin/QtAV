/******************************************************************************
    QtAV Player Demo:  this file is part of QtAV examples
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/


#include "PlayListItem.h"
#include <QtCore/QTime>
#include <QtCore/QDataStream>

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
