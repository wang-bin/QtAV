/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2015 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QAV_AUDIOOUTPUTBACKEND_H
#define QAV_AUDIOOUTPUTBACKEND_H

#include <QtCore/QObject>
#include <QtAV/AudioFormat.h>
#include <QtAV/AudioOutput.h>
#include <QtAV/FactoryDefine.h>

namespace QtAV {

class AudioOutputBackend : public QObject
{
    Q_OBJECT
public:
    AudioOutput *audio;
    int buffer_size;
    int buffer_count;
    AudioFormat format;
    /*!
     * \brief AudioOutputBackend
     * Specify supported features by the backend. Use this for new backends.
     */
    AudioOutputBackend(AudioOutput::DeviceFeatures f, QObject *parent);
    virtual ~AudioOutputBackend() {}
    virtual QString name() const = 0;
    virtual bool open() = 0;
    virtual bool close() = 0;
    virtual bool write(const QByteArray& data) = 0; //MUST
    virtual bool play() = 0; //MUST
    virtual bool isSupported(const AudioFormat& format) const { return isSupported(format.sampleFormat()) && isSupported(format.channelLayout());}
    virtual bool isSupported(AudioFormat::SampleFormat) const { return true;}
    virtual bool isSupported(AudioFormat::ChannelLayout) const { return true;}
    /*!
     * \brief preferredSampleFormat
     * \return the preferred sample format. default is signed16 packed
     *  If the specified format is not supported, resample to preffered format
     */
    virtual AudioFormat::SampleFormat preferredSampleFormat() const { return AudioFormat::SampleFormat_Signed16;}
    /*!
     * \brief preferredChannelLayout
     * \return the preferred channel layout. default is stero
     */
    virtual AudioFormat::ChannelLayout preferredChannelLayout() const { return AudioFormat::ChannelLayout_Stero;}
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
        WritableBytes = 1 << 6,
    };
    virtual BufferControl bufferControl() const = 0;
    // called by callback with Callback control
    virtual void onCallback();
    //default return -1. means not the control
    virtual int getPlayedCount() {return -1;} //PlayedCount
    /*!
     * \brief getPlayedBytes
     * reimplement this if bufferControl() is PlayedBytes.
     * \return the bytes played since last dequeue the buffer queue
     */
    virtual int getPlayedBytes() {return -1;}   // PlayedBytes
    virtual int getOffset() {return -1;}        // OffsetIndex
    virtual int getOffsetByBytes()  {return -1;}// OffsetBytes
    virtual int getWritableBytes() {return -1;} //WritableBytes
    // not virtual. called in ctor
    AudioOutput::DeviceFeatures supportedFeatures() { return m_features;}
    /*!
     * \brief setVolume
     * Set volume by backend api. If backend can not set the given volume, or SetVolume feature is not set, software implemention will be used.
     * \param value >=0
     * \return true if success
     */
    virtual bool setVolume(qreal value) { Q_UNUSED(value); return false;}
    virtual qreal getVolume() const { return 1.0;}
    virtual bool setMute(bool value = true) { Q_UNUSED(value); return false;}
    virtual bool getMute() const { return false;}

Q_SIGNALS:
    /*
     * \brief reportVolume
     * Volume can be changed by per-app volume control from system outside this library. Useful for synchronizing ui to system.
     * Volume control from QtAV may invoke it too. And it may be invoked even if volume is not changed.
     * If volume changed, signal volumeChanged() will be emitted and volume() will be updated.
     * Only supported by some backends, e.g. pulseaudio
     */
    void volumeReported(qreal value);
    void muteReported(bool value);
private:
    AudioOutput::DeviceFeatures m_features;
    Q_DISABLE_COPY(AudioOutputBackend)
};

typedef int AudioOutputBackendId;
FACTORY_DECLARE(AudioOutputBackend)
} //namespace QtAV
#endif //QAV_AUDIOOUTPUTBACKEND_H
