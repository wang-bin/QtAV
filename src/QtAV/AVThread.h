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
class AVClock;
class Q_EXPORT AVThread : public QThread
{
    Q_OBJECT
public:
	explicit AVThread(QObject *parent = 0);
    virtual ~AVThread();

    virtual void stop();
    void wakeAll();

    void setClock(AVClock *clock);
    AVClock* clock() const;

    //void setPacketQueue(QAVPacketQueue *queue);
    QAVPacketQueue* packetQueue() const;

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
