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

#include "QtAV/Packet.h"
#include "QtAV/private/AVCompat.h"
#include "utils/Logger.h"

namespace QtAV {
namespace {
static const struct RegisterMetaTypes {
    inline RegisterMetaTypes() {
        qRegisterMetaType<QtAV::Packet>();
    }
} _registerMetaTypes;
} //namespace

class PacketPrivate : public QSharedData
{
public:
    PacketPrivate()
        : QSharedData()
        , initialized(false)
    {
        av_init_packet(&avpkt);
    }
    PacketPrivate(const PacketPrivate& o)
        : QSharedData(o)
        , initialized(o.initialized)
    { //used by QSharedDataPointer.detach()
        av_init_packet(&avpkt);
        av_packet_ref(&avpkt, (AVPacket*)&o.avpkt);
    }
     ~PacketPrivate() {
        av_packet_unref(&avpkt);
    }
    bool initialized;
    AVPacket avpkt;
};

Packet Packet::createEOF()
{
    Packet pkt;
    pkt.data = QByteArray("eof");
    return pkt;
}

bool Packet::isEOF() const
{
    return data == "eof" && pts < 0.0 && dts < 0.0;
}

Packet Packet::fromAVPacket(const AVPacket *avpkt, double time_base)
{
    Packet pkt;
    if (fromAVPacket(&pkt, avpkt, time_base))
        return pkt;
    return Packet();
}

// time_base: av_q2d(format_context->streams[stream_idx]->time_base)
bool Packet::fromAVPacket(Packet* pkt, const AVPacket *avpkt, double time_base)
{
    if (!pkt || !avpkt)
        return false;

    pkt->position = avpkt->pos;
    pkt->hasKeyFrame = !!(avpkt->flags & AV_PKT_FLAG_KEY);
    // what about marking avpkt as invalid and do not use isCorrupt?
    pkt->isCorrupt = !!(avpkt->flags & AV_PKT_FLAG_CORRUPT);
    if (pkt->isCorrupt)
        qDebug("currupt packet. pts: %f", pkt->pts);

    // from av_read_frame: pkt->pts can be AV_NOPTS_VALUE if the video format has B-frames, so it is better to rely on pkt->dts if you do not decompress the payload.
    // old code set pts as dts is valid
    if (avpkt->pts != (qint64)AV_NOPTS_VALUE)
        pkt->pts = avpkt->pts * time_base;
    else if (avpkt->dts != (qint64)AV_NOPTS_VALUE) // is it ok?
        pkt->pts = avpkt->dts * time_base;
    else
        pkt->pts = 0; // TODO: init value
    if (avpkt->dts != (qint64)AV_NOPTS_VALUE) //has B-frames
        pkt->dts = avpkt->dts * time_base;
    else
        pkt->dts = pkt->pts;
    //qDebug("avpacket pts %lld, dts: %lld ", avpkt->pts, avpkt->dts);
    //TODO: pts must >= 0? look at ffplay
    pkt->pts = qMax<qreal>(0, pkt->pts);
    pkt->dts = qMax<qreal>(0, pkt->dts);

    if (avpkt->duration > 0)
        pkt->duration = avpkt->duration * time_base;
    else
        pkt->duration = 0;
#if (LIBAVCODEC_VERSION_MAJOR < 57) //FF_API_CONVERGENCE_DURATION since 57
    // subtitle always has a key frame? convergence_duration may be 0
    if (avpkt->convergence_duration > 0
            && pkt->hasKeyFrame
#if 0
            && codec->codec_type == AVMEDIA_TYPE_SUBTITLE
#endif
            )
        pkt->duration = avpkt->convergence_duration * time_base;
#endif
    //qDebug("AVPacket.pts=%f, duration=%f, dts=%lld", pkt->pts, pkt->duration, packet.dts);
    pkt->data.clear();
    // TODO: pkt->avpkt. data is not necessary now. see mpv new_demux_packet_from_avpacket
    // copy properties and side data. does not touch data, size and ref
    pkt->d = QSharedDataPointer<PacketPrivate>(new PacketPrivate());
    pkt->d->initialized = true;
    AVPacket *p = &pkt->d->avpkt;
    av_packet_ref(p, (AVPacket*)avpkt);  //properties are copied internally
    // add ref without copy, bytearray does not copy either. bytearray options linke remove() is safe. omit FF_INPUT_BUFFER_PADDING_SIZE
    pkt->data = QByteArray::fromRawData((const char*)p->data, p->size);
    // QtAV always use ms (1/1000s) and s. As a result no time_base is required in Packet
    p->pts = pkt->pts * 1000.0;
    p->dts = pkt->dts * 1000.0;
    p->duration = pkt->duration * 1000.0;
    return true;
}

Packet::Packet()
    : hasKeyFrame(false)
    , isCorrupt(false)
    , pts(-1)
    , duration(-1)
    , dts(-1)
    , position(-1)
{
}


Packet::Packet(const Packet &other)
    : hasKeyFrame(other.hasKeyFrame)
    , isCorrupt(other.isCorrupt)
    , data(other.data)
    , pts(other.pts)
    , duration(other.duration)
    , dts(other.dts)
    , position(other.position)
    , d(other.d)
{
}

Packet& Packet::operator =(const Packet& other)
{
    if (this == &other)
        return *this;
    d = other.d;
    hasKeyFrame = other.hasKeyFrame;
    isCorrupt = other.isCorrupt;
    pts = other.pts;
    duration = other.duration;
    dts = other.dts;
    position = other.position;
    data = other.data;
    return *this;
}

Packet::~Packet()
{
}

const AVPacket *Packet::asAVPacket() const
{
    if (d.constData()) { //why d->initialized (ref==1) result in detach?
        if (d.constData()->initialized) {//d.data() was 0 if d has not been accessed. now only contains avpkt, check d.constData() is engough
            d->avpkt.data = (uint8_t*)data.constData();
            d->avpkt.size = data.size();
            return &d->avpkt;
        }
    } else {
        d = QSharedDataPointer<PacketPrivate>(new PacketPrivate());
    }

    d->initialized = true;
    AVPacket *p = &d->avpkt;
    p->pts = pts * 1000.0;
    p->dts = dts * 1000.0;
    p->duration = duration * 1000.0;
    p->pos = position;
    if (isCorrupt)
        p->flags |= AV_PKT_FLAG_CORRUPT;
    if (hasKeyFrame)
        p->flags |= AV_PKT_FLAG_KEY;
    if (!data.isEmpty()) {
        p->data = (uint8_t*)data.constData();
        p->size = data.size();
    }
    return p;
}

void Packet::skip(int bytes)
{
    if (!d.constData()) { //not constructed from AVPacket
        d = QSharedDataPointer<PacketPrivate>(new PacketPrivate());
    }
    d->initialized = false;
    data = QByteArray::fromRawData(data.constData() + bytes, data.size() - bytes);
    if (position >= 0)
        position += bytes;
    // TODO: if duration is valid, compute pts/dts and no manually update outside?
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const Packet &pkt)
{
    dbg.nospace() << "QtAV::Packet.data " << hex << (qptrdiff)pkt.data.constData() << "+" << dec << pkt.data.size();
    dbg.nospace() << ", dts: " << pkt.dts;
    dbg.nospace() << ", pts: " << pkt.pts;
    dbg.nospace() << ", duration: " << pkt.duration;
    dbg.nospace() << ", position: " << pkt.position;
    dbg.nospace() << ", hasKeyFrame: " << pkt.hasKeyFrame;
    dbg.nospace() << ", isCorrupt: " << pkt.isCorrupt;
    dbg.nospace() << ", eof: " << pkt.isEOF();
    return dbg.space();
}
#endif //QT_NO_DEBUG_STREAM
} //namespace QtAV
