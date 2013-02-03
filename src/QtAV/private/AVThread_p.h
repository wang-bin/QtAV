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
#include <QtAV/AVDecoder.h>
#include <QtAV/AVOutput.h>
#include <QtAV/QtAV_Global.h>

namespace QtAV {

class AVDecoder;
class Packet;
class AVClock;
class Q_EXPORT AVThreadPrivate : public DPtrPrivate<AVThread>
{
public:
    AVThreadPrivate():paused(false),demux_end(false),stop(false),clock(0)
      ,dec(0),writer(0) {
    }
    //DO NOT delete dec and writer. We do not own them
    virtual ~AVThreadPrivate() {}

    bool paused;
    bool demux_end;
    volatile bool stop; //true when packets is empty and demux is end.
    AVClock *clock;
    PacketQueue packets;
    AVDecoder *dec;
    AVOutput *writer;
    QMutex mutex;
    QWaitCondition cond; //pause
};

} //namespace QtAV
#endif // QTAV_AVTHREAD_P_H
