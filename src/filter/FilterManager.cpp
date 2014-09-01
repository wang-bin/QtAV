/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013 Wang Bin <wbsecg1@gmail.com>

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

#include "filter/FilterManager.h"
#include <QtCore/QList>
#include <QtCore/QMap>
#include "QtAV/AVPlayer.h"
#include "QtAV/Filter.h"
#include "QtAV/AVOutput.h"

namespace QtAV {

class FilterManagerPrivate : public DPtrPrivate<FilterManager>
{
public:
    FilterManagerPrivate()
    {}
    ~FilterManagerPrivate() {

    }

    QList<Filter*> pending_release_filters;
    QMap<Filter*, AVOutput*> filter_out_map;
    QMap<Filter*, AVPlayer*> afilter_player_map;
    QMap<Filter*, AVPlayer*> vfilter_player_map;
};

FilterManager::FilterManager():
    QObject(0)
{
    connect(this, SIGNAL(uninstallInTargetDone(Filter*)), this, SLOT(onUninstallInTargetDone(Filter*)), Qt::QueuedConnection);
}

FilterManager::~FilterManager()
{
}

FilterManager& FilterManager::instance()
{
    static FilterManager sMgr;
    return sMgr;
}

bool FilterManager::registerFilter(Filter *filter, AVOutput *output)
{
    DPTR_D(FilterManager);
    d.pending_release_filters.removeAll(filter); //erase?
    if (d.filter_out_map.contains(filter)|| d.vfilter_player_map.contains(filter) || d.afilter_player_map.contains(filter)) {
        qWarning("Filter %p lready registered!", filter);
        return false;
    }
    d.filter_out_map.insert(filter, output);
    return true;
}

QList<Filter*> FilterManager::outputFilters(AVOutput *output) const
{
    DPTR_D(const FilterManager);
    return d.filter_out_map.keys(output);
}

bool FilterManager::registerAudioFilter(Filter *filter, AVPlayer *player)
{
    DPTR_D(FilterManager);
    d.pending_release_filters.removeAll(filter); //erase?
    if (d.filter_out_map.contains(filter) || d.afilter_player_map.contains(filter) || d.vfilter_player_map.contains(filter)) {
        qWarning("Filter %p lready registered!", filter);
        return false;
    }
    d.afilter_player_map.insert(filter, player);
    return true;
}

QList<Filter*> FilterManager::audioFilters(AVPlayer *player) const
{
    DPTR_D(const FilterManager);
    return d.afilter_player_map.keys(player);
}

bool FilterManager::registerVideoFilter(Filter *filter, AVPlayer *player)
{
    DPTR_D(FilterManager);
    d.pending_release_filters.removeAll(filter); //erase?
    if (d.filter_out_map.contains(filter) || d.vfilter_player_map.contains(filter) || d.afilter_player_map.contains(filter)) {
        qWarning("Filter %p lready registered!", filter);
        return false;
    }
    d.vfilter_player_map.insert(filter, player);
    return true;
}

QList<Filter *> FilterManager::videoFilters(AVPlayer *player) const
{
    DPTR_D(const FilterManager);
    return d.vfilter_player_map.keys(player);
}

// called by AVOutput/AVThread.uninstall imediatly
bool FilterManager::unregisterFilter(Filter *filter)
{
    DPTR_D(FilterManager);
    return d.filter_out_map.remove(filter) || d.vfilter_player_map.remove(filter) || d.afilter_player_map.remove(filter);
}

void FilterManager::onUninstallInTargetDone(Filter *filter)
{
    DPTR_D(FilterManager);
    //d.filter_out_map.remove(filter) || d.vfilter_player_map.remove(filter) || d.afilter_player_map.remove(filter);
    //
    if (d.pending_release_filters.contains(filter)) {
        releaseFilterNow(filter);
    }
}

bool FilterManager::releaseFilter(Filter *filter)
{
    DPTR_D(FilterManager);
    d.pending_release_filters.push_back(filter);
    // will delete filter in slot onUninstallInTargetDone()(signal emitted when uninstall task done)
    return uninstallFilter(filter) || releaseFilterNow(filter);
}

// is it thread safe? always be called in main thread if releaseFilter() and this slot in main thread
bool FilterManager::releaseFilterNow(Filter *filter)
{
    DPTR_D(FilterManager);
    int n = d.pending_release_filters.removeAll(filter);
    if (n > 0) {
        delete filter;
        filter = 0; //filter will not changed!
    }
    return !!n;
}

bool FilterManager::uninstallFilter(Filter *filter)
{
    DPTR_D(FilterManager);
    QMap<Filter*,AVOutput*>::Iterator it = d.filter_out_map.find(filter);
    if (it != d.filter_out_map.end()) {
        AVOutput *out = *it;
        /*
         * remove now so that filter install again will success, even if unregister not completed
         * because the filter will never used in old target
         */

        d.filter_out_map.erase(it); //use unregister()?
        out->uninstallFilter(filter);
        return true;
    }
    QMap<Filter*,AVPlayer*>::Iterator it2 = d.vfilter_player_map.find(filter);
    if (it2 != d.vfilter_player_map.end()) {
        AVPlayer *player = *it2;
        /*
         * remove now so that filter install again will success, even if unregister not completed
         * because the filter will never used in old target
         */

        d.vfilter_player_map.erase(it2); //use unregister()?
        player->uninstallFilter(filter); //post an uninstall task to active player's AVThread
        return true;
    }
    it2 = d.afilter_player_map.find(filter);
    if (it2 != d.afilter_player_map.end()) {
        AVPlayer *player = *it2;
        /*
         * remove now so that filter install again will success, even if unregister not completed
         * because the filter will never used in old target
         */

        d.afilter_player_map.erase(it2); //use unregister()?
        player->uninstallFilter(filter); //post an uninstall task to active player's AVThread
        return true;
    }
    return false;
}

void FilterManager::emitOnUninstallInTargetDone(Filter *filter)
{
    emit onUninstallInTargetDone(filter);
}

} //namespace QtAV
