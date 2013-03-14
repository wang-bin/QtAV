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

#ifndef QTAV_OSDFILTER_P_H
#define QTAV_OSDFILTER_P_H

#include "Filter_p.h"
#include "QtAV/OSDFilter.h"

namespace QtAV {

class OSDFilterPrivate : public FilterPrivate
{
public:
    OSDFilterPrivate():
        show_type(OSDFilter::ShowCurrentAndTotalTime)
      , width(0)
      , height(0)
      , sec_current(0)
      , sec_total(0)
    {
    }
    virtual ~OSDFilterPrivate() {}
    OSDFilter::ShowType show_type;
    int width, height;
    int sec_current, sec_total;
};

} //namespace QtAV
#endif // QTAV_OSDFILTER_P_H
