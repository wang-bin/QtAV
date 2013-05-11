/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2013 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_AVTHREAD_P_H
#define QTAV_AVTHREAD_P_H

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QWaitCondition>

#include <QtAV/Packet.h>
#include <QtAV/QtAV_Global.h>

namespace QtAV {

const double kSyncThreshold = 0.005; // 5 ms

class AVDecoder;
class AVOutput;
class AVClock;
class Filter;
class FilterContext;
class Statistics;
class Q_EXPORT AVThreadPrivate : public DPtrPrivate<AVThread>
{
public:
    AVThreadPrivate():
        paused(false)
      , next_pause(false)
      , demux_end(false)
      , stop(false)
      , clock(0)
      , dec(0)
      , writer(0)
      , delay(0)
      , filter_context(0)
      , statistics(0)
    {
    }
    //DO NOT delete dec and writer. We do not own them
    virtual ~AVThreadPrivate();

    bool paused, next_pause;
    bool demux_end;
    volatile bool stop; //true when packets is empty and demux is end.
    AVClock *clock;
    PacketQueue packets;
    AVDecoder *dec;
    AVOutput *writer;
    QMutex mutex;
    QWaitCondition cond; //pause
    qreal delay;
    FilterContext *filter_context;//TODO: use own smart ptr. QSharedPointer "=" is ugly
    QList<Filter*> filters;
    Statistics *statistics; //not obj. Statistics is unique for the player, which is in AVPlayer
};

} //namespace QtAV
#endif // QTAV_AVTHREAD_P_H
