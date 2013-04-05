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

#ifndef QTAV_FILTER_H
#define QTAV_FILTER_H

#include <QtAV/QtAV_Global.h>

class QByteArray;
namespace QtAV {

class FilterPrivate;
class Q_EXPORT Filter
{
    DPTR_DECLARE_PRIVATE(Filter)
public:
    virtual ~Filter() = 0;

    //TODO: parameter FrameContext
protected:
    /*
     * If the filter is in AVThread, it's safe to operate on ref.
     */
    virtual void process(QByteArray& data) = 0;
    Filter(FilterPrivate& d);

    friend class AVThread;
    friend class VideoThread;
    DPTR_DECLARE(Filter)
};

} //namespace QtAV

#endif // QTAV_FILTER_H
