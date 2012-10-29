#ifndef AVTHREAD_H
#define AVTHREAD_H

#include <QtCore/QThread>
#include <QtCore/QScopedPointer>
#include <QtAV/QtAV_Global.h>

namespace QtAV {
class AVDecoder;
class QAVPacketQueue;
class AVThreadPrivate;
class AVOutput;
class Q_EXPORT AVThread : public QThread
{
    Q_OBJECT
public:
	explicit AVThread(QObject *parent = 0);
    virtual ~AVThread();

    void wakeAll();

    void setPacketQueue(QAVPacketQueue *queue);
    void packetQueue() const;

    void setDecoder(AVDecoder *decoder);
    AVDecoder *decoder() const;

    void setOutput(AVOutput *out);
    AVOutput* output() const;

protected:
    AVThread(AVThreadPrivate& d, QObject *parent = 0);

    Q_DECLARE_PRIVATE(AVThread)
    QScopedPointer<AVThreadPrivate> d_ptr;
};
}

#endif // AVTHREAD_H
