/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2017 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2016-02-11)

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
// FIXME: pause=>resume error
#include "QtAV/private/AudioOutputBackend.h"
#include <QtCore/QQueue>
#include <QtCore/QSemaphore>
#include <QtCore/QThread>
#include <QtCore/QMutex> //qt4
#include <QtCore/QWaitCondition>
#include <AudioToolbox/AudioToolbox.h>
#include "QtAV/private/mkid.h"
#include "QtAV/private/factory.h"
#include "utils/Logger.h"

namespace QtAV {
static const char kName[] = "AudioToolbox";
class AudioOutputAudioToolbox Q_DECL_FINAL: public AudioOutputBackend
{
public:
    AudioOutputAudioToolbox(QObject *parent = 0);
    QString name() const Q_DECL_OVERRIDE { return QLatin1String(kName);}
    bool isSupported(AudioFormat::SampleFormat smpfmt) const Q_DECL_OVERRIDE;
    bool open() Q_DECL_OVERRIDE;
    bool close() Q_DECL_OVERRIDE;
    //bool flush() Q_DECL_OVERRIDE;
    BufferControl bufferControl() const Q_DECL_OVERRIDE;
    void onCallback() Q_DECL_OVERRIDE;
    bool write(const QByteArray& data) Q_DECL_OVERRIDE;
    bool play() Q_DECL_OVERRIDE;
    bool setVolume(qreal value) override;
private:
    static void outCallback(void* inUserData, AudioQueueRef inAQ, AudioQueueBufferRef inBuffer);
    void tryPauseTimeline();

    QVector<AudioQueueBufferRef> m_buffer;
    QVector<AudioQueueBufferRef> m_buffer_fill;
    AudioQueueRef m_queue;
    AudioStreamBasicDescription m_desc;

    bool m_waiting;
    QMutex m_mutex;
    QWaitCondition m_cond;
    QSemaphore sem;
};

typedef AudioOutputAudioToolbox AudioOutputBackendAudioToolbox;
static const AudioOutputBackendId AudioOutputBackendId_AudioToolbox = mkid::id32base36_2<'A', 'T'>::value;
FACTORY_REGISTER(AudioOutputBackend, AudioToolbox, kName)

#define AT_ENSURE(FUNC, ...) AQ_RUN_CHECK(FUNC, return __VA_ARGS__)
#define AT_WARN(FUNC, ...) AQ_RUN_CHECK(FUNC)
#define AQ_RUN_CHECK(FUNC, ...) \
    do { \
        OSStatus ret = FUNC; \
        if (ret != noErr) { \
            qWarning("AudioBackendAudioQueue Error>>> " #FUNC ": %#x", ret); \
            __VA_ARGS__; \
        } \
    } while(0)

static AudioStreamBasicDescription audioFormatToAT(const AudioFormat &format)
{
    // AudioQueue only supports interleave
    AudioStreamBasicDescription desc;
    desc.mSampleRate = format.sampleRate();
    desc.mFormatID = kAudioFormatLinearPCM;
    desc.mFormatFlags = kAudioFormatFlagIsPacked; // TODO: kAudioFormatFlagIsPacked?
    if (format.isFloat())
        desc.mFormatFlags |= kAudioFormatFlagIsFloat;
    else if (!format.isUnsigned())
        desc.mFormatFlags |= kAudioFormatFlagIsSignedInteger;
    desc.mFramesPerPacket = 1;// FIXME:??
    desc.mChannelsPerFrame = format.channels();
    desc.mBitsPerChannel = format.bytesPerSample()*8;
    desc.mBytesPerFrame = format.bytesPerFrame();
    desc.mBytesPerPacket = desc.mBytesPerFrame * desc.mFramesPerPacket;

    return desc;
}

void AudioOutputAudioToolbox::outCallback(void* inUserData, AudioQueueRef inAQ, AudioQueueBufferRef inBuffer)
{
    Q_UNUSED(inAQ);
    AudioOutputAudioToolbox *ao = reinterpret_cast<AudioOutputAudioToolbox*>(inUserData);
    if (ao->bufferControl() & AudioOutputBackend::CountCallback) {
        ao->onCallback();
    }
    QMutexLocker locker(&ao->m_mutex);
    Q_UNUSED(locker);
    ao->m_buffer_fill.push_back(inBuffer);

    if (ao->m_waiting) {
        ao->m_waiting = false;
        qDebug("wake up to fill buffer");
        ao->m_cond.wakeOne();
    }
    //qDebug("callback. sem: %d, fill queue: %d", ao->sem.available(), ao->m_buffer_fill.size());
    ao->tryPauseTimeline();
}

AudioOutputAudioToolbox::AudioOutputAudioToolbox(QObject *parent)
    : AudioOutputBackend(AudioOutput::DeviceFeatures()
                         |AudioOutput::SetVolume
                         , parent)
    , m_queue(NULL)
    , m_waiting(false)
{
    available = false;
    available = true;
}

AudioOutputBackend::BufferControl AudioOutputAudioToolbox::bufferControl() const
{
    return CountCallback;//BufferControl(Callback | PlayedCount);
}

void AudioOutputAudioToolbox::onCallback()
{
    if (bufferControl() & CountCallback)
        sem.release();
}

void AudioOutputAudioToolbox::tryPauseTimeline()
{
    /// All buffers are rendered but the AudioQueue timeline continues. If the next buffer sample time is earlier than AudioQueue timeline value, for example resume after pause, the buffer will not be rendered
    if (sem.available() == buffer_count) {
        AudioTimeStamp t;
        AudioQueueGetCurrentTime(m_queue, NULL, &t, NULL);
        qDebug("pause audio queue timeline @%.3f (sample time)/%lld (host time)", t.mSampleTime, t.mHostTime);
        AT_ENSURE(AudioQueuePause(m_queue));
    }
}

bool AudioOutputAudioToolbox::isSupported(AudioFormat::SampleFormat smpfmt) const
{
    return !IsPlanar(smpfmt);
}

bool AudioOutputAudioToolbox::open()
{
    m_buffer.resize(buffer_count);
    m_desc = audioFormatToAT(format);
    AT_ENSURE(AudioQueueNewOutput(&m_desc, AudioOutputAudioToolbox::outCallback, this, NULL, kCFRunLoopCommonModes/*NULL*/, 0, &m_queue), false);
    for (int i = 0; i < m_buffer.size(); ++i) {
        AT_ENSURE(AudioQueueAllocateBuffer(m_queue, buffer_size, &m_buffer[i]), false);
    }
    m_buffer_fill = m_buffer;

    sem.release(buffer_count - sem.available());
    m_waiting = false;
    return true;
}

bool AudioOutputAudioToolbox::close()
{
    if (!m_queue) {
        qDebug("AudioQueue is not created. skip close");
        return true;
    }
    UInt32 running = 0, s = 0;
    AT_ENSURE(AudioQueueGetProperty(m_queue, kAudioQueueProperty_IsRunning, &running, &s), false);
    if (running)
        AT_ENSURE(AudioQueueStop(m_queue, true), false);
    AT_ENSURE(AudioQueueDispose(m_queue, true), false); // dispose all resouces including buffers, so we can remove AudioQueueFreeBuffer
    m_queue = NULL;
    m_buffer.clear();
    return true;
}

bool AudioOutputAudioToolbox::write(const QByteArray& data)
{
    // blocking queue.
    // if queue not full, fill buffer and enqueue buffer
    //qDebug("write. sem: %d", sem.available());
    if (bufferControl() & CountCallback)
        sem.acquire();

    AudioQueueBufferRef buf = NULL;
    {
        QMutexLocker locker(&m_mutex);
        Q_UNUSED(locker);
        // put to data queue, if has buffer to fill (was available in callback), fill the front data
        if (m_buffer_fill.isEmpty()) {
            qDebug("buffer queue to fill is empty, wait a valid buffer to fill");
            m_waiting = true;
            m_cond.wait(&m_mutex);
        }
        buf = m_buffer_fill.front();
        m_buffer_fill.pop_front();
    }
    assert(buf->mAudioDataBytesCapacity >= (UInt32)data.size() && "too many data to write to audio queue buffer");
    memcpy(buf->mAudioData, data.constData(), data.size());
    buf->mAudioDataByteSize = data.size();
    //buf->mUserData
    AT_ENSURE(AudioQueueEnqueueBuffer(m_queue, buf, 0, NULL), false);
    return true;
}

bool AudioOutputAudioToolbox::play()
{
    OSType err = AudioQueueStart(m_queue, nullptr);
    if (err == '!pla') { //AVAudioSessionErrorCodeCannotStartPlaying
        qWarning("AudioQueueStart error: AVAudioSessionErrorCodeCannotStartPlaying. May play in background");
        close();
        open();
        return false;
    }
    if (err != noErr) {
        qWarning("AudioQueueStart error: %#x", noErr);
        return false;
    }
    return true;
}

bool AudioOutputAudioToolbox::setVolume(qreal value)
{
    // iOS document says the range is [0,1]. But >1.0 works on macOS. So no manually check range here
    AT_ENSURE(AudioQueueSetParameter(m_queue, kAudioQueueParam_Volume, value), false);
    return true;
}
} //namespace QtAV
