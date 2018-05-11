namespace QtAV
{

class AVPlayer : public QObject
{
%TypeHeaderCode
#include <QtAV/AVPlayer.h>
%End

public:
    enum State {
        StoppedState,
        PlayingState,
        PausedState
    };

    static const QStringList& supportedProtocols();

    explicit AVPlayer(QObject *parent /TransferThis/ = 0);
    ~AVPlayer();

//    AVClock* masterClock();
    void setFile(const QString& path);
    QString file() const;
    void setIODevice(QIODevice* device);
//    void setInput(MediaIO* in);
//    MediaIO* input() const;

    bool isLoaded() const;
    void setAsyncLoad(bool value = true);
    bool isAsyncLoad() const;
    void setAutoLoad(bool value = true); // NOT implemented
    bool isAutoLoad() const; // NOT implemented

    MediaStatus mediaStatus() const;
    bool relativeTimeMode() const;

    qint64 absoluteMediaStartPosition() const;
    qreal durationF() const;
    qint64 duration() const;
    qint64 mediaStartPosition() const;
    qint64 mediaStopPosition() const;
    qreal mediaStartPositionF() const;
    qint64 startPosition() const;
    qint64 stopPosition() const;
    qint64 position() const;
    int repeat() const;
    int currentRepeat() const;
    bool setExternalAudio(const QString& file);
    QString externalAudio() const;
    const QVariantList& externalAudioTracks() const;
    const QVariantList& internalAudioTracks() const;
    const QVariantList& internalVideoTracks() const;
    bool setAudioStream(const QString& file, int n = 0);
    bool setAudioStream(int n);
    bool setVideoStream(int n);
    const QVariantList& internalSubtitleTracks() const;
    bool setSubtitleStream(int n);
    int currentAudioStream() const;
    int currentVideoStream() const;
    int currentSubtitleStream() const;
    int audioStreamCount() const;
    int videoStreamCount() const;
    int subtitleStreamCount() const;
//    VideoCapture *videoCapture() const;
    void play(const QString& path);
    bool isPlaying() const;
    bool isPaused() const;
    State state() const;
    void setState(State value);

    void addVideoRenderer(VideoRenderer *renderer);
    void removeVideoRenderer(VideoRenderer *renderer);
    void clearVideoRenderers();
    void setRenderer(VideoRenderer* renderer);
    VideoRenderer* renderer();
    QList<QtAV::VideoRenderer*> videoOutputs();
//    AudioOutput* audio();
    void setSpeed(qreal speed);
    qreal speed() const;

    void setInterruptTimeout(qint64 ms);
    qint64 interruptTimeout() const;
    void setInterruptOnTimeout(bool value);
    bool isInterruptOnTimeout() const;
    void setFrameRate(qreal value);
    qreal forcedFrameRate() const;
//    const Statistics& statistics() const;
//    bool installFilter(AudioFilter* filter, int index = 0x7FFFFFFF);
//    bool installFilter(VideoFilter* filter, int index = 0x7FFFFFFF);
//    bool uninstallFilter(AudioFilter* filter);
//    bool uninstallFilter(VideoFilter* filter);
//    QList<Filter*> audioFilters() const;
//    QList<Filter*> videoFilters() const;
//    void setPriority(const QVector<QtAV::VideoDecoderId>& ids);
    void setVideoDecoderPriority(const QStringList& names);
    QStringList videoDecoderPriority() const;
    //void setPriority(const QVector<AudioOutputId>& ids);
    int brightness() const;
    int contrast() const;
    int hue() const; //not implemented
    int saturation() const;
    unsigned int chapters() const;
    void setOptionsForFormat(const QVariantHash &dict);
    QVariantHash optionsForFormat() const;
    void setOptionsForAudioCodec(const QVariantHash &dict);
    QVariantHash optionsForAudioCodec() const;
    void setOptionsForVideoCodec(const QVariantHash& dict);
    QVariantHash optionsForVideoCodec() const;

//    MediaEndAction mediaEndAction() const;
//    void setMediaEndAction(MediaEndAction value);



public Q_SLOTS:
    bool load();

    void togglePause();
    void pause(bool p = true);
    void play();
    void stop();
    void stepForward();
    void stepBackward();

    void setRelativeTimeMode(bool value);
    void setRepeat(int max);
    void setStartPosition(qint64 pos);
//    void setStopPosition(qint64 pos = std::numeric_limits<qint64>::max());
//    void setTimeRange(qint64 start, qint64 stop = std::numeric_limits<qint64>::max());

    bool isSeekable() const;
    void setPosition(qint64 position);
    void seek(qreal r); // r: [0, 1]
    void seek(qint64 pos); //ms. same as setPosition(pos)
    void seekForward();
    void seekBackward();
    void seekNextChapter();
    void seekPreviousChapter();
//    void setSeekType(SeekType type);
//    SeekType seekType() const;

    qreal bufferProgress() const;
    qreal bufferSpeed() const;
    qint64 buffered() const;
//    void setBufferMode(BufferMode mode);
//    BufferMode bufferMode() const;
    void setBufferValue(qint64 value);
    int bufferValue() const;

    void setNotifyInterval(int msec);
    int notifyInterval() const;
    void updateClock(qint64 msecs); //update AVClock's external clock
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
    void mediaStatusChanged(QtAV::MediaStatus status);
//    void mediaEndActionChanged(QtAV::MediaEndAction action);
    void durationChanged(qint64);
//    void error(const QtAV::AVError& e); //explictly use QtAV::AVError in connection for Qt4 syntax
    void paused(bool p);
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
    void seekFinished(qint64 position);
    void positionChanged(qint64 position);
    void interruptTimeoutChanged();
    void interruptOnTimeoutChanged();
    void notifyIntervalChanged();
    void brightnessChanged(int val);
    void contrastChanged(int val);
    void hueChanged(int val);
    void saturationChanged(int val);
    void chaptersChanged(unsigned int val);
    void subtitleStreamChanged(int value);
    void internalAudioTracksChanged(const QVariantList& tracks);
    void internalVideoTracksChanged(const QVariantList& tracks);
    void externalAudioTracksChanged(const QVariantList& tracks);
    void internalSubtitleTracksChanged(const QVariantList& tracks);
    void internalSubtitleHeaderRead(const QByteArray& codec, const QByteArray& data);
//    void internalSubtitlePacketRead(int track, const QtAV::Packet& packet);

protected:
    virtual void timerEvent(QTimerEvent *);
};

};
