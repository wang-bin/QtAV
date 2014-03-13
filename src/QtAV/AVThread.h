/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2013 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_AVTHREAD_H
#define QTAV_AVTHREAD_H

#include <QtCore/QRunnable>
#include <QtCore/QScopedPointer>
#include <QtCore/QThread>
#include <QtAV/Packet.h>
//TODO: pause functions. AVOutput may be null, use AVThread's pause state

namespace QtAV {

class AVDecoder;
class AVThreadPrivate;
class AVOutput;
class AVClock;
class Filter;
class Statistics;
class OutputSet;
class Q_AV_EXPORT AVThread : public QThread
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(AVThread)
public:
    explicit AVThread(QObject *parent = 0);
    virtual ~AVThread();

    //used for changing some components when running
    Q_DECL_DEPRECATED void lock();
    Q_DECL_DEPRECATED void unlock();

    void setClock(AVClock *clock);
    AVClock* clock() const;

    //void setPacketQueue(PacketQueue *queue);
    PacketQueue* packetQueue() const;

    void setDecoder(AVDecoder *decoder);
    AVDecoder *decoder() const;

    Q_DECL_DEPRECATED void setOutput(AVOutput *out);
    Q_DECL_DEPRECATED AVOutput* output() const;

    void setOutputSet(OutputSet *set);
    OutputSet* outputSet() const;

    void setDemuxEnded(bool ended);

    bool isPaused() const;

    void waitForReady();

    bool installFilter(Filter *filter, bool lock = true);
    bool uninstallFilter(Filter *filter, bool lock = true);
    const QList<Filter *> &filters() const;

    // TODO: resample, resize task etc.
    void scheduleTask(QRunnable *task);

    //only decode video without display or skip decode audio until pts reaches
    void skipRenderUntil(qreal pts);

public slots:
    virtual void stop();
    /*change pause state. the pause/continue action will do in the next loop*/
    void pause(bool p); //processEvents when waiting?
    void nextAndPause(); //process 1 frame and pause

protected:
    AVThread(AVThreadPrivate& d, QObject *parent = 0);
    void resetState();
    /*
     * If the pause state is true setted by pause(true), then block the thread and wait for pause state changed, i.e. pause(false)
     * and return true. Otherwise, return false immediatly.
     */
    // has timeout so that the pending tasks can be processed
    bool tryPause(int timeout = 100);
    bool processNextTask(); //in AVThread

    DPTR_DECLARE(AVThread)

private:
    void setStatistics(Statistics* statistics);
    friend class AVPlayer;
};
}

#endif // QTAV_AVTHREAD_H
