/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2014)

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

#include "QtAV/VideoFrameExtractor.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QQueue>
#include <QtCore/QRunnable>
#include <QtCore/QScopedPointer>
#include <QtCore/QStringList>
#include <QtCore/QThread>
#include "QtAV/VideoCapture.h"
#include "QtAV/VideoDecoder.h"
#include "QtAV/AVDemuxer.h"
#include "QtAV/Packet.h"
#include "utils/BlockingQueue.h"
#include "utils/Logger.h"

// TODO: event and signal do not work
#define ASYNC_SIGNAL 0
#define ASYNC_EVENT 0
#define ASYNC_TASK 1
namespace QtAV {

class ExtractThread : public QThread {
public:
    ExtractThread(QObject *parent = 0)
        : QThread(parent)
        , stop(false)
    {
        tasks.setCapacity(1); // avoid too frequent
    }
    ~ExtractThread() {
        waitStop();
    }
    void waitStop() {
        if (!isRunning())
            return;
        scheduleStop();
        wait();
    }

    void addTask(QRunnable* t) {
        if (tasks.size() >= tasks.capacity()) {
            QRunnable *task = tasks.take(); //clear only for seek task
            if (task->autoDelete())
                delete task;
        }
        tasks.put(t);
    }
    void scheduleStop() {
        class StopTask : public QRunnable {
        public:
            StopTask(ExtractThread* t) : thread(t) {}
            void run() { thread->stop = true;}
        private:
            ExtractThread *thread;
        };
        addTask(new StopTask(this));
    }

protected:
    virtual void run() {
#if ASYNC_TASK
        while (!stop) {
            QRunnable *task = tasks.take();
            if (!task)
                return;
            task->run();
            if (task->autoDelete())
                delete task;
        }
#else
        exec();
#endif //ASYNC_TASK
    }
public:
    volatile bool stop;
private:
    BlockingQueue<QRunnable*> tasks;
};

// FIXME: avcodec_close() crash
const int kDefaultPrecision = 500;
class VideoFrameExtractorPrivate : public DPtrPrivate<VideoFrameExtractor>
{
public:
    VideoFrameExtractorPrivate()
        : extracted(false)
        , abort_seek(false)
        , async(true)
        , has_video(true)
        , auto_extract(true)
        , auto_precision(true)
        , seek_count(0)
        , position(-2*kDefaultPrecision)
        , precision(kDefaultPrecision)
        , decoder(0)
    {
        QVariantHash opt;
        opt[QString::fromLatin1("skip_frame")] = 8; // 8 for "avcodec", "NoRef" for "FFmpeg". see AVDiscard
        opt[QString::fromLatin1("skip_loop_filter")] = 8; //skip all?
        //skip_dict is slower
        dec_opt_framedrop[QString::fromLatin1("avcodec")] = opt;
        opt[QString::fromLatin1("skip_frame")] = 0; // 0 for "avcodec", "Default" for "FFmpeg". see AVDiscard
        opt[QString::fromLatin1("skip_loop_filter")] = 0;
        dec_opt_normal[QString::fromLatin1("avcodec")] = opt; // avcodec need correct string or value in libavcodec
        codecs
#if QTAV_HAVE(DXVA)
                    // << QStringLiteral("DXVA")
#endif //QTAV_HAVE(DXVA)
#if QTAV_HAVE(VAAPI)
                    // << QStringLiteral("VAAPI")
#endif //QTAV_HAVE(VAAPI)
#if QTAV_HAVE(CEDARV)
                    //<< QStringLiteral("Cedarv")
#endif //QTAV_HAVE(CEDARV)
#if QTAV_HAVE(VDA)
                    // << QStringLiteral("VDA") // only 1 app can use VDA at a given time
#endif //QTAV_HAVE(VDA)
                    << QStringLiteral("FFmpeg");
    }
    ~VideoFrameExtractorPrivate() {
        // stop first before demuxer and decoder close to avoid running new seek task after demuxer is closed.
        thread.waitStop();
        releaseResourceInternal();
    }
    bool checkAndOpen() {
        const bool loaded = demuxer.fileName() == source && demuxer.isLoaded();
        if (loaded && decoder)// && !demuxer.atEnd()) //we may seek back later when eof got. TODO: remove demuxer.atEnd()
            return true;
        seek_count = 0;
        decoder.reset(0);
        if (!loaded || demuxer.atEnd()) {
            demuxer.unload();
            demuxer.setMedia(source);
            if (!demuxer.load()) {
                return false;
            }
        }
        has_video = demuxer.videoStreams().size() > 0;
        if (!has_video) {
            demuxer.unload();
            return false;
        }
        if (codecs.isEmpty())
            return false;
        if (auto_precision) {
            if (demuxer.duration() < 10*1000)
                precision = kDefaultPrecision/2;
            else
                precision = kDefaultPrecision;
        }
        foreach (const QString& c, codecs) {
            VideoDecoder *vd = VideoDecoder::create(c.toUtf8().constData());
            if (!vd)
                continue;
            decoder.reset(vd);
            decoder->setCodecContext(demuxer.videoCodecContext());
            if (!decoder->open()) {
                decoder.reset(0);
                continue;
            }
            QVariantHash opt, va;
            // FIXME: why QStringLiteral can't be used as key for vs<2015 but somewhere else it can?  error C2958: the left bracket '[' found at qstringliteral
            va[QString::fromLatin1("display")] = QString::fromLatin1("X11"); // to support swscale
            opt[QString::fromLatin1("vaapi")] = va;
            decoder->setOptions(opt);
            break;
        }
        return !!decoder;
    }

    // return the key frame position
    bool extractInPrecision(qint64 value, int range) {
        abort_seek = false;
        frame = VideoFrame();
        if (value < demuxer.startTime())
            value += demuxer.startTime();
        demuxer.seek(value);
        const int vstream = demuxer.videoStream();
        Packet pkt;
        qint64 pts0 = -1;
        bool warn_bad_seek = true;
        bool warn_out_of_range = true;
        while (!demuxer.atEnd()) {
            if (abort_seek) {
                qDebug("VideoFrameExtractor abort seek before read");
                return false;
            }
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
            qWarning("VideoFrameExtractor failed to get a packet at %lld", value);
            return false;
        }
        decoder->flush(); //must flush otherwise old frames will be decoded at the beginning
        decoder->setOptions(dec_opt_normal);
        // must decode key frame
        int k = 0;
        while (k < 2 && !frame.isValid()) {
            if (abort_seek) {
                qDebug("VideoFrameExtractor abort seek before decoding key frames");
                return false;
            }
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
                return true;
            }
        }
        QVariantHash* dec_opt = &dec_opt_normal; // 0: default, 1: framedrop
        // decode at the given position
        while (!demuxer.atEnd()) {
            if (abort_seek) {
                qDebug("VideoFrameExtractor abort seek after key frame before read");
                return false;
            }
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
            if (seek_count == 0 || diff >= 0)
                dec_opt = &dec_opt_normal;
            else
                dec_opt = &dec_opt_framedrop;
            if (dec_opt != dec_opt_old)
                decoder->setOptions(*dec_opt);
            // invalid packet?
            if (!decoder->decode(pkt)) {
                qWarning("!!!!!!!!!decode failed!!!!");
                frame = VideoFrame();
                return false;
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
                return false;
            }
        }
        ++seek_count;
        // now we get the final frame
        if (demuxer.atEnd())
            releaseResourceInternal();
        return true;
    }
    void releaseResourceInternal() {
        seek_count = 0;
        // close codec context first.
        decoder.reset(0);
        demuxer.unload();
    }

    void safeReleaseResource() {
        class Cleaner : public QRunnable {
            VideoFrameExtractorPrivate *p;
        public:
            Cleaner(VideoFrameExtractorPrivate* pri) : p(pri) {}
            void run() {
                p->releaseResourceInternal();
            }
        };
        thread.addTask(new Cleaner(this));
    }

    bool extracted;
    volatile bool abort_seek;
    bool async;
    bool has_video;
    bool loading;
    bool auto_extract;
    bool auto_precision;
    int seek_count;
    qint64 position;
    int precision;
    QString source;
    AVDemuxer demuxer;
    QScopedPointer<VideoDecoder> decoder;
    VideoFrame frame;
    QStringList codecs;
    ExtractThread thread;
    static QVariantHash dec_opt_framedrop, dec_opt_normal;
};

QVariantHash VideoFrameExtractorPrivate::dec_opt_framedrop;
QVariantHash VideoFrameExtractorPrivate::dec_opt_normal;

VideoFrameExtractor::VideoFrameExtractor(QObject *parent) :
    QObject(parent)
{
    DPTR_D(VideoFrameExtractor);
    moveToThread(&d.thread);
    d.thread.start();
    connect(this, SIGNAL(aboutToExtract(qint64)), SLOT(extractInternal(qint64)));
}

void VideoFrameExtractor::setSource(const QString url)
{
    DPTR_D(VideoFrameExtractor);
    if (url == d.source)
        return;
    d.source = url;
    d.has_video = true;
    Q_EMIT sourceChanged();
    d.frame = VideoFrame();
    d.safeReleaseResource();
}

QString VideoFrameExtractor::source() const
{
    return d_func().source;
}

void VideoFrameExtractor::setAsync(bool value)
{
    DPTR_D(VideoFrameExtractor);
    if (d.async == value)
        return;
    d.async = value;
    Q_EMIT asyncChanged();
}

bool VideoFrameExtractor::async() const
{
    return d_func().async;
}

void VideoFrameExtractor::setAutoExtract(bool value)
{
    DPTR_D(VideoFrameExtractor);
    if (d.auto_extract == value)
        return;
    d.auto_extract = value;
    Q_EMIT autoExtractChanged();
}

bool VideoFrameExtractor::autoExtract() const
{
    return d_func().auto_extract;
}

void VideoFrameExtractor::setPosition(qint64 value)
{
    DPTR_D(VideoFrameExtractor);
    if (!d.has_video)
        return;
    if (qAbs(value - d.position) < precision()) {
        return;
    }
    d.frame = VideoFrame();
    d.extracted = false;
    d.position = value;
    Q_EMIT positionChanged();
    if (!autoExtract())
        return;
    extract();
}

qint64 VideoFrameExtractor::position() const
{
    return d_func().position;
}

void VideoFrameExtractor::setPrecision(int value)
{
    DPTR_D(VideoFrameExtractor);
    if (d.precision == value)
        return;
    d.auto_precision = value < 0;
    // explain why value (p0) is used but not the actual decoded position (p)
    // it's key frame finding rule
    if (value >= 0)
        d.precision = value;
    Q_EMIT precisionChanged();
}

int VideoFrameExtractor::precision() const
{
    return d_func().precision;
}

bool VideoFrameExtractor::event(QEvent *e)
{
    //qDebug("event: %d", e->type());
    if (e->type() != QEvent::User)
        return QObject::event(e);
    extractInternal(position()); // FIXME: wrong position
    return true;
}

void VideoFrameExtractor::extract()
{
    DPTR_D(VideoFrameExtractor);
    if (!d.async) {
        extractInternal(position());
        return;
    }
#if ASYNC_SIGNAL
    else {
        Q_EMIT aboutToExtract(position());
        return;
    }
#endif
#if ASYNC_TASK
    class ExtractTask : public QRunnable {
    public:
        ExtractTask(VideoFrameExtractor *e, qint64 t)
            : extractor(e)
            , position(t)
        {}
        void run() {
            extractor->extractInternal(position);
        }
    private:
        VideoFrameExtractor *extractor;
        qint64 position;
    };
    d.abort_seek = true;
    d.thread.addTask(new ExtractTask(this, position()));
    return;
#endif
#if ASYNC_EVENT
    qApp->postEvent(this, new QEvent(QEvent::User));
#endif //ASYNC_EVENT
}

void VideoFrameExtractor::extractInternal(qint64 pos)
{
    DPTR_D(VideoFrameExtractor);
    int precision_old = precision();
    if (!d.checkAndOpen()) {
        Q_EMIT error();
        //qWarning("can not open decoder....");
        return; // error handling
    }
    if (precision_old != precision()) {
        Q_EMIT precisionChanged();
    }
    d.extracted = d.extractInPrecision(pos, precision());
    if (!d.extracted) {
        Q_EMIT error();
        return;
    }
    Q_EMIT frameExtracted(d.frame);
}

} //namespace QtAV
