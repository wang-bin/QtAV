/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2013 Wang Bin <wbsecg1@gmail.com>

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
    QByteArray data;
    qreal pts, duration;
};

bool Packet::isValid() const
{
    return !data.isNull() && pts >= 0 & duration >= 0; //!data.isEmpty()?
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
