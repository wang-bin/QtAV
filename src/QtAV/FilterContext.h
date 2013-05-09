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

#ifndef QTAV_FILTERCONTEXT_H
#define QTAV_FILTERCONTEXT_H

#include <QtAV/QtAV_Global.h>
#include <QtCore/QByteArray>
#include <QtCore/QRect>
/*
 * QPainterFilterContext, D2DFilterContext, ...
 */

class QPainter;
namespace QtAV {

class FilterContext;
class FilterPrivate;
class Q_EXPORT FilterContext
{
public:
    enum Type { ////audio and video...
        QtPainter,
        OpenGL,
        Direct2D,
        GdiPlus,
        XV,
        None
    };
    static FilterContext* create(Type t);
    virtual ~FilterContext();
    virtual Type type() const = 0;
    QByteArray data;
    //QPainter, paintdevice, surface etc. contains all of them here?
};

class Q_EXPORT VideoFilterContext : public FilterContext
{
public:
    VideoFilterContext();
    QRect rect;
};

class Q_EXPORT QPainterFilterContext : public VideoFilterContext
{
public:
    QPainter *painter;
    virtual Type type() const; //QtPainter
};


} //namespace QtAV

#endif // QTAV_FILTERCONTEXT_H
