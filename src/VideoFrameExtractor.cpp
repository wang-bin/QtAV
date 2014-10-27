#include "QtAV/VideoFrameExtractor.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QRunnable>
#include <QtCore/QScopedPointer>
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
            if (task) {
                task->run();
                if (task->autoDelete())
                    delete task;
            }
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
        , auto_extract(true)
        , position(-2*kDefaultPrecision)
        , precision(kDefaultPrecision)
        , decoder(0)
    {
        codecs
#if QTAV_HAVE(DXVA)
                    << "DXVA"
#endif //QTAV_HAVE(DXVA)
#if QTAV_HAVE(VAAPI)
                    << "VAAPI"
#endif //QTAV_HAVE(VAAPI)
#if QTAV_HAVE(CEDARV)
                    << "Cedarv"
#endif //QTAV_HAVE(CEDARV)
#if QTAV_HAVE(VDA)
                    //<< "VDA"
#endif //QTAV_HAVE(VDA)
                    << "FFmpeg";
    }
    ~VideoFrameExtractorPrivate() {
        demuxer.close();
    }

    bool checkAndOpen() {
        const bool loaded = demuxer.isLoaded(source);
        if (loaded && decoder)
            return true;
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
            va["Display"] = "X11"; // to support swscale
            opt["vaapi"] = va;
            decoder->setOptions(opt);
            break;
        }
        return !!decoder;
    }

    // return the key frame position
    bool extractInPrecision(qint64 value, int range) {
        demuxer.seek(value + range); //
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
            if (demuxer.packet()->hasKeyFrame)
                break;
        }
        decoder->flush();
        const qint64 t_key = qint64(demuxer.packet()->pts * 1000.0);
        //qDebug("delta t = %d, data size: %d", int(value - t_key), demuxer.packet()->data.size());
        // must decode key frame
        if (!decoder->decode(demuxer.packet()->data)) {
            //qWarning("!!!!!!!!!decode failed!!!!!!!!");
            return false;
        }
        // seek backward, so value >= t
        // decode key frame
        if (int(value - t_key) <= range) {
            //qDebug("!!!!!!!!!use key frame!!!!!!!");
            frame = decoder->frame();
            frame.setTimestamp(demuxer.packet()->pts);
            if (frame.isValid()) {
                //qDebug() << "frame found. format: " <<  frame.format();
                return true;
            }
            // TODO: why key frame invalid?
            //qWarning("invalid key frame!!!!!");
        }
        // decode at the given position
        qreal t0 = qreal(value/1000LL);
        while (!demuxer.atEnd()) {
            if (!demuxer.readFrame()) {
                qDebug("!!!!!!----read frame error!!!!!!");
                continue;
            }
            if (demuxer.stream() != vstream) {
                //qDebug("not video packet");
                continue;
            }
            const qreal t = demuxer.packet()->pts;
            //qDebug("video packet: %f, delta=%lld", t, value - qint64(t*1000.0));
            if (qint64(t*1000.0) - value > range) { // use last decoded frame
                //qWarning("out of range");
                return frame.isValid();
            }
            if (demuxer.packet()->hasKeyFrame) {
                // FIXME:
                //qCritical("Internal error. Can not be a key frame!!!!");
                return false; //??
            }
            // invalid packet?
            if (!decoder->decode(demuxer.packet()->data)) {
                //qWarning("!!!!!!!!!decode failed!!!!");
                return false;
            }
            // store the last decoded frame because next frame may be out of range
            frame = decoder->frame();
            //qDebug() << "frame found. format: " <<  frame.format();
            frame.setTimestamp(t);
            if (!frame.isValid()) {
                //qDebug("invalid frame!!!");
                continue;
            }
            // TODO: break if t is in (t0-range, t0+range)
            if (qAbs(value - qint64(t*1000.0)) < range)
                break;
            if (t < t0)
                continue;
            break;
        }
        // now we get the final frame
        return true;
    }

    bool extracted;
    bool async;
    bool loading;
    bool auto_extract;
    qint64 position;
    int precision;
    QString source;
    AVDemuxer demuxer;
    QScopedPointer<VideoDecoder> decoder;
    VideoFrame frame;
    QStringList codecs;
    ExtractThread thread;
};


VideoFrameExtractor::VideoFrameExtractor(QObject *parent) :
    QObject(parent)
{
    DPTR_D(VideoFrameExtractor);
    moveToThread(&d.thread);
    d.thread.start();
    connect(this, SIGNAL(aboutToExtract()), SLOT(extractInternal()));
}

void VideoFrameExtractor::setSource(const QString value)
{
    DPTR_D(VideoFrameExtractor);
    if (value == d.source)
        return;
    d.source = value;
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
    if (qAbs(value - d.position) < precision()) {
        //qDebug("ingore new position~~~~~~~");
        //frameExtracted(d.frame);
        return;
    }
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
    extractInternal();
    return true;
}

void VideoFrameExtractor::extract()
{
    DPTR_D(VideoFrameExtractor);
    if (!d.async) {
        extractInternal();
        return;
    }
#if ASYNC_SIGNAL
    else {
        emit aboutToExtract();
        return;
    }
#endif
#if ASYNC_TASK
    class ExtractTask : public QRunnable {
    public:
        ExtractTask(VideoFrameExtractor *e) : extractor(e) {}
        void run() {
            extractor->extractInternal();
        }
    private:
        VideoFrameExtractor *extractor;
    };
    d.thread.addTask(new ExtractTask(this));
    return;
#endif
#if ASYNC_EVENT
    qApp->postEvent(this, new QEvent(QEvent::User));
#endif //ASYNC_EVENT
}

void VideoFrameExtractor::extractInternal()
{
    DPTR_D(VideoFrameExtractor);
    if (!d.checkAndOpen()) {
        emit error();
        //qWarning("can not open decoder....");
        return; // error handling
    }
    d.extracted = d.extractInPrecision(position(), precision());
    if (!d.extracted) {
        emit error();
        return;
    }
    emit frameExtracted(d.frame);
}

} //namespace QtAV
