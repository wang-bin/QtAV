/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>

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

#include "AVThread.h"
#include "AVThread_p.h"
#include "QtAV/AVOutput.h"
#include "QtAV/Filter.h"
#include "output/OutputSet.h"
#include "utils/Logger.h"

namespace QtAV {

AVThreadPrivate::~AVThreadPrivate() {
    demux_end = true;
    stop = true;
    if (!paused) {
        qDebug("~AVThreadPrivate wake up paused thread");
        paused = false;
        next_pause = false;
        cond.wakeAll();
    }
    ready_cond.wakeAll();
    packets.setBlocking(true); //???
    packets.clear();
    QList<Filter*>::iterator it = filters.begin();
    while (it != filters.end()) {
        if ((*it)->isOwnedByTarget() && !(*it)->parent())
            delete *it;
        ++it;
    }
    filters.clear();
}

AVThread::AVThread(QObject *parent) :
    QThread(parent)
{
}

AVThread::AVThread(AVThreadPrivate &d, QObject *parent)
    :QThread(parent),DPTR_INIT(&d)
{
}

AVThread::~AVThread()
{
    //d_ptr destroyed automatically
}

bool AVThread::isPaused() const
{
    DPTR_D(const AVThread);
    //if d.next_pause is true, the thread will pause soon, may happens before you can handle the result
    return d.paused || d.next_pause;
}

bool AVThread::installFilter(Filter *filter, bool lock)
{
    DPTR_D(AVThread);
    if (lock) {
        QMutexLocker locker(&d.mutex);
        if (d.filters.contains(filter))
            return false;
        d.filters.push_back(filter);
    } else {
        if (d.filters.contains(filter))
            return false;
        d.filters.push_back(filter);
    }
    return true;
}

bool AVThread::uninstallFilter(Filter *filter, bool lock)
{
    DPTR_D(AVThread);
    if (lock) {
        QMutexLocker locker(&d.mutex);
        return d.filters.removeOne(filter);
    } else {
        return d.filters.removeOne(filter);
    }
}

const QList<Filter*>& AVThread::filters() const
{
    return d_func().filters;
}

void AVThread::scheduleTask(QRunnable *task)
{
    d_func().tasks.put(task);
}

void AVThread::skipRenderUntil(qreal pts)
{
    /*
     * Lock here is useless because in Audio/VideoThread, the lock scope is very small.
     * So render_pts0 may be reset to 0 after set here
     */
    DPTR_D(AVThread);
    class SetRenderPTS0Task : public QRunnable {
    public:
        SetRenderPTS0Task(qreal* pts0, qreal value)
            : ptr(pts0)
            , pts(value)
        {}
        void run() {
            *ptr = pts;
        }
    private:
        qreal *ptr;
        qreal pts;
    };

    scheduleTask(new SetRenderPTS0Task(&d.render_pts0, pts));
}

// TODO: shall we close decoder here?
void AVThread::stop()
{
    DPTR_D(AVThread);
    d.stop = true; //stop as soon as possible
    QMutexLocker locker(&d.mutex);
    Q_UNUSED(locker);
    d.packets.setBlocking(false); //stop blocking take()
    d.packets.clear();
    pause(false);
    QMutexLocker lock(&d.ready_mutex);
    d.ready = false;
    //terminate();
}

//TODO: output set
void AVThread::pause(bool p)
{
    DPTR_D(AVThread);
    if (d.paused == p)
        return;
    d.paused = p;
    if (!d.paused) {
        qDebug("wake up paused thread");
        d.next_pause = false;
        d.cond.wakeAll();
    }
}

void AVThread::nextAndPause()
{
    DPTR_D(AVThread);
    d.next_pause = true;
    d.paused = true;
    d.cond.wakeAll();
}

void AVThread::lock()
{
    d_func().mutex.lock();
}

void AVThread::unlock()
{
    d_func().mutex.unlock();
}

void AVThread::setClock(AVClock *clock)
{
    d_func().clock = clock;
}

AVClock* AVThread::clock() const
{
    return d_func().clock;
}

PacketQueue* AVThread::packetQueue() const
{
    return const_cast<PacketQueue*>(&d_func().packets);
}

void AVThread::setDecoder(AVDecoder *decoder)
{
    d_func().dec = decoder;
}

AVDecoder* AVThread::decoder() const
{
    return d_func().dec;
}

void AVThread::setOutput(AVOutput *out)
{
    DPTR_D(AVThread);
    if (!d.outputSet)
        return;
    d_func().outputSet->addOutput(out);
}

AVOutput* AVThread::output() const
{
    DPTR_D(const AVThread);
    if (!d.outputSet || d.outputSet->outputs().isEmpty())
        return 0;
    return d.outputSet->outputs().first();
}

// TODO: remove?
void AVThread::setOutputSet(OutputSet *set)
{
    d_func().outputSet = set;
}

OutputSet* AVThread::outputSet() const
{
    return d_func().outputSet;
}

void AVThread::setDemuxEnded(bool ended)
{
    d_func().demux_end = ended;
}

void AVThread::resetState()
{
    DPTR_D(AVThread);
    pause(false);
    d.render_pts0 = 0;
    d.stop = false;
    d.demux_end = false;
    d.packets.setBlocking(true);
    d.packets.clear();
    QMutexLocker lock(&d.ready_mutex);
    d.ready = true;
    d.ready_cond.wakeOne();
}

bool AVThread::tryPause(unsigned long timeout)
{
    DPTR_D(AVThread);
    if (!isPaused())
        return false;
    QMutexLocker lock(&d.mutex);
    Q_UNUSED(lock);
    return d.cond.wait(&d.mutex, timeout);
    qDebug("paused thread waked up!!!");
    return true;
}

bool AVThread::processNextTask()
{
    DPTR_D(AVThread);
    if (d.tasks.isEmpty())
        return true;
    QRunnable *task = d.tasks.take();
    task->run();
    if (task->autoDelete()) {
        delete task;
    }
    return true;
}

void AVThread::setStatistics(Statistics *statistics)
{
    DPTR_D(AVThread);
    d.statistics = statistics;
}

void AVThread::waitForReady()
{
    QMutexLocker lock(&d_func().ready_mutex);
    while (!d_func().ready) {
        d_func().ready_cond.wait(&d_func().ready_mutex);
    }
}

} //namespace QtAV
