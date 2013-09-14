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

#include <QWidget>
#include "QtAV/AVPlayer.h"
#include "QtAV/OutputSet.h"

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
    qDebug("%s", __FUNCTION__);
    mCond.wakeAll();
    //delete? may be deleted by vo's parent
    clearOutputs();
    qDebug("%s", __FUNCTION__);
}

void OutputSet::lock()
{
    mMutex.lock();
}

void OutputSet::unlock()
{
    mMutex.unlock();
}

QList<AVOutput *> &OutputSet::outputs()
{
    return mOutputs;
}

void OutputSet::sendData(const QByteArray &data)
{
    QMutexLocker lock(&mMutex);
    Q_UNUSED(lock);
    foreach(AVOutput *output, mOutputs) {
        if (!output->isAvailable())
            continue;
        output->writeData(data);
    }
}

void OutputSet::clearOutputs()
{
    QMutexLocker lock(&mMutex);
    Q_UNUSED(lock);
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
/*
    QWidget *vw = renderer->widget();
    if (vw) {
        vw->hide();
        if (vw->testAttribute(Qt::WA_DeleteOnClose)) {
            vw->setParent(0); //avoid delete multiple times?
            delete vw; //do not delete. it may be owned by multiple groups
        }
    }
*/
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

void OutputSet::pauseThread()
{
    QMutexLocker lock(&mMutex);
    Q_UNUSED(lock);
    mCond.wait(&mMutex);
}

void OutputSet::resumeThread()
{
    mCond.wakeAll();
}

} //namespace QtAV
