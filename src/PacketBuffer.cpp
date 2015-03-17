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

PacketBuffer::BufferMode PacketBuffer::bufferMode() const
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

int PacketBuffer::buffered() const
{
    Q_ASSERT(m_value1 > m_value0);
    return m_value1 - m_value0;
}

qreal PacketBuffer::bufferProgress() const
{
    const qreal p = qreal(buffered())/qreal(bufferValue());
    return qMax(qMin(p, 1.0), 0.0);
}

bool PacketBuffer::checkEnough() const
{
    return m_value1 - m_value0 >= m_buffer;
}

bool PacketBuffer::checkFull() const
{
    return m_value1 - m_value0 >= 2*m_buffer;
}

void PacketBuffer::onPut(const Packet &p)
{
    if (m_mode == BufferTime) {
        m_value1 = int(p.pts*1000.0);
    } else if (m_mode == BufferBytes) {
        m_value1 += p.data.size();
    } else {
        m_value1++;
    }
}

void PacketBuffer::onTake(const Packet &p)
{
    if (queue.isEmpty()) {
        m_value0 = 0;
        m_value1 = 0;
        return;
    }
    if (m_mode == BufferTime) {
        m_value0 = int(queue[0].pts*1000.0);
    } else if (m_mode == BufferBytes) {
        m_value1 -= p.data.size();
        m_value1 = qMax(0, m_value1);
    } else {
        m_value1--;
    }
}

} //namespace QtAV
