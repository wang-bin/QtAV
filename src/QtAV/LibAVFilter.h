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

#ifndef QTAV_LIBAVFILTER_H
#define QTAV_LIBAVFILTER_H

#include <QtAV/Filter.h>

namespace QtAV {

class LibAVFilterPrivate;
class Q_AV_EXPORT LibAVFilter : public Filter
{
    DPTR_DECLARE_PRIVATE(LibAVFilter)
public:
    LibAVFilter();
    virtual ~LibAVFilter();

    //store last options. disable then config graph, then enable or restore last options
    bool setOptions(const QString& options);
    QString options() const;

protected:
    virtual void process(Statistics *statistics, Frame *frame);
};

} //namespace QtAV

#endif // QTAV_LIBAVFILTER_H
