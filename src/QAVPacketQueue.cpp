#include <QtAV/QAVPacketQueue.h>
#include <QtAV/QAVPacket.h>

namespace QtAV {

QAVPacketQueue::QAVPacketQueue()
    :mutex(QMutex::Recursive)
{
}

QAVPacketQueue::~QAVPacketQueue()
{
    queue.clear();
}

void QAVPacketQueue::enqueue(const QAVPacket& packet)
{
    QMutexLocker lock(&mutex);
    Q_UNUSED(lock);
    queue.enqueue(packet);
}

QAVPacket QAVPacketQueue::dequeue()
{
    QMutexLocker lock(&mutex);
    Q_UNUSED(lock);
    return queue.dequeue();
}

QAVPacket QAVPacketQueue::head()
{
    QMutexLocker lock(&mutex);
    Q_UNUSED(lock);
    return queue.head();
}

QAVPacket QAVPacketQueue::tail()
{
    QMutexLocker lock(&mutex);
    Q_UNUSED(lock);
    return queue.last();
}

}
