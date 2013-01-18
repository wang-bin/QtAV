/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012 Wang Bin <wbsecg1@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include <QtAV/AVClock.h>
#include <QtAV/AVDemuxer.h>
#include <QtAV/Packet.h>
#include <QtAV/QtAV_Compat.h>
#include <QtCore/QThread>

namespace QtAV {
AVDemuxer::AVDemuxer(const QString& fileName, QObject *parent)
    :QObject(parent),started_(false),eof(false),pkt(new Packet())
    ,ipts(0),stream_idx(-1),audio_stream(-2),video_stream(-2)
    ,subtitle_stream(-2),_is_input(true),format_context(0)
	,a_codec_context(0),v_codec_context(0),_file_name(fileName),master_clock(0)
{
    av_register_all();
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
}


bool AVDemuxer::readFrame()
{
    QMutexLocker lock(&mutex);
    Q_UNUSED(lock);
    AVPacket packet;
    int ret = av_read_frame(format_context, &packet); //0: ok, <0: error/end

    if (ret != 0) {
        if (ret == AVERROR_EOF) { //end of file
            if (!eof) {
                eof = true;
                started_ = false;
                qDebug("End of file. %s %d", __FUNCTION__, __LINE__);
                emit finished();
            }
            return false; //frames after eof are eof frames
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
        qWarning("[AVDemuxer] unknown stream index: %d", stream_idx);
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
    audio_stream = video_stream = subtitle_stream = -2;
    if (a_codec_context) {
        qDebug("closing a_codec_context");
        avcodec_close(a_codec_context);
        a_codec_context = 0;
    }
    if (v_codec_context) {
        qDebug("closing v_codec_context");
        avcodec_close(v_codec_context);
        v_codec_context = 0;
    }
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

//TODO: seek by byte
void AVDemuxer::seek(qreal q)
{
    if ((!a_codec_context && !v_codec_context) || !format_context) {
        qWarning("can not seek. context not ready: %p %p %p", a_codec_context, v_codec_context, format_context);
        return;
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
    qDebug("[AVDemuxer] seek to %f %f %lld / %lld backward=%d", q, pkt->pts, t, duration(), backward);
	//AVSEEK_FLAG_BACKWARD has no effect? because we know the timestamp
	int seek_flag =  (backward ? 0 : AVSEEK_FLAG_BACKWARD); //AVSEEK_FLAG_ANY
	int ret = av_seek_frame(format_context, -1, t, seek_flag);
#endif
    if (ret < 0) {
        qWarning("[AVDemuxer] seek error: %s", av_err2str(ret));
        return;
    }
    if (master_clock) {
        master_clock->updateValue(qreal(t)/qreal(AV_TIME_BASE));
        master_clock->updateExternalClock(t/1000LL); //in msec. ignore usec part using t/1000
    }
    //calc pts
    if (videoCodecContext())
        avcodec_flush_buffers(videoCodecContext());
    if (audioCodecContext())
        avcodec_flush_buffers(audioCodecContext());
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

bool AVDemuxer::loadFile(const QString &fileName)
{
    close();
    qDebug("all closed and reseted");
    _file_name = fileName;
    //deprecated
    // Open an input stream and read the header. The codecs are not opened.
    //if(av_open_input_file(&format_context, _file_name.toLocal8Bit().constData(), NULL, 0, NULL)) {
    int ret = avformat_open_input(&format_context, qPrintable(_file_name), NULL, NULL);
    if (ret < 0) {
    //if (avformat_open_input(&format_context, qPrintable(filename), NULL, NULL)) {
        qWarning("Can't open video: %s", av_err2str(ret));
        return false;
    }
    format_context->flags |= AVFMT_FLAG_GENPTS;
    //deprecated
    //if(av_find_stream_info(format_context)<0) {
    //TODO: avformat_find_stream_info is too slow, only useful for some video format
    ret = avformat_find_stream_info(format_context, NULL);
    if (ret < 0) {
        qWarning("Can't find stream info: %s", av_err2str(ret));
        return false;
    }

    //a_codec_context = format_context->streams[audioStream()]->codec;
    //v_codec_context = format_context->streams[videoStream()]->codec;
    findAVCodec();

    bool _has_audio = a_codec_context != 0;
    if (a_codec_context) {
        AVCodec *aCodec = avcodec_find_decoder(a_codec_context->codec_id);
        if (aCodec) {
            ret = avcodec_open2(a_codec_context, aCodec, NULL);
            if (ret < 0) {
                _has_audio = false;
                qWarning("open audio codec failed: %s", av_err2str(ret));
            }
        } else {
            _has_audio = false;
            qDebug("Unsupported audio codec. id=%d.", a_codec_context->codec_id);
        }
    }
    //FIXME: it will keep the ExternalClock type even if the next media stream has audio. Use global settings?
    if (!_has_audio) {
        qWarning("No audio found or audio not supported. Using ExternalClock");
        master_clock->setClockType(AVClock::ExternalClock);
    }

    bool _has_vedio = v_codec_context != 0;
    if (v_codec_context) {
        AVCodec *vCodec = avcodec_find_decoder(v_codec_context->codec_id);
        if(!vCodec) {
            qWarning("Unsupported video codec. id=%d.", v_codec_context->codec_id);
            _has_vedio = false;
        }
        ////v_codec_context->time_base = (AVRational){1,30};
        //avcodec_open(v_codec_context, vCodec) //deprecated
        ret = avcodec_open2(v_codec_context, vCodec, NULL);
        if (ret < 0) {
            qWarning("open video codec failed: %s", av_err2str(ret));
            _has_vedio = false;
        }
    }
    started_ = false;
    return _has_audio || _has_vedio;
}

AVFormatContext* AVDemuxer::formatContext()
{
    return format_context;
}
/*
static void dump_stream_format(AVFormatContext *ic, int i, int index, int is_output)
{
    char buf[256];
    int flags = (is_output ? ic->oformat->flags : ic->iformat->flags);
    AVStream *st = ic->streams[i];
    int g = av_gcd(st->time_base.num, st->time_base.den);
    AVDictionaryEntry *lang = av_dict_get(st->metadata, "language", NULL, 0);
    avcodec_string(buf, sizeof(buf), st->codec, is_output);
    av_log(NULL, AV_LOG_INFO, "    Stream #%d.%d", index, i);
    // the pid is an important information, so we display it
    // XXX: add a generic system
    if (flags & AVFMT_SHOW_IDS)
        av_log(NULL, AV_LOG_INFO, "[0x%x]", st->id);
    if (lang)
        av_log(NULL, AV_LOG_INFO, "(%s)", lang->value);
    av_log(NULL, AV_LOG_DEBUG, ", %d, %d/%d", st->codec_info_nb_frames, st->time_base.num/g, st->time_base.den/g);
    av_log(NULL, AV_LOG_INFO, ": %s", buf);
    if (st->sample_aspect_ratio.num && // default
        av_cmp_q(st->sample_aspect_ratio, st->codec->sample_aspect_ratio)) {
        AVRational display_aspect_ratio;
        av_reduce(&display_aspect_ratio.num, &display_aspect_ratio.den,
                  st->codec->width*st->sample_aspect_ratio.num,
                  st->codec->height*st->sample_aspect_ratio.den,
                  1024*1024);
        av_log(NULL, AV_LOG_INFO, ", PAR %d:%d DAR %d:%d",
                 st->sample_aspect_ratio.num, st->sample_aspect_ratio.den,
                 display_aspect_ratio.num, display_aspect_ratio.den);
    }
    if(st->codec->codec_type == AVMEDIA_TYPE_VIDEO){
        if(st->avg_frame_rate.den && st->avg_frame_rate.num)
            print_fps(av_q2d(st->avg_frame_rate), "fps");
        if(st->r_frame_rate.den && st->r_frame_rate.num)
            print_fps(av_q2d(st->r_frame_rate), "tbr");
        if(st->time_base.den && st->time_base.num)
            print_fps(1/av_q2d(st->time_base), "tbn");
        if(st->codec->time_base.den && st->codec->time_base.num)
            print_fps(1/av_q2d(st->codec->time_base), "tbc");
    }
    if (st->disposition & AV_DISPOSITION_DEFAULT)
        av_log(NULL, AV_LOG_INFO, " (default)");
    if (st->disposition & AV_DISPOSITION_DUB)
        av_log(NULL, AV_LOG_INFO, " (dub)");
    if (st->disposition & AV_DISPOSITION_ORIGINAL)
        av_log(NULL, AV_LOG_INFO, " (original)");
    if (st->disposition & AV_DISPOSITION_COMMENT)
        av_log(NULL, AV_LOG_INFO, " (comment)");
    if (st->disposition & AV_DISPOSITION_LYRICS)
        av_log(NULL, AV_LOG_INFO, " (lyrics)");
    if (st->disposition & AV_DISPOSITION_KARAOKE)
        av_log(NULL, AV_LOG_INFO, " (karaoke)");
    if (st->disposition & AV_DISPOSITION_FORCED)
        av_log(NULL, AV_LOG_INFO, " (forced)");
    if (st->disposition & AV_DISPOSITION_HEARING_IMPAIRED)
        av_log(NULL, AV_LOG_INFO, " (hearing impaired)");
    if (st->disposition & AV_DISPOSITION_VISUAL_IMPAIRED)
        av_log(NULL, AV_LOG_INFO, " (visual impaired)");
    if (st->disposition & AV_DISPOSITION_CLEAN_EFFECTS)
        av_log(NULL, AV_LOG_INFO, " (clean effects)");
    av_log(NULL, AV_LOG_INFO, "\n");
    dump_metadata(NULL, st->metadata, "    ");
}*/
void AVDemuxer::dump()
{
    av_dump_format(format_context, 0, qPrintable(_file_name), false);
    qDebug("video format: %s [%s]", qPrintable(videoFormatName()), qPrintable(videoFormatLongName()));
    qDebug("Audio: %s [%s]", qPrintable(audioCodecName()), qPrintable(audioCodecLongName()));
    if (a_codec_context)
        qDebug("sample rate: %d, channels: %d", a_codec_context->sample_rate, a_codec_context->channels);
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

qint64 AVDemuxer::duration() const
{
    return format_context->duration;
}

int AVDemuxer::bitRate() const
{
    return format_context->bit_rate;
}

qreal AVDemuxer::frameRate() const
{
    AVStream *stream = format_context->streams[videoStream()];
    return (qreal)stream->r_frame_rate.num / (qreal)stream->r_frame_rate.den;
    //codecCtx->time_base.den / codecCtx->time_base.num
}

qint64 AVDemuxer::frames() const
{
    return format_context->streams[videoStream()]->nb_frames; //0?
}

bool AVDemuxer::isInput() const
{
    return _is_input;
}

int AVDemuxer::audioStream() const
{
    if (audio_stream != -2) //-2: not parsed, -1 not found.
        return audio_stream;
    audio_stream = -1;
    for(unsigned int i=0; i<format_context->nb_streams; ++i) {
        if(format_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream = i;
            break;
        }
    }
    return audio_stream;
}

int AVDemuxer::videoStream() const
{
    if (video_stream != -2) //-2: not parsed, -1 not found.
        return video_stream;
    video_stream = -1;
    for(unsigned int i=0; i<format_context->nb_streams; ++i) {
        if(format_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream = i;
            break;
        }
    }
    return video_stream;
}

int AVDemuxer::subtitleStream() const
{
    if (subtitle_stream != -2) //-2: not parsed, -1 not found.
        return subtitle_stream;
    subtitle_stream = -1;
    for(unsigned int i=0; i<format_context->nb_streams; ++i) {
        if(format_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_SUBTITLE) {
            subtitle_stream = i;
            break;
        }
    }
    return subtitle_stream;
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
AVCodecContext* AVDemuxer::audioCodecContext() const
{
    return a_codec_context;
}

AVCodecContext* AVDemuxer::videoCodecContext() const
{
    return v_codec_context;
}

/*!
    call avcodec_open2() first!
    check null ptr?
*/
QString AVDemuxer::audioCodecName() const
{
    if (a_codec_context)
        return a_codec_context->codec->name;
    //return v_codec_context->codec_name; //codec_name is empty? codec_id is correct
    return "";
}

QString AVDemuxer::audioCodecLongName() const
{
    if (a_codec_context)
        return a_codec_context->codec->long_name;
    return "";
}

QString AVDemuxer::videoCodecName() const
{
    if (v_codec_context)
        return v_codec_context->codec->name;
    //return v_codec_context->codec_name; //codec_name is empty? codec_id is correct
    return "";
}

QString AVDemuxer::videoCodecLongName() const
{
    if (v_codec_context)
        return v_codec_context->codec->long_name;
    return "";
}


bool AVDemuxer::findAVCodec()
{
    if (video_stream != -2 && audio_stream != -2 && subtitle_stream != -2)
        return (video_stream != -1) && (audio_stream != -1) && (subtitle_stream != -1);
    video_stream = audio_stream = subtitle_stream = -1;
    AVMediaType type = AVMEDIA_TYPE_UNKNOWN;
    for (unsigned int i=0; i<format_context->nb_streams; ++i) {
        type = format_context->streams[i]->codec->codec_type;
        if (type == AVMEDIA_TYPE_VIDEO && video_stream < 0) {
            video_stream = i;
            v_codec_context = format_context->streams[video_stream]->codec;
            //if !vaapi
            if (v_codec_context->codec_id == CODEC_ID_H264) {
                v_codec_context->thread_type = FF_THREAD_FRAME; //FF_THREAD_SLICE;
                v_codec_context->thread_count = QThread::idealThreadCount();
            } else {
                //v_codec_context->lowres = lowres;
            }
        } else if (type == AVMEDIA_TYPE_AUDIO && audio_stream < 0) {
            audio_stream = i;
            a_codec_context = format_context->streams[audio_stream]->codec;
        } else if (type == AVMEDIA_TYPE_SUBTITLE && subtitle_stream < 0) {
            subtitle_stream = i;
        }
        if (audio_stream >=0 && video_stream >= 0 && subtitle_stream >= 0)
            return true;
    }
    return false;
}

QString AVDemuxer::formatName(AVFormatContext *ctx, bool longName) const
{
    if (isInput())
        return longName ? ctx->iformat->long_name : ctx->iformat->name;
    else
        return longName ? ctx->oformat->long_name : ctx->oformat->name;
}

} //namespace QtAV
