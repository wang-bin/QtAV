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


#ifndef QTAV_BLOCKINGQUEUE_H
#define QTAV_BLOCKINGQUEUE_H

#include <QtCore/QReadWriteLock>
#include <QtCore/QWaitCondition>

//TODO: block full and empty condition separately
template<typename T> class QQueue;
namespace QtAV {

template <typename T, template <typename> class Container = QQueue>
class BlockingQueue
{
public:
    BlockingQueue();

    void setCapacity(int max); //enqueue is allowed if less than capacity
    void setThreshold(int min); //wake up and enqueue

    void put(const T& t);
    T take();
    void setBlocking(bool block); //will wake if false. called when no more data can enqueue
    void blockEmpty(bool block);
    void blockFull(bool block);
//TODO:setMinBlock,MaxBlock
    inline void clear();
    inline bool isEmpty() const;
    inline int size() const;
    inline int threshold() const;
    inline int capacity() const;

private:
    bool block_empty, block_full;
    int cap, thres; //static?
    Container<T> queue;
    mutable QReadWriteLock lock; //locker in const func
    QWaitCondition cond_full, cond_empty;
};


template <typename T, template <typename> class Container>
BlockingQueue<T, Container>::BlockingQueue()
    :block_empty(true),block_full(true),cap(128*3),thres(128)
{
}

template <typename T, template <typename> class Container>
void BlockingQueue<T, Container>::setCapacity(int max)
{
    QWriteLocker locker(&lock);
    Q_UNUSED(locker);
    cap = max;
}

template <typename T, template <typename> class Container>
void BlockingQueue<T, Container>::setThreshold(int min)
{
    QWriteLocker locker(&lock);
    Q_UNUSED(locker);
    thres = min;
}

template <typename T, template <typename> class Container>
void BlockingQueue<T, Container>::put(const T& t)
{
    QWriteLocker locker(&lock);
    Q_UNUSED(locker);
    if (block_full && queue.size() >= cap)
        cond_full.wait(&lock);
    queue.enqueue(t);
    cond_empty.wakeAll();
}

template <typename T, template <typename> class Container>
T BlockingQueue<T, Container>::take()
{
    QWriteLocker locker(&lock);
    Q_UNUSED(locker);
    if (queue.size() < thres)
        cond_full.wakeAll();
    if (block_empty && queue.isEmpty())//TODO:always block?
        cond_empty.wait(&lock);
    //TODO: Why still empty?
    if (queue.isEmpty()) {
        qWarning("Queue is still empty");
        return T();
    }
    return queue.dequeue();
}

template <typename T, template <typename> class Container>
void BlockingQueue<T, Container>::setBlocking(bool block)
{
    QWriteLocker locker(&lock);
    Q_UNUSED(locker);
    block_empty = block_full = block;
    if (!block) {
        cond_empty.wakeAll(); //empty still wait. setBlock=>setCapacity(-1)
        cond_full.wakeAll();
    }
}

template <typename T, template <typename> class Container>
void BlockingQueue<T, Container>::blockEmpty(bool block)
{
    QWriteLocker locker(&lock);
    Q_UNUSED(locker);
    block_empty = block;
    if (!block) {
        cond_empty.wakeAll();
    }
}

template <typename T, template <typename> class Container>
void BlockingQueue<T, Container>::blockFull(bool block)
{
    QWriteLocker locker(&lock);
    Q_UNUSED(locker);
    block_full = block;
    if (!block) {
        cond_full.wakeAll();
    }
}

template <typename T, template <typename> class Container>
void BlockingQueue<T, Container>::clear()
{
    QWriteLocker locker(&lock);
    //cond_empty.wakeAll();
    cond_full.wakeAll();
    Q_UNUSED(locker);
    queue.clear();
    //TODO: assert not empty
}

template <typename T, template <typename> class Container>
bool BlockingQueue<T, Container>::isEmpty() const
{
    QReadLocker locker(&lock);
    return queue.isEmpty();
}

template <typename T, template <typename> class Container>
int BlockingQueue<T, Container>::size() const
{
    QReadLocker locker(&lock);
    return queue.size();
}

template <typename T, template <typename> class Container>
int BlockingQueue<T, Container>::threshold() const
{
    QReadLocker locker(&lock);
    return thres;
}

template <typename T, template <typename> class Container>
int BlockingQueue<T, Container>::capacity() const
{
    QReadLocker locker(&lock);
    return cap;
}

} //namespace QtAV
#endif // QTAV_BLOCKINGQUEUE_H
