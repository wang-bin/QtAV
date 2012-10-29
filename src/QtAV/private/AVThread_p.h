#ifndef AVTHREAD_P_H
#define AVTHREAD_P_H

//#include <private/qthread_p.h>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QWaitCondition>

#include <QtAV/QAVPacketQueue.h>
#include <QtAV/AVDecoder.h>
#include <QtAV/AVOutput.h>
#include <QtAV/QtAV_Global.h>

namespace QtAV {
class AVDecoder;
class QAVPacket;
class AVThreadPrivate
{
public:
    AVThreadPrivate():stop(false),packets(0),dec(0),writer(0) {}
    virtual ~AVThreadPrivate() {
        /*if (dec) {
            delete dec;
            dec = 0;
        }
        if (writer) {
            delete writer;
            writer = 0;
        }
        if (packets) {
            delete packets;
            packets = 0;
        }*/
    }
    void enqueue(const QAVPacket& pkt) {
        packets->enqueue(pkt);
        not_empty_cond.wakeAll();
    }


    volatile bool stop;
    QAVPacketQueue *packets;
    AVDecoder *dec;
    AVOutput *writer;
    QMutex mutex;
    QWaitCondition not_empty_cond;
};
}

#endif // AVTHREAD_P_H
