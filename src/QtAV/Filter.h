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
 *
 * example:
 *    using namespace QtAV;
 *    Filter *f = new XXFilter();
 *    f.installTo(player);
 *    ...
 *    //f.uninstall() //opional. call it on you need
 *    safeReleaseFilter(&f);
 *    ...
 */

class QByteArray;
namespace QtAV {

class Filter;
/*
 * convenient function to release a filter
 * DO NOT delete a filter call safeReleaseFilter(&filter) instead
 * It will uninstall internally and filter to null
 */
Q_AV_EXPORT void safeReleaseFilter(Filter** ppFilter);

class AVOutput;
class AVPlayer;
class FilterPrivate;
class Statistics;
class Frame;
// TODO: QObject?
class Q_AV_EXPORT Filter
{
    DPTR_DECLARE_PRIVATE(Filter)
public:
    Filter();
    virtual ~Filter();
    //isEnabled() then setContext
    //TODO: parameter FrameContext
    void setEnabled(bool enabled); //AVComponent.enabled
    bool isEnabled() const;

    FilterContext* context();
    virtual FilterContext::Type contextType() const;

    /*
     * filter.installTo(target,...) calls target.installFilter(filter)
     * If filter is already registered in FilterManager, then return false
     * Otherwise, call FilterManager.register(filter) and target.filters.push_back(filter), return true
     * NOTE: the installed filter will be deleted by the target!
     */
    // filter on output. e.g. subtitle
    bool installTo(AVOutput *output);
    bool installToAudioThread(AVPlayer *player);
    bool installToVideoThread(AVPlayer *player);

    /*
     *
     */
    bool uninstall();
    /*!
     * check context and apply the filter
     * if context is null, or contextType() != context->type(), then create a right one and assign it to context.
     */
    void process(FilterContext *&context, Statistics* statistics, Frame* frame = 0);

protected:
    /*
     * If the filter is in AVThread, it's safe to operate on ref.
     */
    Filter(FilterPrivate& d);
    virtual void process();
    virtual void process(Statistics* statistics, Frame* frame);

    DPTR_DECLARE(Filter)
};

} //namespace QtAV

#endif // QTAV_FILTER_H
