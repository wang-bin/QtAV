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

#ifndef QTAV_FILTERMANAGER_H
#define QTAV_FILTERMANAGER_H

#include <QtCore/QObject>
#include <QtAV/QtAV_Global.h>

namespace QtAV {

class AVOutput;
class AVPlayer;
class Filter;
class FilterManagerPrivate;
class Q_AV_EXPORT FilterManager : public QObject
{
    DPTR_DECLARE_PRIVATE(FilterManager)
    Q_OBJECT
    Q_DISABLE_COPY(FilterManager)
public:
    static FilterManager& instance();
    bool registerFilter(Filter *filter, AVOutput *output);
    QList<Filter*> outputFilters(AVOutput* output) const;
    bool registerAudioFilter(Filter *filter, AVPlayer *player);
    QList<Filter *> audioFilters(AVPlayer* player) const;
    bool registerVideoFilter(Filter *filter, AVPlayer *player);
    QList<Filter*> videoFilters(AVPlayer* player) const;
    bool unregisterFilter(Filter *filter);
    // async. release filter until filter is removed from it's target.filters
    bool releaseFilter(Filter *filter);
    bool uninstallFilter(Filter *filter);

    // TODO: use QMetaObject::invokeMethod
    void emitOnUninstallInTargetDone(Filter *filter);
signals:
    // invoked in UninstallFilterTask in AVThread.
    void uninstallInTargetDone(Filter *filter);

private slots:
    // queued connected to uninstallInTargetDone(). also release filter if release was requested
    void onUninstallInTargetDone(Filter *filter);

private:
    //convenient function to uninstall the filter. used by releaseFilter
    bool releaseFilterNow(Filter *filter);
    FilterManager();
    ~FilterManager();

    DPTR_DECLARE(FilterManager)
};

} //namespace QtAV

#endif // QTAV_FILTERMANAGER_H
