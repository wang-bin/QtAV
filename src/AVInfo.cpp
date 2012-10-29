/******************************************************************************
	qavinfo.cpp: description
	Copyright (C) 2012 Wang Bin <wbsecg1@gmail.com>
	
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License along
	with this program; if not, write to the Free Software Foundation, Inc.,
	51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
******************************************************************************/

#include <QtAV/AVInfo.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus
#include <libavformat/avformat.h>
#ifdef __cplusplus
}
#endif //__cplusplus
#include <qthread.h>

namespace QtAV {
AVInfo::AVInfo(const QString& fileName)
	:_is_input(true),_format_context(0),a_codec_context(0),v_codec_context(0)
	,_file_name(fileName)
{
	av_register_all();
	if (!_file_name.isEmpty())
		loadFile(_file_name);
}

AVInfo::~AVInfo()
{
	avcodec_close(a_codec_context);
	avcodec_close(v_codec_context);
	//av_close_input_file(_format_context); //deprecated
	avformat_close_input(&_format_context); //libavf > 53.10.0
}

bool AVInfo::loadFile(const QString &fileName)
{
	_file_name = fileName;
	//deprecated
	// Open an input stream and read the header. The codecs are not opened.
	//if(av_open_input_file(&_format_context, _file_name.toLocal8Bit().constData(), NULL, 0, NULL)) {
	if (avformat_open_input(&_format_context, qPrintable(_file_name), NULL, NULL)) {
	//if (avformat_open_input(&formatCtx, qPrintable(filename), NULL, NULL)) {
		qDebug("Can't open video");
		return false;
	}
    _format_context->flags |= AVFMT_FLAG_GENPTS;
	//deprecated
	//if(av_find_stream_info(_format_context)<0) {
	if (avformat_find_stream_info(_format_context, NULL)<0) {
		qDebug("Can't find stream info");
		return false;
	}

	//a_codec_context = _format_context->streams[audioStream()]->codec;
	//v_codec_context = _format_context->streams[videoStream()]->codec;
	findAVCodec();

	AVCodec *aCodec = avcodec_find_decoder(a_codec_context->codec_id);
	if(aCodec) {
		if(avcodec_open2(a_codec_context, aCodec, NULL)<0) {
			qDebug("open audio codec failed");
		}
	} else {
		qDebug("Unsupported audio codec. id=%d.", v_codec_context->codec_id);
	}
	AVCodec *vCodec = avcodec_find_decoder(v_codec_context->codec_id);
	if(!vCodec) {
		qDebug("Unsupported video codec. id=%d.", v_codec_context->codec_id);
		return false;
	}
	////v_codec_context->time_base = (AVRational){1,30};
	//avcodec_open(v_codec_context, vCodec) //deprecated
	if(avcodec_open2(v_codec_context, vCodec, NULL)<0) {
		qDebug("open video codec failed");
		return false;
	}
	return true;
}

AVFormatContext* AVInfo::formatContext()
{
	return _format_context;
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
void AVInfo::dump()
{
	qDebug("Version\n"
		   "libavcodec: %#x\n"
		   "libavformat: %#x\n"
//	       "libavdevice: %#x\n"
		   "libavutil: %#x"
		   ,avcodec_version()
		   ,avformat_version()
//	       ,avdevice_version()
		   ,avutil_version());
	av_dump_format(_format_context, 0, qPrintable(_file_name), false);
	qDebug("video format: %s [%s]", qPrintable(videoFormatName()), qPrintable(videoFormatLongName()));
	qDebug("Audio: %s [%s]", qPrintable(audioCodecName()), qPrintable(audioCodecLongName()));
	qDebug("sample rate: %d, channels: %d", a_codec_context->sample_rate, a_codec_context->channels);
}

QString AVInfo::fileName() const
{
	return _format_context->filename;
}

QString AVInfo::videoFormatName() const
{
	return formatName(_format_context, false);
}

QString AVInfo::videoFormatLongName() const
{
	return formatName(_format_context, true);
}

qint64 AVInfo::startTime() const
{
	return _format_context->start_time;
}

qint64 AVInfo::duration() const
{
	return _format_context->duration;
}

int AVInfo::bitRate() const
{
	return _format_context->bit_rate;
}

qreal AVInfo::frameRate() const
{
	AVStream *stream = _format_context->streams[videoStream()];
	return (qreal)stream->r_frame_rate.num / (qreal)stream->r_frame_rate.den;
	//codecCtx->time_base.den / codecCtx->time_base.num
}

qint64 AVInfo::frames() const
{
	return _format_context->streams[videoStream()]->nb_frames; //0?
}

bool AVInfo::isInput() const
{
	return _is_input;
}

int AVInfo::audioStream() const
{
	static int audio_stream = -2; //-2: not parsed, -1 not found.
	if (audio_stream != -2)
		return audio_stream;
	audio_stream = -1;
	for(unsigned int i=0; i<_format_context->nb_streams; ++i) {
		if(_format_context->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO) {
			audio_stream = i;
			break;
		}
	}
	return audio_stream;
}

int AVInfo::videoStream() const
{
	static int video_stream = -2; //-2: not parsed, -1 not found.
	if (video_stream != -2)
		return video_stream;
	video_stream = -1;
	for(unsigned int i=0; i<_format_context->nb_streams; ++i) {
		if(_format_context->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
			video_stream = i;
			break;
		}
	}
	return video_stream;
}

int AVInfo::width() const
{
	return videoCodecContext()->width;
}

int AVInfo::height() const
{
	return videoCodecContext()->height;
}

QSize AVInfo::frameSize() const
{
	return QSize(width(), height());
}

//codec
AVCodecContext* AVInfo::audioCodecContext() const
{
    return a_codec_context;
}

AVCodecContext* AVInfo::videoCodecContext() const
{
	return v_codec_context;
}

/*!
	call avcodec_open2() first!
	check null ptr?
*/
QString AVInfo::audioCodecName() const
{
	return a_codec_context->codec->name;
	//return v_codec_context->codec_name; //codec_name is empty? codec_id is correct
}

QString AVInfo::audioCodecLongName() const
{
	return a_codec_context->codec->long_name;
}

QString AVInfo::videoCodecName() const
{
	return v_codec_context->codec->name;
	//return v_codec_context->codec_name; //codec_name is empty? codec_id is correct
}

QString AVInfo::videoCodecLongName() const
{
	return v_codec_context->codec->long_name;
}


bool AVInfo::findAVCodec()
{
	static int video_stream = -2; //-2: not parsed, -1 not found.
	static int audio_stream = -2; //-2: not parsed, -1 not found.
	if (video_stream != -2 && audio_stream != -2)
		return (video_stream != -1) && (audio_stream != -1);
	video_stream = -1;
	audio_stream = -1;
	for(unsigned int i=0; i<_format_context->nb_streams; ++i) {
        if(_format_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO
                && video_stream < 0) {
			video_stream = i;
			v_codec_context = _format_context->streams[video_stream]->codec;
            //if !vaapi
            if (v_codec_context->codec_id == CODEC_ID_H264) {
                v_codec_context->thread_type = FF_THREAD_FRAME; //FF_THREAD_SLICE;
                v_codec_context->thread_count = QThread::idealThreadCount();
            } else {
                //v_codec_context->lowres = lowres;
            }
		}
        if(_format_context->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO
                && audio_stream < 0) {
			audio_stream = i;
			a_codec_context = _format_context->streams[audio_stream]->codec;
		}
		if (audio_stream >=0 && video_stream >= 0)
			return true;
	}
	return false;
}

QString AVInfo::formatName(AVFormatContext *ctx, bool longName) const
{
	if (isInput())
		return longName ? ctx->iformat->long_name : ctx->iformat->name;
	else
		return longName ? ctx->oformat->long_name : ctx->oformat->name;
}

}
