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

#ifndef QAV_DEMUXER_H
#define QAV_DEMUXER_H

#include <QtAV/QtAV_Global.h>
#include <QtCore/QObject>
#include <QtCore/QSize>
#include <QtCore/QMutex>

struct AVFormatContext;
struct AVCodecContext;
struct AVCodec;
struct AVFrame;
struct AVStream;

namespace QtAV {

class Packet;
class Q_EXPORT AVDemuxer : public QObject //QIODevice?
{
    Q_OBJECT
public:
    AVDemuxer(const QString& fileName = QString(), QObject *parent = 0);
    ~AVDemuxer();

    bool atEnd() const;
    bool close();
    bool loadFile(const QString& fileName);
    bool readFrame();
    Packet* packet() const; //current readed packet
    int stream() const; //current readed stream index

    void seek(qreal q); //q: [0,1]
    //seek default steps
    void seekForward();
    void seekBackward();

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
    int subtitleStream() const;

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

signals:
    void finished(); //end of file

private:
    bool eof;
    Packet *pkt;
    qint64 ipts;
    int stream_idx;
    mutable int audio_stream, video_stream, subtitle_stream;

    bool findAVCodec();
    QString formatName(AVFormatContext *ctx, bool longName = false) const;

    bool _is_input;
    AVFormatContext *format_context;
    AVCodecContext *a_codec_context, *v_codec_context;
    //copy the info, not parse the file when constructed, then need member vars
    QString _file_name;
    QMutex mutex; //for seek and readFrame
};

} //namespace QtAV
#endif // QAV_DEMUXER_H
