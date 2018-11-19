/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2016)

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
#include "QtAV/FrameReader.h"
#include <QtCore/QQueue>
#include <QtCore/QThread>
#include "QtAV/AVDemuxer.h"
#include "QtAV/VideoDecoder.h"
#include "utils/BlockingQueue.h"
#include "utils/Logger.h"

namespace QtAV {
typedef QtAV::BlockingQueue<VideoFrame> VideoFrameQueue;
const int kQueueMin = 2;
static QVariantHash dec_opt_framedrop;
static QVariantHash dec_opt_normal;

class FrameReader::Private {
public:
    Private() : nb_seek(0) {
        QVariantHash opt;
        opt[QString::fromLatin1("skip_frame")] = 8; // 8 for "avcodec", "NoRef" for "FFmpeg". see AVDiscard
        opt[QString::fromLatin1("skip_loop_filter")] = 8; //skip all?
        //skip_dict is slower
        dec_opt_framedrop[QString::fromLatin1("avcodec")] = opt;
        opt[QString::fromLatin1("skip_frame")] = 0; // 0 for "avcodec", "Default" for "FFmpeg". see AVDiscard
        opt[QString::fromLatin1("skip_loop_filter")] = 0;
        dec_opt_normal[QString::fromLatin1("avcodec")] = opt; // avcodec need correct string or value in libavcodec

        //decs = QStringList() << "VideoToolbox" << "FFmpeg";
        vframes.setCapacity(4);
        vframes.setThreshold(kQueueMin); //
    }
    ~Private() {
        if (read_thread.isRunning()) {
            read_thread.quit();
            read_thread.wait();
        }
    }

    bool tryLoad();
    qint64 seekInternal(qint64 pos);
    QString url;
    QStringList vdecs;
    AVDemuxer demuxer;
    QScopedPointer<VideoDecoder> decoder;
    VideoFrameQueue vframes;
    QThread read_thread;
    int nb_seek;
};

bool FrameReader::Private::tryLoad()
{
    const bool loaded = demuxer.fileName() == url && demuxer.isLoaded();
    if (loaded && decoder)
        return true;
    if (decoder) { // new source
        decoder->close();
        decoder.reset(0);
    }
    if (!loaded || demuxer.atEnd()) {
        demuxer.unload();
        demuxer.setMedia(url);
        if (!demuxer.load()) {
            return false;
        }
    }
    const bool has_video = demuxer.videoStreams().size() > 0;
    if (!has_video) {
        demuxer.unload();
        return false;
    }
    if (vdecs.isEmpty()) {
        VideoDecoder *vd = VideoDecoder::create("FFmpeg");
        if (vd) {
            decoder.reset(vd);
            decoder->setCodecContext(demuxer.videoCodecContext());
            if (!decoder->open())
                decoder.reset(0);
        }
    } else {
        foreach (const QString& c, vdecs) {
            VideoDecoder *vd = VideoDecoder::create(c.toLatin1().constData());
            if (!vd)
                continue;
            decoder.reset(vd);
            decoder->setCodecContext(demuxer.videoCodecContext());
            decoder->setProperty("copyMode", "OptimizedCopy");
            if (!decoder->open()) {
                decoder.reset(0);
                continue;
            }
            break;
        }
    }
    nb_seek = 0;
    qDebug("decoder: %p", decoder.data());
    vframes.setThreshold(kQueueMin);
    return !!decoder;
}

// code is from QtAV VideoFrameExtractor.cpp
qint64 FrameReader::Private::seekInternal(qint64 value)
{
    if (!tryLoad()) {
        qDebug("load error");
        return -1;
    }
    VideoFrame frame;
    int range = 100;
    demuxer.seek(value);
    const int vstream = demuxer.videoStream();
    Packet pkt;
    qint64 pts0 = -1;
    bool warn_bad_seek = true;
    bool warn_out_of_range = true;
    while (!demuxer.atEnd()) {
        if (!demuxer.readFrame())
            continue;
        if (demuxer.stream() != vstream)
            continue;
        pkt = demuxer.packet();
        if (pts0 < 0LL)
            pts0 = (qint64)(pkt.pts*1000.0);
        if ((qint64)(pkt.pts*1000.0) - value > (qint64)range) {
            if (warn_out_of_range)
                qDebug("read packet out of range");
            warn_out_of_range = false;
            // No return because decoder needs more packets before the desired frame is decoded
            //return false;
        }
        //qDebug("video packet: %f", pkt.pts);
        // TODO: always key frame?
        if (pkt.hasKeyFrame)
            break;
        if (warn_bad_seek)
            qWarning("Not seek to key frame!!!");
        warn_bad_seek = false;
    }
    // enlarge range if seek to key-frame failed
    const qint64 key_pts = (qint64)(pkt.pts*1000.0);
    const bool enlarge_range = pts0 >= 0LL && key_pts - pts0 > 0LL;
    if (enlarge_range) {
        range = qMax<qint64>(key_pts - value, range);
        qDebug() << "enlarge range ==>>>> " << range;
    }
    if (!pkt.isValid()) {
        qWarning("FrameReader failed to get a packet at %lld", value);
        return -1;
    }
    decoder->flush(); //must flush otherwise old frames will be decoded at the beginning
    decoder->setOptions(dec_opt_normal);
    // must decode key frame
    int k = 0;
    while (k < 2 && !frame.isValid()) {
        //qWarning("invalid key frame!!!!! undecoded: %d", decoder->undecodedSize());
        if (decoder->decode(pkt)) {
            frame = decoder->frame();
        }
        ++k;
    }
    // if seek backward correctly to key frame, diff0 = t - value <= 0
    // but sometimes seek to no-key frame(and range is enlarged), diff0 >= 0
    // decode key frame
    const int diff0 = qint64(frame.timestamp()*1000.0) - value;
    if (qAbs(diff0) <= range) { //TODO: flag forward: result pts must >= value
        if (frame.isValid()) {
            qDebug() << "VideoFrameExtractor: key frame found @" << frame.timestamp() <<" diff=" << diff0 << ". format: " <<  frame.format();
            return qint64(frame.timestamp()*1000.0);
        }
    }
    QVariantHash* dec_opt = &dec_opt_normal; // 0: default, 1: framedrop
    // decode at the given position
    while (!demuxer.atEnd()) {
        if (!demuxer.readFrame())
            continue;
        if (demuxer.stream() != vstream)
            continue;
        pkt = demuxer.packet();
        const qreal t = pkt.pts;
        //qDebug("video packet: %f, delta=%lld", t, value - qint64(t*1000.0));
        if (!pkt.isValid()) {
            qWarning("invalid packet. no decode");
            continue;
        }
        if (pkt.hasKeyFrame) {
            // FIXME:
            //qCritical("Internal error. Can not be a key frame!!!!");
            //return false; //??
        }
        qint64 diff = qint64(t*1000.0) - value;
        QVariantHash *dec_opt_old = dec_opt;
        if (nb_seek == 0 || diff >= 0)
            dec_opt = &dec_opt_normal;
        else
            dec_opt = &dec_opt_framedrop;
        if (dec_opt != dec_opt_old)
            decoder->setOptions(*dec_opt);
        // invalid packet?
        if (!decoder->decode(pkt)) {
            qWarning("!!!!!!!!!decode failed!!!!");
            frame = VideoFrame();
            return -1;
        }
        // store the last decoded frame because next frame may be out of range
        const VideoFrame f = decoder->frame();
        if (!f.isValid()) {
            //qDebug("VideoFrameExtractor: invalid frame!!!");
            continue;
        }
        frame = f;
        const qreal pts = frame.timestamp();
        const qint64 pts_ms = pts*1000.0;
        if (pts_ms < value)
            continue; //
        diff = pts_ms - value;
        if (qAbs(diff) <= (qint64)range) {
            qDebug("got frame at %fs, diff=%lld", pts, diff);
            break;
        }
        // if decoder was not flushed, we may get old frame which is acceptable
        if (diff > range && t > pts) {
            qWarning("out pts out of range. diff=%lld, range=%d", diff, range);
            frame = VideoFrame();
            return -1;
        }
    }
    ++nb_seek;
    return qint64(frame.timestamp()*1000.0);
}

FrameReader::FrameReader(QObject *parent)
    : QObject(parent)
    , d(new Private())
{
    moveToThread(&d->read_thread);
    connect(this, SIGNAL(readMoreRequested()), SLOT(readMoreInternal()));
    connect(this, SIGNAL(readEnd()), &d->read_thread, SLOT(quit()));
    connect(this, SIGNAL(seekRequested(qint64)), SLOT(seekInternal(qint64)));
}

FrameReader::~FrameReader()
{
}

void FrameReader::setMedia(const QString &url)
{
    if (url == d->url)
        return;
    d->url = url;
}

QString FrameReader::mediaUrl() const
{
    return d->url;
}

void FrameReader::setVideoDecoders(const QStringList &names)
{
    if (names == d->vdecs)
        return;
    d->vdecs = names;
}

QStringList FrameReader::videoDecoders() const
{
    return d->vdecs;
}

VideoFrame FrameReader::getVideoFrame()
{
    return d->vframes.take();
}

bool FrameReader::hasVideoFrame() const
{
    return !d->vframes.isEmpty();
}

bool FrameReader::hasEnoughVideoFrames() const
{
    return d->vframes.isEnough();
}

bool FrameReader::readMore()
{
    if (d->demuxer.isLoaded() && d->demuxer.atEnd()) {
        if (!d->read_thread.isRunning())
            return false;
        qDebug("wait for read thread quit");
        d->read_thread.quit();
        d->read_thread.wait(); // sync
        return false;
    }
    if (!d->read_thread.isRunning())
        d->read_thread.start();
    Q_EMIT readMoreRequested();
    return true;
}

bool FrameReader::seek(qint64 pos)
{
    if (!d->read_thread.isRunning())
        d->read_thread.start();
    Q_EMIT seekRequested(pos);
    return true;
}

void FrameReader::readMoreInternal()
{
    if (!d->tryLoad()) {
        qDebug("load error");
        return;
    }
    //TODO: decode eof packets
    if (d->demuxer.atEnd())
        return;
    const int vstream = d->demuxer.videoStream();
    Packet pkt;
    while (!d->demuxer.atEnd()) {
        if (!d->demuxer.readFrame()) {
          //  qDebug("demuxer read error");
            continue;
        }
        if (d->demuxer.stream() != vstream) {
           // qDebug("not video stream");
            continue;
        }
        pkt = d->demuxer.packet();
        if (d->decoder->decode(pkt)) {
            const VideoFrame frame(d->decoder->frame());
            if (!frame) {
                qDebug("no frame got, continue to decoder");
                continue;
            }
            d->vframes.put(frame);
            Q_EMIT frameRead(frame);
            //qDebug("frame got @%.3f, queue enough: %d", frame.timestamp(), vframes.isEnough());
            if (d->vframes.isFull())
                break;
        } else {
            qDebug("dec error, continue to decoder");
        }
    }
    if (d->demuxer.atEnd()) {
        d->vframes.setThreshold(1);
        d->vframes.blockFull(false);
        while (d->decoder->decode(Packet::createEOF())) {
            qDebug("decoded buffered packets");
            const VideoFrame frame(d->decoder->frame());
            d->vframes.put(frame);
            Q_EMIT frameRead(frame);
            qDebug("put decoded buffered packets @%.3f", frame.timestamp());
        }
        d->vframes.put(VideoFrame()); //make sure take() will not be blocked
        d->vframes.blockFull(true);
        qDebug("eof");
        Q_EMIT readEnd();
    }
}

bool FrameReader::seekInternal(qint64 value)
{
    qint64 t = !d->seekInternal(value);
    if (t < 0)
        return false;
    // now we get the final frame
    Q_EMIT seekFinished(t);
    return true;
}
} //namespace QtAV
