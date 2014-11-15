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
#include <QtAV/CommonTypes.h>
#include <QtAV/AVError.h>
#include <QtCore/QVariant>
#include <QtCore/QObject>
#include <QtCore/QSize>
#include <QtCore/QMutex>

#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
#include <QtCore/QElapsedTimer>
#else
#include <QtCore/QTime>
typedef QTime QElapsedTimer;
#endif

struct AVFormatContext;
struct AVInputFormat;
struct AVCodecContext;
struct AVCodec;
struct AVFrame;
struct AVStream;
struct AVDictionary;

class QIODevice;
// TODO: force codec name. clean code
namespace QtAV {

class AVError;
class Packet;
class QAVIOContext;

class Q_AV_EXPORT AVDemuxer : public QObject //QIODevice?
{
    Q_OBJECT
public:
    enum StreamType {
        AudioStream,
        VideoStream,
        SubtitleStream,
    };

    enum SeekUnit {
        SeekByTime, // only this is supported now
        SeekByByte,
        SeekByFrame
    };
    enum SeekTarget {
        SeekTarget_KeyFrame,
        SeekTarget_AnyFrame,
        SeekTarget_AccurateFrame
    };

    AVDemuxer(const QString& fileName = QString(), QObject *parent = 0);
    ~AVDemuxer();

    MediaStatus mediaStatus() const;
    bool atEnd() const;
    bool close(); //TODO: rename unload()
    bool loadFile(const QString& fileName);
    bool isLoaded(const QString& fileName) const;
    bool load(QIODevice* iocontext);
    bool prepareStreams(); //called by loadFile(). if change to a new stream, call it(e.g. in AVPlayer)

    void putFlushPacket();
    bool readFrame();
    Packet* packet() const; //current readed packet
    int stream() const; //current readed stream index

    bool isSeekable() const;
    void setSeekUnit(SeekUnit unit);
    SeekUnit seekUnit() const;
    void setSeekTarget(SeekTarget target);
    SeekTarget seekTarget() const;
    bool seek(qint64 pos); //pos: ms
    void seek(qreal q); //q: [0,1]. TODO: what if duration() is not valid?

    //format
    AVFormatContext* formatContext();
    QString fileName() const; //AVFormatContext::filename
    QString audioFormatName() const;
    QString audioFormatLongName() const;
    QString videoFormatName() const; //AVFormatContext::iformat->name
    QString videoFormatLongName() const; //AVFormatContext::iformat->long_name
    // TODO: rename startPosition()
    qint64 startTime() const; //ms, AVFormatContext::start_time/1000
    qint64 duration() const; //ms, AVFormatContext::duration/1000
    qint64 startTimeUs() const; //us, AVFormatContext::start_time
    qint64 durationUs() const; //us, AVFormatContext::duration

    //total bit rate
    int bitRate() const; //AVFormatContext::bit_rate
    int audioBitRate(int stream = -1) const;
    int videoBitRate(int stream = -1) const;

    qreal frameRate() const; //AVStream::avg_frame_rate
    // if stream is -1, return the current video(or audio if no video) stream.
    // TODO: audio/videoFrames?
    qint64 frames(int stream = -1) const; //AVFormatContext::nb_frames
    bool isInput() const;

    bool hasAttacedPicture() const;
    // true: next load with use the best stream instead of specified stream
    void setAutoResetStream(bool reset);
    bool autoResetStream() const;
    //set stream by index in stream list
    bool setStreamIndex(StreamType st, int index);
    // current open stream
    int currentStream(StreamType st) const;
    QList<int> streams(StreamType st) const;
    // current open stream
    int audioStream() const;
    QList<int> audioStreams() const;
    // current open stream
    int videoStream() const;
    QList<int> videoStreams() const;
    // current open stream
    int subtitleStream() const;
    QList<int> subtitleStreams() const;

    int width() const; //AVCodecContext::width;
    int height() const; //AVCodecContext::height
    QSize frameSize() const;

    //codec. stream < 0: the stream going to play
    AVCodecContext* audioCodecContext(int stream = -1) const;
    AVCodecContext* videoCodecContext(int stream = -1) const;
    AVCodecContext* subtitleCodecContext(int stream = -1) const;
    QString audioCodecName(int stream = -1) const;
    QString audioCodecLongName(int stream = -1) const;
    QString videoCodecName(int stream = -1) const;
    QString videoCodecLongName(int stream = -1) const;
    QString subtitleCodecName(int stream = -1) const;
    QString subtitleCodecLongName(int stream = -1) const;

    /**
     * @brief getInterruptTimeout return the interrupt timeout
     */
    qint64 getInterruptTimeout() const;
    /**
     * @brief setInterruptTimeout set the interrupt timeout
     * @param timeout in ms
     */
    void setInterruptTimeout(qint64 timeout);
    /**
     * @brief getInterruptStatus return the interrupt status
     */
    bool getInterruptStatus() const;
    /**
     * @brief setInterruptStatus set the interrupt status
     * @param interrupt true: abort current operation like loading and reading packets. false: no interrupt
     */
    void setInterruptStatus(bool interrupt);
    /*
     * libav's AVDictionary. we can ignore the flags used in av_dict_xxx because we can use hash api.
     * In addition, av_dict is slow.
     * empty means default options in ffmpeg
     */
    // avformat_open_input
    void setOptions(const QVariantHash &dict);
    QVariantHash options() const;

signals:
    void unloaded();
    void userInterrupted(); //NO direct connection because it's emit before interrupted happens
    void loaded();
    /*emit when the first frame is read*/
    void started();
    void finished(); //end of file
    void error(const QtAV::AVError& e); //explictly use QtAV::AVError in connection for Qt4 syntax
    void mediaStatusChanged(QtAV::MediaStatus status);

private:
    void setMediaStatus(MediaStatus status);
    /*!
     * \brief handleError
     * error code (errorCode) and message (msg) may be modified internally
     */
    void handleError(int averr, AVError::ErrorCode* errorCode, QString& msg);

    MediaStatus mCurrentMediaStatus;
    bool has_attached_pic;
    bool started_;
    bool eof;
    bool auto_reset_stream;
    Packet *pkt;
    qint64 ipts;
    int stream_idx;
    // wanted_xx_stream: -1 auto select by ff
    int wanted_audio_stream, wanted_video_stream, wanted_subtitle_stream;
    mutable int audio_stream, video_stream, subtitle_stream;
    mutable QList<int> audio_streams, video_streams, subtitle_streams;

    bool load();

    // set wanted_xx_stream. call openCodecs() to read new stream frames
    bool setStream(StreamType st, int stream);
    bool findStreams();
    QString formatName(AVFormatContext *ctx, bool longName = false) const;

    bool _is_input;
    AVFormatContext *format_context;
    AVCodecContext *a_codec_context, *v_codec_context, *s_codec_contex;
    //copy the info, not parse the file when constructed, then need member vars
    QString _file_name;
    AVInputFormat *_iformat;
    QAVIOContext* m_pQAVIO;
    QMutex mutex; //for seek and readFrame
    QElapsedTimer seek_timer;

    SeekUnit mSeekUnit;
    SeekTarget mSeekTarget;

    class InterruptHandler;
    InterruptHandler *mpInterrup;

    AVDictionary *mpDict;
    QVariantHash mOptions;

    bool m_network;
};

} //namespace QtAV
#endif // QAV_DEMUXER_H
