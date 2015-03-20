/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2015 Wang Bin <wbsecg1@gmail.com>

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

#include "PacketBuffer.h"

namespace QtAV {

PacketBuffer::PacketBuffer()
    : m_mode(BufferTime)
    , m_buffering(true) // in buffering state at the beginning
    , m_max(1.5)
    , m_buffer(0)
    , m_value0(0)
    , m_value1(0)
{
}

PacketBuffer::~PacketBuffer()
{
}

void PacketBuffer::setBufferMode(BufferMode mode)
{
    m_mode = mode;
    if (queue.isEmpty()) {
        m_value0 = m_value1 = 0;
        return;
    }
    if (m_mode == BufferTime) {
        m_value0 = int(queue[0].pts*1000.0);
    } else {
        m_value0 = 0;
    }
}

BufferMode PacketBuffer::bufferMode() const
{
    return m_mode;
}

void PacketBuffer::setBufferValue(int value)
{
    m_buffer = value;
}

int PacketBuffer::bufferValue() const
{
    return m_buffer;
}

void PacketBuffer::setBufferMax(qreal max)
{
    if (max < 1.0) {
        qWarning("max (%f) must >= 1.0", max);
        return;
    }
    m_max = max;
}

qreal PacketBuffer::bufferMax() const
{
    return m_max;
}

int PacketBuffer::buffered() const
{
    Q_ASSERT(m_value1 >= m_value0);
    return m_value1 - m_value0;
}

bool PacketBuffer::isBuffering() const
{
    return m_buffering;
}

qreal PacketBuffer::bufferProgress() const
{
    const qreal p = qreal(buffered())/qreal(bufferValue());
    return qMax(qMin(p, 1.0), 0.0);
}

bool PacketBuffer::checkEnough() const
{
    return buffered() >= bufferValue();
}

bool PacketBuffer::checkFull() const
{
    return buffered() >= int(qreal(bufferValue())*bufferMax());
}

void PacketBuffer::onPut(const Packet &p)
{
    if (m_mode == BufferTime) {
        m_value1 = int(p.pts*1000.0); // FIXME: what if no pts
        m_value0 = int(queue[0].pts*1000.0); // must compute here because it is reset to 0 if take from empty
        //if (isBuffering())
          //  qDebug("+buffering progress: %.1f%%=%.1f/%.1f~%.1fs %d-%d", bufferProgress()*100.0, (qreal)buffered()/1000.0, (qreal)bufferValue()/1000.0, qreal(bufferValue())*bufferMax()/1000.0, m_value1, m_value0);
    } else if (m_mode == BufferBytes) {
        m_value1 += p.data.size();
    } else {
        m_value1++;
    }
    // TODO: compute buffer speed (and auto set the best bufferValue)
    if (!m_buffering)
        return;
    if (checkEnough()) {
        m_buffering = false;
    }
}

void PacketBuffer::onTake(const Packet &p)
{
    if (checkEmpty()) {
        m_buffering = true;
    }
    if (queue.isEmpty()) {
        m_value0 = 0;
        m_value1 = 0;
        return;
    }
    if (m_mode == BufferTime) {
        m_value0 = int(queue[0].pts*1000.0);
        //if (isBuffering())
          //  qDebug("-buffering progress: %.1f=%.1f/%.1fs", bufferProgress(), (qreal)buffered()/1000.0, (qreal)bufferValue()/1000.0);
    } else if (m_mode == BufferBytes) {
        m_value1 -= p.data.size();
        m_value1 = qMax(0, m_value1);
    } else {
        m_value1--;
    }
}

} //namespace QtAV
