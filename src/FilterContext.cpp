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
    case OpenGL:
        ctx = new GLFilterContext();
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

VideoFilterContext::VideoFilterContext()
    : rect(32, 32, 0, 0)
    , painter(0)
    , paint_device(0)
    , own_paint_device(false)
{
}

VideoFilterContext::~VideoFilterContext()
{
    if (paint_device) {
        if (painter) { //painter may assigned by vo
            if (painter->isActive())
                painter->end();
            qDebug("delete painter");
            delete painter;
            painter = 0;
        }
        qDebug("delete paint device %p in %p", paint_device, this);
        if (own_paint_device)
            delete paint_device; //delete recursively for widget
        paint_device = 0;
    }
}

FilterContext::Type QPainterFilterContext::type() const
{
    return FilterContext::QtPainter;
}

void QPainterFilterContext::initializeOnData(QByteArray *data)
{
    if (!data) {
        if (!paint_device) {
            return;
        }
        if (!painter) {
            painter = new QPainter(); //warning: more than 1 painter on 1 device
        }
        painter->begin(paint_device);
        return;
    }
    if (data->isEmpty())
        return;
    if (paint_device) {
        if (painter && painter->isActive()) {
            painter->end(); //destroy a paint device that is being painted is not allowed!
        }
        delete paint_device;
        paint_device = 0;
    }
    Q_ASSERT(video_width > 0 && video_height > 0);
    paint_device = new QImage((uchar*)data->data(), video_width, video_height, QImage::Format_RGB32);
    if (!painter)
        painter = new QPainter();
    own_paint_device = true; //TODO: what about renderer is not a widget?
    painter->begin((QImage*)paint_device);
}

FilterContext::Type GLFilterContext::type() const
{
    return FilterContext::OpenGL;
}

} //namespace QtAV
