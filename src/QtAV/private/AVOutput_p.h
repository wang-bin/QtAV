/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2013 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_AVOUTPUT_P_H
#define QTAV_AVOUTPUT_P_H

#include <QtCore/QList>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtAV/QtAV_Global.h>

namespace QtAV {

class AVOutput;
class AVDecoder;
class Filter;
class Q_EXPORT AVOutputPrivate : public DPtrPrivate<AVOutput>
{
public:
    AVOutputPrivate():
        paused(false)
      , available(true)
    {}
    virtual ~AVOutputPrivate() {
        cond.wakeAll(); //WHY: failed to wake up
    }

    bool paused;
    bool available;
    QMutex mutex; //pause
    QWaitCondition cond; //pause
    QByteArray data;
    QList<Filter*> filters;
};

} //namespace QtAV
#endif // QTAV_AVOUTPUT_P_H
