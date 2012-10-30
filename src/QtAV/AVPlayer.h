#ifndef WIDGET_H
#define WIDGET_H

#include <QtAV/AVDemuxer.h>
#include <QtAV/QAVPacketQueue.h>
#include <QtAV/QtAV_Global.h>

namespace QtAV {
class AudioOutput;
class AudioThread;
class QAVVideoThread;
class AudioDecoder;
class VideoDecoder;
class VideoRenderer;
class AVClock;
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

    AVDemuxer avinfo;
    AVClock *clock;
    VideoRenderer *renderer; //list?
    AudioOutput *audio;
    AudioDecoder *audio_dec;
    VideoDecoder *video_dec;
    AudioThread *audio_thread;
    QAVVideoThread *video_thread;
protected:
	virtual void timerEvent(QTimerEvent *);

};
}

#endif // WIDGET_H
