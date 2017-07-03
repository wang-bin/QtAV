/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2013)

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
#include <QtCore/QMap>
#include "QtAV/AVPlayer.h"
#include "QtAV/Filter.h"
#include "QtAV/AVOutput.h"
#include "utils/Logger.h"

namespace QtAV {

class FilterManagerPrivate : public DPtrPrivate<FilterManager>
{
public:
    FilterManagerPrivate()
    {}
    ~FilterManagerPrivate() {
    }

    QList<Filter*> pending_release_filters;
    QMap<AVOutput*, QList<Filter*> > filter_out_map;
    QMap<AVPlayer*, QList<Filter*> > afilter_player_map;
    QMap<AVPlayer*, QList<Filter*> > vfilter_player_map;
};

FilterManager::FilterManager()
{
}

FilterManager::~FilterManager()
{
}

FilterManager& FilterManager::instance()
{
    static FilterManager sMgr;
    return sMgr;
}

bool FilterManager::insert(Filter *filter, QList<Filter *> &filters, int pos)
{
    int p = pos;
    if (p < 0)
        p += filters.size();
    if (p < 0)
        p = 0;
    if (p > filters.size())
        p = filters.size();
    const int index = filters.indexOf(filter);
    // already installed at desired position
    if (p == index)
        return false;
    if (p >= 0)
        filters.removeAt(p);
    filters.insert(p, filter);
    return true;
}

bool FilterManager::registerFilter(Filter *filter, AVOutput *output, int pos)
{
    DPTR_D(FilterManager);
    d.pending_release_filters.removeAll(filter); //erase?
    QList<Filter*>& fs = d.filter_out_map[output];
    return insert(filter, fs, pos);
}

QList<Filter*> FilterManager::outputFilters(AVOutput *output) const
{
    DPTR_D(const FilterManager);
    return d.filter_out_map.value(output);
}

bool FilterManager::registerAudioFilter(Filter *filter, AVPlayer *player, int pos)
{
    DPTR_D(FilterManager);
    d.pending_release_filters.removeAll(filter); //erase?
    QList<Filter*>& fs = d.afilter_player_map[player];
    return insert(filter, fs, pos);
}

QList<Filter*> FilterManager::audioFilters(AVPlayer *player) const
{
    DPTR_D(const FilterManager);
    return d.afilter_player_map.value(player);
}

bool FilterManager::registerVideoFilter(Filter *filter, AVPlayer *player, int pos)
{
    DPTR_D(FilterManager);
    d.pending_release_filters.removeAll(filter); //erase?
    QList<Filter*>& fs = d.vfilter_player_map[player];
    return insert(filter, fs, pos);
}

QList<Filter *> FilterManager::videoFilters(AVPlayer *player) const
{
    DPTR_D(const FilterManager);
    return d.vfilter_player_map.value(player);
}

// called by AVOutput/AVPlayer.uninstall imediatly
bool FilterManager::unregisterAudioFilter(Filter *filter, AVPlayer *player)
{
    DPTR_D(FilterManager);
    QList<Filter*>& fs = d.afilter_player_map[player];
    bool ret = false;
    if (fs.removeAll(filter) > 0) {
        ret = true;
    }
    if (fs.isEmpty())
        d.afilter_player_map.remove(player);
    return ret;
}

bool FilterManager::unregisterVideoFilter(Filter *filter, AVPlayer *player)
{
    DPTR_D(FilterManager);
    QList<Filter*>& fs = d.vfilter_player_map[player];
    bool ret = false;
    if (fs.removeAll(filter) > 0) {
        ret = true;
    }
    if (fs.isEmpty())
        d.vfilter_player_map.remove(player);
    return ret;
}

bool FilterManager::unregisterFilter(Filter *filter, AVOutput *output)
{
    DPTR_D(FilterManager);
    QList<Filter*>& fs = d.filter_out_map[output];
    bool ret = fs.removeAll(filter) > 0;
    if (fs.isEmpty()) d.filter_out_map.remove(output);
    return ret;
}

bool FilterManager::uninstallFilter(Filter *filter)
{
    DPTR_D(FilterManager);
    QMap<AVPlayer*, QList<Filter*> > map1(d.vfilter_player_map); // NB: copy it for iteration because called code may modify map -- which caused crashes
    QMap<AVPlayer*, QList<Filter*> >::iterator it = map1.begin();
    while (it != map1.end()) {
        if (uninstallVideoFilter(filter, it.key()))
            return true;
        ++it;
    }
    QMap<AVPlayer *, QList<Filter *> > map2(d.afilter_player_map); // copy to avoid crashes when called-code modifies map
    it = map2.begin();
    while (it != map2.end()) {
        if (uninstallAudioFilter(filter, it.key()))
            return true;
        ++it;
    }
    QMap<AVOutput*, QList<Filter*> > map3(d.filter_out_map); // copy to avoid crashes
    QMap<AVOutput*, QList<Filter*> >::iterator it2 = map3.begin();
    while (it2 != map3.end()) {
        if (uninstallFilter(filter, it2.key()))
            return true;
        ++it2;
    }
    return false;
}

bool FilterManager::uninstallAudioFilter(Filter *filter, AVPlayer *player)
{
    if (unregisterAudioFilter(filter, player))
        return player->uninstallFilter(reinterpret_cast<AudioFilter*>(filter));
    return false;
}

bool FilterManager::uninstallVideoFilter(Filter *filter, AVPlayer *player)
{
    if (unregisterVideoFilter(filter, player))
        return player->uninstallFilter(reinterpret_cast<VideoFilter*>(filter));
    return false;
}

bool FilterManager::uninstallFilter(Filter *filter, AVOutput *output)
{
    if (unregisterFilter(filter, output))
        return output->uninstallFilter(filter);
    return false;
}
} //namespace QtAV
