/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2015 Wang Bin <wbsecg1@gmail.com>

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
#include "QtAV/AVInput.h"
#include "QtAV/private/AVCompat.h"
#include "input/QIODeviceInput.h"
#include <QtCore/QStringList>
#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
#include <QtCore/QElapsedTimer>
#else
#include <QtCore/QTime>
typedef QTime QElapsedTimer;
#endif
#include "utils/Logger.h"

namespace QtAV {
static const char kFileScheme[] = "file:";
#define CHAR_COUNT(s) (sizeof(s) - 1) // tail '\0'

/*!
 * \brief getLocalPath
 * get path that works for both ffmpeg and QFile
 * Windows: ffmpeg does not supports file:///C:/xx.mov, only supports file:C:/xx.mov or C:/xx.mov
 * QFile: does not support file: scheme
 * fullPath can be file:///path from QUrl. QUrl.toLocalFile will remove file://
 */
QString getLocalPath(const QString& fullPath)
{
    int pos = fullPath.indexOf(kFileScheme);
    if (pos >= 0) {
        pos += CHAR_COUNT(kFileScheme);
        bool has_slash = false;
        while (fullPath.at(pos) == QChar('/')) {
            has_slash = true;
            ++pos;
        }
        // win: ffmpeg does not supports file:///C:/xx.mov, only supports file:C:/xx.mov or C:/xx.mov
#ifndef Q_OS_WIN // for QUrl
        if (has_slash)
            --pos;
#endif
    }
    // always remove "file:" even thought it works for ffmpeg.but fileName() may be used for QFile which does not file:
    if (pos > 0)
        return fullPath.mid(pos);
    return fullPath;
}
#undef CHAR_COUNT

class AVDemuxer::InterruptHandler : public AVIOInterruptCB
{
public:
    enum Action {
        Open,
        FindStreamInfo,
        Read
    };
    //default network timeout: 30000
    InterruptHandler(AVDemuxer* demuxer, int timeout = 30000)
      : mStatus(0)
      , mTimeout(timeout)
      //, mLastTime(0)
      , mAction(Open)
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
    }
    qint64 getTimeout() const { return mTimeout; }
    void setTimeout(qint64 timeout) { mTimeout = timeout; }
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
        if (handler->getStatus() > 0) {
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
        case Open:
        case FindStreamInfo:
            handler->mpDemuxer->setMediaStatus(LoadingMedia);
            break;
        case Read:
            //handler->mpDemuxer->setMediaStatus(BufferingMedia);
        default:
            break;
        }
        if (!handler->mTimer.isValid()) {
            qDebug("timer is not valid, start it");
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
        qDebug("Timeout expired: %lld/%lld -> quit!", handler->mTimer.elapsed(), handler->mTimeout);
        handler->mTimer.invalidate();
        AVError err;
        if (handler->mAction == Open) {
            err.setError(AVError::OpenTimedout);
        } else if (handler->mAction == FindStreamInfo) {
            err.setError(AVError::FindStreamInfoTimedout);
        } else if (handler->mAction == Read) {
            err.setError(AVError::ReadTimedout);
        }
        QMetaObject::invokeMethod(handler->mpDemuxer, "error", Qt::AutoConnection, Q_ARG(QtAV::AVError, err));
        return 1;
    }
private:
    int mStatus;
    qint64 mTimeout;
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
        , network(false)
        , has_attached_pic(false)
        , started(false)
        , eof(false)
        , media_changed(true)
        , stream(-1)
        , format_ctx(0)
        , input_format(0)
        , input(0)
        , seek_unit(SeekByTime)
        , seek_target(SeekTarget_AccurateFrame)
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
                && file.contains(":")
                && (file.startsWith("http") //http, https, httpproxy
                || file.startsWith("rtmp") //rtmp{,e,s,te,ts}
                || file.startsWith("mms") //mms{,h,t}
                || file.startsWith("ffrtmp") //ffrtmpcrypt, ffrtmphttp
                || file.startsWith("rtp:")
                || file.startsWith("sctp:")
                || file.startsWith("tcp:")
                || file.startsWith("tls:")
                || file.startsWith("udp:")
                || file.startsWith("gopher:")
                )) {
            network = true; //iformat.flags: AVFMT_NOFILE
        }
    }

    MediaStatus media_status;
    bool network;
    bool has_attached_pic;
    bool started;
    bool eof;
    bool media_changed;
    Packet pkt;
    int stream;
    QList<int> audio_streams, video_streams, subtitle_streams;
    AVFormatContext *format_ctx;
    //copy the info, not parse the file when constructed, then need member vars
    QString file;
    QString file_orig;
    AVInputFormat *input_format;
    AVInput *input;

    AVDemuxer::SeekUnit seek_unit;
    AVDemuxer::SeekTarget seek_target;

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
        // wanted_stream is REQUIRED. e.g. always set -1 to indicate the default stream
        int stream, wanted_stream; // -1 default, selected by ff
        int index, wanted_index; // index in a kind of streams
        AVCodecContext *avctx;
    } StreamInfo;
    StreamInfo astream, vstream, sstream;

    AVDemuxer::InterruptHandler *interrupt_hanlder;
};

AVDemuxer::AVDemuxer(QObject *parent)
    : QObject(parent)
    , d(new Private())
{
    class AVInitializer {
    public:
        AVInitializer() {
            avcodec_register_all();
#if QTAV_HAVE(AVDEVICE)
            avdevice_register_all();
#endif
            av_register_all();
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

const QStringList &AVDemuxer::supportedProtocols()
{
    static QStringList protocols;
    if (!protocols.isEmpty())
        return protocols;
#if QTAV_HAVE(AVDEVICE)
    protocols << "avdevice";
#endif
    av_register_all(); // MUST register all input/output formats
    void* opq = 0;
    const char* protocol = avio_enum_protocols(&opq, 0);
    while (protocol) {
        // static string, no deep copy needed. but QByteArray::fromRawData(data,size) assumes data is not null terminated and we must give a size
        protocols.append(protocol);
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
    if (!d->format_ctx)
        return false;
    d->pkt = Packet();
    // no lock required because in AVDemuxThread read and seek are in the same thread
    AVPacket packet;
    d->interrupt_hanlder->begin(InterruptHandler::Read);
    int ret = av_read_frame(d->format_ctx, &packet); //0: ok, <0: error/end
    d->interrupt_hanlder->end();

    if (ret < 0) {
        //ffplay: AVERROR_EOF || url_d->eof() || avsq.empty()
        //end of file. FIXME: why no d->eof if replaying by seek(0)?
        if (ret == AVERROR_EOF
                // AVFMT_NOFILE(e.g. network streams) stream has no pb
                // ffplay check pb && pb->error, mpv does not
                || d->format_ctx->pb/* && d->format_ctx->pb->error*/) {
            if (!d->eof) {
                d->eof = true;
                d->started = false;
                setMediaStatus(EndOfMedia);
                qDebug("End of file. %s %d", __FUNCTION__, __LINE__);
                emit finished();
            }
            // we have to detect false is error or d->eof
            return ret == AVERROR_EOF; //frames after d->eof are d->eof frames
        }
        AVError::ErrorCode ec(AVError::ReadError);
        QString msg(tr("error reading stream data"));
        handleError(ret, &ec, msg);
        qWarning("[AVDemuxer] error: %s", av_err2str(ret));
        return false;
    }
    d->stream = packet.stream_index;
    //check whether the 1st frame is alreay got. emit only once
    if (!d->started) {
        d->started = true;
        emit started();
    }
    if (d->stream != videoStream() && d->stream != audioStream() && d->stream != subtitleStream()) {
        //qWarning("[AVDemuxer] unknown stream index: %d", stream);
        return false;
    }
    d->pkt = Packet::fromAVPacket(&packet, av_q2d(d->format_ctx->streams[d->stream]->time_base));
    av_free_packet(&packet); //important!
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
    return d->eof;
}

bool AVDemuxer::isSeekable() const
{
    return true;
}

void AVDemuxer::setSeekUnit(SeekUnit unit)
{
    d->seek_unit = unit;
}

AVDemuxer::SeekUnit AVDemuxer::seekUnit() const
{
    return d->seek_unit;
}

void AVDemuxer::setSeekTarget(SeekTarget target)
{
    d->seek_target = target;
}

AVDemuxer::SeekTarget AVDemuxer::seekTarget() const
{
    return d->seek_target;
}

//TODO: seek by byte
bool AVDemuxer::seek(qint64 pos)
{
    if (!isLoaded())
        return false;
    //duration: unit is us (10^-6 s, AV_TIME_BASE)
    qint64 upos = pos*1000LL;
    if (upos > startTimeUs() + durationUs() || pos < 0LL) {
        qWarning("Invalid seek position %lld %.2f. valid range [%lld, %lld]", upos, double(upos)/double(durationUs()), startTimeUs(), startTimeUs()+durationUs());
        return false;
    }
    // no lock required because in AVDemuxThread read and seek are in the same thread
#if 0
    //t: unit is s
    qreal t = q;// * (double)d->format_ctx->duration; //
    int ret = av_seek_frame(d->format_ctx, -1, (int64_t)(t*AV_TIME_BASE), t > d->pkt.pts ? 0 : AVSEEK_FLAG_BACKWARD);
    qDebug("[AVDemuxer] seek to %f %f %lld / %lld", q, d->pkt.pts, (int64_t)(t*AV_TIME_BASE), durationUs());
#else
    //TODO: d->pkt.pts may be 0, compute manually.

    bool backward = d->seek_target == SeekTarget_AccurateFrame || upos <= (int64_t)(d->pkt.pts*AV_TIME_BASE);
    //qDebug("[AVDemuxer] seek to %f %f %lld / %lld backward=%d", double(upos)/double(durationUs()), d->pkt.pts, upos, durationUs(), backward);
    //AVSEEK_FLAG_BACKWARD has no effect? because we know the timestamp
    // FIXME: back flag is opposite? otherwise seek is bad and may crash?
    /* If stread->inputdex is (-1), a default
     * stream is selected, and timestamp is automatically converted
     * from AV_TIME_BASE units to the stream specific time_base.
     */
    int seek_flag = (backward ? AVSEEK_FLAG_BACKWARD : 0);
    if (d->seek_target == SeekTarget_AccurateFrame) {
        seek_flag = AVSEEK_FLAG_BACKWARD;
    }
    if (d->seek_target == SeekTarget_AnyFrame) {
        seek_flag = AVSEEK_FLAG_ANY;
    }
    //bool seek_bytes = !!(d->format_ctx->iformat->flags & AVFMT_TS_DISCONT) && strcmp("ogg", d->format_ctx->iformat->name);
    int ret = av_seek_frame(d->format_ctx, -1, upos, seek_flag);
    //int ret = avformat_seek_file(d->format_ctx, -1, INT64_MIN, upos, upos, seek_flag);
    //avformat_seek_file()
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
        d->started = false;
        if (d->astream.avctx)
            d->astream.avctx->frame_number = 0;
        if (d->vstream.avctx)
            d->vstream.avctx->frame_number = 0; //TODO: why frame_number not changed after seek?
        if (d->sstream.avctx)
            d->sstream.avctx->frame_number = 0;
    }
    return true;
}

void AVDemuxer::seek(qreal q)
{
    seek(qint64(q*(double)duration()));
}

QString AVDemuxer::fileName() const
{
    return d->file_orig;
}

QIODevice* AVDemuxer::ioDevice() const
{
    if (!d->input)
        return 0;
    if (d->input->name() != "QIODevice")
        return 0;
    QIODeviceInput* qin = static_cast<QIODeviceInput*>(d->input);
    if (!qin) {
        qWarning("Internal error.");
        return 0;
    }
    return qin->device();
}

AVInput* AVDemuxer::input() const
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
    if (d->file.startsWith("mms:"))
        d->file.insert(3, 'h');
    else if (d->file.startsWith(kFileScheme))
        d->file = getLocalPath(d->file);
    d->media_changed = url_old == d->file;
    // a local file. return here to avoid protocol checking. If path contains ":", protocol checking will fail
    if (d->file.startsWith(QChar('/')))
        return d->media_changed;
    // use AVInput to support protocols not supported by ffmpeg
    int colon = d->file.indexOf(QChar(':'));
    if (colon >= 0) {
#ifdef Q_OS_WIN
        if (colon == 1 && d->file.at(0).isLetter())
            return d->media_changed;
#endif
        const QString scheme = colon == 0 ? "qrc" : d->file.left(colon);
        // supportedProtocols() is not complete. so try AVInput 1st, if not found, fallback to libavformat
        d->input = AVInput::createForProtocol(scheme);
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
        if (d->input->name() != "QIODevice") {
            delete d->input;
            d->input = 0;
        }
    }
    if (!d->input)
        d->input = AVInput::create("QIODevice");
    QIODeviceInput *qin = static_cast<QIODeviceInput*>(d->input);
    if (!qin) {
        qWarning("Internal error: can not create AVInput for QIODevice.");
        return true;
    }
    // TODO: use property?
    d->media_changed = qin->device() == device;
    qin->setIODevice(device); //open outside?
    return d->media_changed;
}

bool AVDemuxer::setMedia(AVInput *in)
{
    d->media_changed = in == d->input;
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

bool AVDemuxer::load()
{
    unload();
    qDebug("all closed and reseted");

    if (d->file.isEmpty() && !d->input) {
        setMediaStatus(NoMedia);
        return false;
    }
    d->checkNetwork();
#if QTAV_HAVE(AVDEVICE)
    static const QString avd_scheme("avdevice:");
    if (d->file.startsWith(avd_scheme)) {
        QStringList parts = d->file.split(":");
        if (parts.count() != 3) {
            qDebug("invalid avdevice specification");
            return false;
        }
        if (d->file.startsWith(avd_scheme + "//")) {
            // avdevice://avfoundation:device_name
            d->input_format = av_find_input_format(parts[1].mid(2).toUtf8().constData());
        } else {
            // avdevice:video4linux2:file_name
            d->input_format = av_find_input_format(parts[1].toUtf8().constData());
        }
        d->file = parts[2];
    }
#endif
    //alloc av format context
    if (!d->format_ctx)
        d->format_ctx = avformat_alloc_context();
    d->format_ctx->flags |= AVFMT_FLAG_GENPTS;
    //install interrupt callback
    d->format_ctx->interrupt_callback = *d->interrupt_hanlder;

    setMediaStatus(LoadingMedia);
    int ret;
    applyOptionsForDict();
    if (d->input) {
        d->format_ctx->pb = (AVIOContext*)d->input->avioContext();
        d->format_ctx->flags |= AVFMT_FLAG_CUSTOM_IO;
        qDebug("avformat_open_input: d->format_ctx:'%p'..., AVInput('%s'): %p", d->format_ctx, d->input->name().toUtf8().constData(), d->input);
        d->interrupt_hanlder->begin(InterruptHandler::Open);
        ret = avformat_open_input(&d->format_ctx, "AVInput", d->input_format, d->options.isEmpty() ? NULL : &d->dict);
        d->interrupt_hanlder->end();
        qDebug("avformat_open_input: (with AVInput) ret:%d", ret);
    } else {
        qDebug("avformat_open_input: d->format_ctx:'%p', url:'%s'...",d->format_ctx, qPrintable(d->file));
        d->interrupt_hanlder->begin(InterruptHandler::Open);
        ret = avformat_open_input(&d->format_ctx, d->file.toUtf8().constData(), d->input_format, d->options.isEmpty() ? NULL : &d->dict);
        d->interrupt_hanlder->end();
        qDebug("avformat_open_input: url:'%s' ret:%d",qPrintable(d->file), ret);
    }
    if (ret < 0) {
        // d->format_ctx is 0
        AVError::ErrorCode ec = AVError::OpenError;
        QString msg = tr("failed to open media");
        handleError(ret, &ec, msg);
        qWarning() << "Can't open media: " << msg;
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
        AVError::ErrorCode ec(AVError::FindStreamInfoError);
        QString msg(tr("failed to find stream info"));
        handleError(ret, &ec, msg);
        qWarning() << "Can't find stream info: " << msg;
        return false;
    }

    if (!prepareStreams()) {
        return false;
    }

    d->started = false;
    setMediaStatus(LoadedMedia);
    emit loaded();
    return true;
}

bool AVDemuxer::unload()
{
    d->network = false;
    d->has_attached_pic = false;
    d->eof = false; // true and set false in load()?
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
        emit unloaded();
    }
    return true;
}

bool AVDemuxer::isLoaded() const
{
    return d->format_ctx && (d->astream.avctx || d->vstream.avctx || d->sstream.avctx);
}

bool AVDemuxer::prepareStreams()
{
    d->has_attached_pic = false;
    d->resetStreams();
    if (!d->format_ctx)
        return false;
    AVMediaType type = AVMEDIA_TYPE_UNKNOWN;
    for (unsigned int i = 0; i < d->format_ctx->nb_streams; ++i) {
        type = d->format_ctx->streams[i]->codec->codec_type;
        if (type == AVMEDIA_TYPE_VIDEO) {
            d->video_streams.push_back(i);
        } else if (type == AVMEDIA_TYPE_AUDIO) {
            d->audio_streams.push_back(i);
        } else if (type == AVMEDIA_TYPE_SUBTITLE) {
            d->subtitle_streams.push_back(i);
        }
    }
    if (d->audio_streams.isEmpty() && d->video_streams.isEmpty() && d->subtitle_streams.isEmpty())
        return false;
    setStream(AudioStream, -1);
    setStream(VideoStream, -1);
    setStream(SubtitleStream, -1);
    return true;
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
    if (index >= streams->size() || index < 0) {
        //si->wanted_stream = -1;
        qWarning("invalid index %d (valid is 0~%d) for stream type %d.", index, streams->size(), st);
        return false;
    }
    if (!setStream(st, streams->at(index)))
        return false;
    si->wanted_index = index;
    return true;
}

bool AVDemuxer::setStream(StreamType st, int streamValue)
{
    if (streamValue < -1)
        streamValue = -1;
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
        s = av_find_best_stream(d->format_ctx
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
    si->avctx = d->format_ctx->streams[s]->codec;
    d->has_attached_pic = !!(d->format_ctx->streams[s]->disposition & AV_DISPOSITION_ATTACHED_PIC);
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
    return d->format_ctx->iformat->name;
}

QString AVDemuxer::formatLongName() const
{
    if (!d->format_ctx)
        return QString();
    return d->format_ctx->iformat->long_name;
}

// convert to s using AV_TIME_BASE then *1000?
qint64 AVDemuxer::startTime() const
{
    return startTimeUs()/1000LL;
}

qint64 AVDemuxer::duration() const
{
    return durationUs()/1000LL; //time base: AV_TIME_BASE
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

/**
 * @brief getInterruptStatus return the interrupt status
 * @return
 */
bool AVDemuxer::getInterruptStatus() const
{
    return d->interrupt_hanlder->getStatus() == 1 ? true : false;
}

/**
 * @brief setInterruptStatus set the interrupt status
 * @param interrupt
 * @return
 */
void AVDemuxer::setInterruptStatus(bool interrupt)
{
    d->interrupt_hanlder->setStatus(interrupt ? 1 : 0);
}

void AVDemuxer::setOptions(const QVariantHash &dict)
{
    d->options = dict;
    applyOptionsForContext(); // apply even if avformat context is open
}

void AVDemuxer::applyOptionsForDict()
{
    if (d->dict) {
        av_dict_free(&d->dict);
        d->dict = 0; //aready 0 in av_free
    }
    if (d->options.isEmpty())
        return;
    QVariant opt(d->options);
    if (d->options.contains("avformat")) {
        opt = d->options.value("avformat");
        if (opt.type() == QVariant::Map) {
            QVariantMap avformat_dict(opt.toMap());
            if (avformat_dict.isEmpty())
                return;
            QMapIterator<QString, QVariant> i(avformat_dict);
            while (i.hasNext()) {
                i.next();
                const QVariant::Type vt = i.value().type();
                if (vt == QVariant::Map)
                    continue;
                const QByteArray key(i.key().toLower().toUtf8());
                av_dict_set(&d->dict, key.constData(), i.value().toByteArray().constData(), 0); // toByteArray: bool is "true" "false"
                qDebug("avformat dict: %s=>%s", i.key().toUtf8().constData(), i.value().toByteArray().constData());
            }
            return;
        }
    }
    QVariantHash avformat_dict(opt.toHash());
    if (avformat_dict.isEmpty())
        return;
    QHashIterator<QString, QVariant> i(avformat_dict);
    while (i.hasNext()) {
        i.next();
        const QVariant::Type vt = i.value().type();
        if (vt == QVariant::Hash)
            continue;
        const QByteArray key(i.key().toLower().toUtf8());
        av_dict_set(&d->dict, key.constData(), i.value().toByteArray().constData(), 0); // toByteArray: bool is "true" "false"
        qDebug("avformat dict: %s=>%s", i.key().toUtf8().constData(), i.value().toByteArray().constData());
    }
}

void AVDemuxer::applyOptionsForContext()
{
    if (!d->format_ctx)
        return;
    if (d->options.isEmpty()) {
        //av_opt_set_defaults(d->format_ctx);  //can't set default values! result maybe unexpected
        return;
    }
    QVariant opt(d->options);
    if (d->options.contains("avformat")) {
        opt = d->options.value("avformat");
        if (opt.type() == QVariant::Map) {
            QVariantMap avformat_dict(opt.toMap());
            if (avformat_dict.isEmpty())
                return;
            QMapIterator<QString, QVariant> i(avformat_dict);
            while (i.hasNext()) {
                i.next();
                const QVariant::Type vt = i.value().type();
                if (vt == QVariant::Map)
                    continue;
                const QByteArray key(i.key().toLower().toUtf8());
                qDebug("avformat option: %s=>%s", i.key().toUtf8().constData(), i.value().toByteArray().constData());
                if (vt == QVariant::Int || vt == QVariant::UInt || vt == QVariant::Bool) {
                    av_opt_set_int(d->format_ctx, key.constData(), i.value().toInt(), 0);
                } else if (vt == QVariant::LongLong || vt == QVariant::ULongLong) {
                    av_opt_set_int(d->format_ctx, key.constData(), i.value().toLongLong(), 0);
                }
            }
            return;
        }
    }
    QVariantHash avformat_dict(opt.toHash());
    if (avformat_dict.isEmpty())
        return;
    QHashIterator<QString, QVariant> i(avformat_dict);
    while (i.hasNext()) {
        i.next();
        const QVariant::Type vt = i.value().type();
        if (vt == QVariant::Hash)
            continue;
        const QByteArray key(i.key().toLower().toUtf8());
        qDebug("avformat option: %s=>%s", i.key().toUtf8().constData(), i.value().toByteArray().constData());
        if (vt == QVariant::Int || vt == QVariant::UInt || vt == QVariant::Bool) {
            av_opt_set_int(d->format_ctx, key.constData(), i.value().toInt(), 0);
        } else if (vt == QVariant::LongLong || vt == QVariant::ULongLong) {
            av_opt_set_int(d->format_ctx, key.constData(), i.value().toLongLong(), 0);
        }
    }
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

    emit mediaStatusChanged(d->media_status);
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
        setMediaStatus(StalledMedia);
        if (getInterruptStatus())
            err_msg += " [" + tr("interrupted by user") + "]";
        else
            err_msg += " [" + tr("timeout") + "]";
        emit userInterrupted();
    } else {
        if (mediaStatus() == LoadingMedia)
            setMediaStatus(InvalidMedia);
    }
    if (!errorCode)
        return;
    AVError::ErrorCode ec(AVError::OpenError);
    if (averr == AVERROR_INVALIDDATA) { // leave it if reading
        if (*errorCode == AVError::OpenError)
            ec = AVError::FormatError;
    } else {
        // Input/output error etc.
        if (d->network)
            ec = AVError::NetworkError;
    }
    AVError err(ec, err_msg, averr);
    emit error(err);
    msg = err_msg;
    *errorCode = ec;
}

} //namespace QtAV
