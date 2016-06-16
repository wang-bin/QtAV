/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2015)

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
#include "utils/ring.h"

namespace QtAV {

/*
 * take empty: start buffering, block at next take if still empty
 * take enough: start to put more packets
 * put enough: end buffering, end take block
 * put full: stop putting more packets
 */
class PacketBuffer : public BlockingQueue<Packet, QQueue>
{
public:
    PacketBuffer();
    ~PacketBuffer();

    void setBufferMode(BufferMode mode);
    BufferMode bufferMode() const;
    /*!
     * \brief setBufferValue
     * Ensure the buffered msecs/bytes/packets in queue is at least the given value
     * \param value BufferBytes: bytes, BufferTime: msecs, BufferPackets: packets count
     */
    void setBufferValue(qint64 value);
    qint64 bufferValue() const;
    /*!
     * \brief setBufferMax
     * stop buffering if max value reached. Real value is bufferValue()*bufferMax()
     * \param max the ratio to bufferValue(). always >= 1.0
     */
    void setBufferMax(qreal max);
    qreal bufferMax() const;
    /*!
     * \brief buffered
     * Current buffered value in the queue
     */
    qint64 buffered() const;
    bool isBuffering() const;
    /*!
     * \brief bufferProgress
     * How much of the data buffer is currently filled, from 0.0 (empty) to 1.0 (enough or full).
     * bufferProgress() can be less than 1 if not isBuffering();
     * \return Percent of buffered time, bytes or packets.
     */
    qreal bufferProgress() const;
    /*!
     * \brief bufferSpeed
     * Depending on BufferMode, the result is delta_pts/s, packets/s or bytes/s
     * \return
     */
    qreal bufferSpeed() const;
    qreal bufferSpeedInBytes() const;
protected:
    bool checkEnough() const Q_DECL_OVERRIDE;
    bool checkFull() const Q_DECL_OVERRIDE;
    void onTake(const Packet &) Q_DECL_OVERRIDE;
    void onPut(const Packet &) Q_DECL_OVERRIDE;
protected:
    typedef BlockingQueue<Packet, QQueue> PQ;
    using PQ::setCapacity;
    using PQ::setThreshold;
    using PQ::capacity;
    using PQ::threshold;

private:
    qreal calc_speed(bool use_bytes) const;

    BufferMode m_mode;
    bool m_buffering;
    qreal m_max;
    // bytes or count
    qint64 m_buffer;
    qint64 m_value0, m_value1;
    typedef struct {
        qint64 v; //pts, total packes or total bytes
        qint64 bytes; //total bytes
        qint64 t;
    } BufferInfo;
    ring<BufferInfo> m_history;
};

} //namespace QtAV
#endif // QTAV_PACKETBUFFER_H
