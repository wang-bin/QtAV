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

#include <QtAV/AVDemuxer.h>
#include <QtAV/AVError.h>
#include <QtAV/Packet.h>
#include <QtAV/QtAV_Compat.h>
#include <QtAV/QAVIOContext.h>
#include <QtCore/QThread>
#include <QtCore/QCoreApplication>

namespace QtAV {

const qint64 kSeekInterval = 168; //ms

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
        mTimer.invalidate();
    }
    void begin(Action act) {
        mAction = act;
        mTimer.start();
    }
    void end() {
        mTimer.invalidate();
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
    //TODO: let demuxer emit a signal, and qApp processEvents
    static int handleTimeout(void* obj) {
        InterruptHandler* handler = static_cast<InterruptHandler*>(obj);
        if (!handler) {
            qWarning("InterruptHandler is null");
            return -1;
        }
        //check manual interruption
        if (handler->getStatus() > 0) {
            qDebug("User Interrupt: -> quit!");
            return 1;//interrupt
        }
        qApp->processEvents();
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
        if (!handler->mTimer.hasExpired(handler->mTimeout)) {
            return 0;
        }
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
    , started_(false)
    , eof(false)
    , auto_reset_stream(true)
    , pkt(new Packet())
    , ipts(0)
    , stream_idx(-1)
    , wanted_audio_stream(-1)
    , wanted_video_stream(-1)
    , wanted_subtitle_stream(-1)
    , audio_stream(-2)
    , video_stream(-2)
    , subtitle_stream(-2)
    , _is_input(true)
    , format_context(0)
    , a_codec_context(0)
    , v_codec_context(0)
    , s_codec_contex(0)
    , m_pQAVIO(0)
    , _file_name(fileName)
    , mSeekUnit(SeekByTime)
    , mSeekTarget(SeekTarget_AnyFrame)
    , mpDict(0)
{
    mpInterrup = new InterruptHandler(this);
    if (!_file_name.isEmpty())
        loadFile(_file_name);
}

AVDemuxer::~AVDemuxer()
{
    close();
    if (pkt) {
        delete pkt;
        pkt = 0;
    }
    if (mpDict) {
        av_dict_free(&mpDict);
    }
    delete mpInterrup;
}

MediaStatus AVDemuxer::mediaStatus() const
{
    return mCurrentMediaStatus;
}

bool AVDemuxer::readFrame()
{
    QMutexLocker lock(&mutex);
    Q_UNUSED(lock);
    AVPacket packet;

    mpInterrup->begin(InterruptHandler::Read);
    int ret = av_read_frame(format_context, &packet); //0: ok, <0: error/end
    mpInterrup->end();

    if (ret != 0) {
        //ffplay: AVERROR_EOF || url_eof() || avsq.empty()
        if (ret == AVERROR_EOF) { //end of file. FIXME: why no eof if replaying by seek(0)?
            if (!eof) {
                eof = true;
                started_ = false;
                pkt->data = QByteArray(); //flush
                pkt->markEnd();
                setMediaStatus(EndOfMedia);
                qDebug("End of file. %s %d", __FUNCTION__, __LINE__);
                emit finished();
                return true;
            }
            //pkt->data = QByteArray(); //flush
            //return true;
            return false; //frames after eof are eof frames
        } else if (ret == AVERROR_INVALIDDATA) {
            emit error(AVError(AVError::ReadError, ret));
            qWarning("AVERROR_INVALIDDATA");
        } else if (ret == AVERROR(EAGAIN)) {
            return true;
        } else {
            emit error(AVError(AVError::ReadError, ret));
        }
        qWarning("[AVDemuxer] error: %s", av_err2str(ret));
        return false;
    }
    stream_idx = packet.stream_index; //TODO: check index
    //check whether the 1st frame is alreay got. emit only once
    if (!started_) {
        started_ = true;
        emit started();
    }
    if (stream_idx != videoStream() && stream_idx != audioStream()) {
        //qWarning("[AVDemuxer] unknown stream index: %d", stream_idx);
        return false;
    }
    pkt->hasKeyFrame = !!(packet.flags & AV_PKT_FLAG_KEY);
    // what about marking packet as invalid and do not use isCorrupt?
    pkt->isCorrupt = !!(packet.flags & AV_PKT_FLAG_CORRUPT);
    pkt->data = QByteArray((const char*)packet.data, packet.size);
    pkt->duration = packet.duration;
    //if (packet.dts == AV_NOPTS_VALUE && )
    if (packet.dts != AV_NOPTS_VALUE) //has B-frames
        pkt->pts = packet.dts;
    else if (packet.pts != AV_NOPTS_VALUE)
        pkt->pts = packet.pts;
    else
        pkt->pts = 0;
    AVStream *stream = format_context->streams[stream_idx];
    pkt->pts *= av_q2d(stream->time_base);
    //TODO: pts must >= 0? look at ffplay
    pkt->pts = qMax<qreal>(0, pkt->pts);
    if (stream->codec->codec_type == AVMEDIA_TYPE_SUBTITLE
            && (packet.flags & AV_PKT_FLAG_KEY)
            &&  packet.convergence_duration != AV_NOPTS_VALUE)
        pkt->duration = packet.convergence_duration * av_q2d(stream->time_base);
    else if (packet.duration > 0)
        pkt->duration = packet.duration * av_q2d(stream->time_base);
    else
        pkt->duration = 0;
    //qDebug("AVPacket.pts=%f, duration=%f, dts=%lld", pkt->pts, pkt->duration, packet.dts);
    if (pkt->isCorrupt)
        qDebug("currupt packet. pts: %f", pkt->pts);

    av_free_packet(&packet); //important!
    return true;
}

Packet* AVDemuxer::packet() const
{
    return pkt;
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
    eof = false;
    stream_idx = -1;
    if (auto_reset_stream) {
        wanted_audio_stream = wanted_subtitle_stream = wanted_video_stream = -1;
    }
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
    }
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
    if (upos > durationUs() || pos < 0LL) {
        qWarning("Invalid seek position %lld %.2f. valid range [0, %lld]", upos, double(upos)/double(durationUs()), durationUs());
        return false;
    }
    if (seek_timer.isValid()) {
        //why sometimes seek_timer.elapsed() < 0
        if (!seek_timer.hasExpired(kSeekInterval)) {
            qDebug("seek too frequent. ignore");
            return false;
        }
        seek_timer.restart();
    } else {
        seek_timer.start();
    }

    QMutexLocker lock(&mutex);
    Q_UNUSED(lock);
#if 0
    //t: unit is s
    qreal t = q;// * (double)format_context->duration; //
    int ret = av_seek_frame(format_context, -1, (int64_t)(t*AV_TIME_BASE), t > pkt->pts ? 0 : AVSEEK_FLAG_BACKWARD);
    qDebug("[AVDemuxer] seek to %f %f %lld / %lld", q, pkt->pts, (int64_t)(t*AV_TIME_BASE), durationUs());
#else
    //TODO: pkt->pts may be 0, compute manually.

    bool backward = upos <= (int64_t)(pkt->pts*AV_TIME_BASE);
    qDebug("[AVDemuxer] seek to %f %f %lld / %lld backward=%d", double(upos)/double(durationUs()), pkt->pts, upos, durationUs(), backward);
    //AVSEEK_FLAG_BACKWARD has no effect? because we know the timestamp
    // FIXME: back flag is opposite? otherwise seek is bad and may crash?
    /* If stream_index is (-1), a default
     * stream is selected, and timestamp is automatically converted
     * from AV_TIME_BASE units to the stream specific time_base.
     */
    int seek_flag = (backward ? 0 : AVSEEK_FLAG_BACKWARD); //AVSEEK_FLAG_ANY
    //bool seek_bytes = !!(format_context->iformat->flags & AVFMT_TS_DISCONT) && strcmp("ogg", format_context->iformat->name);
    int ret = av_seek_frame(format_context, -1, upos, seek_flag);
    //avformat_seek_file()
#endif
    if (ret < 0) {
        qWarning("[AVDemuxer] seek error: %s", av_err2str(ret));
        return false;
    }
    //replay
    qDebug("startTime: %lld", startTime());
    if (upos <= startTime()) {
        qDebug("************seek to beginning. started = false");
        started_ = false;
        if (a_codec_context)
            a_codec_context->frame_number = 0;
        if (v_codec_context)
            v_codec_context->frame_number = 0; //TODO: why frame_number not changed after seek?
        if (s_codec_contex)
            s_codec_contex->frame_number = 0;
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
    return (fileName == _file_name || m_pQAVIO) && (a_codec_context || v_codec_context || s_codec_contex);
}

bool AVDemuxer::loadFile(const QString &fileName)
{
    _file_name = fileName.trimmed();
    if (_file_name.startsWith("mms:"))
        _file_name.insert(3, 'h');
    else if (_file_name.startsWith("file://"))
        _file_name.remove("file://");
    m_pQAVIO = 0;
    return load();
}

bool AVDemuxer::load(QAVIOContext* iocon)
{
    m_pQAVIO = iocon;
    return load();
}

bool AVDemuxer::load()
{
    class AVInitializer {
    public:
        AVInitializer() {
            qDebug("av_register_all and avformat_network_init");
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
    close();
    qDebug("all closed and reseted");

    if (_file_name.isEmpty()) {
        setMediaStatus(NoMedia);
        return false;
    }

    //alloc av format context
    if (!format_context)
        format_context = avformat_alloc_context();
    format_context->flags |= AVFMT_FLAG_GENPTS;
    //install interrupt callback
    format_context->interrupt_callback = *mpInterrup;

    setMediaStatus(LoadingMedia);
    int ret;
    if (m_pQAVIO) {
        format_context->pb = m_pQAVIO->context();
        format_context->flags |= AVFMT_FLAG_CUSTOM_IO;

        qDebug("avformat_open_input: format_context:'%p'...",format_context);
        mpInterrup->begin(InterruptHandler::Open);
        ret = avformat_open_input(&format_context, "iodevice", NULL, mOptions.isEmpty() ? NULL : &mpDict);
        mpInterrup->end();
        qDebug("avformat_open_input: (with io device) ret:%d", ret);
    } else {
        qDebug("avformat_open_input: format_context:'%p', url:'%s'...",format_context, qPrintable(_file_name));
        mpInterrup->begin(InterruptHandler::Open);
        ret = avformat_open_input(&format_context, qPrintable(_file_name), NULL, mOptions.isEmpty() ? NULL : &mpDict);
        mpInterrup->end();
        qDebug("avformat_open_input: url:'%s' ret:%d",qPrintable(_file_name), ret);
    }

    if (ret < 0) {
        setMediaStatus(InvalidMedia);
        AVError err(AVError::OpenError, ret);
        emit error(err);
        qWarning("Can't open media: %s", qPrintable(err.string()));
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
        AVError err(AVError::FindStreamInfoError, ret);
        emit error(err);
        qWarning("Can't find stream info: %s", qPrintable(err.string()));
        return false;
    }

    if (!prepareStreams()) {
        return false;
    }

    started_ = false;
    setMediaStatus(LoadedMedia);
    return true;
}

bool AVDemuxer::prepareStreams()
{
    if (!findStreams())
        return false;
    // wanted_xx_stream < nb_streams and +valied is always true because setStream() and setStreamIndex() ensure it correct
    int stream = wanted_audio_stream < 0 ? audioStream() : wanted_audio_stream;
    if (stream >= 0) {
        a_codec_context = format_context->streams[stream]->codec;
        audio_stream = stream; //audio_stream is the currently opened stream
    }
    stream = wanted_video_stream < 0 ? videoStream() : wanted_video_stream;
    if (stream >= 0) {
        v_codec_context = format_context->streams[stream]->codec;
        video_stream = stream; //video_stream is the currently opened stream
    }
    stream = wanted_subtitle_stream < 0 ? subtitleStream() : wanted_subtitle_stream;
    if (stream >= 0) {
        s_codec_contex = format_context->streams[stream]->codec;;
        subtitle_stream = stream; //subtitle_stream is the currently opened stream
    }
    return true;
}

void AVDemuxer::setAutoResetStream(bool reset)
{
    auto_reset_stream = reset;
}

bool AVDemuxer::autoResetStream() const
{
    return auto_reset_stream;
}
//TODO: code like setStream, simplify
bool AVDemuxer::setStreamIndex(StreamType st, int index)
{
    QList<int> *streams = 0;
    int *wanted_stream = 0;
    if (st == AudioStream) {
        if (audio_stream == -2) {
            audioStream();
        }
        wanted_stream = &wanted_audio_stream;
        streams = &audio_streams;
    } else if (st == VideoStream) {
        if (video_stream == -2) {
            videoStream();
        }
        wanted_stream = &wanted_video_stream;
        streams = &video_streams;
    } else if (st == SubtitleStream) {
        if (subtitle_stream == -2) {
            subtitleStream();
        }
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
        *wanted_stream = -1;
        qWarning("invalid index %d (valid is 0~%d) for stream type %d.", index, streams->size(), st);
        return false;
    }
    return setStream(st, streams->at(index));
}

bool AVDemuxer::setStream(StreamType st, int stream)
{
    int *wanted_stream = 0;
    QList<int> *streams = 0;
    if (st == AudioStream) {
        wanted_stream = &wanted_audio_stream;
        streams = &audio_streams;
    } else if (st == VideoStream) {
        wanted_stream = &wanted_video_stream;
        streams = &video_streams;
    } else if (st == SubtitleStream) {
        wanted_stream = &wanted_subtitle_stream;
        streams = &subtitle_streams;
    }
    if (!wanted_stream || *wanted_stream == stream) {
        qWarning("stream type %d not found or stream %d not changed", st, stream);
        return false;
    }
    if (!streams->contains(stream)) {
        qWarning("%d is not a valid stream for stream type %d", stream, st);
        return false;
    }
    *wanted_stream = stream;
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
    if (!format_context)
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
    AVStream *stream = format_context->streams[videoStream()];
    return (qreal)stream->r_frame_rate.num / (qreal)stream->r_frame_rate.den;
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

bool AVDemuxer::isInput() const
{
    return _is_input;
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
    if (audio_stream != -2) //-2: not parsed, -1 not found.
        return audio_stream;
    audio_stream = -1;
    for (unsigned int i=0; i<format_context->nb_streams; ++i) {
        if(format_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_streams.push_back(i);
        }
    }
    if (!audio_streams.isEmpty()) {
        // ffplay use video stream as related_stream. find order: v-a-s
        // if ff has no av_find_best_stream, add it and return 0
        audio_stream = av_find_best_stream(format_context, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
        //audio_stream = audio_streams.first();
    }
    if (audio_stream < 0) {
        qDebug("audio stream not found: %s", av_err2str(audio_stream));
        audio_stream = -1;
    }
    return audio_stream;
}

QList<int> AVDemuxer::audioStreams() const
{
    if (audio_stream == -2) { //not parsed
        audioStream();
    }
    return audio_streams;
}

int AVDemuxer::videoStream() const
{
    if (video_stream != -2) //-2: not parsed, -1 not found.
        return video_stream;
    video_stream = -1;
    for (unsigned int i=0; i<format_context->nb_streams; ++i) {
        if(format_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_streams.push_back(i);
        }
    }
    if (!video_streams.isEmpty()) {
        // ffplay use video stream as related_stream. find order: v-a-s
        // if ff has no av_find_best_stream, add it and return 0
        video_stream = av_find_best_stream(format_context, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
        //audio_stream = audio_streams.first();
    }
    if (video_stream < 0) {
        qDebug("video stream not found: %s", av_err2str(video_stream));
        video_stream = -1;
    }
    return video_stream;
}

QList<int> AVDemuxer::videoStreams() const
{
    if (video_stream == -2) { //not parsed
        videoStream();
    }
    return video_streams;
}

int AVDemuxer::subtitleStream() const
{
    if (subtitle_stream != -2) //-2: not parsed, -1 not found.
        return subtitle_stream;
    subtitle_stream = -1;
    for (unsigned int i=0; i<format_context->nb_streams; ++i) {
        if(format_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_SUBTITLE) {
            subtitle_streams.push_back(i);
        }
    }
    if (!subtitle_streams.isEmpty()) {
        // ffplay use video stream as related_stream. find order: v-a-s
        // if ff has no av_find_best_stream, add it and return 0
        subtitle_stream = av_find_best_stream(format_context, AVMEDIA_TYPE_SUBTITLE, -1, -1, NULL, 0);
        //audio_stream = audio_streams.first();
    }
    if (subtitle_stream < 0) {
        qDebug("subtitle stream not found: %s", av_err2str(subtitle_stream));
        subtitle_stream = -1;
    }
    return subtitle_stream;
}

QList<int> AVDemuxer::subtitleStreams() const
{
    if (subtitle_stream == -2) { //not parsed
        subtitleStream();
    }
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

QSize AVDemuxer::frameSize() const
{
    return QSize(width(), height());
}

//codec
AVCodecContext* AVDemuxer::audioCodecContext(int stream) const
{
    if (stream < 0)
        stream = audioStream();
    if (stream < 0) {
        return 0;
    }
    if (stream > (int)format_context->nb_streams)
        return 0;
    return format_context->streams[stream]->codec;
}

AVCodecContext* AVDemuxer::videoCodecContext(int stream) const
{
    if (stream < 0)
        stream = videoStream();
    if (stream < 0) {
        return 0;
    }
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
    return avctx->codec->name;
    //AVCodecContext.codec_name is empty? codec_id is correct
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
    return avctx->codec->name;
    //AVCodecContext.codec_name is empty? codec_id is correct
}

QString AVDemuxer::videoCodecLongName(int stream) const
{
    AVCodecContext *avctx = videoCodecContext(stream);
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
            if (video_stream < 0) {
                video_stream = i;
            }
        } else if (type == AVMEDIA_TYPE_AUDIO) {
            audio_streams.push_back(i);
            if (audio_stream < 0) {
                audio_stream = i;
            }
        } else if (type == AVMEDIA_TYPE_SUBTITLE) {
            subtitle_streams.push_back(i);
            if (subtitle_stream < 0) {
                subtitle_stream = i;
            }
        }
    }
    return !audio_streams.isEmpty() || !video_streams.isEmpty() || !subtitle_streams.isEmpty();
}

QString AVDemuxer::formatName(AVFormatContext *ctx, bool longName) const
{
    if (isInput())
        return longName ? ctx->iformat->long_name : ctx->iformat->name;
    else
        return longName ? ctx->oformat->long_name : ctx->oformat->name;
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
int AVDemuxer::getInterruptStatus() const{
    return mpInterrup->getStatus();
}

/**
 * @brief setInterruptStatus set the interrupt status
 * @param interrupt
 * @return
 */
void AVDemuxer::setInterruptStatus(int interrupt){
    mpInterrup->setStatus(interrupt);
}

void AVDemuxer::setOptions(const QHash<QByteArray, QByteArray> &dict)
{
    mOptions = dict;
    if (mpDict) {
        av_dict_free(&mpDict);
        mpDict = 0; //aready 0 in av_free
    }
    if (dict.isEmpty())
        return;
    QHashIterator<QByteArray, QByteArray> i(dict);
    while (i.hasNext()) {
        i.next();
        av_dict_set(&mpDict, i.key().constData(), i.value().constData(), 0);
        qDebug("avformat option: %s=>%s", i.key().constData(), i.value().constData());
    }
}

QHash<QByteArray, QByteArray> AVDemuxer::options() const
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

} //namespace QtAV
