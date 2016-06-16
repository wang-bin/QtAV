/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2013)

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

#include "output/OutputSet.h"
#include "QtAV/AVPlayer.h"
#include "QtAV/VideoRenderer.h"

namespace QtAV {

OutputSet::OutputSet(AVPlayer *player):
    QObject(player)
  , mCanPauseThread(false)
  , mpPlayer(player)
  , mPauseCount(0)
{
}

OutputSet::~OutputSet()
{
    mCond.wakeAll();
    //delete? may be deleted by vo's parent
    clearOutputs();
}

void OutputSet::lock()
{
    mMutex.lock();
}

void OutputSet::unlock()
{
    mMutex.unlock();
}

QList<AVOutput *> OutputSet::outputs()
{
    return mOutputs;
}

void OutputSet::sendVideoFrame(const VideoFrame &frame)
{
    if (mOutputs.isEmpty())
        return;
    VideoFrame f(frame);
    foreach(AVOutput *output, mOutputs) {
        if (!output->isAvailable())
            continue;
        VideoRenderer *vo = static_cast<VideoRenderer*>(output);
        // TODO: sort vo by supported formats when a new vo is added to reduce convertion
        if (!vo->isSupported(frame.pixelFormat()))
            f = frame.to(vo->preferredPixelFormat());
        vo->receive(f);
    }
}

void OutputSet::clearOutputs()
{
    QMutexLocker lock(&mMutex);
    Q_UNUSED(lock);
    if (mOutputs.isEmpty())
        return;
    foreach(AVOutput *output, mOutputs) {
        output->removeOutputSet(this);
    }
    mOutputs.clear();
}

void OutputSet::addOutput(AVOutput *output)
{
    QMutexLocker lock(&mMutex);
    Q_UNUSED(lock);
    mOutputs.append(output);
    output->addOutputSet(this);
}

void OutputSet::removeOutput(AVOutput *output)
{
    QMutexLocker lock(&mMutex);
    Q_UNUSED(lock);
    mOutputs.removeAll(output);
    output->removeOutputSet(this);
}

void OutputSet::notifyPauseChange(AVOutput *output)
{
    if (output->isPaused()) {
        mPauseCount++;
        if (mPauseCount == mOutputs.size()) {
            mCanPauseThread = true;
        }
        //DO NOT pause here because it must be paused in AVThread
    } else {
        mPauseCount--;
        mCanPauseThread = false;
        if (mPauseCount == mOutputs.size() - 1) {
            resumeThread();
        }
    }
}

bool OutputSet::canPauseThread() const
{
    return mCanPauseThread;
}

bool OutputSet::pauseThread(unsigned long timeout)
{
    QMutexLocker lock(&mMutex);
    Q_UNUSED(lock);
    return mCond.wait(&mMutex, timeout);
}

void OutputSet::resumeThread()
{
    mCond.wakeAll();
}

} //namespace QtAV
