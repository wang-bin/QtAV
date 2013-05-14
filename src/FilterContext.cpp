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

#include "QtAV/FilterContext.h"
#include <QtGui/QImage>
#include <QtGui/QPainter>

namespace QtAV {

FilterContext* FilterContext::create(Type t)
{
    FilterContext *ctx = 0;
    switch (t) {
    case QtPainter:
        ctx = new QPainterFilterContext();
        break;
    default:
        break;
    }
    return ctx;
}

FilterContext::FilterContext():
    video_width(0)
  , video_height(0)
{
}

FilterContext::~FilterContext()
{
}

void FilterContext::initializeOnData(QByteArray *data)
{
    Q_UNUSED(data);
}

VideoFilterContext::VideoFilterContext():
    rect(32, 32, 0, 0)
{
}

QPainterFilterContext::QPainterFilterContext():
    painter(0)
  , paint_device(0)
{
}

QPainterFilterContext::~QPainterFilterContext()
{
    if (paint_device) {
        if (painter) { //painter may assigned by vo
            painter->end();
            qDebug("delete painter");
            delete painter;
            painter = 0;
        }
        qDebug("delete paint device");
        delete paint_device;
        paint_device = 0;
    }
}

FilterContext::Type QPainterFilterContext::type() const
{
    return FilterContext::QtPainter;
}

void QPainterFilterContext::initializeOnData(QByteArray *data)
{
    Q_ASSERT(video_width > 0 && video_height > 0);
    if (!data)
        return;
    if (data->isEmpty())
        return;
    if (paint_device) {
        if (painter) {
            painter->end(); //destroy a paint device that is being painted is not allowed!
        }
        delete paint_device;
        paint_device = 0;
    }
    paint_device = new QImage((uchar*)data->data(), video_width, video_height, QImage::Format_RGB32);
    if (!painter)
        painter = new QPainter();
    painter->begin((QImage*)paint_device);
}


} //namespace QtAV
