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

#ifndef QTAV_AVPLAYER_H
#define QTAV_AVPLAYER_H

#include <QtAV/AVClock.h>
#include <QtAV/AVDemuxer.h>
#include <QtAV/Statistics.h>

namespace QtAV {

class AVOutput;
class AudioOutput;
class AVThread;
class AudioThread;
class VideoThread;
class AudioDecoder;
class VideoDecoder;
class VideoRenderer;
class AVClock;
class AVDemuxThread;
class VideoCapture;
class Q_EXPORT AVPlayer : public QObject
{
    Q_OBJECT
public:
    explicit AVPlayer(QObject *parent = 0);
    ~AVPlayer();

    //NOT const. This is the only way to access the clock.
    AVClock* masterClock();
    void setFile(const QString& path);
	QString file() const;
    bool load(const QString& path);
    bool load();
    bool isLoaded() const;
    qreal duration() const; //unit: s, This function may be removed in the future.
    /*!
     * \brief capture and save current frame to "appdir/filename_pts.png".
     * To capture with custom configurations, such as name and dir, use
     * VideoCapture api through AVPlayer::videoCapture()
     * \return
     */
    bool captureVideo();
    VideoCapture *videoCapture();
    bool play(const QString& path);
	bool isPlaying() const;
    bool isPaused() const;
    //this will install the default EventFilter. To use customized filter, register after this
    //TODO: addRenderer; renderers()
    void setRenderer(VideoRenderer* renderer);
    VideoRenderer* renderer();
    void setAudioOutput(AudioOutput* ao);
    /*!
     * To change audio format, you should set both AudioOutput's format and AudioResampler's format
     * So signals/slots is a better solution.
     * TODO: AudioOutput.audioFormatChanged (signal)---AudioResampler.setOutAudioFormat (slot)
     */
    AudioOutput* audio();
    void setMute(bool mute);
    bool isMute() const;
    /*!
     * \brief setSpeed set playing speed.
     * \param speed  speed > 0. 1.0: normal speed
     */
    void setSpeed(qreal speed);
    qreal speed() const;
    /*only 1 event filter is available. the previous one will be removed. setPlayerEventFilter(0) will remove the event filter*/
    void setPlayerEventFilter(QObject *obj);

    Statistics& statistics();
    const Statistics& statistics() const;
signals:
    void paused(bool p);
    void started();
    void stopped();
    void speedChanged(qreal speed);

public slots:
    void togglePause();
    void pause(bool p);
    void play(); //replay
    void stop();
    void playNextFrame();
    void seek(qreal pos);
    void seekForward();
    void seekBackward();
    void updateClock(qint64 msecs); //update AVClock's external clock

protected slots:
    void resizeRenderer(const QSize& size);

private:
    void initStatistics();
    void setupAudioThread();
    void setupVideoThread();
    void setupAVThread(AVThread*& thread, AVCodecContext* ctx);
    template<class Out>
    void setAVOutput(Out*& pOut, Out* pNew, AVThread* thread);
    //TODO: addAVOutput()

    bool loaded;
    AVFormatContext	*formatCtx; //changed when reading a packet
    AVCodecContext *aCodecCtx, *vCodecCtx; //set once and not change
    QString path;

    //the following things are required and must be setted not null
    AVDemuxer demuxer;
    AVDemuxThread *demuxer_thread;
    AVClock *clock;
    VideoRenderer *_renderer; //list?
    AudioOutput *_audio;
    AudioDecoder *audio_dec;
    VideoDecoder *video_dec;
    AudioThread *audio_thread;
    VideoThread *video_thread;

    //tODO: (un)register api
    QObject *event_filter;
    VideoCapture *video_capture;
    Statistics mStatistics;
    qreal mSpeed;
};

} //namespace QtAV
#endif // QTAV_AVPLAYER_H
