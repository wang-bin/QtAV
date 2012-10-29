#ifndef WIDGET_H
#define WIDGET_H

#include <qwidget.h>
#include <qdatetime.h>
#include <qimage.h>

class T_FPS
{
public:
    QTime m_time_line;
    double m_fps;
    int m_count;
    explicit T_FPS() : m_fps(0), m_count(0) {}
    bool wake() {
        if(!m_time_line.isNull()) return false;
        m_time_line.start();
        return true;
    }
    void restart() {
        m_count = 0;
        m_time_line.start();
    }
    void add(int a) {
        m_count += a;
    }
    double value() {
        if(m_time_line.isNull()) {
			qDebug("[T_FPS.value()] warning: T_FPS.wake() was not called.");
            return 0;
        } else if(m_time_line.elapsed() < 1000) {
            return m_fps;
        } else {
            m_fps = 1000.0 * m_count / m_time_line.elapsed();
            restart();
            return m_fps;
        }
    }
};

#include <QtAV/AVInfo.h>
#include <QtAV/QAVPacketQueue.h>
#include <QtAV/QtAV_Global.h>

namespace QtAV {
class AudioOutput;
class AudioThread;
class QAVVideoThread;
class AudioDecoder;
class VideoRenderer;
class Q_EXPORT AVPlayer : public QObject
{
    Q_OBJECT
public:
    explicit AVPlayer(QObject *parent = 0);
    ~AVPlayer();
    
    void setFile(const QString& fileName) {filename=fileName;}
    bool play(const QString& path = QString());
    void setRenderer(VideoRenderer* renderer);

protected:
	int avTimerId;
    AVFormatContext	*formatCtx; //changed when reading a packet
    AVCodecContext *aCodecCtx, *vCodecCtx; //set once and not change
    AVFrame *aFrame, *vFrame; //set once and not change
	int		frameFinished;
    QString		filename;
    int frameNo;
    QImage m_displayBuffer;
    //T_AVPicture *m_av_picture;
    QTime m_time_line;
    int m_drop_count;
	T_FPS m_fps1;

    AVInfo avinfo;
    VideoRenderer *renderer; //list?
    AudioOutput *audio;
    AudioDecoder *audio_dec;
    AudioThread *audio_thread;
    QAVVideoThread *video_thread;
    QAVPacketQueue audio_queue, video_queue;
protected:
	virtual void timerEvent(QTimerEvent *);

	void render();

};
}

#endif // WIDGET_H
