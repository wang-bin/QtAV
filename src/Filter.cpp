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

#include "QtAV/Filter.h"
#include "private/Filter_p.h"
#include "QtAV/Statistics.h"

namespace QtAV {

Filter::Filter(FilterPrivate &d)
    :DPTR_INIT(&d)
{

}

Filter::~Filter()
{
}

void Filter::process(FilterContext *&context, Statistics *statistics, QByteArray* data)
{
    DPTR_D(Filter);
    if(context)
        context->type();
    if (!context || context->type() != contextType()) {
        if (context) {
            qDebug("incompatible context type");
            //we don't delete it because the context MUST be used by other filters
            //TODO: How to release them at the end? refcount?
        } else {
            qDebug("null context");
        }
        //use internal context
        if (d.context) {
            qDebug("replace context with internal one");
            context = d.context;
        } else {
            context = FilterContext::create(contextType());
            qDebug("new filter context=%p, type=%d", context, contextType());
            context->video_width = statistics->video_only.width;
            context->video_height = statistics->video_only.height;
        }
    }
    if (data)
        context->initializeOnData(data);
    d.context = context;
    d.statistics = statistics;
    process();
}

void Filter::setEnabled(bool enabled)
{
    DPTR_D(Filter);
    d.enabled = enabled;
}

bool Filter::isEnabled() const
{
    DPTR_D(const Filter);
    return d.enabled;
}

void Filter::setOpacity(qreal o)
{
    DPTR_D(Filter);
    d.opacity = o;
}

qreal Filter::opacity() const
{
    return d_func().opacity;
}

FilterContext::Type Filter::contextType() const
{
    return FilterContext::None;
}

} //namespace QtAV
