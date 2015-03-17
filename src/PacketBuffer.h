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

#ifndef QTAV_PACKETBUFFER_H
#define QTAV_PACKETBUFFER_H

#include <QtCore/QQueue>
#include <QtAV/Packet.h>
#include "utils/BlockingQueue.h"

namespace QtAV {

class PacketBuffer : BlockingQueue<Packet, QQueue>
{
public:
    enum BufferMode {
        BufferTime,
        BufferBytes,
        BufferPackets
    };

    PacketBuffer();
    ~PacketBuffer();

    void setBufferMode(BufferMode mode);
    BufferMode bufferMode() const;
    /*!
     * \brief setBufferValue
     * Ensure the buffered msecs/bytes/packets in queue is at least the given value
     * \param value BufferBytes: bytes, BufferTime: msecs, BufferPackets: packets count
     */
    void setBufferValue(int value);
    int bufferValue() const;
    //void setBufferMax(int max);
    /*!
     * \brief buffered
     * Current buffered value in the queue
     */
    int buffered() const;
    /*!
     * \brief bufferProgress
     * How much of the data buffer is currently filled, from 0.0 (empty) to 1.0 (enough or full).
     * bufferProgress() == 1
     * \return Percent of buffered time, bytes or packets.
     */
    qreal bufferProgress() const;

protected:
    bool checkEnough() const Q_DECL_OVERRIDE;
    bool checkFull() const Q_DECL_OVERRIDE;
    void onTake(const Packet &) Q_DECL_OVERRIDE;
    void onPut(const Packet &) Q_DECL_OVERRIDE;
private:
    BufferMode m_mode;
    // bytes or count
    quint32 m_buffer;
    qint32 m_value0, m_value1;
};

} //namespace QtAV
#endif // QTAV_PACKETBUFFER_H
