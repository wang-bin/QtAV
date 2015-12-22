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

#include "AVThread.h"
#include "AVThread_p.h"
#include "QtAV/AVClock.h"
#include "QtAV/AVDecoder.h"
#include "QtAV/AVOutput.h"
#include "QtAV/Filter.h"
#include "output/OutputSet.h"
#include "utils/Logger.h"

namespace QtAV {

QVariantHash AVThreadPrivate::dec_opt_framedrop;
QVariantHash AVThreadPrivate::dec_opt_normal;

AVThreadPrivate::~AVThreadPrivate() {
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

bool AVThread::installFilter(Filter *filter, int index, bool lock)
{
    DPTR_D(AVThread);
    int p = index;
    if (p < 0)
        p += d.filters.size();
    if (p < 0)
        p = 0;
    if (p > d.filters.size())
        p = d.filters.size();
    const int old = d.filters.indexOf(filter);
    // already installed at desired position
    if (p == old)
        return true;
    if (lock) {
        QMutexLocker locker(&d.mutex);
        if (p >= 0)
            d.filters.removeAt(p);
        d.filters.insert(p, filter);
        onUpdateFilters();
    } else {
        if (p >= 0)
            d.filters.removeAt(p);
        d.filters.insert(p, filter);
        onUpdateFilters();
    }
    return true;
}

bool AVThread::uninstallFilter(Filter *filter, bool lock)
{
    DPTR_D(AVThread);
    if (lock) {
        QMutexLocker locker(&d.mutex);
        const bool ok = d.filters.removeOne(filter);
        if (ok)
            onUpdateFilters();
        return ok;
    }
    const bool ok = d.filters.removeOne(filter);
    if (ok)
        onUpdateFilters();
    return ok;
}

const QList<Filter*>& AVThread::filters() const
{
    return d_func().filters;
}

void AVThread::scheduleTask(QRunnable *task)
{
    d_func().tasks.put(task);
}

void AVThread::scheduleFrameDrop(bool value)
{
    class FrameDropTask : public QRunnable {
        AVDecoder *decoder;
        bool drop;
    public:
        FrameDropTask(AVDecoder *dec, bool value) : decoder(dec), drop(value) {}
        void run() Q_DECL_OVERRIDE {
            if (!decoder)
                return;
            if (drop)
                decoder->setOptions(AVThreadPrivate::dec_opt_framedrop);
            else
                decoder->setOptions(AVThreadPrivate::dec_opt_normal);
        }
    };
    scheduleTask(new FrameDropTask(decoder(), value));
}

qreal AVThread::previousHistoryPts() const
{
    DPTR_D(const AVThread);
    if (d.pts_history.empty()) {
        qDebug("pts history is EMPTY");
        return 0;
    }
    if (d.pts_history.size() == 1)
        return -d.pts_history.back();
    const qreal current_pts = d.pts_history.back();
    for (int i = d.pts_history.size() - 2; i > 0; --i) {
        if (d.pts_history.at(i) < current_pts)
            return d.pts_history.at(i);
    }
    return -d.pts_history.front();
}

qreal AVThread::decodeFrameRate() const
{
    DPTR_D(const AVThread);
    if (d.pts_history.size() <= 1)
        return 0;
    const qreal dt = d.pts_history.back() - d.pts_history.front();
    if (dt <= 0)
        return 0;
    return d.pts_history.size()/dt;
}

void AVThread::setDropFrameOnSeek(bool value)
{
    d_func().drop_frame_seek = value;
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

PacketBuffer* AVThread::packetQueue() const
{
    return const_cast<PacketBuffer*>(&d_func().packets);
}

void AVThread::setDecoder(AVDecoder *decoder)
{
    DPTR_D(AVThread);
    QMutexLocker lock(&d.mutex);
    Q_UNUSED(lock);
    d.dec = decoder;
}

AVDecoder* AVThread::decoder() const
{
    return d_func().dec;
}

void AVThread::setOutput(AVOutput *out)
{
    DPTR_D(AVThread);
    QMutexLocker lock(&d.mutex);
    Q_UNUSED(lock);
    if (!d.outputSet)
        return;
    if (!out) {
        d.outputSet->clearOutputs();
        return;
    }
    d.outputSet->addOutput(out);
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

void AVThread::resetState()
{
    DPTR_D(AVThread);
    pause(false);
    d.tasks.clear();
    d.render_pts0 = -1;
    d.stop = false;
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

void AVThread::waitAndCheck(ulong value, qreal pts)
{
    DPTR_D(AVThread);
    if (value <= 0)
        return;
    //qDebug("wating for %lu msecs", value);
    ulong us = value * 1000UL;
    static const ulong kWaitSlice = 20 * 1000UL; //20ms
    while (us > kWaitSlice) {
        usleep(kWaitSlice);
        if (d.stop)
            us = 0;
        else
            us -= kWaitSlice;
        if (pts > 0)
            us = qMin(us, ulong((double)(qMax<qreal>(0, pts - d.clock->value()))*1000000.0));
        //qDebug("us: %ul, pts: %f, clock: %f", us, pts, d.clock->value());
        processNextTask();
    }
    if (us > 0) {
        usleep(us);
        processNextTask();
    }
}

} //namespace QtAV
