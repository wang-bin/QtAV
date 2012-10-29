#ifndef QAVDEMUXTHREAD_H
#define QAVDEMUXTHREAD_H

#include <QtCore/QThread>
#include <QtAV/QtAV_Global.h>

namespace QtAV {
class AVDemuxer;
class QAVPacketQueue;
class Q_EXPORT AVDemuxThread : public QThread
{
    Q_OBJECT
public:
    explicit AVDemuxThread(QObject *parent = 0);
    
protected:
    virtual void run();
    
private:
    AVDemuxer *demuxer;
    QAVPacketQueue *audio_queue, *video_queue;

};
}

#endif // QAVDEMUXTHREAD_H
