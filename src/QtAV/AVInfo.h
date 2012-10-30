/******************************************************************************
	qavinfo.h: description
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


#ifndef QAVINFO_H
#define QAVINFO_H

#include <QtAV/QtAV_Global.h>
#include <qstring.h>
#include <qsize.h>

struct AVFormatContext;
struct AVCodecContext;
struct AVCodec;
struct AVFrame;
struct AVStream;
//TODO: use AVDemuxer

namespace QtAV {
class Q_EXPORT AVInfo
{
public:
    AVInfo(const QString& fileName = QString()); //parse the file
    ~AVInfo();
	bool loadFile(const QString& fileName);

	//format
	AVFormatContext* formatContext();
	void dump();
	QString fileName() const; //AVFormatContext::filename
	QString audioFormatName() const;
	QString audioFormatLongName() const;
	QString videoFormatName() const; //AVFormatContext::iformat->name
	QString videoFormatLongName() const; //AVFormatContext::iformat->long_name
	qint64 startTime() const; //AVFormatContext::start_time
	qint64 duration() const; //AVFormatContext::duration
	int bitRate() const; //AVFormatContext::bit_rate
	qreal frameRate() const; //AVFormatContext::r_frame_rate
	qint64 frames() const; //AVFormatContext::nb_frames
	bool isInput() const;
	int audioStream() const;
	int videoStream() const;

	int width() const; //AVCodecContext::width;
	int height() const; //AVCodecContext::height
	QSize frameSize() const;

	//codec
	AVCodecContext* audioCodecContext() const;
	AVCodecContext* videoCodecContext() const;
	QString audioCodecName() const;
	QString audioCodecLongName() const;
	QString videoCodecName() const;
	QString videoCodecLongName() const;

private:
	bool findAVCodec();
	QString formatName(AVFormatContext *ctx, bool longName = false) const;

	bool _is_input;
	AVFormatContext *_format_context;
	AVCodecContext *a_codec_context, *v_codec_context;
	//copy the info, not parse the file when constructed, then need member vars
	QString _file_name;
};
}

#endif // QAVINFO_H
