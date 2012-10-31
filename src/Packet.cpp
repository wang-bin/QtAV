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

#include <QtAV/Packet.h>

namespace QtAV {

Packet::Packet()
    :pts(0),duration(0)
{
}

#if THREAD_SAFE_QUEUE
PacketQueue::PacketQueue()
    :mutex(QMutex::Recursive)
{
}

PacketQueue::~PacketQueue()
{
    queue.clear();
}

void PacketQueue::enqueue(const Packet& packet)
{
    QMutexLocker lock(&mutex);
    Q_UNUSED(lock);
    queue.enqueue(packet);
}

Packet PacketQueue::dequeue()
{
    QMutexLocker lock(&mutex);
    Q_UNUSED(lock);
    return queue.dequeue();
}

Packet PacketQueue::head()
{
    QMutexLocker lock(&mutex);
    Q_UNUSED(lock);
    return queue.head();
}

Packet PacketQueue::tail()
{
    QMutexLocker lock(&mutex);
    Q_UNUSED(lock);
    return queue.last();
}
#endif //THREAD_SAFE_QUEUE

} //namespace QtAV
