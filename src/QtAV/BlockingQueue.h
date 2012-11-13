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


#ifndef QTAV_BLOCKINGQUEUE_H
#define QTAV_BLOCKINGQUEUE_H

#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>

template<typename T> class QQueue;
namespace QtAV {

template <typename T, typename Container = QQueue<T> >
class BlockingQueue
{
public:
    BlockingQueue();

    void setCapacity(int max); //enqueue is allowed if less than capacity
    void setThreshold(int min); //wake up and enqueue

    void put(const T& t);
    T take();
    void setBlocking(bool block); //will wake if false. called when no more data can enqueue

    inline void clear();
    inline bool isEmpty() const;
    inline int size() const;

private:
    bool block;
    int cap, thres;
    Container queue;
    QMutex mutex;
    QWaitCondition cond_full, cond_empty;
};


template <typename T, typename Container>
BlockingQueue<T, Container>::BlockingQueue()
    :block(true),cap(512),thres(256)
{
}

template <typename T, typename Container>
void BlockingQueue<T, Container>::setCapacity(int max)
{
    QMutexLocker lock(&mutex);
    Q_UNUSED(lock);
    cap = max;
}

template <typename T, typename Container>
void BlockingQueue<T, Container>::setThreshold(int min)
{
    QMutexLocker lock(&mutex);
    Q_UNUSED(lock);
    thres = min;
}

template <typename T, typename Container>
void BlockingQueue<T, Container>::put(const T& t)
{
    QMutexLocker lock(&mutex);
    Q_UNUSED(lock);
    if (block && queue.size() >= cap)
        cond_full.wait(&mutex);
    cond_empty.wakeAll();
    queue.enqueue(t);
}

template <typename T, typename Container>
T BlockingQueue<T, Container>::take()
{
    QMutexLocker lock(&mutex);
    Q_UNUSED(lock);
    if (queue.size() < thres)
        cond_full.wakeAll();
    if (block && queue.isEmpty())
        cond_empty.wait(&mutex);
    return queue.dequeue();
}

template <typename T, typename Container>
void BlockingQueue<T, Container>::setBlocking(bool block)
{
    QMutexLocker lock(&mutex);
    Q_UNUSED(lock);
    this->block = block;
    if (!block) {
        cond_empty.wakeAll();
        cond_full.wakeAll();
    }
}

template <typename T, typename Container>
void BlockingQueue<T, Container>::clear()
{
    cond_empty.wakeAll();
    cond_full.wakeAll();
    QMutexLocker lock(&mutex);
    Q_UNUSED(lock);
    queue.clear();
}

template <typename T, typename Container>
bool BlockingQueue<T, Container>::isEmpty() const
{
    return queue.isEmpty();
}

template <typename T, typename Container>
int BlockingQueue<T, Container>::size() const
{
    return queue.size();
}

} //namespace QtAV
#endif // QTAV_BLOCKINGQUEUE_H
