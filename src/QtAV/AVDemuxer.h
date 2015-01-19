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

#ifndef QAV_DEMUXER_H
#define QAV_DEMUXER_H

#include <QtAV/CommonTypes.h>
#include <QtAV/AVError.h>
#include <QtAV/Packet.h>
#include <QtCore/QVariant>
#include <QtCore/QObject>
#include <QtCore/QScopedPointer>

struct AVFormatContext;
struct AVCodecContext;
class QIODevice;
// TODO: force codec name. clean code
namespace QtAV {
class AVError;
class AVInput;
class Q_AV_EXPORT AVDemuxer : public QObject
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
    /// Supported ffmpeg/libav input protocols(not complete). A static string list
    static const QStringList& supportedProtocols();

    AVDemuxer(QObject *parent = 0);
    ~AVDemuxer();
    MediaStatus mediaStatus() const;
    bool atEnd() const;
    QString fileName() const;
    QIODevice* ioDevice() const;
    /// not null for QIODevice, custom protocols
    AVInput* input() const;
    /*!
     * \brief setMedia
     * \return whether the media source is changed
     */
    bool setMedia(const QString& fileName);
    bool setMedia(QIODevice* dev);
    bool setMedia(AVInput* in);
    bool load();
    bool unload();
    bool isLoaded() const;
    /*!
     * \brief readFrame
     * Read a packet from 1 of the streams. use packet() to get the result packet. packet() returns last valid packet.
     * So do not use packet() if readFrame() failed.
     * Call readFrame() and seek() in the same thread.
     * \return true if no error or eof. false if error occurs, interrupted by user or time out(getInterruptTimeout())
     */
    bool readFrame(); // TODO: rename int readPacket(), return stream number
    /*!
     * \brief packet
     * return the packet read by demuxer. packet is invalid if readFrame() returns false.
     */
    Packet packet() const;
    /*!
     * \brief stream
     * Current readFrame() readed stream index.
     */
    int stream() const;

    bool isSeekable() const;
    void setSeekUnit(SeekUnit unit);
    SeekUnit seekUnit() const;
    void setSeekTarget(SeekTarget target);
    SeekTarget seekTarget() const;
    bool seek(qint64 pos); //pos: ms
    void seek(qreal q); //q: [0,1]. TODO: what if duration() is not valid?
    AVFormatContext* formatContext();
    QString formatName() const;
    QString formatLongName() const;
    // TODO: rename startPosition()
    qint64 startTime() const; //ms, AVFormatContext::start_time/1000
    qint64 duration() const; //ms, AVFormatContext::duration/1000
    qint64 startTimeUs() const; //us, AVFormatContext::start_time
    qint64 durationUs() const; //us, AVFormatContext::duration
    //total bit rate
    int bitRate() const; //AVFormatContext::bit_rate
    qreal frameRate() const; //deprecated AVStream::avg_frame_rate
    // if stream is -1, return the current video(or audio if no video) stream.
    // TODO: audio/videoFrames?
    qint64 frames(int stream = -1) const; //AVFormatContext::nb_frames
    bool hasAttacedPicture() const;
    /*!
     * \brief setStreamIndex
     * Set stream by index in stream list. call it after loaded.
     * Stream/index will not change in next load() unless media source changed
     * index < 0 is invalid
     */
    bool setStreamIndex(StreamType st, int index);
    // current open stream
    int currentStream(StreamType st) const;
    QList<int> streams(StreamType st) const;
    // current open stream
    int audioStream() const;
    QList<int> audioStreams() const;
    int videoStream() const;
    QList<int> videoStreams() const;
    int subtitleStream() const;
    QList<int> subtitleStreams() const;
    //codec. stream < 0: the stream going to play (or the stream set by setStreamIndex())
    AVCodecContext* audioCodecContext(int stream = -1) const;
    AVCodecContext* videoCodecContext(int stream = -1) const;
    AVCodecContext* subtitleCodecContext(int stream = -1) const;
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
    /*!
     * \brief setOptions
     * libav's AVDictionary. we can ignore the flags used in av_dict_xxx because we can use hash api.
     * empty value does nothing to current context if it is open, but will change AVDictionary options to null in next open.
     * AVDictionary is used in avformat_open_input() and will not change unless user call setOptions()
     */
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
    bool prepareStreams(); //called by loadFile(). if change to a new stream, call it(e.g. in AVPlayer)
    // set wanted_xx_stream. call openCodecs() to read new stream frames
    // stream < 0 is choose best
    bool setStream(StreamType st, int stream);
    void applyOptionsForDict();
    void applyOptionsForContext();
    void setMediaStatus(MediaStatus status);
    // error code (errorCode) and message (msg) may be modified internally
    void handleError(int averr, AVError::ErrorCode* errorCode, QString& msg);

    class Private;
    QScopedPointer<Private> d;
    class InterruptHandler;
};

} //namespace QtAV
#endif // QAV_DEMUXER_H
