#include "QtAV/VideoFrameExtractor.h"
#include <QtCore/QScopedPointer>
#include "QtAV/VideoCapture.h"
#include "QtAV/VideoDecoder.h"
#include "QtAV/AVDemuxer.h"
#include "QtAV/Packet.h"
#include "utils/Logger.h"

namespace QtAV {

const int kDefaultPrecision = 500;
class VideoFrameExtractorPrivate : public DPtrPrivate<VideoFrameExtractor>
{
public:
    VideoFrameExtractorPrivate()
        : extracted(false)
        , auto_extract(true)
        , position(-2*kDefaultPrecision)
        , precision(kDefaultPrecision)
        , decoder(0)
    {
        codecs << "DXVA" << "CUDA" << "VAAPI" << "VDA" << "Cedarv" << "FFmpeg";
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
            if (!demuxer.loadFile(source))
                return false;
        }
        if (codecs.isEmpty())
            return false;
        qDebug("creating decoder...");
        foreach (const QString& c, codecs) {
            qDebug() << "codec " << c;
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
                qDebug("!!!!!!read frame error!!!!!!");
                continue;
            }
            if (demuxer.stream() != vstream)
                continue;
            qDebug("video packet: %f", demuxer.packet()->pts);
            if (demuxer.packet()->hasKeyFrame)
                break;
        }
        decoder->flush();
        const qint64 t_key = qint64(demuxer.packet()->pts * 1000.0);
        qDebug("delta t = %d, data size: %d", int(value - t_key), demuxer.packet()->data.size());
        // must decode key frame
        if (!decoder->decode(demuxer.packet()->data)) {
            qWarning("!!!!!!!!!decode failed!!!!!!!!");
            return false;
        }
        // seek backward, so value >= t
        // decode key frame
        if (int(value - t_key) <= range) {
            qDebug("!!!!!!!!!use key frame!!!!!!!");
            frame = decoder->frame();
            frame.setTimestamp(demuxer.packet()->pts);
            if (frame.isValid()) {
                qDebug() << "frame found. format: " <<  frame.format();
                return true;
            }
            // why key frame invalid?
            qWarning("invalid key frame!!!!!");
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
            qDebug("video packet: %f, delta=%lld", t, value - qint64(t*1000.0));
            if (qint64(t*1000.0) - value > range) { // use last decoded frame
                qWarning("out of range");
                return frame.isValid();
            }
            if (demuxer.packet()->hasKeyFrame) {
                qCritical("Internal error. Can not be a key frame!!!!");
            }
            // invalid packet?
            if (!decoder->decode(demuxer.packet()->data)) {
                qWarning("!!!!!!!!!decode failed!!!!");
                return false;
            }
            // store the last decoded frame because next frame may be out of range
            frame = decoder->frame();
            qDebug() << "frame found. format: " <<  frame.format();
            frame.setTimestamp(t);
            if (!frame.isValid()) {
                qDebug("invalid frame!!!");
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
    bool auto_extract;
    qint64 position;
    int precision;
    QString source;
    AVDemuxer demuxer;
    QScopedPointer<VideoDecoder> decoder;
    VideoFrame frame;
    QStringList codecs;
};


VideoFrameExtractor::VideoFrameExtractor(QObject *parent) :
    QObject(parent)
{
}

VideoFrameExtractor::VideoFrameExtractor(VideoFrameExtractorPrivate &d, QObject *parent)
    : QObject(parent)
    , DPTR_INIT(&d)
{}

void VideoFrameExtractor::setSource(const QString value)
{
    DPTR_D(VideoFrameExtractor);
    if (value == d.source)
        return;
    d.source = value;
    emit sourceChanged();
}

QString VideoFrameExtractor::source() const
{
    return d_func().source;
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

// what if >= mediaStopPosition()?
void VideoFrameExtractor::setPosition(qint64 value)
{
    DPTR_D(VideoFrameExtractor);
    if (qAbs(value - d.position) < precision()) {
        frameExtracted();
        return;
    }
    d.extracted = false;
    d.position = value;
    emit positionChanged();
    if (!autoExtract())
        return;
    qDebug("extractiong...");
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

VideoFrame VideoFrameExtractor::frame()
{
    return d_func().frame;
}

void VideoFrameExtractor::extract()
{
    DPTR_D(VideoFrameExtractor);
    if (!d.checkAndOpen()) {
        emit error();
        qWarning("can not open decoder....");
        return; // error handling
    }
    qDebug("start to extract");
    d.extracted = d.extractInPrecision(position(), precision());
    if (!d.extracted) {
        emit error();
        return;
    }
    emit frameExtracted();
}

} //namespace QtAV
