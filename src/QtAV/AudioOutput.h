/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2015 Wang Bin <wbsecg1@gmail.com>

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

/*!
 * AudioOutput *ao = AudioOutputFactory::create(AudioOutputId_OpenAL);
 * ao->setAudioFormat(fmt);
 * ao->open();
 * while (has_data) {
 *     data = read_data(ao->bufferSize());
 *     ao->play(data, pts);
 * }
 * ao->close();
 * See QtAV/tests/ao/main.cpp for detail
 *
 * Add A New Backend:
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
    Q_ENUMS(DeviceFeature)
    Q_FLAGS(DeviceFeatures)
    Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool mute READ isMute WRITE setMute NOTIFY muteChanged)
    Q_PROPERTY(DeviceFeatures deviceFeatures READ deviceFeatures WRITE setDeviceFeatures NOTIFY deviceFeaturesChanged)
public:
    /*!
     * \brief DeviceFeature Feature enum
     * features supported by the audio playback api (we call device or backend here)
     */
    enum DeviceFeature {
        NoFeature = 0,
        SetVolume = 1, /// NOT IMPLEMENTED. Use backend volume control api rather than software scale. Ignore if backend does not support.
        SetMute = 1 << 1, /// NOT IMPLEMENTED
        SetSampleRate = 1 << 2, /// NOT IMPLEMENTED
    };
    Q_DECLARE_FLAGS(DeviceFeatures, DeviceFeature)
    /*!
     * \brief AudioOutput
     * Audio format set to preferred sample format and channel layout
     */
    AudioOutput(QObject *parent = 0);
    virtual ~AudioOutput();
    virtual bool open() = 0;
    virtual bool close() = 0;
    /*!
     * \brief play
     * Play out the given audio data. It may block current thread until the data can be written to audio device
     * for async playback backend, or until the data is completely played for blocking playback backend.
     * \param data Audio data to play
     * \param pts Timestamp for this data. Useful if need A/V sync. Ignore it if only play audio
     * \return true if play successfully
     */
    bool play(const QByteArray& data, qreal pts = 0.0);
    /*!
     * \brief setAudioFormat
     * Remain the old value if not supported
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
     * Set volume level.
     * If SetVolume feature is not set or not supported, software implemention will be used.
     * \param volume linear. 1.0: original volume.
     */
    void setVolume(qreal volume);
    qreal volume() const;
    /*!
     * \brief setMute
     * If SetMute feature is not set or not supported, software implemention will be used.
     */
    void setMute(bool value = true);
    bool isMute() const;
    /*!
     * \brief setSpeed  set audio playing speed
     * Currently only store the value and does nothing else in audio output. You may change sample rate to get the same effect.
     * The speed affects the playing only if audio is available and clock type is
     * audio clock. For example, play a video contains audio without special configurations.
     * To change the playing speed in other cases, use AVPlayer::setSpeed(qreal)
     * \param speed linear. > 0
     * TODO: resample internally
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
     * \return the preferred sample format. default is signed16 packed
     *  If the specified format is not supported, resample to preffered format
     */
    virtual AudioFormat::SampleFormat preferredSampleFormat() const;
    /*!
     * \brief preferredChannelLayout
     * \return the preferred channel layout. default is stero
     */
    virtual AudioFormat::ChannelLayout preferredChannelLayout() const;
    /*!
     * \brief bufferSize
     * chunk size that audio output accept. feed the audio output this size of data every time
     */
    int bufferSize() const;
    void setBufferSize(int value);
    // for internal use
    int bufferCount() const;
    void setBufferCount(int value);
    int bufferSizeTotal() const { return bufferCount() * bufferSize();}
    /*!
     * \brief setDeviceFeatures
     * Unsupported features will not be set.
     * You can call this in a backend ctor.
     */
    void setDeviceFeatures(DeviceFeatures value);
    /*!
     * \brief deviceFeatures
     * \return features set by setFeatures() excluding unsupported features
     */
    DeviceFeatures deviceFeatures() const;
    /*!
     * \brief supportedDeviceFeatures
     * Supported features of the backend, defined by AudioOutput(DeviceFeatures,AudioOutput&,QObject*) in a backend ctor
     */
    DeviceFeatures supportedDeviceFeatures() const;
    qreal timestamp() const;
    // timestamp of current playing data
signals:
    void volumeChanged(qreal);
    void muteChanged(bool);
    void deviceFeaturesChanged();
protected:
    // Store and fill data to audio buffers
    bool receiveData(const QByteArray &data, qreal pts = 0.0);
    /*!
     * \brief waitForNextBuffer
     * wait until you can feed more data
     */
    virtual void waitForNextBuffer();
    virtual bool write(const QByteArray& data) = 0; //MUST
    virtual bool play() = 0; //MUST
    /*!
     * \brief The BufferControl enum
     * Used to adapt to different audio playback backend. Usually you don't need this in application level development.
    */
    enum BufferControl {
        User = 0,    // You have to reimplement waitForNextBuffer()
        Blocking = 1,
        Callback = 1 << 1,
        PlayedCount = 1 << 2, //number of buffers played since last buffer dequeued
        PlayedBytes = 1 << 3,
        OffsetIndex = 1 << 4, //current playing offset
        OffsetBytes = 1 << 5, //current playing offset by bytes
    };
    virtual BufferControl bufferControl() const = 0;
    // called by callback with Callback control
    void onCallback(); // virtual?
    //default return -1. means not the control
    virtual int getPlayedCount(); //PlayedCount
    /*!
     * \brief getPlayedBytes
     * reimplement this if bufferControl() is PlayedBytes.
     * \return the bytes played since last dequeue the buffer queue
     */
    virtual int getPlayedBytes(); // PlayedBytes
    virtual int getOffset();      // OffsetIndex
    virtual int getOffsetByBytes(); // OffsetBytes
    /*!
     * \brief deviceSetVolume
     * Set volume by backend api. If backend can not set the given volume, or SetVolume feature is not set, software implemention will be used.
     * Make sure onSetFeatures(SetVolume) returns true.
     * \param value >=0
     * \return true if success
     */
    virtual bool deviceSetVolume(qreal value);
    virtual qreal deviceGetVolume() const;
    virtual bool deviceSetMute(bool value = true);
    // reset internal status. MUST call this at the begining of open()
    void resetStatus();

    /*!
     * \brief AudioOutput
     * Specify supported features for the backend. Use this for new backends.
     */
    AudioOutput(DeviceFeatures featuresSupported, AudioOutputPrivate& d, QObject *parent = 0);
};

} //namespace QtAV
#endif // QAV_AUDIOOUTPUT_H
