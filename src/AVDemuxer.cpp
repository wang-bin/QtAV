/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2013 Wang Bin <wbsecg1@gmail.com>

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

#include <QtAV/AVClock.h>
#include <QtAV/AVDemuxer.h>
#include <QtAV/Packet.h>
#include <QtAV/QtAV_Compat.h>
#include <QtCore/QThread>
#include <QtCore/QCoreApplication>

namespace QtAV {

const qint64 kSeekInterval = 168; //ms
AVDemuxer::AVDemuxer(const QString& fileName, QObject *parent)
    :QObject(parent)
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
    , _file_name(fileName)
    , master_clock(0)
    , mSeekUnit(SeekByTime)
    , mSeekTarget(SeekTarget_AnyFrame)
    ,__interrupt_status(0)
{
    if (!_file_name.isEmpty())
        loadFile(_file_name);

    //default network timeout
    __interrupt_timeout = 30000;
}

AVDemuxer::~AVDemuxer()
{
    close();
    if (pkt) {
        delete pkt;
        pkt = 0;
    }
}

/*
 * metodo per interruzione loop ffmpeg
 * @param void*obj: classe attuale
  * @return
 *  >0 Interruzione loop di ffmpeg!
*/
int AVDemuxer::__interrupt_cb(void *obj){
    int ret = 0;
    AVDemuxer* demuxer;
    //qApp->processEvents();
    if (!obj){
        qWarning("Passed Null object!");
        return(-1);
    }
    demuxer = (AVDemuxer*)obj;
    //qDebug("Timer:%lld, timeout:%lld",demuxer->__interrupt_timer.elapsed(), demuxer->__interrupt_timeout);

    //check manual interruption
    if (demuxer->__interrupt_status > 0) {
        qDebug("User Interrupt: -> quit!");
        ret = 1;//interrupt
    } else if((demuxer->__interrupt_timer.isValid()) && (demuxer->__interrupt_timer.hasExpired(demuxer->__interrupt_timeout)) ) {
        qDebug("Timeout expired: %lld/%lld -> quit!",demuxer->__interrupt_timer.elapsed(), demuxer->__interrupt_timeout);
        //TODO: emit a signal
        ret = 1;//interrupt
    }

    //qDebug(" END ret:%d\n",ret);
    return(ret);
}//AVDemuxer::__interrupt_cb()


bool AVDemuxer::readFrame()
{
    QMutexLocker lock(&mutex);
    Q_UNUSED(lock);
    AVPacket packet;
    //start timeout timer and timeout
    __interrupt_timer.start();

    int ret = av_read_frame(format_context, &packet); //0: ok, <0: error/end

    //invalidate the timer
    __interrupt_timer.invalidate();

    if (ret != 0) {
        //ffplay: AVERROR_EOF || url_eof() || avsq.empty()
        if (ret == AVERROR_EOF) { //end of file. FIXME: why no eof if replaying by seek(0)?
            if (!eof) {
                eof = true;
                started_ = false;
                pkt->data = QByteArray(); //flush
                pkt->markEnd();
                qDebug("End of file. %s %d", __FUNCTION__, __LINE__);
                emit finished();
                return true;
            }
            //pkt->data = QByteArray(); //flush
            //return true;
            return false; //frames after eof are eof frames
        } else if (ret == AVERROR_INVALIDDATA) {
            qWarning("AVERROR_INVALIDDATA");
        } else if (ret == AVERROR(EAGAIN)) {
            return true;
        }
        qWarning("[AVDemuxer] error: %s", av_err2str(ret));
        return false;
    }

    stream_idx = packet.stream_index; //TODO: check index
    //check whether the 1st frame is alreay got. emit only once
    if (!started_ && v_codec_context && v_codec_context->frame_number == 0) {
        started_ = true;
        emit started();
    } else if (!started_ && a_codec_context && a_codec_context->frame_number == 0) {
        started_ = true;
        emit started();
    }
    if (stream_idx != videoStream() && stream_idx != audioStream()) {
        //qWarning("[AVDemuxer] unknown stream index: %d", stream_idx);
        return false;
    }
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
    __interrupt_status = 0;
    //av_close_input_file(format_context); //deprecated
    if (format_context) {
        qDebug("closing format_context");
        avformat_close_input(&format_context); //libavf > 53.10.0
        format_context = 0;
    }
    return true;
}

void AVDemuxer::setClock(AVClock *c)
{
    master_clock = c;
}

AVClock* AVDemuxer::clock() const
{
    return master_clock;
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
void AVDemuxer::seek(qreal q)
{
    if ((!a_codec_context && !v_codec_context) || !format_context) {
        qWarning("can not seek. context not ready: %p %p %p", a_codec_context, v_codec_context, format_context);
        return;
    }
    if (seek_timer.isValid()) {
        //why sometimes seek_timer.elapsed() < 0
        if (!seek_timer.hasExpired(kSeekInterval)) {
            qDebug("seek too frequent. ignore");
            return;
        }
        seek_timer.restart();
    } else {
        seek_timer.start();
    }
    QMutexLocker lock(&mutex);
    Q_UNUSED(lock);
    q = qMax<qreal>(0.0, q);
    if (q >= 1.0) {
        qWarning("Invalid seek position %f/1.0", q);
        return;
    }
#if 0
    //t: unit is s
    qreal t = q;// * (double)format_context->duration; //
    int ret = av_seek_frame(format_context, -1, (int64_t)(t*AV_TIME_BASE), t > pkt->pts ? 0 : AVSEEK_FLAG_BACKWARD);
    qDebug("[AVDemuxer] seek to %f %f %lld / %lld", q, pkt->pts, (int64_t)(t*AV_TIME_BASE), duration());
#else
    //t: unit is us (10^-6 s, AV_TIME_BASE)
    int64_t t = int64_t(q*duration());///AV_TIME_BASE;
    //TODO: pkt->pts may be 0, compute manually. Check wether exceed the length
    if (t >= duration()) {
        qWarning("Invailid seek position: %lld/%lld", t, duration());
        return;
    }
    bool backward = t <= (int64_t)(pkt->pts*AV_TIME_BASE);
    qDebug("[AVDemuxer] seek to %f %f %lld / %lld backward=%lld", q, pkt->pts, t, duration(), backward);
    //AVSEEK_FLAG_BACKWARD has no effect? because we know the timestamp
    // FIXME: back flag is opposite? otherwise seek is bad and may crash?
    int seek_flag = (backward ? 0 : AVSEEK_FLAG_BACKWARD); //AVSEEK_FLAG_ANY
    //bool seek_bytes = !!(format_context->iformat->flags & AVFMT_TS_DISCONT) && strcmp("ogg", format_context->iformat->name);
    int ret = av_seek_frame(format_context, -1, t, seek_flag);
    //avformat_seek_file()
#endif
    if (ret < 0) {
        qWarning("[AVDemuxer] seek error: %s", av_err2str(ret));
        return;
    }
    //replay
    if (q == 0) {
        qDebug("************seek to 0. started = false");
        started_ = false;
        if (a_codec_context)
            a_codec_context->frame_number = 0;
        if (v_codec_context)
            v_codec_context->frame_number = 0; //TODO: why frame_number not changed after seek?
        if (s_codec_contex)
            s_codec_contex->frame_number = 0;
    }
    if (master_clock) {
        master_clock->updateValue(qreal(t)/qreal(AV_TIME_BASE));
        master_clock->updateExternalClock(t/1000LL); //in msec. ignore usec part using t/1000
    }
}

/*
 TODO: seek by byte/frame
  We need to know current playing packet but not current demuxed packet which
  may blocked for a while
*/
void AVDemuxer::seekForward()
{
    if ((!a_codec_context && !v_codec_context) || !format_context) {
        qWarning("can not seek. context not ready: %p %p %p", a_codec_context, v_codec_context, format_context);
        return;
    }
    double pts = pkt->pts;
    if (master_clock) {
        pts = master_clock->value();
    } else {
        qWarning("[AVDemux] No master clock!");
    }
    double q = (double)((pts + 16)*AV_TIME_BASE)/(double)duration();
    seek(q);
}

void AVDemuxer::seekBackward()
{
    if ((!a_codec_context && !v_codec_context) || !format_context) {
        qWarning("can not seek. context not ready: %p %p %p", a_codec_context, v_codec_context, format_context);
        return;
    }
    double pts = pkt->pts;
    if (master_clock) {
        pts = master_clock->value();
    } else {
        qWarning("[AVDemux] No master clock!");
    }
    double q = (double)((pts - 16)*AV_TIME_BASE)/(double)duration();
    seek(q);
}

bool AVDemuxer::isLoaded(const QString &fileName) const
{
    return fileName == _file_name && (a_codec_context || v_codec_context || s_codec_contex);
}

bool AVDemuxer::loadFile(const QString &fileName)
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
    _file_name = fileName.trimmed();
    if (_file_name.startsWith("mms:"))
        _file_name.insert(3, 'h');
    else if (_file_name.startsWith("file://"))
        _file_name.remove("file://");
    //deprecated
    // Open an input stream and read the header. The codecs are not opened.
    //if(av_open_input_file(&format_context, _file_name.toLocal8Bit().constData(), NULL, 0, NULL)) {

    //alloc av format context
    if(!format_context)
        format_context = avformat_alloc_context();

    //install interrupt callback
    format_context->interrupt_callback.callback = __interrupt_cb;
    format_context->interrupt_callback.opaque = this;

    qDebug("avformat_open_input: format_context:'%p', url:'%s'...",format_context, qPrintable(_file_name));

    //start timeout timer and timeout
    __interrupt_timer.start();

    int ret = avformat_open_input(&format_context, qPrintable(_file_name), NULL, NULL);

    //invalidate the timer
    __interrupt_timer.invalidate();

    qDebug("avformat_open_input: url:'%s' ret:%d",qPrintable(_file_name), ret);

    if (ret < 0) {
    //if (avformat_open_input(&format_context, qPrintable(filename), NULL, NULL)) {
        qWarning("Can't open media: %s", av_err2str(ret));
        return false;
    }
    format_context->flags |= AVFMT_FLAG_GENPTS;
    //deprecated
    //if(av_find_stream_info(format_context)<0) {
    //TODO: avformat_find_stream_info is too slow, only useful for some video format
    __interrupt_timer.start();
    ret = avformat_find_stream_info(format_context, NULL);
    __interrupt_timer.invalidate();
    if (ret < 0) {
        qWarning("Can't find stream info: %s", av_err2str(ret));
        return false;
    }

    if (!prepareStreams()) {
        return false;
    }

    started_ = false;
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

qint64 AVDemuxer::startTime() const
{
    return format_context->start_time;
}

//AVFrameContext use AV_TIME_BASE as time base.
qint64 AVDemuxer::duration() const
{
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
    return __interrupt_timeout;
}

/**
 * @brief setInterruptTimeout set the interrupt timeout
 * @param timeout
 * @return
 */
void AVDemuxer::setInterruptTimeout(qint64 timeout)
{
    __interrupt_timeout = timeout;
}

/**
 * @brief getInterruptStatus return the interrupt status
 * @return
 */
int AVDemuxer::getInterruptStatus() const{
    return(__interrupt_status);
}

/**
 * @brief setInterruptStatus set the interrupt status
 * @param interrupt
 * @return
 */
int AVDemuxer::setInterruptStatus(int interrupt){
    __interrupt_status = (interrupt>0) ? 1 : 0;
    return(__interrupt_status);
}

} //namespace QtAV
