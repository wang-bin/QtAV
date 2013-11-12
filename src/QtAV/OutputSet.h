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

#ifndef QTAV_OUTPUTSET_H
#define QTAV_OUTPUTSET_H

#include <QtCore/QObject>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtAV/QtAV_Global.h>
#include <QtAV/AVOutput.h>

namespace QtAV {

class AVPlayer;
class VideoFrame;
class Q_AV_EXPORT OutputSet : public QObject
{
    Q_OBJECT
public:
    OutputSet(AVPlayer *player);
    virtual ~OutputSet();

    //required when accessing renderers
    void lock();
    void unlock();

    //implicity shared
    //QList<AVOutput*> outputs();
    QList<AVOutput*> outputs();

    //each(OutputOperation(data))
    //
    void sendData(const QByteArray& data);
    void sendVideoFrame(const VideoFrame& frame);

    void clearOutputs();
    void addOutput(AVOutput* output);

    void notifyPauseChange(AVOutput *output);
    bool canPauseThread() const;
    //in AVThread
    bool pauseThread(int timeout = ULONG_MAX); //There are 2 ways to pause AVThread: 1. pause thread directly. 2. pause all outputs
    /*
     * in user thread when pause count < set size.
     * 1. AVPlayer.pause(false) in player thread then call each output pause(false)
     * 2. shortcut for AVOutput.pause(false)
     */
    void resumeThread();

public slots:
    //connect to renderer->aboutToClose(). test whether delete on close
    void removeOutput(AVOutput *output);

private:
    volatile bool mCanPauseThread;
    AVPlayer *mpPlayer;
    int mPauseCount; //pause AVThread if equals to mOutputs.size()
    QList<AVOutput*> mOutputs;
    QMutex mMutex;
    QWaitCondition mCond; //pause
};

} //namespace QtAV

#endif // QTAV_OUTPUTSET_H
