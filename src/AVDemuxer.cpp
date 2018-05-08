/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2018 Wang Bin <wbsecg1@gmail.com>

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
#include "QtAV/AVDemuxer.h"
#include "QtAV/MediaIO.h"
#include "QtAV/private/AVCompat.h"
#include <QtCore/QMutex>
#include <QtCore/QStringList>
#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
#include <QtCore/QElapsedTimer>
#else
#include <QtCore/QTime>
typedef QTime QElapsedTimer;
#endif
#include "utils/internal.h"
#include "utils/Logger.h"

namespace QtAV {
static const char kFileScheme[] = "file:";

class AVDemuxer::InterruptHandler : public AVIOInterruptCB
{
public:
    enum Action {
        Unknown = -1,
        Open,
        FindStreamInfo,
        Read
    };
    //default network timeout: 30000
    InterruptHandler(AVDemuxer* demuxer, int timeout = 30000)
      : mStatus(0)
      , mTimeout(timeout)
      , mTimeoutAbort(true)
      , mEmitError(true)
      //, mLastTime(0)
      , mAction(Unknown)
      , mpDemuxer(demuxer)
    {
        callback = handleTimeout;
        opaque = this;
    }
    ~InterruptHandler() {
#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
        mTimer.invalidate();
#else
        mTimer.stop();
#endif
    }
    void begin(Action act) {
        if (mStatus > 0)
            mStatus = 0;
        mEmitError = true;
        mAction = act;
        mTimer.start();
    }
    void end() {
#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
        mTimer.invalidate();
#else
        mTimer.stop();
#endif
        switch (mAction) {
        case Read:
            //mpDemuxer->setMediaStatus(BufferedMedia);
            break;
        default:
            break;
        }
        mAction = Unknown;
    }
    qint64 getTimeout() const { return mTimeout; }
    void setTimeout(qint64 timeout) { mTimeout = timeout; }
    bool setInterruptOnTimeout(bool value) {
        if (mTimeoutAbort == value)
            return false;
        mTimeoutAbort = value;
        if (mTimeoutAbort) {
            mEmitError = true;
        }
        return true;
    }
    bool isInterruptOnTimeout() const {return mTimeoutAbort;}
    int getStatus() const { return mStatus; }
    void setStatus(int status) { mStatus = status; }
    /*
     * metodo per interruzione loop ffmpeg
     * @param void*obj: classe attuale
      * @return
     *  >0 Interruzione loop di ffmpeg!
    */
    static int handleTimeout(void* obj) {
        InterruptHandler* handler = static_cast<InterruptHandler*>(obj);
        if (!handler) {
            qWarning("InterruptHandler is null");
            return -1;
        }
        //check manual interruption
        if (handler->getStatus() < 0) {
            qDebug("User Interrupt: -> quit!");
            // DO NOT call setMediaStatus() here.
            /* MUST make sure blocking functions (open, read) return before we change the status
             * because demuxer may be closed in another thread at the same time if status is not LoadingMedia
             * use handleError() after blocking functions return is good
             */
            // 1: blocking operation will be aborted.
            return 1;//interrupt
        }
        // qApp->processEvents(); //FIXME: qml crash
        switch (handler->mAction) {
        case Unknown: //callback is not called between begin()/end()
            //qWarning("Unknown timeout action");
            break;
        case Open:
        case FindStreamInfo:
            //qDebug("set loading media for %d from: %d", handler->mAction, handler->mpDemuxer->mediaStatus());
            handler->mpDemuxer->setMediaStatus(LoadingMedia);
            break;
        case Read:
            //handler->mpDemuxer->setMediaStatus(BufferingMedia);
        default:
            break;
        }
        if (handler->mTimeout < 0)
            return 0;
        if (!handler->mTimer.isValid()) {
            //qDebug("timer is not valid, start it");
            handler->mTimer.start();
            //handler->mLastTime = handler->mTimer.elapsed();
            return 0;
        }
        //use restart
#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
        if (!handler->mTimer.hasExpired(handler->mTimeout))
#else
        if (handler->mTimer.elapsed() < handler->mTimeout)
#endif
            return 0;
        qDebug("status: %d, Timeout expired: %lld/%lld -> quit!", (int)handler->mStatus, handler->mTimer.elapsed(), handler->mTimeout);
        handler->mTimer.invalidate();
        if (handler->mStatus == 0) {
            AVError::ErrorCode ec(AVError::ReadTimedout);
            if (handler->mAction == Open) {
                ec = AVError::OpenTimedout;
            } else if (handler->mAction == FindStreamInfo) {
                ec = AVError::ParseStreamTimedOut;
            } else if (handler->mAction == Read) {
                ec = AVError::ReadTimedout;
            }
            handler->mStatus = (int)ec;
            // maybe changed in other threads
            //handler->mStatus.testAndSetAcquire(0, ec);
        }
        if (handler->mTimeoutAbort)
            return 1;
        // emit demuxer error, handleerror
        if (handler->mEmitError) {
            handler->mEmitError = false;
            AVError::ErrorCode ec = AVError::ErrorCode(handler->mStatus); //FIXME: maybe changed in other threads
            QString es;
            handler->mpDemuxer->handleError(AVERROR_EXIT, &ec, es);
        }
        return 0;
    }
private:
    int mStatus;
    qint64 mTimeout;
    bool mTimeoutAbort;
    bool mEmitError;
    //qint64 mLastTime;
    Action mAction;
    AVDemuxer *mpDemuxer;
    QElapsedTimer mTimer;
};

class AVDemuxer::Private
{
public:
    Private()
        : media_status(NoMedia)
        , seekable(false)
        , network(false)
        , has_attached_pic(false)
        , started(false)
        , max_pts(0.0)
        , eof(false)
        , media_changed(true)
        , buf_pos(0)
        , stream(-1)
        , format_ctx(0)
        , input_format(0)
        , input(0)
        , seek_unit(SeekByTime)
        , seek_type(AccurateSeek)
        , dict(0)
        , interrupt_hanlder(0)
    {}
    ~Private() {
        delete interrupt_hanlder;
        if (dict) {
            av_dict_free(&dict);
            dict = 0;
        }
        if (input) {
            delete input;
            input = 0;
        }
    }
    void applyOptionsForDict();
    void applyOptionsForContext();
    void resetStreams() {
        stream = -1;
        if (media_changed)
            astream = vstream = sstream = StreamInfo();
        else
            astream.avctx = vstream.avctx = sstream.avctx = 0;
        audio_streams.clear();
        video_streams.clear();
        subtitle_streams.clear();
    }
    void checkNetwork() {
        // FIXME: is there a good way to check network? now use URLContext.flags == URL_PROTOCOL_FLAG_NETWORK
        // not network: concat cache pipe avdevice crypto?
        if (!file.isEmpty()
                && file.contains(QLatin1String(":"))
                && (file.startsWith(QLatin1String("http")) //http, https, httpproxy
                || file.startsWith(QLatin1String("rtmp")) //rtmp{,e,s,te,ts}
                || file.startsWith(QLatin1String("mms")) //mms{,h,t}
                || file.startsWith(QLatin1String("ffrtmp")) //ffrtmpcrypt, ffrtmphttp
                || file.startsWith(QLatin1String("rtp:"))
                || file.startsWith(QLatin1String("rtsp:"))
                || file.startsWith(QLatin1String("sctp:"))
                || file.startsWith(QLatin1String("tcp:"))
                || file.startsWith(QLatin1String("tls:"))
                || file.startsWith(QLatin1String("udp:"))
                || file.startsWith(QLatin1String("gopher:"))
                )) {
            network = true;
        }
    }
    bool checkSeekable() {
        bool s = false;
        if (!format_ctx)
            return s;
        // io.seekable: byte seeking
        if (input)
            s |= input->isSeekable();
        if (format_ctx->pb)
            s |= !!format_ctx->pb->seekable;
        // format.read_seek: time seeking. For example, seeking on hls stream steps: find segment, read packet in segment and drop until desired pts got
        s |= format_ctx->iformat->read_seek || format_ctx->iformat->read_seek2;
        return s;
    }
    // set wanted_xx_stream. call openCodecs() to read new stream frames
    // stream < 0 is choose best
    bool setStream(AVDemuxer::StreamType st, int streamValue);
    //called by loadFile(). if change to a new stream, call it(e.g. in AVPlayer)
    bool prepareStreams();

    MediaStatus media_status;
    bool seekable;
    bool network;
    bool has_attached_pic;
    bool started;
    qreal max_pts; // max pts read
    bool eof;
    bool media_changed;
    mutable qptrdiff buf_pos; // detect eof for dynamic size (growing) stream even if detectDynamicStreamInterval() is not set
    Packet pkt;
    int stream;
    QList<int> audio_streams, video_streams, subtitle_streams;
    AVFormatContext *format_ctx;
    //copy the info, not parse the file when constructed, then need member vars
    QString file;
    QString file_orig;
    AVInputFormat *input_format;
    QString format_forced;
    MediaIO *input;

    SeekUnit seek_unit;
    SeekType seek_type;

    AVDictionary *dict;
    QVariantHash options;
    typedef struct StreamInfo {
        StreamInfo()
            : stream(-1)
            , wanted_stream(-1)
            , index(-1)
            , wanted_index(-1)
            , avctx(0)
        {}
        // wanted_stream is REQUIRED. e.g. always set -1 to indicate the default stream, -2 to disable
        int stream, wanted_stream; // -1 default, selected by ff
        int index, wanted_index; // index in a kind of streams
        AVCodecContext *avctx;
    } StreamInfo;
    StreamInfo astream, vstream, sstream;

    AVDemuxer::InterruptHandler *interrupt_hanlder;
    QMutex mutex; //TODO: remove if load, read, seek is called in 1 thread
};

AVDemuxer::AVDemuxer(QObject *parent)
    : QObject(parent)
    , d(new Private())
{
    // TODO: xxx_register_all already use static var
    class AVInitializer {
    public:
        AVInitializer() {
#if !AVCODEC_STATIC_REGISTER
            avcodec_register_all();
#endif
#if QTAV_HAVE(AVDEVICE)
            avdevice_register_all();
#endif
#if !AVFORMAT_STATIC_REGISTER
            av_register_all();
#endif
            avformat_network_init();
        }
        ~AVInitializer() {
            avformat_network_deinit();
        }
    };
    static AVInitializer sAVInit;
    Q_UNUSED(sAVInit);
    d->interrupt_hanlder = new InterruptHandler(this);
}

AVDemuxer::~AVDemuxer()
{
    unload();
}

static void getFFmpegInputFormats(QStringList* formats, QStringList* extensions)
{
    static QStringList exts;
    static QStringList fmts;
    if (exts.isEmpty() && fmts.isEmpty()) {
        QStringList e, f;
#if AVFORMAT_STATIC_REGISTER
        const AVInputFormat *i = NULL;
        void* it = NULL;
        while ((i = av_demuxer_iterate(&it))) {
#else
        AVInputFormat *i = NULL;
        av_register_all(); // MUST register all input/output formats
        while ((i = av_iformat_next(i))) {
#endif
            if (i->extensions)
                e << QString::fromLatin1(i->extensions).split(QLatin1Char(','), QString::SkipEmptyParts);
            if (i->name)
                f << QString::fromLatin1(i->name).split(QLatin1Char(','), QString::SkipEmptyParts);
        }
        foreach (const QString& v, e) {
            exts.append(v.trimmed());
        }
        foreach (const QString& v, f) {
            fmts.append(v.trimmed());
        }
        exts.removeDuplicates();
        fmts.removeDuplicates();
    }
    if (formats)
        *formats = fmts;
    if (extensions)
        *extensions = exts;
}

const QStringList& AVDemuxer::supportedFormats()
{
    static QStringList fmts;
    if (fmts.isEmpty())
        getFFmpegInputFormats(&fmts, NULL);
    return fmts;
}

const QStringList& AVDemuxer::supportedExtensions()
{
    static QStringList exts;
    if (exts.isEmpty())
        getFFmpegInputFormats(NULL, &exts);
    return exts;
}

const QStringList &AVDemuxer::supportedProtocols()
{
    static QStringList protocols;
    if (!protocols.isEmpty())
        return protocols;
#if QTAV_HAVE(AVDEVICE)
    protocols << QStringLiteral("avdevice");
#endif
#if !AVFORMAT_STATIC_REGISTER
    av_register_all(); // MUST register all input/output formats
#endif
    void* opq = 0;
    const char* protocol = avio_enum_protocols(&opq, 0);
    while (protocol) {
        // static string, no deep copy needed. but QByteArray::fromRawData(data,size) assumes data is not null terminated and we must give a size
        protocols.append(QString::fromUtf8(protocol));
        protocol = avio_enum_protocols(&opq, 0);
    }
    return protocols;
}

MediaStatus AVDemuxer::mediaStatus() const
{
    return d->media_status;
}

bool AVDemuxer::readFrame()
{
    QMutexLocker lock(&d->mutex);
    Q_UNUSED(lock);
    if (!d->format_ctx)
        return false;
    d->pkt = Packet();
    // no lock required because in AVDemuxThread read and seek are in the same thread
    AVPacket packet;
    av_init_packet(&packet);
    d->interrupt_hanlder->begin(InterruptHandler::Read);
    int ret = av_read_frame(d->format_ctx, &packet); //0: ok, <0: error/end
    d->interrupt_hanlder->end();
    // TODO: why return 0 if interrupted by user?
    if (ret < 0) {
        //end of file. FIXME: why no d->eof if replaying by seek(0)?
        // ffplay also check pb && pb->error and exit read thread
        if (ret == AVERROR_EOF
                || avio_feof(d->format_ctx->pb)) {
            if (!d->eof) {
                if (getInterruptStatus()) {
                    AVError::ErrorCode ec(AVError::ReadError);
                    QString msg(tr("error reading stream data"));
                    handleError(ret, &ec, msg);
                }
                if (mediaStatus() != StalledMedia) {
                    d->eof = true;
#if 0 // EndOfMedia when demux thread finished
                    d->started = false;
                    setMediaStatus(EndOfMedia);
                    Q_EMIT finished();
#endif
                    qDebug("End of file. erreof=%d feof=%d", ret == AVERROR_EOF, avio_feof(d->format_ctx->pb));
                }
            }
            av_packet_unref(&packet); //important!
            return false;
        }
        if (ret == AVERROR(EAGAIN)) {
            qWarning("demuxer EAGAIN :%s", av_err2str(ret));
            av_packet_unref(&packet); //important!
            return false;
        }
        AVError::ErrorCode ec(AVError::ReadError);
        QString msg(tr("error reading stream data"));
        handleError(ret, &ec, msg);
        qWarning("[AVDemuxer] error: %s", av_err2str(ret));
        av_packet_unref(&packet); //important!
        return false;
    }
    d->stream = packet.stream_index;
    //check whether the 1st frame is alreay got. emit only once
    if (!d->started) {
        d->started = true;
        Q_EMIT started();
    }
    if (d->stream != videoStream() && d->stream != audioStream() && d->stream != subtitleStream()) {
        //qWarning("[AVDemuxer] unknown stream index: %d", stream);
        av_packet_unref(&packet); //important!
        return false;
    }
    // TODO: v4l2 copy
    d->pkt = Packet::fromAVPacket(&packet, av_q2d(d->format_ctx->streams[d->stream]->time_base));
    av_packet_unref(&packet); //important!
    d->eof = false;
    if (d->pkt.pts > qreal(duration())/1000.0) {
        d->max_pts = d->pkt.pts;
    }
    return true;
}

Packet AVDemuxer::packet() const
{
    return d->pkt;
}

int AVDemuxer::stream() const
{
    return d->stream;
}

bool AVDemuxer::atEnd() const
{
    if (!d->format_ctx)
        return false;
    if (d->format_ctx->pb)  {
        AVIOContext *pb = d->format_ctx->pb;
        //qDebug("pb->error: %#x, eof: %d, pos: %lld, bufptr: %p", pb->error, pb->eof_reached, pb->pos, pb->buf_ptr);
        if (d->eof && (qptrdiff)pb->buf_ptr == d->buf_pos)
            return true;
        d->buf_pos = (qptrdiff)pb->buf_ptr;
        return false;
    }
    return d->eof;
}

bool AVDemuxer::isSeekable() const
{
    return d->seekable;
}

void AVDemuxer::setSeekUnit(SeekUnit unit)
{
    d->seek_unit = unit;
}

SeekUnit AVDemuxer::seekUnit() const
{
    return d->seek_unit;
}

void AVDemuxer::setSeekType(SeekType target)
{
    d->seek_type = target;
}

SeekType AVDemuxer::seekType() const
{
    return d->seek_type;
}

//TODO: seek by byte
bool AVDemuxer::seek(qint64 pos)
{
    if (!isLoaded())
        return false;
    //duration: unit is us (10^-6 s, AV_TIME_BASE)
    qint64 upos = pos*1000LL; // TODO: av_rescale
    if (upos > startTimeUs() + durationUs() || pos < 0LL) {
        if (pos >= 0LL && d->input && d->input->isSeekable() && d->input->isVariableSize()) {
            qDebug("Seek for variable size hack. %lld %.2f. valid range [%lld, %lld]", upos, double(upos)/double(durationUs()), startTimeUs(), startTimeUs()+durationUs());
        } else if (d->max_pts > qreal(duration())/1000.0) { //FIXME
            qDebug("Seek (%lld) when video duration is growing %lld=>%lld", pos, duration(), qint64(d->max_pts*1000.0));
        } else {
            qWarning("Invalid seek position %lld %.2f. valid range [%lld, %lld]", upos, double(upos)/double(durationUs()), startTimeUs(), startTimeUs()+durationUs());
            return false;
        }
    }
    d->eof = false;
    // no lock required because in AVDemuxThread read and seek are in the same thread
#if 0
    //t: unit is s
    qreal t = q;// * (double)d->format_ctx->duration; //
    int ret = av_seek_frame(d->format_ctx, -1, (int64_t)(t*AV_TIME_BASE), t > d->pkt.pts ? 0 : AVSEEK_FLAG_BACKWARD);
    qDebug("[AVDemuxer] seek to %f %f %lld / %lld", q, d->pkt.pts, (int64_t)(t*AV_TIME_BASE), durationUs());
#else
    //TODO: d->pkt.pts may be 0, compute manually.

    bool backward = d->seek_type == AccurateSeek || upos <= (int64_t)(d->pkt.pts*AV_TIME_BASE);
    //qDebug("[AVDemuxer] seek to %f %f %lld / %lld backward=%d", double(upos)/double(durationUs()), d->pkt.pts, upos, durationUs(), backward);
    //AVSEEK_FLAG_BACKWARD has no effect? because we know the timestamp
    // FIXME: back flag is opposite? otherwise seek is bad and may crash?
    /* If stread->inputdex is (-1), a default
     * stream is selected, and timestamp is automatically converted
     * from AV_TIME_BASE units to the stream specific time_base.
     */
    int seek_flag = (backward ? AVSEEK_FLAG_BACKWARD : 0);
    if (d->seek_type == AccurateSeek) {
        seek_flag = AVSEEK_FLAG_BACKWARD;
    }
    if (d->seek_type == AnyFrameSeek) {
        seek_flag |= AVSEEK_FLAG_ANY;
    }
    //qDebug("seek flag: %d", seek_flag);
    //bool seek_bytes = !!(d->format_ctx->iformat->flags & AVFMT_TS_DISCONT) && strcmp("ogg", d->format_ctx->iformat->name);
    int ret = av_seek_frame(d->format_ctx, -1, upos, seek_flag);
    //int ret = avformat_seek_file(d->format_ctx, -1, INT64_MIN, upos, upos, seek_flag);
    //avformat_seek_file()
    if (ret < 0 && (seek_flag & AVSEEK_FLAG_BACKWARD)) {
        // seek to 0?
        qDebug("av_seek_frame error with flag AVSEEK_FLAG_BACKWARD: %s. try to seek without the flag", av_err2str(ret));
        seek_flag &= ~AVSEEK_FLAG_BACKWARD;
        ret = av_seek_frame(d->format_ctx, -1, upos, seek_flag);
    }
    //qDebug("av_seek_frame ret: %d", ret);
#endif
    if (ret < 0) {
        AVError::ErrorCode ec(AVError::SeekError);
        QString msg(tr("seek error"));
        handleError(ret, &ec, msg);
        return false;
    }
    // TODO: replay
    if (upos <= startTime()) {
        qDebug("************seek to beginning. started = false");
        d->started = false; //???
        if (d->astream.avctx)
            d->astream.avctx->frame_number = 0;
        if (d->vstream.avctx)
            d->vstream.avctx->frame_number = 0; //TODO: why frame_number not changed after seek?
        if (d->sstream.avctx)
            d->sstream.avctx->frame_number = 0;
    }
    return true;
}

bool AVDemuxer::seek(qreal q)
{
    if (duration() <= 0) {
        qWarning("duration() must be valid for percentage seek");
        return false;
    }
    return seek(qint64(q*(double)duration()));
}

QString AVDemuxer::fileName() const
{
    return d->file_orig;
}

QIODevice* AVDemuxer::ioDevice() const
{
    if (!d->input)
        return 0;
    if (d->input->name() != QLatin1String("QIODevice"))
        return 0;
    return d->input->property("device").value<QIODevice*>();
}

MediaIO* AVDemuxer::mediaIO() const
{
    return d->input;
}

bool AVDemuxer::setMedia(const QString &fileName)
{
    if (d->input) {
        delete d->input;
        d->input = 0;
    }
    d->file_orig = fileName;
    const QString url_old(d->file);
    d->file = fileName.trimmed();
    if (d->file.startsWith(QLatin1String("mms:")))
        d->file.insert(3, QLatin1Char('h'));
    else if (d->file.startsWith(QLatin1String(kFileScheme)))
        d->file = Internal::Path::toLocal(d->file);
    int colon = d->file.indexOf(QLatin1Char(':'));
    if (colon == 1) {
#ifdef Q_OS_WINRT
        d->file.prepend(QStringLiteral("qfile:"));
#endif
    }
    d->media_changed = url_old != d->file;
    if (d->media_changed) {
        d->format_forced.clear();
    }
    // a local file. return here to avoid protocol checking. If path contains ":", protocol checking will fail
    if (d->file.startsWith(QLatin1Char('/')))
        return d->media_changed;
    // use MediaIO to support protocols not supported by ffmpeg
    colon = d->file.indexOf(QLatin1Char(':'));
    if (colon >= 0) {
#ifdef Q_OS_WIN
        if (colon == 1 && d->file.at(0).isLetter())
            return d->media_changed;
#endif
        const QString scheme = colon == 0 ? QStringLiteral("qrc") : d->file.left(colon);
        // supportedProtocols() is not complete. so try MediaIO 1st, if not found, fallback to libavformat
        d->input = MediaIO::createForProtocol(scheme);
        if (d->input) {
            d->input->setUrl(d->file);
        }
    }
    return d->media_changed;
}

bool AVDemuxer::setMedia(QIODevice* device)
{
    d->file = QString();
    d->file_orig = QString();
    if (d->input) {
        if (d->input->name() != QLatin1String("QIODevice")) {
            delete d->input;
            d->input = 0;
        }
    }
    if (!d->input)
        d->input = MediaIO::create("QIODevice");
    QIODevice* old_dev = d->input->property("device").value<QIODevice*>();
    d->media_changed = old_dev != device;
    if (d->media_changed) {
        d->format_forced.clear();
    }
    d->input->setProperty("device", QVariant::fromValue(device)); //open outside?
    return d->media_changed;
}

bool AVDemuxer::setMedia(MediaIO *in)
{
    d->media_changed = in != d->input;
    if (d->media_changed) {
        d->format_forced.clear();
    }
    d->file = QString();
    d->file_orig = QString();
    if (!d->input)
        d->input = in;
    if (d->input != in) {
        delete d->input;
        d->input = in;
    }
    return d->media_changed;
}

void AVDemuxer::setFormat(const QString &fmt)
{
    d->format_forced = fmt;
}

QString AVDemuxer::formatForced() const
{
    return d->format_forced;
}

bool AVDemuxer::load()
{
    unload();
    qDebug("all closed and reseted");

    if (d->file.isEmpty() && !d->input) {
        setMediaStatus(NoMedia);
        return false;
    }
    QMutexLocker lock(&d->mutex); // TODO: load in AVDemuxThread and remove all locks
    Q_UNUSED(lock);
    setMediaStatus(LoadingMedia);
    d->checkNetwork();
#if QTAV_HAVE(AVDEVICE)
    static const QString avd_scheme(QStringLiteral("avdevice:"));
    if (d->file.startsWith(avd_scheme)) {
        QStringList parts = d->file.split(QStringLiteral(":"));
        int s0 = avd_scheme.size();
        const int s1 = d->file.indexOf(QChar(':'), s0);
        if (s1 < 0) {
            qDebug("invalid avdevice specification");
            setMediaStatus(InvalidMedia);
            return false;
        }
        if (d->file.at(s0) == QChar('/') && d->file.at(s0+1) == QChar('/')) {
            // avdevice://avfoundation:device_name
            s0 += 2;
        } // else avdevice:video4linux2:file_name
        d->input_format = av_find_input_format(d->file.mid(s0, s1-s0).toUtf8().constData());
        d->file = d->file.mid(s1+1);
    }
#endif
    //alloc av format context
    if (!d->format_ctx)
        d->format_ctx = avformat_alloc_context();
    d->format_ctx->flags |= AVFMT_FLAG_GENPTS;
    //install interrupt callback
    d->format_ctx->interrupt_callback = *d->interrupt_hanlder;

    d->applyOptionsForDict();
    // check special dict keys
    // d->format_forced can be set from AVFormatContext.format_whitelist
    if (!d->format_forced.isEmpty()) {
        d->input_format = av_find_input_format(d->format_forced.toUtf8().constData());
        qDebug() << "force format: " << d->format_forced;
    }
    int ret = 0;
    // used dict entries will be removed in avformat_open_input
    d->interrupt_hanlder->begin(InterruptHandler::Open);
    if (d->input) {
        if (d->input->accessMode() == MediaIO::Write) {
            qWarning("wrong MediaIO accessMode. MUST be Read");
        }
        d->format_ctx->pb = (AVIOContext*)d->input->avioContext();
        d->format_ctx->flags |= AVFMT_FLAG_CUSTOM_IO;
        qDebug("avformat_open_input: d->format_ctx:'%p'..., MediaIO('%s'): %p", d->format_ctx, d->input->name().toUtf8().constData(), d->input);
        ret = avformat_open_input(&d->format_ctx, "MediaIO", d->input_format, d->options.isEmpty() ? NULL : &d->dict);
        qDebug("avformat_open_input: (with MediaIO) ret:%d", ret);
    } else {
        qDebug("avformat_open_input: d->format_ctx:'%p', url:'%s'...",d->format_ctx, qPrintable(d->file));
        ret = avformat_open_input(&d->format_ctx, d->file.toUtf8().constData(), d->input_format, d->options.isEmpty() ? NULL : &d->dict);
        qDebug("avformat_open_input: url:'%s' ret:%d",qPrintable(d->file), ret);
    }
    d->interrupt_hanlder->end();
    if (ret < 0) {
        // d->format_ctx is 0
        AVError::ErrorCode ec = AVError::OpenError;
        QString msg = tr("failed to open media");
        handleError(ret, &ec, msg);
        qWarning() << "Can't open media: " << msg;
        if (mediaStatus() == LoadingMedia) //workaround for timeout but not interrupted
            setMediaStatus(InvalidMedia);
        Q_EMIT unloaded(); //context not ready. so will not emit in unload()
        return false;
    }
    //deprecated
    //if(av_find_stread->inputfo(d->format_ctx)<0) {
    //TODO: avformat_find_stread->inputfo is too slow, only useful for some video format
    d->interrupt_hanlder->begin(InterruptHandler::FindStreamInfo);
    ret = avformat_find_stream_info(d->format_ctx, NULL);
    d->interrupt_hanlder->end();

    if (ret < 0) {
        setMediaStatus(InvalidMedia);
        AVError::ErrorCode ec(AVError::ParseStreamError);
        QString msg(tr("failed to find stream info"));
        handleError(ret, &ec, msg);
        qWarning() << "Can't find stream info: " << msg;
        // context is ready. unloaded() will be emitted in unload()
        if (mediaStatus() == LoadingMedia) //workaround for timeout but not interrupted
            setMediaStatus(InvalidMedia);
        return false;
    }

    if (!d->prepareStreams()) {
        if (mediaStatus() == LoadingMedia)
            setMediaStatus(InvalidMedia);
        return false;
    }
    d->started = false;
    setMediaStatus(LoadedMedia);
    Q_EMIT loaded();
    const bool was_seekable = d->seekable;
    d->seekable = d->checkSeekable();
    if (was_seekable != d->seekable)
        Q_EMIT seekableChanged();
    qDebug("avfmtctx.flags: %d, iformat.flags", d->format_ctx->flags, d->format_ctx->iformat->flags);
    if (getInterruptStatus() < 0) {
        QString msg;
        qDebug("AVERROR_EXIT: %d", AVERROR_EXIT);
        handleError(AVERROR_EXIT, 0, msg);
        qWarning() << "User interupted: " << msg;
        return false;
    }
    return true;
}

bool AVDemuxer::unload()
{
    QMutexLocker lock(&d->mutex);
    Q_UNUSED(lock);
    /*
    if (d->seekable) {
        d->seekable = false; //
        Q_EMIT seekableChanged();
    }
    */
    d->network = false;
    d->has_attached_pic = false;
    d->eof = false; // true and set false in load()?
    d->buf_pos = 0;
    d->started = false;
    d->max_pts = 0.0;
    d->resetStreams();
    d->interrupt_hanlder->setStatus(0);
    //av_close_input_file(d->format_ctx); //deprecated
    if (d->format_ctx) {
        qDebug("closing d->format_ctx");
        avformat_close_input(&d->format_ctx); //libavf > 53.10.0
        d->format_ctx = 0;
        d->input_format = 0;
        // no delete. may be used in next load
        if (d->input)
            d->input->release();
        Q_EMIT unloaded();
    }
    return true;
}

bool AVDemuxer::isLoaded() const
{
    return d->format_ctx && (d->astream.avctx || d->vstream.avctx || d->sstream.avctx);
}

bool AVDemuxer::hasAttacedPicture() const
{
    return d->has_attached_pic;
}

bool AVDemuxer::setStreamIndex(StreamType st, int index)
{
    QList<int> *streams = 0;
    Private::StreamInfo *si = 0;
    if (st == AudioStream) { // TODO: use a struct
        si = &d->astream;
        streams = &d->audio_streams;
    } else if (st == VideoStream) {
        si = &d->vstream;
        streams = &d->video_streams;
    } else if (st == SubtitleStream) {
        si = &d->sstream;
        streams = &d->subtitle_streams;
    }
    if (!si) {
        qWarning("stream type %d for index %d not found", st, index);
        return false;
    }
    if (index >= streams->size()) {// || index < 0) { //TODO: disable if <0
        //si->wanted_stream = -1;
        qWarning("invalid index %d (valid is 0~%d) for stream type %d.", index, streams->size(), st);
        return false;
    }
    if (index < 0) {
        qDebug("disable %d stream", st);
        si->stream = -1;
        si->wanted_index = -1;
        si->wanted_stream = -1;
        return true;
    }
    if (!d->setStream(st, streams->at(index)))
        return false;
    si->wanted_index = index;
    return true;
}

AVFormatContext* AVDemuxer::formatContext()
{
    return d->format_ctx;
}

QString AVDemuxer::formatName() const
{
    if (!d->format_ctx)
        return QString();
    return QLatin1String(d->format_ctx->iformat->name);
}

QString AVDemuxer::formatLongName() const
{
    if (!d->format_ctx)
        return QString();
    return QLatin1String(d->format_ctx->iformat->long_name);
}

// convert to s using AV_TIME_BASE then *1000?
qint64 AVDemuxer::startTime() const
{
    return startTimeUs()/1000LL; //TODO: av_rescale
}

qint64 AVDemuxer::duration() const
{
    return durationUs()/1000LL; //time base: AV_TIME_BASE TODO: av_rescale
}

//AVFrameContext use AV_TIME_BASE as time base. AVStream use their own timebase
qint64 AVDemuxer::startTimeUs() const
{
    // start time may be not null for network stream
    if (!d->format_ctx || d->format_ctx->start_time == AV_NOPTS_VALUE)
        return 0;
    return d->format_ctx->start_time;
}

qint64 AVDemuxer::durationUs() const
{
    if (!d->format_ctx || d->format_ctx->duration == AV_NOPTS_VALUE)
        return 0;
    return d->format_ctx->duration; //time base: AV_TIME_BASE
}

int AVDemuxer::bitRate() const
{
    return d->format_ctx->bit_rate;
}

qreal AVDemuxer::frameRate() const
{
    if (videoStream() < 0)
        return 0;
    AVStream *stream = d->format_ctx->streams[videoStream()];
    return av_q2d(stream->avg_frame_rate);
    //codecCtx->time_base.den / codecCtx->time_base.num
}

qint64 AVDemuxer::frames(int stream) const
{
    if (stream == -1) {
        stream = videoStream();
        if (stream < 0)
            stream = audioStream();
        if (stream < 0)
            return 0;
    }
    return d->format_ctx->streams[stream]->nb_frames;
}

int AVDemuxer::currentStream(StreamType st) const
{
    if (st == AudioStream)
        return audioStream();
    else if (st == VideoStream)
        return videoStream();
    else if (st == SubtitleStream)
        return subtitleStream();
    return -1;
}

QList<int> AVDemuxer::streams(StreamType st) const
{
    if (st == AudioStream)
        return audioStreams();
    else if (st == VideoStream)
        return videoStreams();
    else if (st == SubtitleStream)
        return subtitleStreams();
    return QList<int>();
}

int AVDemuxer::audioStream() const
{
    return d->astream.stream;
}

QList<int> AVDemuxer::audioStreams() const
{
    return d->audio_streams;
}

int AVDemuxer::videoStream() const
{
    return d->vstream.stream;
}

QList<int> AVDemuxer::videoStreams() const
{
    return d->video_streams;
}

int AVDemuxer::subtitleStream() const
{
    return d->sstream.stream;
}

QList<int> AVDemuxer::subtitleStreams() const
{
    return d->subtitle_streams;
}

AVCodecContext* AVDemuxer::audioCodecContext(int stream) const
{
    if (stream < 0)
        return d->astream.avctx;
    if (stream > (int)d->format_ctx->nb_streams)
        return 0;
    AVCodecContext *avctx = d->format_ctx->streams[stream]->codec;
    if (avctx->codec_type == AVMEDIA_TYPE_AUDIO)
        return avctx;
    return 0;
}

AVCodecContext* AVDemuxer::videoCodecContext(int stream) const
{
    if (stream < 0)
        return d->vstream.avctx;
    if (stream > (int)d->format_ctx->nb_streams)
        return 0;
    AVCodecContext *avctx = d->format_ctx->streams[stream]->codec;
    if (avctx->codec_type == AVMEDIA_TYPE_VIDEO)
        return avctx;
    return 0;
}

AVCodecContext* AVDemuxer::subtitleCodecContext(int stream) const
{
    if (stream < 0)
        return d->sstream.avctx;
    if (stream > (int)d->format_ctx->nb_streams)
        return 0;
    AVCodecContext *avctx = d->format_ctx->streams[stream]->codec;
    if (avctx->codec_type == AVMEDIA_TYPE_SUBTITLE)
        return avctx;
    return 0;
}

/**
 * @brief getInterruptTimeout return the interrupt timeout
 * @return
 */
qint64 AVDemuxer::getInterruptTimeout() const
{
    return d->interrupt_hanlder->getTimeout();
}

/**
 * @brief setInterruptTimeout set the interrupt timeout
 * @param timeout
 * @return
 */
void AVDemuxer::setInterruptTimeout(qint64 timeout)
{
    d->interrupt_hanlder->setTimeout(timeout);
}

bool AVDemuxer::isInterruptOnTimeout() const
{
    return d->interrupt_hanlder->isInterruptOnTimeout();
}

void AVDemuxer::setInterruptOnTimeout(bool value)
{
    d->interrupt_hanlder->setInterruptOnTimeout(value);
}

int AVDemuxer::getInterruptStatus() const
{
    return d->interrupt_hanlder->getStatus();
}

void AVDemuxer::setInterruptStatus(int interrupt)
{
    d->interrupt_hanlder->setStatus(interrupt);
}

void AVDemuxer::setOptions(const QVariantHash &dict)
{
    d->options = dict;
    d->applyOptionsForContext(); // apply even if avformat context is open
}

QVariantHash AVDemuxer::options() const
{
    return d->options;
}

void AVDemuxer::setMediaStatus(MediaStatus status)
{
    if (d->media_status == status)
        return;

    //if (status == NoMedia || status == InvalidMedia)
    //    Q_EMIT durationChanged(0);

    d->media_status = status;

    Q_EMIT mediaStatusChanged(d->media_status);
}

void AVDemuxer::Private::applyOptionsForDict()
{
    if (dict) {
        av_dict_free(&dict);
        dict = 0; //aready 0 in av_free
    }
    if (options.isEmpty())
        return;
    QVariant opt(options);
    if (options.contains(QStringLiteral("avformat")))
        opt = options.value(QStringLiteral("avformat"));
    Internal::setOptionsToDict(opt, &dict);

    if (opt.type() == QVariant::Map) {
        QVariantMap avformat_dict(opt.toMap());
        if (avformat_dict.contains(QStringLiteral("format_whitelist"))) {
            const QString fmts(avformat_dict[QStringLiteral("format_whitelist")].toString());
            if (!fmts.contains(QLatin1Char(',')) && !fmts.isEmpty())
                format_forced = fmts; // reset when media changed
        }
    } else if (opt.type() == QVariant::Hash) {
        QVariantHash avformat_dict(opt.toHash());
        if (avformat_dict.contains(QStringLiteral("format_whitelist"))) {
            const QString fmts(avformat_dict[QStringLiteral("format_whitelist")].toString());
            if (!fmts.contains(QLatin1Char(',')) && !fmts.isEmpty())
                format_forced = fmts; // reset when media changed
        }
    }
}

void AVDemuxer::Private::applyOptionsForContext()
{
    if (!format_ctx)
        return;
    if (options.isEmpty()) {
        //av_opt_set_defaults(format_ctx);  //can't set default values! result maybe unexpected
        return;
    }
    QVariant opt(options);
    if (options.contains(QStringLiteral("avformat")))
        opt = options.value(QStringLiteral("avformat"));
    Internal::setOptionsToFFmpegObj(opt, format_ctx);
}

void AVDemuxer::handleError(int averr, AVError::ErrorCode *errorCode, QString &msg)
{
    if (averr >= 0)
        return;
    // d->format_ctx is 0
    // TODO: why sometimes AVERROR_EXIT does not work?
    bool interrupted = (averr == AVERROR_EXIT) || getInterruptStatus();
    QString err_msg(msg);
    if (interrupted) { // interrupted by callback, so can not determine whether the media is valid
        // insufficient buffering or other interruptions
        if (getInterruptStatus() < 0) {
            setMediaStatus(StalledMedia);
            Q_EMIT userInterrupted();
            err_msg += QStringLiteral(" [%1]").arg(tr("interrupted by user"));
        } else {
            // FIXME: if not interupt on timeout and ffmpeg exits, still LoadingMedia
            if (isInterruptOnTimeout())
                setMediaStatus(StalledMedia);
            // averr is eof for open timeout
            err_msg += QStringLiteral(" [%1]").arg(tr("timeout"));
        }
    } else {
        if (mediaStatus() == LoadingMedia)
            setMediaStatus(InvalidMedia);
    }
    msg = err_msg;
    if (!errorCode)
        return;
    AVError::ErrorCode ec(*errorCode);
    if (averr == AVERROR_INVALIDDATA) { // leave it if reading
        if (*errorCode == AVError::OpenError)
            ec = AVError::FormatError;
    } else {
        // Input/output error etc.
        if (d->network)
            ec = AVError::NetworkError;
    }
    AVError err(ec, err_msg, averr);
    Q_EMIT error(err);
    *errorCode = ec;
}

bool AVDemuxer::Private::setStream(AVDemuxer::StreamType st, int streamValue)
{
    if (streamValue < -1)
        streamValue = -1;
    QList<int> *streams = 0;
    Private::StreamInfo *si = 0;
    if (st == AudioStream) { // TODO: use a struct
        si = &astream;
        streams = &audio_streams;
    } else if (st == VideoStream) {
        si = &vstream;
        streams = &video_streams;
    } else if (st == SubtitleStream) {
        si = &sstream;
        streams = &subtitle_streams;
    }
    if (!si /*|| si->wanted_stream == streamValue*/) { //init -2
        qWarning("stream type %d not found", st);
        return false;
    }
    //if (!streams->contains(si->stream)) {
      //  qWarning("%d is not a valid stream for stream type %d", si->stream, st);
        //return false;
    //}
    bool index_valid = si->wanted_index >= 0 && si->wanted_index < streams->size();
    int s = AVERROR_STREAM_NOT_FOUND;
    if (streamValue >= 0 || !index_valid) {
        // or simply set s to streamValue if value is contained in streams?
        s = av_find_best_stream(format_ctx
                                , st == AudioStream ? AVMEDIA_TYPE_AUDIO
                                : st == VideoStream ? AVMEDIA_TYPE_VIDEO
                                : st == SubtitleStream ? AVMEDIA_TYPE_SUBTITLE
                                : AVMEDIA_TYPE_UNKNOWN
                                , streamValue, -1, NULL, 0); // streamValue -1 is ok
    } else { //index_valid
        s = streams->at(si->wanted_index);
    }
    if (s == AVERROR_STREAM_NOT_FOUND)
        return false;
    // don't touch wanted index
    si->stream = s;
    si->wanted_stream = streamValue;
    si->avctx = format_ctx->streams[s]->codec;
    has_attached_pic = !!(format_ctx->streams[s]->disposition & AV_DISPOSITION_ATTACHED_PIC);
    return true;
}

bool AVDemuxer::Private::prepareStreams()
{
    has_attached_pic = false;
    resetStreams();
    if (!format_ctx)
        return false;
    AVMediaType type = AVMEDIA_TYPE_UNKNOWN;
    for (unsigned int i = 0; i < format_ctx->nb_streams; ++i) {
        type = format_ctx->streams[i]->codec->codec_type;
        if (type == AVMEDIA_TYPE_VIDEO) {
            video_streams.push_back(i);
        } else if (type == AVMEDIA_TYPE_AUDIO) {
            audio_streams.push_back(i);
        } else if (type == AVMEDIA_TYPE_SUBTITLE) {
            subtitle_streams.push_back(i);
        }
    }
    if (audio_streams.isEmpty() && video_streams.isEmpty() && subtitle_streams.isEmpty())
        return false;
    setStream(AVDemuxer::AudioStream, -1);
    setStream(AVDemuxer::VideoStream, -1);
    setStream(AVDemuxer::SubtitleStream, -1);
    return true;
}
} //namespace QtAV
