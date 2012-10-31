#ifndef QAVPACKET_H
#define QAVPACKET_H

#include <QtCore/QByteArray>
#include <QtCore/QQueue>
#include <QtCore/QMutex>
#include <QtAV/QtAV_Global.h>

namespace QtAV {
class Q_EXPORT Packet
{
public:
    Packet();

    QByteArray data;
    qreal pts, duration;
};

#define THREAD_SAFE_QUEUE 1
#if !THREAD_SAFE_QUEUE
typedef QQueue<Packet> PacketQueue;
#else
class Q_EXPORT PacketQueue
{
public:
    PacketQueue();
    ~PacketQueue();

    inline bool isEmpty() const;
    inline int size() const;
    void enqueue(const Packet& packet);
    Packet dequeue();
    Packet head();
    Packet tail();

private:
    QQueue<Packet> queue;
    QMutex mutex;
};

bool PacketQueue::isEmpty() const
{
    return queue.isEmpty();
}

int PacketQueue::size() const
{
    return queue.size();
}
#endif //THREAD_SAFE_QUEUE
}

#endif // QAVPACKET_H
