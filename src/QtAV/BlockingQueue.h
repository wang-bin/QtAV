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
    inline bool isEnough() const; //size > thres
    inline bool isFull() const; //size >= cap
    inline int size() const;
    inline int threshold() const;
    inline int capacity() const;

    class StateChangeCallback
    {
    public:
        virtual ~StateChangeCallback(){}
        virtual void call() = 0;
    };
    void setEmptyCallback(StateChangeCallback* call);
    void setThresholdCallback(StateChangeCallback* call);
    void setFullCallback(StateChangeCallback* call);

private:
    bool block_empty, block_full;
    int cap, thres; //static?
    Container<T> queue;
    mutable QReadWriteLock lock; //locker in const func
    QReadWriteLock block_change_lock;
    QWaitCondition cond_full, cond_empty;
    //upto_threshold_callback, downto_threshold_callback
    StateChangeCallback *empty_callback, *threshold_callback, *full_callback;
};

/* cap - thres = 24, about 1s
 * if fps is large, then larger capacity and threshold is preferred
 */
template <typename T, template <typename> class Container>
BlockingQueue<T, Container>::BlockingQueue()
    :block_empty(true),block_full(true),cap(48),thres(32)
    , empty_callback(0)
    , threshold_callback(0)
    , full_callback(0)
{
}

template <typename T, template <typename> class Container>
void BlockingQueue<T, Container>::setCapacity(int max)
{
    qDebug("queue capacity==>>%d", max);
    QWriteLocker locker(&lock);
    Q_UNUSED(locker);
    cap = max;
}

template <typename T, template <typename> class Container>
void BlockingQueue<T, Container>::setThreshold(int min)
{
    qDebug("queue threshold==>>%d", min);
    QWriteLocker locker(&lock);
    Q_UNUSED(locker);
    thres = min;
}

template <typename T, template <typename> class Container>
void BlockingQueue<T, Container>::put(const T& t)
{
    QWriteLocker locker(&lock);
    Q_UNUSED(locker);
    if (queue.size() >= cap) {
        //qDebug("queue full"); //too frequent
        if (full_callback) {
            full_callback->call();
        }
        if (block_full)
            cond_full.wait(&lock);
    }
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
    if (queue.isEmpty()) {//TODO:always block?
        qDebug("queue empty!!");
        if (empty_callback) {
            empty_callback->call();
        }
        if (block_empty)
            cond_empty.wait(&lock);
    }
    //TODO: Why still empty?
    if (queue.isEmpty()) {
        qWarning("Queue is still empty");
        if (empty_callback) {
            empty_callback->call();
        }
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
    if (!block) {
        cond_empty.wakeAll();
    }
    QWriteLocker locker(&block_change_lock);
    Q_UNUSED(locker);
    block_empty = block;
}

template <typename T, template <typename> class Container>
void BlockingQueue<T, Container>::blockFull(bool block)
{
    if (!block) {
        cond_full.wakeAll();
    }
    //DO NOT use the same lock that put() get() use. it may be already locked
    //this function usualy called in demux thread, so no lock is ok
    QWriteLocker locker(&block_change_lock);
    Q_UNUSED(locker);
    block_full = block;
}

template <typename T, template <typename> class Container>
void BlockingQueue<T, Container>::clear()
{
    QWriteLocker locker(&lock);
    Q_UNUSED(locker);
    //cond_empty.wakeAll();
    cond_full.wakeAll();
    queue.clear();
    //TODO: assert not empty
}

template <typename T, template <typename> class Container>
bool BlockingQueue<T, Container>::isEmpty() const
{
    QReadLocker locker(&lock);
    Q_UNUSED(locker);
    return queue.isEmpty();
}

template <typename T, template <typename> class Container>
bool BlockingQueue<T, Container>::isEnough() const
{
    QReadLocker locker(&lock);
    Q_UNUSED(locker);
    return queue.size() >= thres;
}

template <typename T, template <typename> class Container>
bool BlockingQueue<T, Container>::isFull() const
{
    QReadLocker locker(&lock);
    Q_UNUSED(locker);
    return queue.size() >= cap;
}

template <typename T, template <typename> class Container>
int BlockingQueue<T, Container>::size() const
{
    QReadLocker locker(&lock);
    Q_UNUSED(locker);
    return queue.size();
}

template <typename T, template <typename> class Container>
int BlockingQueue<T, Container>::threshold() const
{
    QReadLocker locker(&lock);
    Q_UNUSED(locker);
    return thres;
}

template <typename T, template <typename> class Container>
int BlockingQueue<T, Container>::capacity() const
{
    QReadLocker locker(&lock);
    Q_UNUSED(locker);
    return cap;
}

template <typename T, template <typename> class Container>
void BlockingQueue<T, Container>::setEmptyCallback(StateChangeCallback *call)
{
    QWriteLocker locker(&lock);
    Q_UNUSED(locker);
    if (empty_callback)
        delete empty_callback;
    empty_callback = call;
}

template <typename T, template <typename> class Container>
void BlockingQueue<T, Container>::setThresholdCallback(StateChangeCallback *call)
{
    QWriteLocker locker(&lock);
    Q_UNUSED(locker);
    if (threshold_callback)
        delete threshold_callback;
    threshold_callback = call;
}

template <typename T, template <typename> class Container>
void BlockingQueue<T, Container>::setFullCallback(StateChangeCallback *call)
{
    QWriteLocker locker(&lock);
    Q_UNUSED(locker);
    if (full_callback)
        delete full_callback;
    full_callback = call;
}


} //namespace QtAV
#endif // QTAV_BLOCKINGQUEUE_H
