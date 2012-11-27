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

#ifndef QTAV_AVPLAYER_H
#define QTAV_AVPLAYER_H

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
class EventFilter;
class Q_EXPORT AVPlayer : public QObject
{
    Q_OBJECT
public:
    explicit AVPlayer(QObject *parent = 0);
    ~AVPlayer();

    void setFile(const QString& path);
	QString file() const;
    //capture is a jpeg image by default
    void setCaptureName(const QString& name);//base name
    void setCaptureSaveDir(const QString& dir);
    bool capture();
    bool play(const QString& path);
	bool isPlaying() const;
    void pause(bool p);
    bool isPaused() const;
    void setRenderer(VideoRenderer* renderer);

    void setMute(bool mute);
    bool isMute() const;

public slots:
    void play(); //replay
    void stop();
    void playNextFrame();
    void seekForward();
    void seekBackward();

protected slots:
    void resizeVideo(const QSize& size);

protected:
	int avTimerId;
    AVFormatContext	*formatCtx; //changed when reading a packet
    AVCodecContext *aCodecCtx, *vCodecCtx; //set once and not change
    QString path;
    QString capture_name, capture_dir;
    int m_drop_count;

    //the following things are required and must be setted not null
    AVDemuxer demuxer;
    AVDemuxThread *demuxer_thread;
    AVClock *clock;
    VideoRenderer *renderer; //list?
    AudioOutput *audio;
    AudioDecoder *audio_dec;
    VideoDecoder *video_dec;
    AudioThread *audio_thread;
    VideoThread *video_thread;

    //tODO: (un)register api
    friend class EventFilter;
    EventFilter *event_filter;
protected:
    virtual void timerEvent(QTimerEvent *);
};

} //namespace QtAV
#endif // QTAV_AVPLAYER_H
