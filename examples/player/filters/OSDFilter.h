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

#ifndef QTAV_OSDFILTER_H
#define QTAV_OSDFILTER_H

#include <QtAV/Filter.h>
#include <QtAV/FilterContext.h>
#include "OSD.h"

using namespace QtAV;
class OSDFilter : public VideoFilter, public OSD
{
public:
    OSDFilter(QObject *parent = 0);
    bool isSupported(VideoFilterContext::Type ct) const {
        return ct == VideoFilterContext::QtPainter || ct == VideoFilterContext::X11;
    }
protected:
    void process(Statistics* statistics, VideoFrame* frame);
};

#endif // QTAV_OSDFILTER_H
