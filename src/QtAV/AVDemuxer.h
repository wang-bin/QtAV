#ifndef QAVDEMUXER_H
#define QAVDEMUXER_H

#include <QtAV/QtAV_Global.h>
#include <qobject.h>
#include <qsize.h>

struct AVFormatContext;
struct AVCodecContext;
struct AVCodec;
struct AVFrame;
struct AVStream;

namespace QtAV {
class QAVPacket;
class Q_EXPORT AVDemuxer : public QObject //QIODevice?
{
    Q_OBJECT
public:
    AVDemuxer(const QString& fileName = QString(), QObject *parent = 0);
    ~AVDemuxer();

    bool atEnd() const;
    bool loadFile(const QString& fileName);
    bool readPacket();
    QAVPacket* packet() const; //current readed packet
    int stream() const; //current readed stream index

    //void seek();


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

signals:
    void finished(); //end of file

private:
    bool eof;
    QAVPacket *pkt;
    int stream_idx;
    bool findAVCodec();
    QString formatName(AVFormatContext *ctx, bool longName = false) const;

    bool _is_input;
    AVFormatContext *format_context;
    AVCodecContext *a_codec_context, *v_codec_context;
    //copy the info, not parse the file when constructed, then need member vars
    QString _file_name;
};
}

#endif // QAVDEMUXER_H
