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
}
