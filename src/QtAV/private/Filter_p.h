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

#ifndef QTAV_FILTER_P_H
#define QTAV_FILTER_P_H

#include <QtAV/QtAV_Global.h>

namespace QtAV {

class Filter;
class VideoFilterContext;
class Statistics;
class Q_AV_PRIVATE_EXPORT FilterPrivate : public DPtrPrivate<Filter>
{
public:
    FilterPrivate():
        enabled(true)
      , owned_by_target(false)
    {}
    virtual ~FilterPrivate() {}

    bool enabled;
    bool owned_by_target;
};

class Q_AV_PRIVATE_EXPORT VideoFilterPrivate : public FilterPrivate
{
public:
    VideoFilterPrivate() :
        context(0)
    {}
    VideoFilterContext *context; //used only when is necessary
};

class Q_AV_PRIVATE_EXPORT AudioFilterPrivate : public FilterPrivate
{
};

} //namespace QtAV

#endif // QTAV_FILTER_P_H
