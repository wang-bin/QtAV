/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012 Wang Bin <wbsecg1@gmail.com>

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
