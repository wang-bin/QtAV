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

//#include <private/qthread_p.h>
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
class AVThreadPrivate
{
public:
    AVThreadPrivate():stop(false),clock(0),dec(0),writer(0) {}
    virtual ~AVThreadPrivate() {
        /*if (dec) {
            delete dec;
            dec = 0;
        }
        if (writer) {
            delete writer;
            writer = 0;
        }
        if (packets) {
            delete packets;
            packets = 0;
        }*/
    }
    void enqueue(const Packet& pkt) {
        packets.enqueue(pkt);
        condition.wakeAll();
    }

    volatile bool stop;
    AVClock *clock;
    PacketQueue packets;
    AVDecoder *dec;
    AVOutput *writer;
    QMutex mutex;
    QWaitCondition condition;
};

} //namespace QtAV
#endif // QTAV_AVTHREAD_P_H
