#ifndef QAVPACKETQUEUE_H
#define QAVPACKETQUEUE_H

#include <QtCore/QQueue>
#include <QtCore/QMutex>
#include <QtAV/QtAV_Global.h>

namespace QtAV {
class QAVPacket;
class Q_EXPORT QAVPacketQueue
{
public:
    QAVPacketQueue();
    ~QAVPacketQueue();

    inline bool isEmpty() const;
    inline int size() const;
    void enqueue(const QAVPacket& packet);
    QAVPacket dequeue();
    QAVPacket head();
    QAVPacket tail();

private:
    QQueue<QAVPacket> queue;
    QMutex mutex;
};

bool QAVPacketQueue::isEmpty() const
{
    return queue.isEmpty();
}

int QAVPacketQueue::size() const
{
    return queue.size();
}
}

#endif // QAVPACKETQUEUE_H
