/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>

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

#include <QtCore/QHash>
#include <QtCore/QScopedPointer>
#include <QtAV/AVClock.h>
#include <QtAV/Statistics.h>
#include <QtAV/VideoDecoderTypes.h>
#include <QtAV/AudioOutputTypes.h>
#include <QtAV/AVError.h>

class QIODevice;

namespace QtAV {

class AudioOutput;
class VideoRenderer;
class AVClock;
class Filter;
class VideoCapture;

class Q_AV_EXPORT AVPlayer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool autoLoad READ isAutoLoad WRITE setAutoLoad NOTIFY autoLoadChanged)
    Q_PROPERTY(bool asyncLoad READ isAsyncLoad WRITE setAsyncLoad NOTIFY asyncLoadChanged)
    Q_PROPERTY(bool mute READ isMute WRITE setMute NOTIFY muteChanged)
    Q_PROPERTY(qint64 position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(qint64 startPosition READ startPosition WRITE setStartPosition NOTIFY startPositionChanged)
    Q_PROPERTY(qint64 stopPosition READ stopPosition WRITE setStopPosition NOTIFY stopPositionChanged)
    Q_PROPERTY(qint64 repeat READ repeat WRITE setRepeat NOTIFY repeatChanged)
    Q_PROPERTY(int currentRepeat READ currentRepeat NOTIFY currentRepeatChanged)
    Q_PROPERTY(int brightness READ brightness WRITE setBrightness NOTIFY brightnessChanged)
    Q_PROPERTY(int contrast READ contrast WRITE setContrast NOTIFY contrastChanged)
    Q_PROPERTY(int saturation READ saturation WRITE setSaturation NOTIFY saturationChanged)
public:
    explicit AVPlayer(QObject *parent = 0);
    ~AVPlayer();

    //NOT const. This is the only way to access the clock.
    AVClock* masterClock();
    // If path is different from previous one, the stream to play will be reset to default.
    /*!
     * \brief setFile
     * TODO: Set current media source if current media is invalid or auto load is enabled.
     * Otherwise set as the pendding media and it becomes the current media if the next
     * load(), play() is called
     * \param path
     */
    void setFile(const QString& path);
    QString file() const;
    //QIODevice support
    void setIODevice(QIODevice* device);

    // force reload even if already loaded. otherwise only reopen codecs if necessary
    QTAV_DEPRECATED bool load(const QString& path, bool reload = true); //deprecated
    QTAV_DEPRECATED bool load(bool reload); //deprecated
    //
    bool isLoaded() const;
    /*!
     * \brief load
     * Load the current media set by setFile();. If already loaded, does nothing and return true.
     * If async load, mediaStatus() becomes LoadingMedia and user should connect signal loaded()
     * or mediaStatusChanged(QtAV::LoadedMedia) to a slot
     * \return true if success or already loaded.
     */
    bool load(); //NOT implemented.
    /*!
     * \brief unload
     * If the media is loading or loaded but not playing, unload it.
     * Does nothing if isPlaying()
     */
    void unload(); //TODO: emit signal?
    /*!
     * \brief setAsyncLoad
     * async load is enabled by default
     */
    void setAsyncLoad(bool value = true);
    bool isAsyncLoad() const;
    /*!
     * \brief setAutoLoad
     * true: current media source changed immediatly and stop current playback if new media source is set.
     * status becomes LoadingMedia=>LoadedMedia before play( and BufferedMedia when playing?)
     * false:
     * Default is false
     */
    void setAutoLoad(bool value = true); // NOT implemented
    bool isAutoLoad() const; // NOT implemented

    MediaStatus mediaStatus() const;

    qreal durationF() const; //unit: s, This function may be removed in the future.
    qint64 duration() const; //unit: ms. media duration. network stream may be very small, why?
    // the media's property.
    qint64 mediaStartPosition() const;
    qint64 mediaStopPosition() const;
    qreal mediaStartPositionF() const; //unit: s
    qreal mediaStopPositionF() const; //unit: s
    // can set by user. may be not the real media start position.
    qint64 startPosition() const;
    /*!
     * \brief stopPosition: the position at which player should stop playing
     * \return
     * If media stream is not a local file, stopPosition()==max value of qint64
     */
    qint64 stopPosition() const; //unit: ms
    QTAV_DEPRECATED qreal positionF() const; //unit: s.
    qint64 position() const; //unit: ms
    //0: play once. N: play N+1 times. <0: infinity
    int repeat() const; //or repeatMax()?
    int currentRepeat() const;
    /*
     * set audio/video/subtitle stream to n. n=0, 1, 2..., means the 1st, 2nd, 3rd audio/video/subtitle stream
     * if now==true, player will change to new stream immediatly. otherwise, you should call
     * play() to change to new stream
     * If a new file is set(except the first time) then a best stream will be selected. If the file not changed,
     * e.g. replay, then the stream not change
     * return: false if stream not changed, not valid
     */
    /*
     * steps to change stream:
     *    player.setFile(file); //will reset wanted stream to default
     *    player.setAudioStream(N, true)
     * or player.setAudioStream(N) && player.play()
     * player then will play from previous position. call
     *    player.seek(player.startPosition())
     * to play at beginning
     */
    bool setAudioStream(int n, bool now = false);
    bool setVideoStream(int n, bool now = false);
    bool setSubtitleStream(int n, bool now = false);
    int currentAudioStream() const;
    int currentVideoStream() const;
    int currentSubtitleStream() const;
    int audioStreamCount() const;
    int videoStreamCount() const;
    int subtitleStreamCount() const;
    /*!
     * \brief capture and save current frame to "$HOME/.QtAV/filename_pts.png".
     * To capture with custom configurations, such as name and dir, use
     * VideoCapture api through AVPlayer::videoCapture()
     * \return
     */
    bool captureVideo();
    VideoCapture *videoCapture();
    /*
     * replay without parsing the stream if it's already loaded. (not implemented)
     * to force reload the stream, unload() then play()
     */
    //TODO: no replay
    bool play(const QString& path);
    bool isPlaying() const;
    bool isPaused() const;
    // TODO: use id as parameter and return ptr?
    void addVideoRenderer(VideoRenderer *renderer);
    void removeVideoRenderer(VideoRenderer *renderer);
    void clearVideoRenderers();
    void setRenderer(VideoRenderer* renderer);
    VideoRenderer* renderer();
    QList<VideoRenderer*> videoOutputs();
    void setAudioOutput(AudioOutput* ao);
    //default has 1 audiooutput
    //void addAudioOutput(AudioOutput* ao);
    //void removeAudioOutput(AudioOutput* ao);
    //QList<AudioOutput*> audioOutputs();
    /*!
     * To change audio format, you should set both AudioOutput's format and AudioResampler's format
     * So signals/slots is a better solution.
     * TODO: AudioOutput.audioFormatChanged (signal)---AudioResampler.setOutAudioFormat (slot)
     */
    AudioOutput* audio();
    void enableAudio(bool enable = true);
    void disableAudio(bool disable = true);
    void setMute(bool mute);
    bool isMute() const;
    /*!
     * \brief setSpeed set playing speed.
     * \param speed  speed > 0. 1.0: normal speed
     * TODO: playbackRate
     */
    void setSpeed(qreal speed);
    qreal speed() const;

    Statistics& statistics();
    const Statistics& statistics() const;
    /*
     * install the filter in AVThread. Filter will apply before rendering data
     * return false if filter is already registered or audio/video thread is not ready(will install when ready)
     */
    bool installAudioFilter(Filter *filter);
    bool installVideoFilter(Filter *filter);
    bool uninstallFilter(Filter *filter);

    void setPriority(const QVector<VideoDecoderId>& ids);
    //void setPriority(const QVector<AudioOutputId>& ids);
    /*!
     * below APIs are deprecated.
     * TODO: setValue("key", value) or setOption("key", value) ?
     * enum OptionKey { Brightness, ... VideoCodec, FilterOptions...}
     * or use QString as keys?
     */
    int brightness() const;
    int contrast() const;
    int hue() const; //not implemented
    int saturation() const;
    /*
     * libav's AVDictionary. we can ignore the flags used in av_dict_xxx because we can use hash api.
     * In addition, av_dict is slow.
     */
    // avformat_open_input
    void setOptionsForFormat(const QVariantHash &dict);
    QVariantHash optionsForFormat() const;
    // avcodec_open2. TODO: the same for audio/video codec?
    /*!
     * \sa AVDecoder::setOptions()
     * example:
     *  "avcodec": {"vismv":"pf"}, "vaapi":{"display":"DRM"}
     * equals
     *  "vismv":"pf", "vaapi":{"display":"DRM"}
     */
    // QVariantHash deprecated, use QVariantMap to get better js compatibility
    void setOptionsForAudioCodec(const QVariantHash &dict);
    QVariantHash optionsForAudioCodec() const;
    void setOptionsForVideoCodec(const QVariantHash& dict);
    QVariantHash optionsForVideoCodec() const;
    // avfilter_init_dict

public slots:
    void togglePause();
    void pause(bool p);
    /*!
     * \brief play
     * If media is not loaded, load()
     */
    void play(); //replay
    void stop();
    void playNextFrame();

    /*!
     * \brief setRepeat
     *  repeat max times between startPosition() and endPosition()
     *  max==0: no repeat
     *  max<0: infinity. std::numeric_limits<int>::max();
     * \param max
     */
    void setRepeat(int max);
    /*!
     * \brief startPosition
     *  Used to repeat from startPosition() to endPosition().
     *  You can also start to play at a given position
     * \code
     *     player->setStartPosition();
     *     player->play("some video");
     * \endcode
     *  pos < 0, equals duration()+pos
     *  pos == 0, means start at the beginning of media stream
     *  (may be not exactly equals 0, seek to demuxer.startPosition()/startTime())
     *  pos > media end position: no effect
     */
    void setStartPosition(qint64 pos);
    /*!
     * \brief stopPosition
     *  pos = 0: mediaStopPosition()
     *  pos < 0: duration() + pos
     */
    void setStopPosition(qint64 pos);
    /*!
     * \brief setPosition equals to seek(qreal)
     *  position < 0, position() equals duration()+position
     * \param position in ms
     *
     */
    void setPosition(qint64 position);
    void seek(qreal r); // r: [0, 1]
    void seek(qint64 pos); //ms. same as setPosition(pos)
    void seekForward();
    void seekBackward();
    void updateClock(qint64 msecs); //update AVClock's external clock

    // for all renderers. val: [-100, 100]. other value changes nothing
    void setBrightness(int val);
    void setContrast(int val);
    void setHue(int val);  //not implemented
    void setSaturation(int val);

signals:
    void autoLoadChanged();
    void asyncLoadChanged();
    void muteChanged();
    void sourceChanged();
    void loaded(); // == mediaStatusChanged(QtAV::LoadedMedia)
    void mediaStatusChanged(QtAV::MediaStatus status); //explictly use QtAV::MediaStatus
    void error(const QtAV::AVError& e); //explictly use QtAV::AVError in connection for Qt4 syntax
    void paused(bool p);
    void started();
    void stopped();
    void speedChanged(qreal speed);
    void repeatChanged(int r);
    void currentRepeatChanged(int r);
    void startPositionChanged(qint64 position);
    void stopPositionChanged(qint64 position);
    void positionChanged(qint64 position);
    void brightnessChanged(int val);
    void contrastChanged(int val);
    void hueChanged(int val);
    void saturationChanged(int val);

private slots:
    void loadInternal(); // simply load
    void unloadInternal();
    void playInternal(); // simply play
    void loadAndPlay();
    void stopFromDemuxerThread();
    void aboutToQuitApp();
    // start/stop notify timer in this thread. use QMetaObject::invokeMethod
    void startNotifyTimer();
    void stopNotifyTimer();
    void onStarted();

protected:
    // TODO: set position check timer interval
    virtual void timerEvent(QTimerEvent *);

private:
    class Private;
    QScopedPointer<Private> d;
};

} //namespace QtAV
#endif // QTAV_AVPLAYER_H
