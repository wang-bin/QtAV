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

#ifndef QAV_AVOUTPUT_H
#define QAV_AVOUTPUT_H

#include <QtCore/QByteArray>
#include <QtAV/QtAV_Global.h>

/*!
 * TODO: add api id(), name(), detail()
 */

namespace QtAV {

class AVDecoder;
class AVOutputPrivate;
class Filter;
class FilterContext;
class Statistics;
class OutputSet;
class Q_AV_EXPORT AVOutput
{
    DPTR_DECLARE_PRIVATE(AVOutput)
public:
    AVOutput();
    virtual ~AVOutput();
    bool isAvailable() const;

    //void addSource(AVPlayer* player); //call player.addVideoRenderer(this)
    //void removeSource(AVPlayer* player);

    Q_DECL_DEPRECATED virtual bool open() = 0;
    Q_DECL_DEPRECATED virtual bool close() = 0;

//    virtual bool prepare() {}
//    virtual bool finish() {}
    //Demuxer thread automatically paused because packets will be full
    //only pause the renderering, the thread going on. If all outputs are paused, then pause the thread(OutputSet.tryPause)
    void pause(bool p); //processEvents when waiting?
    bool isPaused() const;

    /* check context.type, if not compatible(e.g. type is QtPainter but vo is d2d)
     * but type is not same is also ok if we just use render engine in vo but not in context.
     * TODO:
     * what if multiple vo(different render engines) share 1 player?
     * private?: set in AVThread, context is used by this class internally
     */
    virtual int filterContextType() const;
    //No filters() api, they are used internally?
    QList<Filter*>& filters();
    bool installFilter(Filter *filter);
    bool uninstallFilter(Filter *filter);
protected:
    AVOutput(AVOutputPrivate& d);
    /*
     * If the pause state is true setted by pause(true), then block the thread and wait for pause state changed, i.e. pause(false)
     * and return true. Otherwise, return false immediatly.
     */
    Q_DECL_DEPRECATED bool tryPause(); //move to OutputSet
    //TODO: we need an active set
    void addOutputSet(OutputSet *set);
    void removeOutputSet(OutputSet *set);
    void attach(OutputSet *set); //add this to set
    void detach(OutputSet *set = 0); //detatch from (all, if 0) output set(s)
    // for thread safe
    void hanlePendingTasks();

    DPTR_DECLARE(AVOutput)

private:
    // for proxy VideoOutput
    virtual void setStatistics(Statistics* statistics); //called by friend AVPlayer
    virtual bool onInstallFilter(Filter *filter);
    virtual bool onUninstallFilter(Filter *filter);
    virtual void onAddOutputSet(OutputSet *set);
    virtual void onRemoveOutputSet(OutputSet *set);
    virtual void onAttach(OutputSet *set); //add this to set
    virtual void onDetach(OutputSet *set = 0); //detatch from (all, if 0) output set(s)
    // only called in handlePaintEvent. But filters may change. so required by proxy to update it's filters
    virtual bool onHanlePendingTasks(); //return true: proxy update filters
    friend class AVPlayer;
    friend class OutputSet;
    friend class VideoOutput;
};

} //namespace QtAV
#endif //QAV_AVOUTPUT_H
