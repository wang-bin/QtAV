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
	QThread(parent),d_ptr(new AVThreadPrivate())
{
}

AVThread::AVThread(AVThreadPrivate &d, QObject *parent)
    :QThread(parent),d_ptr(&d)
{
}

AVThread::~AVThread()
{
}

void AVThread::stop()
{
    d_ptr->stop = true;
    //terminate();
    d_ptr->packets.setBlocking(false); //stop blocking take()
    d_ptr->packets.clear();
    //d_ptr->mutex.unlock(); //put it to run(). or unlock after terminate()
}

void AVThread::setClock(AVClock *clock)
{
    d_ptr->clock = clock;
}

AVClock* AVThread::clock() const
{
    return d_ptr->clock;
}

PacketQueue* AVThread::packetQueue() const
{
    return &d_ptr->packets;
}

void AVThread::setDecoder(AVDecoder *decoder)
{
    d_ptr->dec = decoder;
}

AVDecoder* AVThread::decoder() const
{
    return d_ptr->dec;
}

void AVThread::setOutput(AVOutput *out)
{
    d_ptr->writer = out;
}

AVOutput* AVThread::output() const
{
    return d_ptr->writer;
}

void AVThread::setDemuxEnded(bool ended)
{
    d_ptr->demux_end = ended;
}

void AVThread::resetState()
{
    d_ptr->stop = false;
    d_ptr->demux_end = false;
    d_ptr->packets.setBlocking(true);
    d_ptr->mutex.unlock();
}

} //namespace QtAV
