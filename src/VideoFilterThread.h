/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2015 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
******************************************************************************/

#ifndef QTAV_VIDEOFILTERTHREAD_H
#define QTAV_VIDEOFILTERTHREAD_H

#include <QtCore/QQueue>
#include <QtCore/QMutex>
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
    ~VideoFilterThread();

    BlockingQueue<VideoFrame>* frameQueue() { return &m_queue;}
    void setClock(AVClock *c) { m_clock = c;}
    void setStatistics(Statistics* st) { m_statistics = st;}
    void setOutputSet(OutputSet *os) { m_outset = os;}
    void setFilters(const QList<Filter*>& fs) {
        QMutexLocker lock(&m_mutex);
        Q_UNUSED(lock);
        m_filters = fs;
    }

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
    QMutex m_mutex;
};
}
#endif // QTAV_VIDEOFILTERTHREAD_H
