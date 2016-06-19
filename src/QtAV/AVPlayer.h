/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

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

#include <limits>
#include <QtCore/QHash>
#include <QtCore/QScopedPointer>
#include <QtAV/AudioOutput.h>
#include <QtAV/AVClock.h>
#include <QtAV/Statistics.h>
#include <QtAV/VideoDecoder.h>
#include <QtAV/AVError.h>

QT_BEGIN_NAMESPACE
class QIODevice;
QT_END_NAMESPACE

namespace QtAV {

class MediaIO;
class AudioOutput;
class VideoRenderer;
class Filter;
class AudioFilter;
class VideoFilter;
class VideoCapture;
/*!
 * \brief The AVPlayer class
 * Preload:
 * \code
 *  player->setFile(...);
 *  player->load();
 *  do some thing...
 *  player->play();
 * \endcode
 * No preload:
 * \code
 *  player->setFile(...);
 *  player->play();
 * \endcode
 */
class Q_AV_EXPORT AVPlayer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool relativeTimeMode READ relativeTimeMode WRITE setRelativeTimeMode NOTIFY relativeTimeModeChanged)
    Q_PROPERTY(bool autoLoad READ isAutoLoad WRITE setAutoLoad NOTIFY autoLoadChanged)
    Q_PROPERTY(bool asyncLoad READ isAsyncLoad WRITE setAsyncLoad NOTIFY asyncLoadChanged)
    Q_PROPERTY(qreal bufferProgress READ bufferProgress NOTIFY bufferProgressChanged)
    Q_PROPERTY(bool seekable READ isSeekable NOTIFY seekableChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(qint64 position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(qint64 startPosition READ startPosition WRITE setStartPosition NOTIFY startPositionChanged)
    Q_PROPERTY(qint64 stopPosition READ stopPosition WRITE setStopPosition NOTIFY stopPositionChanged)
    Q_PROPERTY(qint64 repeat READ repeat WRITE setRepeat NOTIFY repeatChanged)
    Q_PROPERTY(int currentRepeat READ currentRepeat NOTIFY currentRepeatChanged)
    Q_PROPERTY(qint64 interruptTimeout READ interruptTimeout WRITE setInterruptTimeout NOTIFY interruptTimeoutChanged)
    Q_PROPERTY(bool interruptOnTimeout READ isInterruptOnTimeout WRITE setInterruptOnTimeout NOTIFY interruptOnTimeoutChanged)
    Q_PROPERTY(int notifyInterval READ notifyInterval WRITE setNotifyInterval NOTIFY notifyIntervalChanged)
    Q_PROPERTY(int brightness READ brightness WRITE setBrightness NOTIFY brightnessChanged)
    Q_PROPERTY(int contrast READ contrast WRITE setContrast NOTIFY contrastChanged)
    Q_PROPERTY(int saturation READ saturation WRITE setSaturation NOTIFY saturationChanged)
    Q_PROPERTY(State state READ state WRITE setState NOTIFY stateChanged)
    Q_PROPERTY(QtAV::MediaStatus mediaStatus READ mediaStatus NOTIFY mediaStatusChanged)
    Q_PROPERTY(QtAV::MediaEndAction mediaEndAction READ mediaEndAction WRITE setMediaEndAction NOTIFY mediaEndActionChanged)
    Q_ENUMS(State)
public:
    /*!
     * \brief The State enum
     * The playback state. It's different from MediaStatus. MediaStatus indicates media stream state
     */
    enum State {
        StoppedState,
        PlayingState, /// Start to play if it was stopped, or resume if it was paused
        PausedState
    };

    /// Supported input protocols. A static string list
    static const QStringList& supportedProtocols();

    explicit AVPlayer(QObject *parent = 0);
    ~AVPlayer();

    /*!
     * \brief masterClock
     * setClockType() should call when playback started.
     * \return
     */
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
    /*!
     * \brief setIODevice
     * Play media stream from QIODevice. AVPlayer does not take the ownership. You have to manage device lifetime.
     */
    void setIODevice(QIODevice* device);
    /*!
     * \brief setInput
     * Play media stream from custom MediaIO. AVPlayer's demuxer takes the ownership. Call it when player is stopped.
     */
    void setInput(MediaIO* in);
    MediaIO* input() const;

    bool isLoaded() const;
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
    // TODO: add hasAudio, hasVideo, isMusic(has pic)
    /*!
     * \brief relativeTimeMode
     * true (default): mediaStartPosition() is always 0. All time related API, for example setPosition(), position() and positionChanged()
     * use relative time instead of real pts
     * false: mediaStartPosition() is from media stream itself, same as absoluteMediaStartPosition()
     * To get real start time, use statistics().start_time. Or setRelativeTimeMode(false) first but may affect playback when playing.
     */
    bool relativeTimeMode() const;
    /// Media stream property. The first timestamp in the media
    qint64 absoluteMediaStartPosition() const;
    qreal durationF() const; //unit: s, This function may be removed in the future.
    qint64 duration() const; //unit: ms. media duration. network stream may be very small, why?
    /*!
     * \brief mediaStartPosition
     * If relativeTimeMode() is true (default), it's 0. Otherwise is the same as absoluteMediaStartPosition()
     */
    qint64 mediaStartPosition() const;
    /// mediaStartPosition() + duration().
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
    qint64 position() const; //unit: ms
    //0: play once. N: play N+1 times. <0: infinity
    int repeat() const; //or repeatMax()?
    int currentRepeat() const;
    /*!
     * \brief setExternalAudio
     * set audio track from an external audio stream. this will try to load the external audio and
     * select the 1st audio stream. If no error happens, the external audio stream will be set to
     * current audio track.
     * If external audio stream <0 before play, stream is auto selected
     * You have to manually empty value to unload the external audio!
     * \param file external audio file path. Set empty to use internal audio tracks. TODO: reset stream number if switch to internal
     * \return true if no error happens
     */
    bool setExternalAudio(const QString& file);
    QString externalAudio() const;
    /*!
     * \brief externalAudioTracks
     * A list of QVariantMap. Using QVariantMap and QVariantList is mainly for QML support.
     * [ {id: 0, file: abc.dts, language: eng, title: xyz}, ...]
     * id: used for setAudioStream(id)
     * \sa externalAudioTracksChanged
     */
    const QVariantList& externalAudioTracks() const;
    const QVariantList& internalAudioTracks() const;
    /*!
     * \brief setAudioStream
     * set an external audio file and stream number as audio track
     * \param file external audio file. set empty to use internal audio tracks
     * \param n audio stream number n=0, 1, .... n<0: disable audio thread
     * \return false if fail
     */
    bool setAudioStream(const QString& file, int n = 0);
    /*!
     * set audio/video/subtitle stream to n. n=0, 1, 2..., means the 1st, 2nd, 3rd audio/video/subtitle stream
     * If n < 0, there will be no audio thread and sound/
     * If a new file is set(except the first time) then a best stream will be selected. If the file not changed,
     * e.g. replay, then the stream not change
     * return: false if stream not changed, not valid
     * TODO: rename to track instead of stream
     */
    /*!
     * \brief setAudioStream
     * Set audio stream number in current media or external audio file
     */
    bool setAudioStream(int n);
    //TODO: n<0, no video thread
    bool setVideoStream(int n);
    /*!
     * \brief internalAudioTracks
     * A list of QVariantMap. Using QVariantMap and QVariantList is mainly for QML support.
     * [ {id: 0, file: abc.dts, language: eng, title: xyz}, ...]
     * id: used for setSubtitleStream(id)
     * \sa internalSubtitleTracksChanged;
     * Different with external audio tracks, the external subtitle is supported by class Subtitle.
     */
    const QVariantList& internalSubtitleTracks() const;
    bool setSubtitleStream(int n);
    int currentAudioStream() const;
    int currentVideoStream() const;
    int currentSubtitleStream() const;
    int audioStreamCount() const;
    int videoStreamCount() const;
    int subtitleStreamCount() const;
    /*!
     * \brief videoCapture
     * Capture the current frame using videoCapture()->capture()
     * \sa VideoCapture
     */
    VideoCapture *videoCapture() const;
    //TODO: no replay, replay without parsing the stream if it's already loaded. (not implemented). to force reload the stream, unload() then play()
    /*!
     * \brief play
     * If isAsyncLoad() is true (default), play() will return immediately. Signals started() and stateChanged() will be emitted if media is loaded and playback starts.
     */
    void play(const QString& path);
    bool isPlaying() const;
    bool isPaused() const;
    /*!
     * \brief state
     * Player's playback state. Default is StoppedState.
     * setState() is a replacement of play(), stop(), pause(bool)
     * \return
     */
    State state() const;
    void setState(State value);

    // TODO: use id as parameter and return ptr?
    void addVideoRenderer(VideoRenderer *renderer);
    void removeVideoRenderer(VideoRenderer *renderer);
    void clearVideoRenderers();
    void setRenderer(VideoRenderer* renderer);
    VideoRenderer* renderer();
    QList<VideoRenderer*> videoOutputs();
    /*!
     * \brief audio
     * AVPlayer always has an AudioOutput instance. You can access or control audio output properties through audio().
     * To disable audio output, set audio()->setBackends(QStringList() << "null") before starting playback
     * \return
     */
    AudioOutput* audio();
    /*!
     * \brief setSpeed set playback speed.
     * \param speed  speed > 0. 1.0: normal speed
     * TODO: playbackRate
     */
    void setSpeed(qreal speed);
    qreal speed() const;

    /*!
     * \brief setInterruptTimeout
     * Emit error(usually network error) if open/read spends too much time.
     * If isInterruptOnTimeout() is true, abort current operation and stop playback
     * \param ms milliseconds. <0: never interrupt.
     */
    /// TODO: rename to timeout
    void setInterruptTimeout(qint64 ms);
    qint64 interruptTimeout() const;
    /*!
     * \brief setInterruptOnTimeout
     * \param value
     */
    void setInterruptOnTimeout(bool value);
    bool isInterruptOnTimeout() const;
    /*!
     * \brief setFrameRate
     * Force the (video) frame rate to a given value.
     * Call it before playback start.
     * If frame rate is set to a valid value(>0), the clock type will be set to
     * User configuration of AVClock::ClockType and autoClock will be ignored.
     * \param value <=0: ignore the value. normal playback ClockType and AVCloc
     * >0: force to a given (video) frame rate
     */
    void setFrameRate(qreal value);
    qreal forcedFrameRate() const;
    //Statistics& statistics();
    const Statistics& statistics() const;
    /*!
     * \brief installFilter
     * Insert a filter at position 'index' of current filter list.
     * If the filter is already installed, it will move to the correct index.
     * \param index A nagative index == size() + index. If index >= size(), append at last
     * \return false if audio/video thread is not ready. But the filter will be installed when thread is ready.
     * false if already installed.
     */
    bool installFilter(AudioFilter* filter, int index = 0x7FFFFFFF);
    bool installFilter(VideoFilter* filter, int index = 0x7FFFFFFF);
    bool uninstallFilter(AudioFilter* filter);
    bool uninstallFilter(VideoFilter* filter);
    QList<Filter*> audioFilters() const;
    QList<Filter*> videoFilters() const;
    /*!
     * \brief setPriority
     * A suitable decoder will be applied when video is playing. The decoder does not change in current playback if no decoder is found.
     * If not playing or no decoder found, the decoder will be changed at the next playback
     * \param ids
     */
    void setPriority(const QVector<VideoDecoderId>& ids);
    /*!
     * \brief setVideoDecoderPriority
     * also can set in opt.priority
     * \param names the video decoder name list in priority order. Name can be "FFmpeg", "CUDA", "DXVA", "D3D11", "VAAPI", "VDA", "VideoToolbox", case insensitive
     */
    void setVideoDecoderPriority(const QStringList& names);
    QStringList videoDecoderPriority() const;
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
    /*!
     * \sa AVDemuxer::setOptions()
     * example:
     * QVariantHash opt;
     * opt["rtsp_transport"] = "tcp"
     * player->setOptionsForFormat(opt);
     */
    // avformat_open_input
    void setOptionsForFormat(const QVariantHash &dict);
    QVariantHash optionsForFormat() const;
    // avcodec_open2. TODO: the same for audio/video codec?
    /*!
     * \sa AVDecoder::setOptions()
     * example:
     * QVariantHash opt, vaopt, ffopt;
     * vaopt["display"] = "X11";
     * opt["vaapi"] = vaopt; // only apply for va-api decoder
     * ffopt["vismv"] = "pf";
     * opt["ffmpeg"] = ffopt; // only apply for ffmpeg software decoder
     * player->setOptionsForVideoCodec(opt);
     */
    // QVariantHash deprecated, use QVariantMap to get better js compatibility
    void setOptionsForAudioCodec(const QVariantHash &dict);
    QVariantHash optionsForAudioCodec() const;
    void setOptionsForVideoCodec(const QVariantHash& dict);
    QVariantHash optionsForVideoCodec() const;

    /*!
     * \brief mediaEndAction
     * The action at the end of media or when playback is stopped. Default is quit threads and clear video renderers.
     * If the flag MediaEndAction_KeepDisplay is set, the last video frame will keep displaying in video renderers.
     * If MediaEndAction_Pause is set, you can still seek and resume the playback because no thread exits.
     */
    MediaEndAction mediaEndAction() const;
    void setMediaEndAction(MediaEndAction value);

public slots:
    /*!
     * \brief load
     * Load the current media set by setFile(); Can be used to reload a media and call play() later. If already loaded, does nothing and return true.
     * If async load, mediaStatus() becomes LoadingMedia and user should connect signal loaded()
     * or mediaStatusChanged(QtAV::LoadedMedia) to a slot
     * \return true if success or already loaded.
     */
    bool load();

    void togglePause();
    void pause(bool p = true);
    /*!
     * \brief play
     * Load media and start playback. If current media is playing and media source is not changed, nothing to do. If media source is not changed, try to load (not in LoadingStatus or LoadedStatus) and start playback. If media source changed, reload and start playback.
     */
    void play();
    /*!
     * \brief stop
     * Stop playback. It blocks current thread until the playback is stopped. Will emit signal stopped(). startPosition(), stopPosition(), repeat() are reset
     */
    void stop();
    /*!
     * \brief stepForward
     * Play the next frame and pause
     */
    void stepForward();
    /*!
     * \brief stepBackward
     * Play the previous frame and pause. Currently only support the previous decoded frames
     */
    void stepBackward();

    void setRelativeTimeMode(bool value);
    /*!
     * \brief setRepeat
     *  Repeat max times between startPosition() and endPosition(). It's reset if playback is stopped.
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
     *  pos < 0: equals duration()+pos
     *  pos == 0, means start at the beginning of media stream
     *  pos > media end position, or pos > normalized stopPosition(): undefined
     */
    void setStartPosition(qint64 pos);
    /*!
     * \brief stopPosition
     *  pos > mediaStopPosition(): mediaStopPosition()
     *  pos < 0: duration() + pos
     * With the default value, the playback will not stop until the end of media (including dynamically changed media duration, e.g. recording video)
     */
    void setStopPosition(qint64 pos = std::numeric_limits<qint64>::max());
    /*!
     * \brief setTimeRange
     * Set startPosition and stopPosition. Make sure start <= stop.
     */
    void setTimeRange(qint64 start, qint64 stop = std::numeric_limits<qint64>::max());

    bool isSeekable() const;
    /*!
     * \brief setPosition equals to seek(qreal)
     *  position < 0: 0
     * \param position in ms
     */
    void setPosition(qint64 position);
    void seek(qreal r); // r: [0, 1]
    void seek(qint64 pos); //ms. same as setPosition(pos)
    void seekForward();
    void seekBackward();
    void setSeekType(SeekType type);
    SeekType seekType() const;

    /*!
     * \brief bufferProgress
     * How much the data buffer is currently filled. From 0.0 to 1.0.
     * Playback can start or resume only when the buffer is entirely filled.
     */
    qreal bufferProgress() const;
    /*!
     * \brief bufferSpeed
     * Bytes/s
     * \return 0 if not buffering. >= 0 if buffering
     */
    qreal bufferSpeed() const;
    /*!
     * \brief buffered
     * Current buffered value in msecs, bytes or packet count depending on bufferMode()
     */
    qint64 buffered() const;
    void setBufferMode(BufferMode mode);
    BufferMode bufferMode() const;
    /*!
     * \brief setBufferValue
     * Ensure the buffered msecs/bytes/packets in queue is at least the given value before playback starts.
     * Set before playback starts.
     * \param value <0: auto; BufferBytes: bytes, BufferTime: msecs, BufferPackets: packets count
     */
    void setBufferValue(qint64 value);
    int bufferValue() const;

    /*!
     * \brief setNotifyInterval
     * The interval at which progress will update
     * \param msec <=0: auto and compute internally depending on duration and fps
     */
    void setNotifyInterval(int msec);
    /// The real notify interval. Always > 0
    int notifyInterval() const;
    void updateClock(qint64 msecs); //update AVClock's external clock
    // for all renderers. val: [-100, 100]. other value changes nothing
    void setBrightness(int val);
    void setContrast(int val);
    void setHue(int val);  //not implemented
    void setSaturation(int val);

Q_SIGNALS:
    void bufferProgressChanged(qreal);
    void relativeTimeModeChanged();
    void autoLoadChanged();
    void asyncLoadChanged();
    void muteChanged();
    void sourceChanged();
    void loaded(); // == mediaStatusChanged(QtAV::LoadedMedia)
    void mediaStatusChanged(QtAV::MediaStatus status); //explictly use QtAV::MediaStatus
    void mediaEndActionChanged(QtAV::MediaEndAction action);
    /*!
     * \brief durationChanged emit when media is loaded/unloaded
     */
    void durationChanged(qint64);
    void error(const QtAV::AVError& e); //explictly use QtAV::AVError in connection for Qt4 syntax
    void paused(bool p);
    /*!
     * \brief started
     * Emitted when playback is started. Some functions that control playback should be called after playback is started, otherwise they won't work, e.g. setPosition(), pause(). stop() can be called at any time.
     */
    void started();
    void stopped();
    void stoppedAt(qint64 position);
    void stateChanged(QtAV::AVPlayer::State state);
    void speedChanged(qreal speed);
    void repeatChanged(int r);
    void currentRepeatChanged(int r);
    void startPositionChanged(qint64 position);
    void stopPositionChanged(qint64 position);
    void seekableChanged();
    /*!
     * \brief seekFinished
     * If there is a video stream currently playing, emitted when video seek is finished. If only an audio stream is playing, emitted when audio seek is finished. The position() is the master clock value, It can be very different from video timestamp at this time.
     * \param position The video or audio timestamp when seek is finished
     */
    void seekFinished(qint64 position);
    void positionChanged(qint64 position);
    void interruptTimeoutChanged();
    void interruptOnTimeoutChanged();
    void notifyIntervalChanged();
    void brightnessChanged(int val);
    void contrastChanged(int val);
    void hueChanged(int val);
    void saturationChanged(int val);
    void subtitleStreamChanged(int value);
    /*!
     * \brief internalAudioTracksChanged
     * Emitted when media is loaded. \sa internalAudioTracks
     */
    void internalAudioTracksChanged(const QVariantList& tracks);
    void externalAudioTracksChanged(const QVariantList& tracks);
    void internalSubtitleTracksChanged(const QVariantList& tracks);
    /*!
     * \brief internalSubtitleHeaderRead
     * Emitted when internal subtitle is loaded. Empty data if no data.
     * codec is used by subtitle processors
     */
    void internalSubtitleHeaderRead(const QByteArray& codec, const QByteArray& data);
    void internalSubtitlePacketRead(int track, const QtAV::Packet& packet);
private Q_SLOTS:
    void loadInternal(); // simply load
    void playInternal(); // simply play
    void stopFromDemuxerThread();
    void aboutToQuitApp();
    // start/stop notify timer in this thread. use QMetaObject::invokeMethod
    void startNotifyTimer();
    void stopNotifyTimer();
    void onStarted();
    void updateMediaStatus(QtAV::MediaStatus status);
    void onSeekFinished(qint64 value);
    void tryClearVideoRenderers();
protected:
    // TODO: set position check timer interval
    virtual void timerEvent(QTimerEvent *);
private:
    /*!
     * \brief unload
     * If the media is loading or loaded but not playing, unload it. Internall use only.
     */
    void unload(); //TODO: private. call in stop() if not load() by user? or always unload() in stop()?
    qint64 normalizedPosition(qint64 pos);
    class Private;
    QScopedPointer<Private> d;
};
} //namespace QtAV
Q_DECLARE_METATYPE(QtAV::AVPlayer::State)
#endif // QTAV_AVPLAYER_H
