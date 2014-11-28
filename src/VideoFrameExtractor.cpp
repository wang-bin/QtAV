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
            QRunnable *task = tasks.take();
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
        , async(true)
        , has_video(true)
        , auto_extract(true)
        , seek_count(0)
        , position(-2*kDefaultPrecision)
        , precision(kDefaultPrecision)
        , decoder(0)
    {
        QVariantHash opt;
        opt["skip_frame"] = 8; // 8 for "avcodec", "NoRef" for "FFmpeg". see AVDiscard
        dec_opt_framedrop["avcodec"] = opt;
        opt["skip_frame"] = 0; // 0 for "avcodec", "Default" for "FFmpeg". see AVDiscard
        dec_opt_normal["avcodec"] = opt; // avcodec need correct string or value in libavcodec
        codecs
#if QTAV_HAVE(DXVA)
                     << "DXVA"
#endif //QTAV_HAVE(DXVA)
#if QTAV_HAVE(VAAPI)
                     << "VAAPI"
#endif //QTAV_HAVE(VAAPI)
#if QTAV_HAVE(CEDARV)
                    //<< "Cedarv"
#endif //QTAV_HAVE(CEDARV)
#if QTAV_HAVE(VDA)
                    // << "VDA" // only 1 app can use VDA at a given time
#endif //QTAV_HAVE(VDA)
                    << "FFmpeg";
    }
    ~VideoFrameExtractorPrivate() {
        // stop first before demuxer and decoder close to avoid running new seek task after demuxer is closed.
        thread.waitStop();
        // close codec context first.
        decoder.reset(0);
        demuxer.close();
    }

    bool checkAndOpen() {
        const bool loaded = demuxer.isLoaded(source);
        if (loaded && decoder)
            return true;
        seek_count = 0;
        if (decoder) { // new source
            decoder->close();
            decoder.reset(0);
        }
        if (!loaded) {
            demuxer.close();
            if (!demuxer.loadFile(source)) {
                return false;
            }
        }
        has_video = demuxer.videoStreams().size() > 0;
        if (!has_video) {
            demuxer.close();
            return false;
        }
        if (codecs.isEmpty())
            return false;
        foreach (const QString& c, codecs) {
            VideoDecoderId cid = VideoDecoderFactory::id(c.toUtf8().constData());
            VideoDecoder *vd = VideoDecoderFactory::create(cid);
            if (!vd)
                continue;
            decoder.reset(vd);
            decoder->setCodecContext(demuxer.videoCodecContext());
            if (!decoder->prepare()) {
                decoder.reset(0);
                continue;
            }
            if (!decoder->open()) {
                decoder.reset(0);
                continue;
            }
            QVariantHash opt, va;
            va["display"] = "X11"; // to support swscale
            opt["vaapi"] = va;
            decoder->setOptions(opt);
            break;
        }
        return !!decoder;
    }

    // return the key frame position
    bool extractInPrecision(qint64 value, int range) {
        demuxer.seek(value);
        const int vstream = demuxer.videoStream();
        while (!demuxer.atEnd()) {
            if (!demuxer.readFrame()) {
                //qDebug("!!!!!!read frame error!!!!!!");
                continue;
            }
            if (demuxer.stream() != vstream)
                continue;
            if ((qint64)(demuxer.packet()->pts*1000.0) - value > (qint64)range)
                return false;
            //qDebug("video packet: %f", demuxer.packet()->pts);
            // TODO: always key frame?
            if (demuxer.packet()->hasKeyFrame)
                break;
        }
        decoder->flush(); //must flush otherwise old frames will be decoded at the beginning
        decoder->setOptions(dec_opt_normal);
        const qint64 t_key = qint64(demuxer.packet()->pts * 1000.0);
        //qDebug("delta t = %d, data size: %d", int(value - t_key), demuxer.packet()->data.size());
        // must decode key frame
        // because current decode() only accept data, no dts pts, so we can't get correct decoded dts pts.
        // it's a workaround to decode until decoded key frame is valid
        int k = 0;
        while (k < 5 && !frame.isValid()) {
            //qWarning("invalid key frame!!!!! undecoded: %d", decoder->undecodedSize());
            if (!decoder->decode(demuxer.packet()->data)) {
                //qWarning("!!!!!!!!!decode key failed!!!!!!!!");
                return false;
            }
            frame = decoder->frame();
            ++k;
        }
        frame.setTimestamp(demuxer.packet()->pts);

        // seek backward, so value >= t
        // decode key frame
        if (int(value - t_key) <= range) {
            qDebug("!!!!!!!!!use key frame!!!!!!!");
            if (frame.isValid()) {
                qDebug() << "frame found. format: " <<  frame.format();
                return true;
            }
        }
        static const int kNoFrameDrop = 0;
        static const int kFrameDrop = 1;
        int dec_opt_state = kNoFrameDrop; // 0: default, 1: framedrop

        // decode at the given position
        qreal t0 = qreal(value/1000LL);
        while (!demuxer.atEnd()) {
            if (!demuxer.readFrame()) {
                //qDebug("!!!!!!----read frame error!!!!!!");
                continue;
            }
            if (demuxer.stream() != vstream) {
                //qDebug("not video packet");
                continue;
            }
            const qreal t = demuxer.packet()->pts;
            const qint64 diff = qint64(t*1000.0) - value;
            //qDebug("video packet: %f, delta=%lld", t, value - qint64(t*1000.0));
            if (diff > (qint64)range) { // use last decoded frame
                //qWarning("out of range");
                return frame.isValid();
            }
            if (demuxer.packet()->hasKeyFrame) {
                // FIXME:
                //qCritical("Internal error. Can not be a key frame!!!!");
                //return false; //??
            }
            if (seek_count == 0 || diff >= 0) {
                if (dec_opt_state == kFrameDrop) {
                    dec_opt_state = kNoFrameDrop;
                    decoder->setOptions(dec_opt_normal);
                }
            } else {
                if (dec_opt_state == kNoFrameDrop) {
                    dec_opt_state = kFrameDrop;
                    decoder->setOptions(dec_opt_framedrop);
                }
            }
            // invalid packet?
            if (!decoder->decode(demuxer.packet()->data)) {
                //qWarning("!!!!!!!!!decode failed!!!!");
                return false;
            }
            // store the last decoded frame because next frame may be out of range
            const VideoFrame f = decoder->frame();
            //qDebug() << "frame found. format: " <<  frame.format();
            if (!f.isValid()) {
                //qDebug("invalid frame!!!");
                continue;
            }
            frame = f;
            frame.setTimestamp(t);
            // TODO: break if t is in (t0-range, t0+range)
            if (diff >= 0 || -diff < range)
                break;
            if (t < t0)
                continue;
            break;
        }
        ++seek_count;
        // now we get the final frame
        return true;
    }

    bool extracted;
    bool async;
    bool has_video;
    bool loading;
    bool auto_extract;
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

void VideoFrameExtractor::setSource(const QString value)
{
    DPTR_D(VideoFrameExtractor);
    if (value == d.source)
        return;
    d.source = value;
    d.has_video = true;
    emit sourceChanged();
    d.frame = VideoFrame();
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
    emit asyncChanged();
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
    emit autoExtractChanged();
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
    emit positionChanged();
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
    // explain why value (p0) is used but not the actual decoded position (p)
    // it's key frame finding rule
    d.precision = value;
    emit precisionChanged();
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
        emit aboutToExtract(position());
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
    if (!d.checkAndOpen()) {
        emit error();
        //qWarning("can not open decoder....");
        return; // error handling
    }
    d.extracted = d.extractInPrecision(pos, precision());
    if (!d.extracted) {
        emit error();
        return;
    }
    emit frameExtracted(d.frame);
}

} //namespace QtAV
