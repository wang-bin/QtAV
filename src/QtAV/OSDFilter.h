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

#ifndef QTAV_OSDFILTER_H
#define QTAV_OSDFILTER_H

#include <QtAV/Filter.h>
#include <QtAV/OSD.h>
#include <QtAV/FilterContext.h>
namespace QtAV {

class OSDFilterPrivate;
//TODO: not template. OSDFilter : public Filter, public OSD
class OSDFilter : public Filter, public OSD
{
protected:
    DPTR_DECLARE_PRIVATE(OSDFilter)
    OSDFilter(OSDFilterPrivate& d);
};

template<class Context>
class OSDFilterImpl : public OSDFilter
{
    //DPTR_DECLARE_PRIVATE(OSDFilterImpl<Context>)
public:
    OSDFilterImpl();
    ~OSDFilterImpl() {}
    FilterContext::Type contextType() const {
        return (FilterContext::Type)TypeTrait<Context>::type;
    }
protected:
    virtual void process();
};

typedef OSDFilterImpl<QPainterFilterContext> OSDFilterQPainter;
typedef OSDFilterImpl<GLFilterContext> OSDFilterGL;

} //namespace QtAV

#endif // QTAV_OSDFILTER_H
