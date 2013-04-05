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

#ifndef QAV_DEMUXER_H
#define QAV_DEMUXER_H

#include <QtAV/QtAV_Global.h>
#include <QtCore/QObject>
#include <QtCore/QSize>
#include <QtCore/QMutex>

#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
#include <QtCore/QElapsedTimer>
#else
#include <QtCore/QTimer>
typedef QTimer QElapsedTimer
#endif

struct AVFormatContext;
struct AVCodecContext;
struct AVCodec;
struct AVFrame;
struct AVStream;

namespace QtAV {

class AVClock;
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

	void setClock(AVClock *c);
	AVClock *clock() const;
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

    /**
     * @brief getInterruptTimeout return the interrupt timeout
     * @return
     */
    qint64 getInterruptTimeout() const;

    /**
     * @brief setInterruptTimeout set the interrupt timeout
     * @param timeout
     * @return
     */
    void setInterruptTimeout(qint64 timeout);

    /**
     * @brief getInterruptStatus return the interrupt status
     * @return
     */
    int getInterruptStatus() const;

    /**
     * @brief setInterruptStatus set the interrupt status
     * @param interrupt
     * @return
     */
    int setInterruptStatus(int interrupt);

signals:
    /*emit when the first frame is read*/
    void started();
    void finished(); //end of file

private:
    bool started_;
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
	AVClock *master_clock;
    QElapsedTimer seek_timer;

    /**
     * interrupt callback for ffmpeg
     * @param void*obj: actual object
     * @return
     *  >0 interrupt ffmpeg loop!
     */
    static int __interrupt_cb(void *obj);
    // timer for interrupt timeout
    QElapsedTimer __interrupt_timer;
    //interrupt timeout
    qint64 __interrupt_timeout;

    //interrupt status
    int __interrupt_status;

};

} //namespace QtAV
#endif // QAV_DEMUXER_H
