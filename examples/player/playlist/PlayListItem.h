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


#ifndef PLAYLISTITEM_H
#define PLAYLISTITEM_H

#include <QtCore/QString>
#include <QtCore/QVariant>

class PlayListItem
{
public:
    PlayListItem();
    void setTitle(const QString& title);
    QString title() const;
    void setUrl(const QString& url);
    QString url() const;
    void setStars(int s);
    int stars() const;
    void setLastTime(qint64 ms);
    qint64 lastTime() const;
    QString lastTimeString() const;
    void setDuration(qint64 ms);
    qint64 duration() const;
    QString durationString() const;
    //icon
private:
    QString mTitle;
    QString mUrl;
    int mStars;
    qint64 mLastTime, mDuration;
    QString mLastTimeS, mDurationS;
};

Q_DECLARE_METATYPE(PlayListItem);

QDataStream& operator>> (QDataStream& s, PlayListItem& p);
QDataStream& operator<< (QDataStream& s, const PlayListItem& p);

#endif // PLAYLISTITEM_H
