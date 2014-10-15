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

#include "QtAV/AVOutput.h"
#include "QtAV/private/AVOutput_p.h"
#include "QtAV/Filter.h"
#include "QtAV/FilterContext.h"
#include "filter/FilterManager.h"
#include "output/OutputSet.h"
#include "utils/Logger.h"

namespace QtAV {

AVOutputPrivate::~AVOutputPrivate() {
    cond.wakeAll(); //WHY: failed to wake up
    if (filter_context) {
        delete filter_context;
        filter_context = 0;
    }
    foreach (Filter *f, pending_uninstall_filters) {
        filters.removeAll(f);
    }
    QList<Filter*>::iterator it = filters.begin();
    while (it != filters.end()) {
        // 1 filter has 1 target. so if has output filter in manager, the output is this output
        /*FilterManager::instance().hasOutputFilter(*it) && */
        if ((*it)->isOwnedByTarget() && !(*it)->parent())
            delete *it;
        ++it;
    }
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
    pause(false); //Does not work. cond may still waiting when destroyed
    detach();
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
    onAddOutputSet(set);
}

void AVOutput::onAddOutputSet(OutputSet *set)
{
    d_func().output_sets.append(set);
}

void AVOutput::removeOutputSet(OutputSet *set)
{
    onRemoveOutputSet(set);
}

void AVOutput::onRemoveOutputSet(OutputSet *set)
{
    d_func().output_sets.removeAll(set);
}

void AVOutput::attach(OutputSet *set)
{
    onAttach(set);
}

void AVOutput::onAttach(OutputSet *set)
{
    set->addOutput(this);
}

void AVOutput::detach(OutputSet *set)
{
    onDetach(set);
}

void AVOutput::onDetach(OutputSet *set)
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
    return onInstallFilter(filter);
}

bool AVOutput::onInstallFilter(Filter *filter)
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
    return onUninstallFilter(filter);
}

bool AVOutput::onUninstallFilter(Filter *filter)
{
    if (!FilterManager::instance().unregisterFilter(filter)) {
        qWarning("unregister filter %p failed", filter);
        //return false; //already removed in FilterManager::uninstallFilter()
    }
    DPTR_D(AVOutput);
    d.pending_uninstall_filters.push_back(filter);
    return true;
}

void AVOutput::hanlePendingTasks()
{
    onHanlePendingTasks();
}

bool AVOutput::onHanlePendingTasks()
{
    DPTR_D(AVOutput);
    if (d.pending_uninstall_filters.isEmpty())
        return false;
    foreach (Filter *filter, d.pending_uninstall_filters) {
        d.filters.removeAll(filter);
        //QMetaObject::invokeMethod(FilterManager::instance(), "onUninstallInTargetDone", Qt::AutoConnection, Q_ARG(Filter*, filter));
        FilterManager::instance().emitOnUninstallInTargetDone(filter);
    }
    d.pending_uninstall_filters.clear();
    return true;
}

} //namespace QtAV
