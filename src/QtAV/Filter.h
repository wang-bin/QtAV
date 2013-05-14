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
#include <QtAV/FilterContext.h>
/*
 * QPainterFilter, D2DFilter, ...
 *
 *TODO: force apply. e.g. an animation filter on vo, update vo and apply filter even not video is
 * playing.
 */

class QByteArray;
namespace QtAV {

class FilterPrivate;
class Statistics;
class Q_EXPORT Filter
{
    DPTR_DECLARE_PRIVATE(Filter)
public:
    virtual ~Filter();
    //isEnabled() then setContext
    //TODO: parameter FrameContext
    void setEnabled(bool enabled); //AVComponent.enabled
    bool isEnabled() const;

    qreal opacity() const;
    void setOpacity(qreal o);

    virtual FilterContext::Type contextType() const;

    /*!
     * check context and apply the filter
     * if context is null, or contextType() != context->type(), then create a right one and assign it to context.
     */
    void process(FilterContext *&context, Statistics* statistics, QByteArray* data = 0);

protected:
    /*
     * If the filter is in AVThread, it's safe to operate on ref.
     */
    Filter(FilterPrivate& d);
    virtual void process() = 0;

    DPTR_DECLARE(Filter)
};

} //namespace QtAV

#endif // QTAV_FILTER_H
