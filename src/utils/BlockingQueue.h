/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>
    
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
#include <QtCore/QScopedPointer>
#include <QtCore/QWaitCondition>

//TODO: block full and empty condition separately
QT_BEGIN_NAMESPACE
template<typename T> class QQueue;
QT_END_NAMESPACE
namespace QtAV {

template <typename T, template <typename> class Container = QQueue>
class BlockingQueue
{
public:
    BlockingQueue();
    virtual ~BlockingQueue() {}

    void setCapacity(int max); //enqueue is allowed if less than capacity
    /*!
     * \brief setThreshold
     * do nothing if min >= capacity()
     * \param min
     */
    void setThreshold(int min); //wake up and enqueue

    /*! \brief put
     *  put t into the queue. Will block if blockFull is set to true (and optionally can specify a wait timeout in ms).
     *  \param t the item to copy into the queue
     *  \param wait_timeout_ms this parameter is used if blockFull == true (the default).
     *         If the queue is full, optionally wait for the queue to not be full a maximum of wait_timeout_ms milliseconds,
     *         and return false if timeout expires. ULONG_MAX means wait indefinitely.
     *  \return true if item was successfully placed in the queue and the queue was not full.
     *          false is returned if the queue is (still) full. The item is still placed into a full queue!
     *          Note that even a 'full' queue will accept new items and t WILL be placed in the queue regardless of return value.
     */
    bool put(const T& t, unsigned long wait_timeout_ms = ULONG_MAX);
    /*! \brief take
     *   Dequeue 1 item from queue, optionally blocking.
     * \param wait_timeout_ms this parameter is used if blockEmpty == true (the default).
     *         If the queue is empty, optionally wait for the queue to not be empty a maximum of wait_timeout_ms milliseconds.
     *         ULONG_MAX means wait indefinitely.
     * \param isValid a pointer to a bool (optional).  If isValid is set to true after a call, the returned item is valid. False means the queue was empty or the timeout expired.
     * \return the item taken.  It may not be valid if the queue was empty and timeout expired. Check optional isValid flag to determine if that is the case.
     */
    T take(unsigned long wait_timeout_ms = ULONG_MAX, bool *isValid = 0);
    void setBlocking(bool block); //will wake if false. called when no more data can enqueue
    void blockEmpty(bool block);
    void blockFull(bool block);
//TODO:setMinBlock,MaxBlock
    inline void clear();
    // TODO: checkEmpty, Enough, Full?
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

protected:
    /*!
     * \brief checkFull
     * Check whether the queue is full. Default implemention is compare queue size to capacity.
     * Full is now a more generic notion. You can implement it as checking queued bytes etc.
     * \return true if queue is full
     */
    virtual bool checkFull() const;
    virtual bool checkEmpty() const;
    virtual bool checkEnough() const;

    virtual void onPut(const T&) {}
    virtual void onTake(const T&) {}

    bool block_empty, block_full;
    int cap, thres;
    Container<T> queue;
private:
    mutable QReadWriteLock lock; //locker in const func
    QReadWriteLock block_change_lock;
    QWaitCondition cond_full, cond_empty;
    //upto_threshold_callback, downto_threshold_callback
    QScopedPointer<StateChangeCallback> empty_callback, threshold_callback, full_callback;
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
    //qDebug("queue capacity==>>%d", max);
    QWriteLocker locker(&lock);
    Q_UNUSED(locker);
    cap = max;
    if (thres > cap)
        thres = cap;
}

template <typename T, template <typename> class Container>
void BlockingQueue<T, Container>::setThreshold(int min)
{
    //qDebug("queue threshold==>>%d", min);
    QWriteLocker locker(&lock);
    Q_UNUSED(locker);
    if (min > cap)
        return;
    thres = min;
}

template <typename T, template <typename> class Container>
bool BlockingQueue<T, Container>::put(const T& t, unsigned long timeout_ms)
{
    bool ret = true;
    QWriteLocker locker(&lock);
    Q_UNUSED(locker);
    if (checkFull()) {
        ret = false;
        //qDebug("queue full"); //too frequent
        if (full_callback) {
            full_callback->call();
        }
        if (block_full)
            ret = cond_full.wait(&lock, timeout_ms);
        // uncomment here to reject placing items into a full queue -- update API docs if you do this.
        // if (!ret) return false;
    }
    queue.enqueue(t);
    onPut(t); // emit bufferProgressChanged here if buffering
    if (checkEnough()) {
        cond_empty.wakeOne(); //emit buffering finished here
        //qDebug("queue is enough: %d/%d~%d", queue.size(), thres, cap);
    } else {
        //qDebug("buffering: %d/%d~%d", queue.size(), thres, cap);
    }
    return ret;
}

template <typename T, template <typename> class Container>
T BlockingQueue<T, Container>::take(unsigned long timeout_ms, bool *isValid)
{
    if (isValid) *isValid = false;
    QWriteLocker locker(&lock);
    Q_UNUSED(locker);
    if (checkEmpty()) {//TODO:always block?
        //qDebug("queue empty!!");
        if (empty_callback) {
            empty_callback->call();
        }
        if (block_empty)
            cond_empty.wait(&lock,timeout_ms); //block when empty only
    }
    if (checkEmpty()) {
        //qWarning("Queue is still empty");
        if (empty_callback) {
            empty_callback->call();
        }
        return T();
    }
    T t(queue.dequeue());
    if (isValid) *isValid = true;
    cond_full.wakeOne();
    onTake(t); // emit start buffering here if empty
    return t;
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
    onTake(T());
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
    empty_callback.reset(call);
}

template <typename T, template <typename> class Container>
void BlockingQueue<T, Container>::setThresholdCallback(StateChangeCallback *call)
{
    QWriteLocker locker(&lock);
    Q_UNUSED(locker);
    threshold_callback.reset(call);
}

template <typename T, template <typename> class Container>
void BlockingQueue<T, Container>::setFullCallback(StateChangeCallback *call)
{
    QWriteLocker locker(&lock);
    Q_UNUSED(locker);
    full_callback.reset(call);
}

template <typename T, template <typename> class Container>
bool BlockingQueue<T, Container>::checkFull() const
{
    return queue.size() >= cap;
}

template <typename T, template <typename> class Container>
bool BlockingQueue<T, Container>::checkEmpty() const
{
    return queue.isEmpty();
}

template <typename T, template <typename> class Container>
bool BlockingQueue<T, Container>::checkEnough() const
{
    return queue.size() >= thres && !checkEmpty();
}
} //namespace QtAV
#endif // QTAV_BLOCKINGQUEUE_H
