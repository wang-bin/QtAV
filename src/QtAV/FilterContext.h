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
class QPaintDevice;
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

    FilterContext();
    virtual ~FilterContext();
    virtual Type type() const = 0;
    //TODO: how to move them to VideoFilterContext?
    int video_width, video_height; //original size
    //QPainter, paintdevice, surface etc. contains all of them here?
protected:
    virtual void initializeOnData(QByteArray *data); //private?
    friend class Filter;
};

class Q_EXPORT VideoFilterContext : public FilterContext
{
public:
    VideoFilterContext();
    QRect rect;
};

//TODO: font, pen, brush etc?
class Q_EXPORT QPainterFilterContext : public VideoFilterContext
{
public:
    QPainterFilterContext();
    virtual ~QPainterFilterContext();

    QPainter *painter;
    /*
     * for the filters apply on decoded data, paint_device must be initialized once the data changes
     * can we allocate memory on stack?
     */
    QPaintDevice *paint_device;
    virtual Type type() const; //QtPainter
protected:
    virtual void initializeOnData(QByteArray* data);
};

class Q_EXPORT GLFilterContext : public VideoFilterContext
{
public:
    GLFilterContext();
    virtual ~GLFilterContext();
    virtual Type type() const; //OpenGL
    QPaintDevice *paint_device;
};

template<class T> struct TypeTrait {};
template<> struct TypeTrait<QPainterFilterContext> {
    enum {
        type = FilterContext::QtPainter
    };
};
template<> struct TypeTrait<GLFilterContext> {
    enum {
        type = FilterContext::OpenGL
    };
};

} //namespace QtAV

#endif // QTAV_FILTERCONTEXT_H
