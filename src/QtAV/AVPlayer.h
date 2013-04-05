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

namespace QtAV {

class AudioOutput;
class AudioThread;
class VideoThread;
class AudioDecoder;
class VideoDecoder;
class VideoRenderer;
class AVClock;
class AVDemuxThread;
class VideoCapture;
class OSDFilter;
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
    qreal duration() const; //This function may be removed in the future.
    /*
     * default: [fmt: PNG, dir: capture, name: basename]
     * replace the existing capture; return the replaced one
     * set 0 will disable the capture
     */
    VideoCapture* setVideoCapture(VideoCapture* cap);
    VideoCapture *videoCapture();
    void setCaptureName(const QString& name);//TODO: remove. base name
    void setCaptureSaveDir(const QString& dir); //TODO: remove
    bool captureVideo();
    OSDFilter* setOSDFilter(OSDFilter* osd);
    OSDFilter* osdFilter();
    bool play(const QString& path);
	bool isPlaying() const;
    bool isPaused() const;
    //this will install the default EventFilter. To use customized filter, register after this
    VideoRenderer* setRenderer(VideoRenderer* renderer);
    VideoRenderer* renderer();
    AudioOutput* audio();
    void setMute(bool mute);
    bool isMute() const;
    /*only 1 event filter is available. the previous one will be removed. setPlayerEventFilter(0) will remove the event filter*/
    void setPlayerEventFilter(QObject *obj);

signals:
    void started();
    void stopped();

public slots:
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

protected:
    bool loaded;
    AVFormatContext	*formatCtx; //changed when reading a packet
    AVCodecContext *aCodecCtx, *vCodecCtx; //set once and not change
    QString path;
    QString capture_name, capture_dir;

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
    OSDFilter *osd;
};

} //namespace QtAV
#endif // QTAV_AVPLAYER_H
