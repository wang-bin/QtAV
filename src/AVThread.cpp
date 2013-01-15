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

#include <QtAV/AVThread.h>
#include <private/AVThread_p.h>

namespace QtAV {
AVThread::AVThread(QObject *parent) :
    QThread(parent)
{
}

AVThread::AVThread(AVThreadPrivate &d, QObject *parent)
    :QThread(parent),DPTR_INIT(&d)
{
}

AVThread::~AVThread()
{
    //d_ptr destroyed automatically
}

void AVThread::stop()
{
    DPTR_D(AVThread);
    if (d.writer)
        d.writer->pause(false); //stop waiting
    d.stop = true;
    //terminate();
    d.packets.setBlocking(false); //stop blocking take()
    d.packets.clear();
}

void AVThread::setClock(AVClock *clock)
{
    d_func().clock = clock;
}

AVClock* AVThread::clock() const
{
    return d_func().clock;
}

PacketQueue* AVThread::packetQueue() const
{
    return const_cast<PacketQueue*>(&d_func().packets);
}

void AVThread::setDecoder(AVDecoder *decoder)
{
    d_func().dec = decoder;
}

AVDecoder* AVThread::decoder() const
{
    return d_func().dec;
}

void AVThread::setOutput(AVOutput *out)
{
    d_func().writer = out;
}

AVOutput* AVThread::output() const
{
    return d_func().writer;
}

void AVThread::setDemuxEnded(bool ended)
{
    d_func().demux_end = ended;
}

void AVThread::resetState()
{
    DPTR_D(AVThread);
    if (d.writer)
        d.writer->pause(false); //stop waiting. Important when replay
    d.stop = false;
    d.demux_end = false;
    d.packets.setBlocking(true);
    d.packets.clear();
}

} //namespace QtAV
