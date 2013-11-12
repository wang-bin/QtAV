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

#include <QtAV/AVThread.h>
#include <private/AVThread_p.h>
#include <QtAV/AVOutput.h>
#include <QtAV/Filter.h>
#include <QtAV/OutputSet.h>

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
    //not neccesary context is managed by filters.
    filter_context = 0;
    qDeleteAll(filters); //TODO: is it safe?
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
    DPTR_D(AVThread);
    // FIXME: shall we lock?
    QMutexLocker locker(&d.mutex);
    Q_UNUSED(locker);
    d.tasks.push_back(task);
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
    if (d.writer)
        d.writer->pause(false); //stop waiting
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
    d_func().writer = out;
}

AVOutput* AVThread::output() const
{
    return d_func().writer;
}

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
    if (d.writer)
        d.writer->pause(false); //stop waiting. Important when replay
    d.stop = false;
    d.demux_end = false;
    d.packets.setBlocking(true);
    d.packets.clear();
    //not neccesary context is managed by filters.
    d.filter_context = 0;
    QMutexLocker lock(&d.ready_mutex);
    d.ready = true;
    d.ready_cond.wakeOne();
}

bool AVThread::tryPause(int timeout)
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
    QRunnable *task = d.tasks.takeFirst();
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
