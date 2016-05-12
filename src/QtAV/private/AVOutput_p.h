/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
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
class VideoFilterContext;
class Statistics;
class OutputSet;
class Q_AV_PRIVATE_EXPORT AVOutputPrivate : public DPtrPrivate<AVOutput>
{
public:
    AVOutputPrivate():
        paused(false)
      , available(true)
      , statistics(0)
      , filter_context(0)
    {}
    virtual ~AVOutputPrivate();

    bool paused;
    bool available;
    QMutex mutex; //pause
    QWaitCondition cond; //pause

    //paintEvent is in main thread, copy it(only dynamic information) is better.
    //the static data are copied from AVPlayer when open
    Statistics *statistics; //do not own the ptr. just use AVPlayer's statistics ptr
    VideoFilterContext *filter_context; //create internally by the renderer with correct type
    QList<Filter*> filters;
    QList<Filter*> pending_uninstall_filters;
    QList<OutputSet*> output_sets;
};

} //namespace QtAV
#endif // QTAV_AVOUTPUT_P_H
