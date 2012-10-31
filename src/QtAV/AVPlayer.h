#ifndef WIDGET_H
#define WIDGET_H

#include <QtAV/AVDemuxer.h>

namespace QtAV {
class AudioOutput;
class AudioThread;
class VideoThread;
class AudioDecoder;
class VideoDecoder;
class VideoRenderer;
class AVClock;
class AVDemuxThread;
class Q_EXPORT AVPlayer : public QObject
{
    Q_OBJECT
public:
    explicit AVPlayer(QObject *parent = 0);
    ~AVPlayer();
    
    void setFile(const QString& fileName) {filename=fileName;}
    bool play(const QString& path = QString());
    void setRenderer(VideoRenderer* renderer);

protected slots:
    void resizeVideo(const QSize& size);

protected:
	int avTimerId;
    AVFormatContext	*formatCtx; //changed when reading a packet
    AVCodecContext *aCodecCtx, *vCodecCtx; //set once and not change
    QString		filename;
    int m_drop_count;

    AVDemuxer demuxer;
    AVDemuxThread *demuxer_thread;
    AVClock *clock;
    VideoRenderer *renderer; //list?
    AudioOutput *audio;
    AudioDecoder *audio_dec;
    VideoDecoder *video_dec;
    AudioThread *audio_thread;
    VideoThread *video_thread;
protected:
	virtual void timerEvent(QTimerEvent *);

};
}

#endif // WIDGET_H
