/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>

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

const qreal Packet::kEndPts = -0.618;

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

    if (avpkt->pts != AV_NOPTS_VALUE)
        pkt->pts = avpkt->pts * time_base;
    else if (avpkt->dts != AV_NOPTS_VALUE) // is it ok?
        pkt->pts = avpkt->dts * time_base;
    else
        pkt->pts = 0; // TODO: init value
    if (avpkt->dts != AV_NOPTS_VALUE) //has B-frames
        pkt->dts = avpkt->dts * time_base;
    else if (avpkt->pts != AV_NOPTS_VALUE) // is it ok?
        pkt->dts = avpkt->pts * time_base;
    else
        pkt->dts = 0; // TODO: init value
    //TODO: pts must >= 0? look at ffplay
    pkt->pts = qMax<qreal>(0, pkt->pts);
    pkt->dts = qMax<qreal>(0, pkt->dts);


    // subtitle always has a key frame? convergence_duration may be 0
    if (avpkt->convergence_duration > 0  // mpv demux_lavf only check this
            && pkt->hasKeyFrame
#if 0
            && codec->codec_type == AVMEDIA_TYPE_SUBTITLE
#endif
            )
        pkt->duration = avpkt->convergence_duration * time_base;
    else if (avpkt->duration > 0)
        pkt->duration = avpkt->duration * time_base;
    else
        pkt->duration = 0;

    //qDebug("AVPacket.pts=%f, duration=%f, dts=%lld", pkt->pts, pkt->duration, packet.dts);
#if NO_PADDING_DATA
    pkt->data.clear();
    if (avpkt->data)
        pkt->data = QByteArray((const char*)avpkt->data, avpkt->size);
#else
    /*!
      larger than the actual read bytes because some optimized bitstream readers read 32 or 64 bits at once and could read over the end.
      The end of the input buffer avpkt->data should be set to 0 to ensure that no overreading happens for damaged MPEG streams
     */
    QByteArray encoded;
    if (avpkt->data) {
        encoded.reserve(avpkt->size + FF_INPUT_BUFFER_PADDING_SIZE);
        encoded.resize(avpkt->size);
        // also copy  padding data(usually 0)
        memcpy(encoded.data(), avpkt->data, encoded.capacity()); // encoded.capacity() is always > 0 even if packet.data, so must check avpkt->data null
    }
    pkt->data = encoded;
#endif //NO_PADDING_DATA

    // TODO: pkt->avpkt. data is not necessary now. see mpv new_demux_packet_from_avpacket
    // copy properties and side data. does not touch data, size and ref
    // libav has no av_copy_packet_side_data() in public api
    AVPacket *p = new AVPacket();
    av_init_packet(p);
    av_packet_copy_props(p, avpkt);
    if (!pkt->data.isEmpty()) {
        p->data = (uint8_t*)pkt->data.constData();
        p->size = pkt->data.size();
    }
    // QtAV always use ms (1/1000s) and s. As a result no time_base is required in Packet
    p->pts = pkt->pts * 1000.0;
    p->dts = pkt->dts * 1000.0;
    p->duration = pkt->duration * 1000.0;
    pkt->avpkt = p;
    return true;
}

Packet::Packet()
    : hasKeyFrame(false)
    , isCorrupt(false)
    , pts(0)
    , duration(0)
    , dts(0)
    , position(0)
    , avpkt(0)
{
}

Packet::Packet(const Packet &other)
{
    if (this == &other)
        return;
    hasKeyFrame = other.hasKeyFrame;
    isCorrupt = other.isCorrupt;
    pts = other.pts;
    duration = other.duration;
    dts = other.dts;
    position = other.position;
    data = other.data;
    avpkt = new AVPacket();
    av_init_packet(avpkt);
    av_packet_copy_props(avpkt, other.toAVPacket());
}

Packet& Packet::operator =(const Packet& other)
{
    if (this == &other)
        return *this;
    hasKeyFrame = other.hasKeyFrame;
    isCorrupt = other.isCorrupt;
    pts = other.pts;
    duration = other.duration;
    dts = other.dts;
    position = other.position;
    data = other.data;
    if (!avpkt) {
        avpkt = new AVPacket();
        av_init_packet(avpkt);
    } else {
        av_free_packet(avpkt); // free old side data and ref
    }
    av_packet_copy_props(avpkt, other.toAVPacket());
    return *this;
}

Packet::~Packet()
{
    if (avpkt) {
        // only free side data and ref, does not release data
        av_free_packet(avpkt);

        delete avpkt;
        avpkt = 0;
    }
}

void Packet::markEnd()
{
    qDebug("mark as end packet");
    pts = kEndPts;
}

AVPacket* Packet::toAVPacket() const
{
    if (avpkt)
        return avpkt;

    avpkt = new AVPacket();
    av_init_packet(avpkt);
    avpkt->pts = pts * 1000.0;
    avpkt->dts = dts * 1000.0;
    avpkt->duration = duration * 1000.0;
    if (isCorrupt)
        avpkt->flags |= AV_PKT_FLAG_CORRUPT;
    if (hasKeyFrame)
        avpkt->flags |= AV_PKT_FLAG_KEY;
    if (!data.isEmpty()) {
        avpkt->data = (uint8_t*)data.constData();
        avpkt->size = data.size();
    }
    return avpkt;
}

} //namespace QtAV
