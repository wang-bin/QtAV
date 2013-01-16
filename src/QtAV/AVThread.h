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
//TODO: pause functions. AVOutput may be null, use AVThread's pause state
namespace QtAV {

class AVDecoder;
class AVThreadPrivate;
class AVOutput;
class AVClock;
class Q_EXPORT AVThread : public QThread
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(AVThread)
public:
	explicit AVThread(QObject *parent = 0);
    virtual ~AVThread();

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

protected:
    AVThread(AVThreadPrivate& d, QObject *parent = 0);
    void resetState();
    void tryPause();

    DPTR_DECLARE(AVThread)
};
}

#endif // QTAV_AVTHREAD_H
