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

#ifndef QTAV_FILTER_P_H
#define QTAV_FILTER_P_H

#include <QtAV/QtAV_Global.h>

namespace QtAV {

class Filter;
class FilterContext;
class Statistics;
class FilterPrivate : public DPtrPrivate<Filter>
{
public:
    FilterPrivate():
        enabled(true)
      , context(0)
      , statistics(0)
      , opacity(1)
    {}
    virtual ~FilterPrivate() {}

    bool enabled;
    FilterContext *context; //used only when is necessary
    Statistics *statistics;
    qreal opacity;
};

} //namespace QtAV

#endif // QTAV_FILTER_P_H
