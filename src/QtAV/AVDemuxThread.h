#ifndef QAVDEMUXTHREAD_H
#define QAVDEMUXTHREAD_H

#include <QtCore/QThread>
#include <QtAV/QtAV_Global.h>

namespace QtAV {
class AVDemuxer;
class AVThread;
class Q_EXPORT AVDemuxThread : public QThread
{
    Q_OBJECT
public:
    explicit AVDemuxThread(QObject *parent = 0);
    explicit AVDemuxThread(AVDemuxer *dmx, QObject *parent = 0);
    void setDemuxer(AVDemuxer *dmx);
    void setAudioThread(AVThread *thread);
    void setVideoThread(AVThread *thread);

public slots:
    void stop();

protected:
    virtual void run();
    
private:
    volatile bool end;
    AVDemuxer *demuxer;
    AVThread *audio_thread, *video_thread;
    int audio_stream, video_stream;
};
}

#endif // QAVDEMUXTHREAD_H
