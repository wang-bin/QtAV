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

#ifndef QTAV_FILTERMANAGER_H
#define QTAV_FILTERMANAGER_H

#include <QtCore/QList>
#include "QtAV/Filter.h" // signal qobj parameter
#include "QtAV/QtAV_Global.h"

namespace QtAV {

class AVOutput;
class AVPlayer;
class FilterManagerPrivate;
class FilterManager
{
    DPTR_DECLARE_PRIVATE(FilterManager)
    Q_DISABLE_COPY(FilterManager)
public:
    static FilterManager& instance();
    /*!
     * \brief registerFilter
     * record the filter in manager
     * target.installXXXFilter/filter.installTo(target) must call registerXXXFilter
     */
    bool registerFilter(Filter *filter, AVOutput *output, int pos = 0x7FFFFFFF);
    QList<Filter*> outputFilters(AVOutput* output) const;
    bool registerAudioFilter(Filter *filter, AVPlayer *player, int pos = 0x7FFFFFFF);
    QList<Filter *> audioFilters(AVPlayer* player) const;
    bool registerVideoFilter(Filter *filter, AVPlayer *player, int pos = 0x7FFFFFFF);
    QList<Filter*> videoFilters(AVPlayer* player) const;
    bool unregisterAudioFilter(Filter *filter, AVPlayer *player);
    bool unregisterVideoFilter(Filter *filter, AVPlayer *player);
    bool unregisterFilter(Filter *filter, AVOutput *output);
    // unregister and call target.uninstall
    bool uninstallFilter(Filter *filter); //called by filter.uninstall
    bool uninstallAudioFilter(Filter *filter, AVPlayer* player);
    bool uninstallVideoFilter(Filter *filter, AVPlayer* player);
    bool uninstallFilter(Filter *filter, AVOutput* output);
private:
    // return bool is for AVPlayer.installAudio/VideoFilter compatibility
    bool insert(Filter* filter, QList<Filter*>& filters, int pos);
    FilterManager();
    ~FilterManager();

    DPTR_DECLARE(FilterManager)
};

} //namespace QtAV

#endif // QTAV_FILTERMANAGER_H
