/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QAV_PACKET_H
#define QAV_PACKET_H

#include <QtCore/QSharedData>
#include <QtAV/QtAV_Global.h>

struct AVPacket;

namespace QtAV {

class PacketPrivate;
class Q_AV_EXPORT Packet
{
public:
    static Packet fromAVPacket(const AVPacket* avpkt, double time_base);
    static bool fromAVPacket(Packet *pkt, const AVPacket *avpkt, double time_base);
    static Packet createEOF();

    Packet();
    ~Packet();

    // required if no defination of PacketPrivate
    Packet(const Packet& other);
    Packet& operator =(const Packet& other);

    bool isEOF()const;
    inline bool isValid() const;
    /*!
     * \brief asAVPacket
     * If Packet is constructed from AVPacket, then data and properties are the same as that AVPacket.
     * Otherwise, Packet's data and properties are used and no side data.
     * Packet takes the owner ship. time unit is always ms even constructed from AVPacket.
     */
    const AVPacket* asAVPacket() const;
    /*!
     * \brief skip
     * Skip bytes of packet data. User has to update pts, dts etc to new values.
     * Useful for asAVPakcet(). When asAVPakcet() is called, AVPacket->pts/dts will be updated to new values.
     */
    void skip(int bytes);

    bool hasKeyFrame;
    bool isCorrupt;
    QByteArray data;
    // time unit is s.
    qreal pts, duration;
    qreal dts;
    qint64 position; // position in source file byte stream

private:
    // we must define  default/copy ctor, dtor and operator= so that we can provide only forward declaration of PacketPrivate
    mutable QSharedDataPointer<PacketPrivate> d;
};

bool Packet::isValid() const
{
    return !isCorrupt && !data.isEmpty() && pts >= 0 && duration >= 0; //!data.isEmpty()?
}

#ifndef QT_NO_DEBUG_STREAM
Q_AV_EXPORT QDebug operator<<(QDebug debug, const Packet &pkt);
#endif
} //namespace QtAV
Q_DECLARE_METATYPE(QtAV::Packet)
#endif // QAV_PACKET_H
