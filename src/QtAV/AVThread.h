/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012 Wang Bin <wbsecg1@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#ifndef QTAV_AVTHREAD_H
#define QTAV_AVTHREAD_H

#include <QtCore/QThread>
#include <QtCore/QScopedPointer>
#include <QtAV/Packet.h>

namespace QtAV {

class AVDecoder;
class AVThreadPrivate;
class AVOutput;
class AVClock;
class AVDemuxThread;
class Q_EXPORT AVThread : public QThread
{
    Q_OBJECT
public:
	explicit AVThread(QObject *parent = 0);
    virtual ~AVThread();

    virtual void stop();

    void setClock(AVClock *clock);
    AVClock* clock() const;

    //void setPacketQueue(PacketQueue *queue);
    PacketQueue* packetQueue() const;

    void setDecoder(AVDecoder *decoder);
    AVDecoder *decoder() const;

    void setOutput(AVOutput *out);
    AVOutput* output() const;

private:
    friend class AVDemuxThread;
    //notify demux thread to read more packets. call it internally when demux thread sets the avthread
   void setDemuxThread(AVDemuxThread *thread);
   AVDemuxThread* demuxThread() const;

protected:
    AVThread(AVThreadPrivate& d, QObject *parent = 0);

    Q_DECLARE_PRIVATE(AVThread)
    QScopedPointer<AVThreadPrivate> d_ptr;
};
}

#endif // QTAV_AVTHREAD_H
