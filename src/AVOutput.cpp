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

#include <QtAV/AVOutput.h>
#include <private/AVOutput_p.h>
#include <QtAV/Filter.h>
#include <QtAV/FilterContext.h>
#include <QtAV/FilterManager.h>
#include <QtAV/OutputSet.h>
//#include <QtCore/QMetaObject>

namespace QtAV {

AVOutputPrivate::~AVOutputPrivate() {
    cond.wakeAll(); //WHY: failed to wake up
    if (filter_context) {
        delete filter_context;
        filter_context = 0;
    }
    qDeleteAll(filters);
    filters.clear();
}

AVOutput::AVOutput()
{
}

AVOutput::AVOutput(AVOutputPrivate &d)
    :DPTR_INIT(&d)
{
}

AVOutput::~AVOutput()
{
    DPTR_D(AVOutput);
    pause(false); //Does not work. cond may still waiting when destroyed
    detach();
}

bool AVOutput::writeData(const QByteArray &data)
{
    Q_UNUSED(data);
    //DPTR_D(AVOutput);
    //locker(&mutex)
    //TODO: make sure d.data thread safe. lock around here? for audio and video(main thread problem)?
    /* you can use d.data directly in AVThread. In other thread, it's not safe, you must do something
     * to make sure the data is not be modified in AVThread when using it*/
    //d_func().data = data;
    if (d_func().paused)
        return false;
    convertData(data); //TODO: only once for all outputs
	bool result = write();
	//write then pause: if capture when pausing, the displayed picture is captured
    //tryPause(); ///DO NOT pause the thread so that other outputs can still run
	return result;
}

bool AVOutput::isAvailable() const
{
    return d_func().available;
}

void AVOutput::pause(bool p)
{
    DPTR_D(AVOutput);
    if (d.paused == p)
        return;
    d.paused = p;
}

bool AVOutput::isPaused() const
{
    return d_func().paused;
}

//TODO: how to call this automatically before write()?
bool AVOutput::tryPause()
{
    DPTR_D(AVOutput);
    if (!d.paused)
        return false;
    QMutexLocker lock(&d.mutex);
    Q_UNUSED(lock);
    d.cond.wait(&d.mutex);
    return true;
}

void AVOutput::addOutputSet(OutputSet *set)
{
    d_func().output_sets.append(set);
}

void AVOutput::removeOutputSet(OutputSet *set)
{
    d_func().output_sets.removeAll(set);
}

void AVOutput::attach(OutputSet *set)
{
    set->addOutput(this);
}

void AVOutput::detach(OutputSet *set)
{
    DPTR_D(AVOutput);
    if (set) {
        set->removeOutput(this);
        return;
    }
    foreach(OutputSet *set, d.output_sets) {
        set->removeOutput(this);
    }
}

void AVOutput::convertData(const QByteArray &data)
{
    //TODO: make sure d.data thread safe. lock here?
    DPTR_D(AVOutput);
    d.data = data;
}

int AVOutput::filterContextType() const
{
    return FilterContext::None;
}

QList<Filter*>& AVOutput::filters()
{
    return d_func().filters;
}

void AVOutput::setStatistics(Statistics *statistics)
{
    DPTR_D(AVOutput);
    d.statistics = statistics;
}

bool AVOutput::installFilter(Filter *filter)
{
    if (!FilterManager::instance().registerFilter(filter, this)) {
        return false;
    }
    DPTR_D(AVOutput);
    d.filters.push_back(filter);
    return true;
}

/*
 * FIXME: how to ensure thread safe using mutex etc? for a video filter, both are in main thread.
 * an audio filter on audio output may be in audio thread
 */
bool AVOutput::uninstallFilter(Filter *filter)
{
    if (!FilterManager::instance().unregisterFilter(filter)) {
        qWarning("unregister filter %p failed", filter);
        return false;
    }
    DPTR_D(AVOutput);
    d.pending_uninstall_filters.push_back(filter);
    return true;
}

void AVOutput::hanlePendingTasks()
{
    DPTR_D(AVOutput);
    if (d.pending_uninstall_filters.isEmpty())
        return;
    foreach (Filter *filter, d.pending_uninstall_filters) {
        d.filters.removeAll(filter);
        //QMetaObject::invokeMethod(FilterManager::instance(), "onUninstallInTargetDone", Qt::AutoConnection, Q_ARG(Filter*, filter));
        FilterManager::instance().emitOnUninstallInTargetDone(filter);
    }
    d.pending_uninstall_filters.clear();
}

} //namespace QtAV
