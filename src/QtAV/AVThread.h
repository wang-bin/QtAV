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

#ifndef QTAV_AVTHREAD_H
#define QTAV_AVTHREAD_H

#include <QtCore/QThread>
#include <QtCore/QScopedPointer>
#include <QtAV/Packet.h>
//TODO: pause functions. AVOutput may be null, use AVThread's pause state
namespace QtAV {

class AVDecoder;
class AVThreadPrivate;
class AVOutput;
class AVClock;
class Statistics;
class Q_EXPORT AVThread : public QThread
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(AVThread)
public:
	explicit AVThread(QObject *parent = 0);
    virtual ~AVThread();

    //used for changing some components when running
    void lock();
    void unlock();

    void setClock(AVClock *clock);
    AVClock* clock() const;

    //void setPacketQueue(PacketQueue *queue);
    PacketQueue* packetQueue() const;

    void setDecoder(AVDecoder *decoder);
    AVDecoder *decoder() const;

    void setOutput(AVOutput *out);
    AVOutput* output() const;

    void setDemuxEnded(bool ended);

    bool isPaused() const;
public slots:
    virtual void stop();
    /*change pause state. the pause/continue action will do in the next loop*/
    void pause(bool p); //processEvents when waiting?
    void nextAndPause(); //process 1 frame and pause

protected:
    AVThread(AVThreadPrivate& d, QObject *parent = 0);
    void resetState();
    /*
     * If the pause state is true setted by pause(true), then block the thread and wait for pause state changed, i.e. pause(false)
     * and return true. Otherwise, return false immediatly.
     */
    bool tryPause();

    DPTR_DECLARE(AVThread)

private:
    void setStatistics(Statistics* statistics);
    friend class AVPlayer;
};
}

#endif // QTAV_AVTHREAD_H
