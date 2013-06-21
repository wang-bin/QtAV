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

#ifndef QAV_PACKET_H
#define QAV_PACKET_H

#include <queue>
#include <QtCore/QByteArray>
#include <QtCore/QQueue>
#include <QtCore/QMutex>
#include <QtAV/BlockingQueue.h>
//#include <QtAV/BlockingRing.h>
#include <QtAV/QtAV_Global.h>

namespace QtAV {
class Q_EXPORT Packet
{
public:
    Packet();
    inline bool isValid() const;
    inline bool isEnd() const;
    void markEnd();

    QByteArray data;
    qreal pts, duration;
private:
    static const qreal kEndPts;
};

bool Packet::isValid() const
{
    return !data.isNull() && pts >= 0 && duration >= 0; //!data.isEmpty()?
}

bool Packet::isEnd() const
{
    return pts == kEndPts;
}

template <typename T> class StdQueue : public std::queue<T>
{
public:
	bool isEmpty() const { return this->empty(); }
	void clear() { while (!this->empty()) { this->pop(); } }
	T dequeue() { this->pop(); return this->front(); }
	void enqueue(const T& t) { this->push(t); }
};
typedef BlockingQueue<Packet, QQueue> PacketQueue;
//typedef BlockingQueue<Packet, StdQueue> PacketQueue;
//typedef BlockingRing<Packet> PacketQueue;
} //namespace QtAV

#endif // QAV_PACKET_H
