#ifndef VIDEOFILTERTHREAD_H
#define VIDEOFILTERTHREAD_H

#include <QtCore/QQueue>
#include <QtCore/QThread>
#include "QtAV/AVClock.h"
#include "utils/BlockingQueue.h"
#include "QtAV/VideoFrame.h"
#include "output/OutputSet.h"
namespace QtAV {
class VideoFilterContext;
class VideoFilterThread : public QThread
{
    Q_OBJECT
public:
    VideoFilterThread(QObject* parent = 0);

    BlockingQueue<VideoFrame>* frameQueue() { return &m_queue;}
    void setClock(AVClock *c) { m_clock = c;}
    void setStatistics(Statistics* st) { m_statistics = st;}
    void setOutputSet(OutputSet *os) { m_outset = os;}
Q_SIGNALS:
    void frameDelivered();

public Q_SLOTS:
    void stop() { m_stop = true;}
protected:
    void waitAndCheck(ulong value, qreal pts);
protected:
    void applyFilters(VideoFrame& frame);
    // deliver video frame to video renderers. frame may be converted to a suitable format for renderer
    bool deliverVideoFrame(VideoFrame &frame);
    virtual void run();

private:
    volatile bool m_stop; //true when packets is empty and demux is end.
    volatile bool m_seeking;
    AVClock* m_clock;
    BlockingQueue<VideoFrame> m_queue;
    OutputSet *m_outset;
    VideoFrameConverter m_conv;
    VideoFilterContext *m_filter_context;//TODO: use own smart ptr. QSharedPointer "=" is ugly
    QList<Filter*> m_filters;
    Statistics *m_statistics;
};
}
#endif // VIDEOFILTERTHREAD_H
