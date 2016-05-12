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

#ifndef QAV_AUDIOOUTPUT_H
#define QAV_AUDIOOUTPUT_H

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtAV/AVOutput.h>
#include <QtAV/AudioFrame.h>

/*!
 * AudioOutput ao;
 * ao.setAudioFormat(fmt);
 * ao.open();
 * while (has_data) {
 *     data = read_data(ao->bufferSize());
 *     ao->play(data, pts);
 * }
 * ao->close();
 * See QtAV/tests/ao/main.cpp for detail
 */
namespace QtAV {

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
    Q_PROPERTY(QStringList backends READ backends WRITE setBackends NOTIFY backendsChanged)
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
     * \brief backendsAvailable
     * All registered backends in default priority order
     * \return
     */
    static QStringList backendsAvailable();
    /*!
     * \brief AudioOutput
     * Audio format set to preferred sample format and channel layout
     */
    AudioOutput(QObject *parent = 0);
    virtual ~AudioOutput();
    /*!
     * \brief setBackends
     * set the given backends. Old backend instance and backend() is updated soon if backendsChanged.
     * It is called internally with a default backend names when AudioOutput is created.
     */
    void setBackends(const QStringList &backendNames = QStringList());
    QStringList backends() const;
    /*!
     * \brief backend
     * backend name currently in use
     */
    QString backend() const;
    /*!
     * \brief flush
     * Play the buffered audio data
     * \return
     */
    void flush();
    /*!
     * \brief clear
     * Clear audio buffers and set time to 0. The default behavior is flush and reset time
     */
    void clear();
    bool open();
    bool close();
    bool isOpen() const;
    /*!
     * \brief play
     * Play out the given audio data. It may block current thread until the data can be written to audio device
     * for async playback backend, or until the data is completely played for blocking playback backend.
     * \param data Audio data to play
     * \param pts Timestamp for this data. Useful if need A/V sync. Ignore it if only play audio
     * \return true if play successfully
     */
    bool play(const QByteArray& data, qreal pts = 0.0);
    /// TODO: requestAudioFormat(): check support after open, use the nearest format if not supported. Or use suitableFormat(AudioFormat requestedFmt) if requestedFmt is not supported.
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
     * Call this after open(), because it will call backend api if SetVolume feature is enabled
     * \param volume linear. 1.0: original volume.
     */
    void setVolume(qreal value);
    qreal volume() const;
    /*!
     * \brief setMute
     * If SetMute feature is not set or not supported, software implemention will be used.
     * Call this after open(), because it will call backend api if SetMute feature is enabled
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
    bool isSupported(const AudioFormat& format) const;
    bool isSupported(AudioFormat::SampleFormat sampleFormat) const;
    bool isSupported(AudioFormat::ChannelLayout channelLayout) const;
    /*!
     * \brief preferredSampleFormat
     * \return the preferred sample format. default is signed16 packed
     *  If the specified format is not supported, resample to preffered format
     */
    AudioFormat::SampleFormat preferredSampleFormat() const;
    /*!
     * \brief preferredChannelLayout
     * \return the preferred channel layout. default is stereo
     */
    AudioFormat::ChannelLayout preferredChannelLayout() const;
    /*!
     * \brief bufferSamples
     * Number of samples that audio output accept in 1 buffer. Feed the audio output this size of data every time.
     * Smaller buffer samples gives more buffers for a given data to avoid stutter. But if it's too small, the duration of 1 buffer will be too small to play, for example 1ms. Currently the default value is 512.
     * Some backends(OpenAL) are affected significantly by this property
     */
    int bufferSamples() const;
    void setBufferSamples(int value);
    int bufferSize() const; /// bufferSamples()*bytesPerSample
    /*!
     * \brief bufferCount
     * Total buffer count. If it's not large enough, playing high sample rate audio may be poor.
     * The default value is 16. TODO: depending on audio format(sample rate?)
     * Some backends(OpenAL) are affected significantly by this property
     */
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
Q_SIGNALS:
    void volumeChanged(qreal);
    void muteChanged(bool);
    void deviceFeaturesChanged();
    void backendsChanged();
protected:
    // Store and fill data to audio buffers
    bool receiveData(const QByteArray &data, qreal pts = 0.0);
    /*!
     * \brief waitForNextBuffer
     * wait until you can feed more data
     */
    virtual bool waitForNextBuffer();
private Q_SLOTS:
    void reportVolume(qreal value);
    void reportMute(bool value);
private:
    void onCallback();
    friend class AudioOutputBackend;
    Q_DISABLE_COPY(AudioOutput)
};

} //namespace QtAV
#endif // QAV_AUDIOOUTPUT_H
