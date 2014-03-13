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

class QRunnable;
namespace QtAV {

const double kSyncThreshold = 0.2; // 200 ms

class AVDecoder;
class AVOutput;
class AVClock;
class Filter;
class FilterContext;
class Statistics;
class OutputSet;
class Q_AV_EXPORT AVThreadPrivate : public DPtrPrivate<AVThread>
{
public:
    AVThreadPrivate():
        paused(false)
      , next_pause(false)
      , demux_end(false)
      , stop(false)
      , clock(0)
      , dec(0)
      , outputSet(0)
      , delay(0)
      , filter_context(0)
      , statistics(0)
      , ready(false)
      , render_pts0(0)
    {
    }
    virtual ~AVThreadPrivate();

    bool paused, next_pause;
    bool demux_end;
    volatile bool stop; //true when packets is empty and demux is end.
    AVClock *clock;
    PacketQueue packets;
    AVDecoder *dec;
    OutputSet *outputSet;
    QMutex mutex;
    QWaitCondition cond; //pause
    qreal delay;
    FilterContext *filter_context;//TODO: use own smart ptr. QSharedPointer "=" is ugly
    QList<Filter*> filters;
    Statistics *statistics; //not obj. Statistics is unique for the player, which is in AVPlayer
    QList<QRunnable*> tasks;
    QWaitCondition ready_cond;
    QMutex ready_mutex;
    bool ready;
    //only decode video without display or skip decode audio until pts reaches
    qreal render_pts0;
};

} //namespace QtAV
#endif // QTAV_AVTHREAD_P_H
