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

AVDemuxer::AVDemuxer(const QString& fileName, QObject *parent)
    :QObject(parent)
    , mCurrentMediaStatus(NoMedia)
    , has_attached_pic(false)
    , started_(false)
    , eof(false)
    , auto_reset_stream(true)
    , ipts(0)
    , stream_idx(-1)
    , wanted_audio_stream(-1)
    , wanted_video_stream(-1)
    , wanted_subtitle_stream(-1)
    , audio_stream(-2)
    , video_stream(-2)
    , subtitle_stream(-2)
    , format_context(0)
    , a_codec_context(0)
    , v_codec_context(0)
    , s_codec_context(0)
    , _file_name(fileName)
    , _iformat(0)
    , m_in(0)
    , mSeekUnit(SeekByTime)
    , mSeekTarget(SeekTarget_AccurateFrame)
    , mpDict(0)
    , m_network(false)
{
    class AVInitializer {
    public:
        AVInitializer() {
            //qDebug("av_register_all, avcodec_register_all, avformat_network_init");
            avcodec_register_all();
#if QTAV_HAVE(AVDEVICE)
            avdevice_register_all();
#endif
            av_register_all();
            avformat_network_init();
        }
        ~AVInitializer() {
            qDebug("avformat_network_deinit");
            avformat_network_deinit();
        }
    };
    static AVInitializer sAVInit;
    Q_UNUSED(sAVInit);
    mpInterrup = new InterruptHandler(this);
    if (!_file_name.isEmpty())
        loadFile(_file_name);
}

AVDemuxer::~AVDemuxer()
{
    close();
    if (mpDict) {
        av_dict_free(&mpDict);
    }
    delete mpInterrup;
    if (m_in)
        delete m_in;
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
    return mCurrentMediaStatus;
}

bool AVDemuxer::readFrame()
{
    if (!format_context)
        return false;
    m_pkt = Packet();
    // no lock required because in AVDemuxThread read and seek are in the same thread
    AVPacket packet;
    mpInterrup->begin(InterruptHandler::Read);
    int ret = av_read_frame(format_context, &packet); //0: ok, <0: error/end
    mpInterrup->end();

    if (ret < 0) {
        //ffplay: AVERROR_EOF || url_eof() || avsq.empty()
        //end of file. FIXME: why no eof if replaying by seek(0)?
        if (ret == AVERROR_EOF
                // AVFMT_NOFILE(e.g. network streams) stream has no pb
                // ffplay check pb && pb->error, mpv does not
                || format_context->pb/* && format_context->pb->error*/) {
            if (!eof) {
                eof = true;
                started_ = false;
                setMediaStatus(EndOfMedia);
                qDebug("End of file. %s %d", __FUNCTION__, __LINE__);
                emit finished();
            }
            return false; //frames after eof are eof frames
        }
        AVError::ErrorCode ec(AVError::ReadError);
        QString msg(tr("error reading stream data"));
        handleError(ret, &ec, msg);
        qWarning("[AVDemuxer] error: %s", av_err2str(ret));
        return false;
    }
    stream_idx = packet.stream_index; //TODO: check index
    //check whether the 1st frame is alreay got. emit only once
    if (!started_) {
        started_ = true;
        emit started();
    }
    if (stream_idx != videoStream() && stream_idx != audioStream() && stream_idx != subtitleStream()) {
        //qWarning("[AVDemuxer] unknown stream index: %d", stream_idx);
        return false;
    }
    m_pkt = Packet::fromAVPacket(&packet, av_q2d(format_context->streams[stream_idx]->time_base));
    av_free_packet(&packet); //important!
    return true;
}

Packet AVDemuxer::packet() const
{
    return m_pkt;
}

int AVDemuxer::stream() const
{
    return stream_idx;
}

bool AVDemuxer::atEnd() const
{
    return eof;
}

bool AVDemuxer::close()
{
    m_network = false;
    has_attached_pic = false;
    eof = false; // true and set false in load()?
    stream_idx = -1;
    if (auto_reset_stream) {
        wanted_audio_stream = wanted_subtitle_stream = wanted_video_stream = -1;
    }
    a_codec_context = v_codec_context = s_codec_context = 0;
    audio_stream = video_stream = subtitle_stream = -2;
    audio_streams.clear();
    video_streams.clear();
    subtitle_streams.clear();
    mpInterrup->setStatus(0);
    //av_close_input_file(format_context); //deprecated
    if (format_context) {
        qDebug("closing format_context");
        avformat_close_input(&format_context); //libavf > 53.10.0
        format_context = 0;
        _iformat = 0;
        // no delete. may be used in next load
        if (m_in)
            m_in->release();
    }
    emit unloaded();
    return true;
}

bool AVDemuxer::isSeekable() const
{
    return true;
}

void AVDemuxer::setSeekUnit(SeekUnit unit)
{
    mSeekUnit = unit;
}

AVDemuxer::SeekUnit AVDemuxer::seekUnit() const
{
    return mSeekUnit;
}

void AVDemuxer::setSeekTarget(SeekTarget target)
{
    mSeekTarget = target;
}

AVDemuxer::SeekTarget AVDemuxer::seekTarget() const
{
    return mSeekTarget;
}

//TODO: seek by byte
bool AVDemuxer::seek(qint64 pos)
{
    if ((!a_codec_context && !v_codec_context) || !format_context) {
        qWarning("can not seek. context not ready: %p %p %p", a_codec_context, v_codec_context, format_context);
        return false;
    }
    //duration: unit is us (10^-6 s, AV_TIME_BASE)
    qint64 upos = pos*1000LL;
    if (upos > startTimeUs() + durationUs() || pos < 0LL) {
        qWarning("Invalid seek position %lld %.2f. valid range [%lld, %lld]", upos, double(upos)/double(durationUs()), startTimeUs(), startTimeUs()+durationUs());
        return false;
    }
    // no lock required because in AVDemuxThread read and seek are in the same thread
#if 0
    //t: unit is s
    qreal t = q;// * (double)format_context->duration; //
    int ret = av_seek_frame(format_context, -1, (int64_t)(t*AV_TIME_BASE), t > m_pkt.pts ? 0 : AVSEEK_FLAG_BACKWARD);
    qDebug("[AVDemuxer] seek to %f %f %lld / %lld", q, m_pkt.pts, (int64_t)(t*AV_TIME_BASE), durationUs());
#else
    //TODO: m_pkt.pts may be 0, compute manually.

    bool backward = mSeekTarget == SeekTarget_AccurateFrame || upos <= (int64_t)(m_pkt.pts*AV_TIME_BASE);
    //qDebug("[AVDemuxer] seek to %f %f %lld / %lld backward=%d", double(upos)/double(durationUs()), m_pkt.pts, upos, durationUs(), backward);
    //AVSEEK_FLAG_BACKWARD has no effect? because we know the timestamp
    // FIXME: back flag is opposite? otherwise seek is bad and may crash?
    /* If stream_index is (-1), a default
     * stream is selected, and timestamp is automatically converted
     * from AV_TIME_BASE units to the stream specific time_base.
     */
    int seek_flag = (backward ? AVSEEK_FLAG_BACKWARD : 0);
    if (mSeekTarget == SeekTarget_AccurateFrame) {
        seek_flag = AVSEEK_FLAG_BACKWARD;
    }
    if (mSeekTarget == SeekTarget_AnyFrame) {
        seek_flag = AVSEEK_FLAG_ANY;
    }
    //bool seek_bytes = !!(format_context->iformat->flags & AVFMT_TS_DISCONT) && strcmp("ogg", format_context->iformat->name);
    int ret = av_seek_frame(format_context, -1, upos, seek_flag);
    //int ret = avformat_seek_file(format_context, -1, INT64_MIN, upos, upos, seek_flag);
    //avformat_seek_file()
#endif
    if (ret < 0) {
        AVError::ErrorCode ec(AVError::SeekError);
        QString msg(tr("seek error"));
        handleError(ret, &ec, msg);
        return false;
    }
    //replay
    if (upos <= startTime()) {
        qDebug("************seek to beginning. started = false");
        started_ = false;
        if (a_codec_context)
            a_codec_context->frame_number = 0;
        if (v_codec_context)
            v_codec_context->frame_number = 0; //TODO: why frame_number not changed after seek?
        if (s_codec_context)
            s_codec_context->frame_number = 0;
    }
    return true;
}

void AVDemuxer::seek(qreal q)
{
    seek(qint64(q*(double)duration()));
}

/*
 TODO: seek by byte/frame
  We need to know current playing packet but not current demuxed packet which
  may blocked for a while
*/
bool AVDemuxer::isLoaded(const QString &fileName) const
{
    // loadFile() modified the original path
    bool same_path = fileName == _file_name;
    if (!same_path) {
        // _file_name is already C:path for windows
        if (fileName.startsWith(kFileScheme)) { // for QUrl
            int idx = fileName.indexOf(_file_name);
            same_path = idx > 0 && fileName.midRef(CHAR_COUNT(kFileScheme), idx - CHAR_COUNT(kFileScheme)).count(QChar('/')) == idx - (int)CHAR_COUNT(kFileScheme);
        }
    }
    if (!same_path) {
        if (_file_name.startsWith("mms:")) // compare with mmsh:
            same_path = _file_name.midRef(5) == fileName.midRef(4);
    }
    return same_path && (a_codec_context || v_codec_context || s_codec_context);
}

bool AVDemuxer::isLoaded(QIODevice *dev) const
{
    if (!m_in)
        return false;
    if (m_in->name() != "QIODevice")
        return false;
    QIODeviceInput* qin = static_cast<QIODeviceInput*>(m_in);
    if (!qin) {
        qWarning("Internal error.");
        return false;
    }
    if (qin->device() != dev)
        return false;
    return a_codec_context || v_codec_context || s_codec_context;
}

bool AVDemuxer::isLoaded(AVInput *in) const
{
    if (m_in != in)
        return false;
    return a_codec_context || v_codec_context || s_codec_context;
}

bool AVDemuxer::loadFile(const QString &fileName)
{
    if (m_in) {
        delete m_in;
        m_in = 0;
    }
    _file_name = fileName.trimmed();
    if (_file_name.startsWith("mms:"))
        _file_name.insert(3, 'h');
    else if (_file_name.startsWith(kFileScheme))
        _file_name = getLocalPath(_file_name);
    // a local file. return here to avoid protocol checking. If path contains ":", protocol checking will fail
    if (_file_name.startsWith(QChar('/')))
        return load();
    // use AVInput to support protocols not supported by ffmpeg
    int colon = _file_name.indexOf(QChar(':'));
    if (colon >= 0) {
#ifdef Q_OS_WIN
        if (colon == 1 && _file_name.at(0).isLetter())
            return load();
#endif
        const QString scheme = colon == 0 ? "qrc" : _file_name.left(colon);
        // supportedProtocols() is not complete. so try AVInput 1st, if not found, fallback to libavformat
        m_in = AVInput::createForProtocol(scheme);
        if (m_in) {
            m_in->setUrl(_file_name);
        }
    }
    return load();
}
#undef CHAR_COUNT

bool AVDemuxer::load(QIODevice* device)
{
    _file_name = QString();
    if (m_in) {
        if (m_in->name() != "QIODevice") {
            delete m_in;
            m_in = 0;
        }
    }
    if (!m_in)
        m_in = AVInput::create("QIODevice");
    QIODeviceInput *qin = static_cast<QIODeviceInput*>(m_in);
    if (!qin) {
        qWarning("Internal error: can not create AVInput for QIODevice.");
        return false;
    }
    // TODO: use property?
    qin->setIODevice(device); //open outside?
    return load();
}

bool AVDemuxer::load(AVInput *in)
{
    _file_name = QString();
    if (!m_in)
        m_in = in;
    if (m_in != in) {
        delete m_in;
        m_in = in;
    }
    return load();
}

bool AVDemuxer::load()
{
    close();
    qDebug("all closed and reseted");

    if (_file_name.isEmpty() && !m_in) {
        setMediaStatus(NoMedia);
        return false;
    }
    // FIXME: is there a good way to check network? now use URLContext.flags == URL_PROTOCOL_FLAG_NETWORK
    // not network: concat cache pipe avdevice crypto?
    if (!_file_name.isEmpty()
            && _file_name.contains(":")
            && (_file_name.startsWith("http") //http, https, httpproxy
            || _file_name.startsWith("rtmp") //rtmp{,e,s,te,ts}
            || _file_name.startsWith("mms") //mms{,h,t}
            || _file_name.startsWith("ffrtmp") //ffrtmpcrypt, ffrtmphttp
            || _file_name.startsWith("rtp:")
            || _file_name.startsWith("sctp:")
            || _file_name.startsWith("tcp:")
            || _file_name.startsWith("tls:")
            || _file_name.startsWith("udp:")
            || _file_name.startsWith("gopher:")
            )) {
        m_network = true; //iformat.flags: AVFMT_NOFILE
    }
#if QTAV_HAVE(AVDEVICE)
    static const QString avd_scheme("avdevice:");
    if (_file_name.startsWith(avd_scheme)) {
        QStringList parts = _file_name.split(":");
        if (parts.count() != 3) {
            qDebug("invalid avdevice specification");
            return false;
        }
        if (_file_name.startsWith(avd_scheme + "//")) {
            // avdevice://avfoundation:device_name
            _iformat = av_find_input_format(parts[1].mid(2).toUtf8().constData());
        } else {
            // avdevice:video4linux2:file_name
            _iformat = av_find_input_format(parts[1].toUtf8().constData());
        }
        _file_name = parts[2];
    }
#endif
    //alloc av format context
    if (!format_context)
        format_context = avformat_alloc_context();
    format_context->flags |= AVFMT_FLAG_GENPTS;
    //install interrupt callback
    format_context->interrupt_callback = *mpInterrup;

    setMediaStatus(LoadingMedia);
    int ret;
    applyOptionsForDict();
    if (m_in) {
        format_context->pb = (AVIOContext*)m_in->avioContext();
        format_context->flags |= AVFMT_FLAG_CUSTOM_IO;
        qDebug("avformat_open_input: format_context:'%p'..., AVInput('%s'): %p", format_context, m_in->name().toUtf8().constData(), m_in);
        mpInterrup->begin(InterruptHandler::Open);
        ret = avformat_open_input(&format_context, "AVInput", _iformat, mOptions.isEmpty() ? NULL : &mpDict);
        mpInterrup->end();
        qDebug("avformat_open_input: (with AVInput) ret:%d", ret);
    } else {
        qDebug("avformat_open_input: format_context:'%p', url:'%s'...",format_context, qPrintable(_file_name));
        mpInterrup->begin(InterruptHandler::Open);
        ret = avformat_open_input(&format_context, _file_name.toUtf8().constData(), _iformat, mOptions.isEmpty() ? NULL : &mpDict);
        mpInterrup->end();
        qDebug("avformat_open_input: url:'%s' ret:%d",qPrintable(_file_name), ret);
    }
    if (ret < 0) {
        // format_context is 0
        AVError::ErrorCode ec = AVError::OpenError;
        QString msg = tr("failed to open media");
        handleError(ret, &ec, msg);
        qWarning() << "Can't open media: " << msg;
        return false;
    }
    //deprecated
    //if(av_find_stream_info(format_context)<0) {
    //TODO: avformat_find_stream_info is too slow, only useful for some video format
    mpInterrup->begin(InterruptHandler::FindStreamInfo);
    ret = avformat_find_stream_info(format_context, NULL);
    mpInterrup->end();
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

    started_ = false;
    setMediaStatus(LoadedMedia);
    emit loaded();
    return true;
}

bool AVDemuxer::prepareStreams()
{
    has_attached_pic = false;
    if (!findStreams())
        return false;
    setStream(AudioStream, -1);
    setStream(VideoStream, -1);
    setStream(SubtitleStream, -1);
    return true;
}

bool AVDemuxer::hasAttacedPicture() const
{
    return has_attached_pic;
}

void AVDemuxer::setAutoResetStream(bool reset)
{
    auto_reset_stream = reset;
}

bool AVDemuxer::autoResetStream() const
{
    return auto_reset_stream;
}

bool AVDemuxer::setStreamIndex(StreamType st, int index)
{
    QList<int> *streams = 0;
    int *wanted_stream = 0;
    int *wanted_index = 0;
    if (st == AudioStream) {
        wanted_index = &wanted_audio_index;
        wanted_stream = &wanted_audio_stream;
        streams = &audio_streams;
    } else if (st == VideoStream) {
        wanted_index = &wanted_video_index;
        wanted_stream = &wanted_video_stream;
        streams = &video_streams;
    } else if (st == SubtitleStream) {
        wanted_index = &wanted_subtitle_index;
        wanted_stream = &wanted_subtitle_stream;
        streams = &subtitle_streams;
    }
    if (!streams) {
        qWarning("stream type %d for index %d not found", st, index);
        return false;
    }
    if (!wanted_stream) {
        qWarning("invalid stream type");
        return false;
    }
    if (index >= streams->size() || index < 0) {
        //*wanted_stream = -1;
        qWarning("invalid index %d (valid is 0~%d) for stream type %d.", index, streams->size(), st);
        return false;
    }
    if (!setStream(st, streams->at(index)))
        return false;
    *wanted_index = index;
    return true;
}

bool AVDemuxer::setStream(StreamType st, int streamValue)
{
    int *wanted_stream = 0;
    int *wanted_index = 0;
    int *stream = 0;
    AVCodecContext **avctx = 0;
    QList<int> *streams = 0;
    if (st == AudioStream) {
        stream = &audio_stream;
        avctx = &a_codec_context;
        wanted_index = &wanted_audio_index;
        wanted_stream = &wanted_audio_stream;
        streams = &audio_streams;
    } else if (st == VideoStream) {
        stream = &video_stream;
        avctx = &v_codec_context;
        wanted_index = &wanted_video_index;
        wanted_stream = &wanted_video_stream;
        streams = &video_streams;
    } else if (st == SubtitleStream) {
        stream = &subtitle_stream;
        avctx = &s_codec_context;
        wanted_index = &wanted_subtitle_index;
        wanted_stream = &wanted_subtitle_stream;
        streams = &subtitle_streams;
    }
    if (!wanted_stream/* || *wanted_stream == stream*/) { //init -2
        qWarning("stream type %d not found or stream %d not changed", st, stream);
        return false;
    }
    //if (!streams->contains(stream)) {
      //  qWarning("%d is not a valid stream for stream type %d", stream, st);
        //return false;
    //}
    if (streamValue < -1)
        streamValue = -1;
    bool index_valid = *wanted_index >= 0 && *wanted_index < streams->size();
    int s = AVERROR_STREAM_NOT_FOUND;
    if (streamValue >= 0 || !index_valid) {
        // or simply set s to streamValue if value is contained in streams?
        s = av_find_best_stream(format_context
                                , st == AudioStream ? AVMEDIA_TYPE_AUDIO
                                : st == VideoStream ? AVMEDIA_TYPE_VIDEO
                                : st == SubtitleStream ? AVMEDIA_TYPE_SUBTITLE
                                : AVMEDIA_TYPE_UNKNOWN
                                , streamValue, -1, NULL, 0);
    } else { //index_valid
        s = streams->at(*wanted_index);
    }
    if (s == AVERROR_STREAM_NOT_FOUND)
        return false;
    // don't touch wanted index
    *stream = s;
    *wanted_stream = streamValue;
    *avctx = format_context->streams[*stream]->codec;
    has_attached_pic = !!(format_context->streams[*stream]->disposition & AV_DISPOSITION_ATTACHED_PIC);
    return true;
}

AVFormatContext* AVDemuxer::formatContext()
{
    return format_context;
}

QString AVDemuxer::fileName() const
{
    return format_context->filename;
}

QString AVDemuxer::videoFormatName() const
{
    return formatName(format_context, false);
}

QString AVDemuxer::videoFormatLongName() const
{
    return formatName(format_context, true);
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
    if (!format_context || format_context->start_time == AV_NOPTS_VALUE)
        return 0;
    return format_context->start_time;
}

qint64 AVDemuxer::durationUs() const
{
    if (!format_context || format_context->duration == AV_NOPTS_VALUE)
        return 0;
    return format_context->duration; //time base: AV_TIME_BASE
}

int AVDemuxer::bitRate() const
{
    return format_context->bit_rate;
}

int AVDemuxer::audioBitRate(int stream) const
{
    AVCodecContext *avctx = audioCodecContext(stream);
    if (!avctx)
        return 0;
    return avctx->bit_rate;
}

int AVDemuxer::videoBitRate(int stream) const
{
    AVCodecContext *avctx = videoCodecContext(stream);
    if (!avctx)
        return 0;
    return avctx->bit_rate;
}

qreal AVDemuxer::frameRate() const
{
    if (videoStream() < 0)
        return 0;
    AVStream *stream = format_context->streams[videoStream()];
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
    return format_context->streams[stream]->nb_frames;
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
    return audio_stream;
}

QList<int> AVDemuxer::audioStreams() const
{
    return audio_streams;
}

int AVDemuxer::videoStream() const
{
    return video_stream;
}

QList<int> AVDemuxer::videoStreams() const
{
    return video_streams;
}

int AVDemuxer::subtitleStream() const
{
    return subtitle_stream;
}

QList<int> AVDemuxer::subtitleStreams() const
{
    return subtitle_streams;
}

int AVDemuxer::width() const
{
    return videoCodecContext()->width;
}

int AVDemuxer::height() const
{
    return videoCodecContext()->height;
}

//codec
AVCodecContext* AVDemuxer::audioCodecContext(int stream) const
{
    if (stream < 0)
        return a_codec_context;
    if (stream > (int)format_context->nb_streams)
        return 0;
    return format_context->streams[stream]->codec;
}

AVCodecContext* AVDemuxer::videoCodecContext(int stream) const
{
    if (stream < 0)
        return v_codec_context;
    if (stream > (int)format_context->nb_streams)
        return 0;
    return format_context->streams[stream]->codec;
}

AVCodecContext* AVDemuxer::subtitleCodecContext(int stream) const
{
    if (stream < 0)
        return s_codec_context;
    if (stream > (int)format_context->nb_streams)
        return 0;
    return format_context->streams[stream]->codec;
}

/*!
    call avcodec_open2() first!
    check null ptr?
*/
QString AVDemuxer::audioCodecName(int stream) const
{
    AVCodecContext *avctx = audioCodecContext(stream);
    if (!avctx)
        return QString();
    // AVCodecContext.codec_name is deprecated. use avcodec_get_name. check null avctx->codec?
    return avcodec_get_name(avctx->codec_id);
}

QString AVDemuxer::audioCodecLongName(int stream) const
{
    AVCodecContext *avctx = audioCodecContext(stream);
    if (!avctx)
        return QString();
    return avctx->codec->long_name;
}

QString AVDemuxer::videoCodecName(int stream) const
{
    AVCodecContext *avctx = videoCodecContext(stream);
    if (!avctx)
        return QString();
    return avcodec_get_name(avctx->codec_id);
}

QString AVDemuxer::videoCodecLongName(int stream) const
{
    AVCodecContext *avctx = videoCodecContext(stream);
    if (!avctx)
        return QString();
    return avctx->codec->long_name;
}

QString AVDemuxer::subtitleCodecName(int stream) const
{
    AVCodecContext *avctx = subtitleCodecContext(stream);
    if (!avctx)
        return QString();
    return avcodec_get_name(avctx->codec_id);
}

QString AVDemuxer::subtitleCodecLongName(int stream) const
{
    AVCodecContext *avctx = subtitleCodecContext(stream);
    if (!avctx)
        return QString();
    return avctx->codec->long_name;
}

// TODO: use wanted_xx_stream?
bool AVDemuxer::findStreams()
{
    if (!format_context)
        return false;
    // close codecs?
    video_streams.clear();
    audio_streams.clear();
    subtitle_streams.clear();
    AVMediaType type = AVMEDIA_TYPE_UNKNOWN;
    for (unsigned int i=0; i<format_context->nb_streams; ++i) {
        type = format_context->streams[i]->codec->codec_type;
        if (type == AVMEDIA_TYPE_VIDEO) {
            video_streams.push_back(i);
        } else if (type == AVMEDIA_TYPE_AUDIO) {
            audio_streams.push_back(i);
        } else if (type == AVMEDIA_TYPE_SUBTITLE) {
            subtitle_streams.push_back(i);
        }
    }
    return !audio_streams.isEmpty() || !video_streams.isEmpty() || !subtitle_streams.isEmpty();
}

QString AVDemuxer::formatName(AVFormatContext *ctx, bool longName) const
{
    return longName ? ctx->iformat->long_name : ctx->iformat->name;
}

/**
 * @brief getInterruptTimeout return the interrupt timeout
 * @return
 */
qint64 AVDemuxer::getInterruptTimeout() const
{
    return mpInterrup->getTimeout();
}

/**
 * @brief setInterruptTimeout set the interrupt timeout
 * @param timeout
 * @return
 */
void AVDemuxer::setInterruptTimeout(qint64 timeout)
{
    mpInterrup->setTimeout(timeout);
}

/**
 * @brief getInterruptStatus return the interrupt status
 * @return
 */
bool AVDemuxer::getInterruptStatus() const
{
    return mpInterrup->getStatus() == 1 ? true : false;
}

/**
 * @brief setInterruptStatus set the interrupt status
 * @param interrupt
 * @return
 */
void AVDemuxer::setInterruptStatus(bool interrupt)
{
    mpInterrup->setStatus(interrupt ? 1 : 0);
}

void AVDemuxer::setOptions(const QVariantHash &dict)
{
    mOptions = dict;
    applyOptionsForContext(); // apply even if avformat context is open
}

void AVDemuxer::applyOptionsForDict()
{
    if (mpDict) {
        av_dict_free(&mpDict);
        mpDict = 0; //aready 0 in av_free
    }
    if (mOptions.isEmpty())
        return;
    QVariant opt(mOptions);
    if (mOptions.contains("avformat")) {
        opt = mOptions.value("avformat");
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
                av_dict_set(&mpDict, key.constData(), i.value().toByteArray().constData(), 0); // toByteArray: bool is "true" "false"
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
        av_dict_set(&mpDict, key.constData(), i.value().toByteArray().constData(), 0); // toByteArray: bool is "true" "false"
        qDebug("avformat dict: %s=>%s", i.key().toUtf8().constData(), i.value().toByteArray().constData());
    }
}

void AVDemuxer::applyOptionsForContext()
{
    if (!format_context)
        return;
    if (mOptions.isEmpty()) {
        //av_opt_set_defaults(format_context);  //can't set default values! result maybe unexpected
        return;
    }
    QVariant opt(mOptions);
    if (mOptions.contains("avformat")) {
        opt = mOptions.value("avformat");
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
                    av_opt_set_int(format_context, key.constData(), i.value().toInt(), 0);
                } else if (vt == QVariant::LongLong || vt == QVariant::ULongLong) {
                    av_opt_set_int(format_context, key.constData(), i.value().toLongLong(), 0);
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
            av_opt_set_int(format_context, key.constData(), i.value().toInt(), 0);
        } else if (vt == QVariant::LongLong || vt == QVariant::ULongLong) {
            av_opt_set_int(format_context, key.constData(), i.value().toLongLong(), 0);
        }
    }
}

QVariantHash AVDemuxer::options() const
{
    return mOptions;
}

void AVDemuxer::setMediaStatus(MediaStatus status)
{
    if (mCurrentMediaStatus == status)
        return;

    //if (status == NoMedia || status == InvalidMedia)
    //    Q_EMIT durationChanged(0);

    mCurrentMediaStatus = status;

    emit mediaStatusChanged(mCurrentMediaStatus);
}

void AVDemuxer::handleError(int averr, AVError::ErrorCode *errorCode, QString &msg)
{
    if (averr >= 0)
        return;
    // format_context is 0
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
        if (m_network)
            ec = AVError::NetworkError;
    }
    AVError err(ec, err_msg, averr);
    emit error(err);
    msg = err_msg;
    *errorCode = ec;
}

} //namespace QtAV
