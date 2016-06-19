/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

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
    //Demuxer thread automatically paused because packets will be full
    //only pause the renderering, the thread going on. If all outputs are paused, then pause the thread(OutputSet.tryPause)
    //TODO: what about audio's pause api?
    void pause(bool p); //processEvents when waiting?
    bool isPaused() const;
    QList<Filter*>& filters();
    /*!
     * \brief installFilter
     * Insert a filter at position 'index' of current filter list.
     * If the filter is already installed, it will move to the correct index.
     * \param index A nagative index == size() + index. If index >= size(), append at last
     * \return false if already installed
     */
    bool installFilter(Filter *filter, int index = 0x7fffffff);
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
    virtual bool onInstallFilter(Filter *filter, int index);
    virtual bool onUninstallFilter(Filter *filter);
    // only called in handlePaintEvent. But filters may change. so required by proxy to update it's filters
    virtual bool onHanlePendingTasks(); //return true: proxy update filters
    friend class AVPlayer;
    friend class OutputSet;
    friend class VideoOutput;
};

} //namespace QtAV
#endif //QAV_AVOUTPUT_H
