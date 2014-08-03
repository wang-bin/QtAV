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

#ifndef QAV_AUDIOOUTPUT_H
#define QAV_AUDIOOUTPUT_H

#include <QtCore/QObject>
#include <QtAV/AVOutput.h>
#include <QtAV/FactoryDefine.h>
#include <QtAV/AudioFrame.h>
//TODO: audio device class. isAsync()
//bool setXXX(); false if not supported

/*!
 * How to work with different audio APIs?
 * AudioOutputPrivate::frame_infos stores the timestamps and buffer sizes for pending audio buffers.
 * 1. Blocking API: for example, portaudio(also supports async api).
 *    In waitForNextBuffer(), usually AudioOutputPrivate::bufferRemoved() is enough.
 * 2. Async API: for example, OpenAL and OpenSL.
 *    In waitForNextBuffer() you must wait until you can put a new audio buffer to the queue.
 *    a) callback (OpenSL, SDL): return if AudioutputPrivate::canAddBuffer() is true;
 *    b) state querying api (OpenAL): always query the number of processed buffers even if AudioutputPrivate::canAddBuffer() is true
 *       because we don't know when a buffer is processed. so must query the state every time.
 */

/*!
 * while (has_data) {
 *     data = read_data(ao->bufferSize());
 *     ao->waitForNextBuffer();
 *     ao->receiveData(data, pts);
 *     ao->play();
 * }
 *
 */
namespace QtAV {

typedef int AudioOutputId;
class AudioOutput;
FACTORY_DECLARE(AudioOutput)

class AudioFormat;
class AudioOutputPrivate;
class Q_AV_EXPORT AudioOutput : public QObject, public AVOutput
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(AudioOutput)
    Q_ENUMS(BufferControl)
    Q_FLAGS(BufferControls)
    Q_ENUMS(Feature)
    Q_FLAGS(Features)
    Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool mute READ isMute WRITE setMute NOTIFY muteChanged)
    Q_PROPERTY(Feature features READ features WRITE setFeatures NOTIFY featuresChanged)
public:
    /*!
     * \brief The BufferControl enum
     * Used to adapt to different audio playback api. Usually you don't need this in application level development.
    */
    enum BufferControl {
        User = 0,
        Blocking = 1,
        Callback = 1 << 1,
        PlayedCount = 1 << 2, //number of buffers played since last buffer dequeued
        PlayedBytes = 1 << 3,
        OffsetIndex = 1 << 4, //current playing offset
        OffsetBytes = 1 << 5, //current playing offset by bytes
    };
    Q_DECLARE_FLAGS(BufferControls, BufferControl)
    /*!
     * \brief The Feature enum
     * features (set when playing) supported by the audio playback api.
     */
    enum Feature {
        SetVolume = 1,
        SetMuted = 1 << 1,
        SetSampleRate = 1 << 2,
    };
    Q_DECLARE_FLAGS(Features, Feature)
    /*!
     * \brief AudioOutput
     * Audio format set to preferred sample format and channel layout
     */
    AudioOutput();
    virtual ~AudioOutput() = 0;
    // store and fill data to audio buffers
    bool receiveData(const QByteArray &data, qreal pts);

    int maxChannels() const;
    /*!
     * \brief setAudioFormat
     * Remain the old value if not supported
     * \param format
     * TODO: return bool
     */
    void setAudioFormat(const AudioFormat& format);
    AudioFormat& audioFormat();
    const AudioFormat& audioFormat() const;

    void setSampleRate(int rate); //deprecated
    int sampleRate() const; //deprecated
    void setChannels(int channels); //deprecated
    int channels() const; //deprecated
    /*!
     * \brief setVolume
     * If SetVolume feature is not set or not supported, only store the value and you should process the audio data outside to the given volume value.
     * Otherwise, call this also set the volume by the audio playback api //in slot?
     * \param volume
     */
    void setVolume(qreal volume);
    qreal volume() const;
    void setMute(bool value);
    bool isMute() const;
    /*!
     * \brief setSpeed  set audio playing speed
     *
     * The speed affects the playing only if audio is available and clock type is
     * audio clock. For example, play a video contains audio without special configurations.
     * To change the playing speed in other cases, use AVPlayer::setSpeed(qreal)
     * \param speed
     */
    void setSpeed(qreal speed);
    qreal speed() const;

    /*!
     * \brief isSupported
     *  check \a isSupported(format.sampleFormat()) and \a isSupported(format.channelLayout())
     * \param format
     * \return true if \a format is supported. default is true
     */
    virtual bool isSupported(const AudioFormat& format) const;
    virtual bool isSupported(AudioFormat::SampleFormat sampleFormat) const;
    virtual bool isSupported(AudioFormat::ChannelLayout channelLayout) const;
    /*!
     * \brief preferredSampleFormat
     * \return the preferred sample format. default is float32 packed
     *  If the specified format is not supported, resample to preffered format
     */
    virtual AudioFormat::SampleFormat preferredSampleFormat() const;
    /*!
     * \brief preferredChannelLayout
     * \return the preferred channel layout. default is stero
     */
    virtual AudioFormat::ChannelLayout preferredChannelLayout() const;

    // thrunk size in buffer queue
    int bufferSize() const;
    void setBufferSize(int value);
    int bufferCount() const;
    void setBufferCount(int value);
    int bufferSizeTotal() const { return bufferCount() * bufferSize();}

    void setBufferControl(BufferControl value);
    BufferControl bufferControl() const;
    /*!
     * \brief supportedBufferControl
     * \return default is User
     */
    virtual BufferControl supportedBufferControl() const;
    /*!
     * \brief setFeatures
     * do nothing if onSetFeatures() returns false, which means current api does not support the features
     */
    void setFeatures(Feature value);
    Feature features() const;
    void setFeature(Feature value, bool on = true);
    bool hasFeatures(Feature value) const;
    /*!
     * \brief waitForNextBuffer
     */
    virtual void waitForNextBuffer();
    qreal timestamp() const;

    virtual bool play() = 0; //MUST
signals:
    void volumeChanged(qreal);
    void muteChanged(bool);
    void featuresChanged();
protected:
    virtual bool write(const QByteArray& data) = 0; //MUST
    // called by callback with Callback control
    void onCallback();
    //default return -1. means not the control
    virtual int getPlayedCount();
    /*!
     * \brief getPlayedBytes
     * reimplement this if bufferControl() is PlayedBytes.
     * \return the bytes played since last dequeue the buffer queue
     */
    virtual int getPlayedBytes();
    virtual int getOffset();
    virtual int getOffsetByBytes();
    // \return false by default
    virtual bool onSetFeatures(Feature value);
    // reset internal status. MUST call this at the begining of open()
    void resetStatus();

    AudioOutput(AudioOutputPrivate& d);
};

} //namespace QtAV
#endif // QAV_AUDIOOUTPUT_H
