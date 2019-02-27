/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2014)

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

#include "QtAV/private/AudioOutputBackend.h"
#include <QtCore/QSemaphore>
#include <QtCore/QThread>
#include <SLES/OpenSLES.h>
#ifdef Q_OS_ANDROID
#include <SLES/OpenSLES_Android.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>
#include <sys/system_properties.h>
#include <cmath>
#endif
#include "QtAV/private/mkid.h"
#include "QtAV/private/factory.h"
#include "utils/Logger.h"

// TODO: native sample rate, so AUDIO_OUTPUT_FLAG_FAST is enabled
namespace QtAV {

// OpenSL 1.1, or __ANDROID_API__ >= 21
#ifndef SL_DATAFORMAT_PCM_EX
#define SL_PCM_REPRESENTATION_SIGNED_INT    ((SLuint32) 0x00000001)
#define SL_PCM_REPRESENTATION_UNSIGNED_INT  ((SLuint32) 0x00000002)
#define SL_PCM_REPRESENTATION_FLOAT         ((SLuint32) 0x00000003)
#define SL_DATAFORMAT_PCM_EX                ((SLuint32) 0x00000004)
struct SLDataFormat_PCM_EX : SLDataFormat_PCM {
    SLuint32 representation;
};
#endif

static const char kName[] = "OpenSL";
class AudioOutputOpenSL Q_DECL_FINAL: public AudioOutputBackend
{
public:
    AudioOutputOpenSL(QObject *parent = 0);
    ~AudioOutputOpenSL();

    int engineVersion() const { return m_sl_major*100+m_sl_minor*10+m_sl_step;}
    QString name() const Q_DECL_OVERRIDE { return QLatin1String(kName);}
    bool isSupported(const AudioFormat& format) const Q_DECL_OVERRIDE;
    bool isSupported(AudioFormat::SampleFormat sampleFormat) const Q_DECL_OVERRIDE;
    bool isSupported(AudioFormat::ChannelLayout channelLayout) const Q_DECL_OVERRIDE;
    bool open() Q_DECL_OVERRIDE;
    bool close() Q_DECL_OVERRIDE;
    BufferControl bufferControl() const Q_DECL_OVERRIDE;
    void onCallback() Q_DECL_OVERRIDE;
    void acquireNextBuffer() Q_DECL_OVERRIDE;
    bool write(const QByteArray& data) Q_DECL_OVERRIDE;
    bool play() Q_DECL_OVERRIDE;
    //default return -1. means not the control
    int getPlayedCount() Q_DECL_OVERRIDE;
    bool setVolume(qreal value) Q_DECL_OVERRIDE;
    qreal getVolume() const Q_DECL_OVERRIDE;
    bool setMute(bool value = true) Q_DECL_OVERRIDE;

#ifdef Q_OS_ANDROID
    static void bufferQueueCallbackAndroid(SLAndroidSimpleBufferQueueItf bufferQueue, void *context);
#endif
    static void bufferQueueCallback(SLBufferQueueItf bufferQueue, void *context);
    static void playCallback(SLPlayItf player, void *ctx, SLuint32 event);
private:
    SLDataFormat_PCM_EX audioFormatToSL(const AudioFormat &format);

    SLObjectItf engineObject;
    SLEngineItf engine;
    SLEngineCapabilitiesItf m_cap;
    SLObjectItf m_outputMixObject;
    SLObjectItf m_playerObject;
    SLPlayItf m_playItf;
    SLVolumeItf m_volumeItf;
    SLBufferQueueItf m_bufferQueueItf;
#ifdef Q_OS_ANDROID
    SLAndroidSimpleBufferQueueItf m_bufferQueueItf_android; // supports decoding, recording
#endif
    bool m_android;
    int m_android_api_level;
    SLint16 m_sl_major, m_sl_minor, m_sl_step;
    SLint32 m_streamType;
    quint32 buffers_queued;
    QSemaphore sem;

    // Enqueue does not copy data. We MUST keep the data until it is played out
    int queue_data_write;
    QByteArray queue_data;
};

typedef AudioOutputOpenSL AudioOutputBackendOpenSL;
static const AudioOutputBackendId AudioOutputBackendId_OpenSL = mkid::id32base36_6<'O', 'p', 'e', 'n', 'S', 'L'>::value;
FACTORY_REGISTER(AudioOutputBackend, OpenSL, kName)

#define SL_ENSURE(FUNC, ...) \
    do { \
        SLresult ret = FUNC; \
        if (ret != SL_RESULT_SUCCESS) { \
            qWarning("AudioOutputOpenSL Error>>> " #FUNC " (%lu)", ret); \
            return __VA_ARGS__; \
        } \
    } while(0)

SLDataFormat_PCM_EX AudioOutputOpenSL::audioFormatToSL(const AudioFormat &format)
{
    SLDataFormat_PCM_EX format_pcm;
    if (format.isFloat()) {
        if (format.sampleSize() == sizeof(float))
            format_pcm.representation = SL_PCM_REPRESENTATION_FLOAT;
    } else if (format.isUnsigned()) {
        format_pcm.representation = SL_PCM_REPRESENTATION_UNSIGNED_INT;
    } else {
        format_pcm.representation = SL_PCM_REPRESENTATION_SIGNED_INT;
    }
    format_pcm.formatType = m_android_api_level >= 21 || engineVersion() >= 110 ? SL_DATAFORMAT_PCM_EX : SL_DATAFORMAT_PCM;
    format_pcm.numChannels = format.channels();
    format_pcm.samplesPerSec = format.sampleRate() * 1000;
    format_pcm.bitsPerSample = format.bytesPerSample()*8; //raw sample size, e.g. s24 is 24. currently we do not support such formats
    format_pcm.containerSize = format_pcm.bitsPerSample;
    // TODO: more layouts
    format_pcm.channelMask = format.channels() == 1 ? SL_SPEAKER_FRONT_CENTER : SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
#ifdef SL_BYTEORDER_NATIVE
    format_pcm.endianness = SL_BYTEORDER_NATIVE;
#else
    union { unsigned short num; char buf[sizeof(unsigned short)]; } endianness;
    endianness.num = 1;
    format_pcm.endianness = endianness.buf[0] ? SL_BYTEORDER_LITTLEENDIAN : SL_BYTEORDER_BIGENDIAN;
#endif
    return format_pcm;
}

#ifdef Q_OS_ANDROID
void AudioOutputOpenSL::bufferQueueCallbackAndroid(SLAndroidSimpleBufferQueueItf bufferQueue, void *context)
{
#if 0
    SLAndroidSimpleBufferQueueState state;
    (*bufferQueue)->GetState(bufferQueue, &state);
    qDebug(">>>>>>>>>>>>>>bufferQueueCallback state.count=%lu .playIndex=%lu", state.count, state.playIndex);
#endif
    AudioOutputOpenSL *ao = reinterpret_cast<AudioOutputOpenSL*>(context);
    if (ao->bufferControl() & AudioOutputBackend::CountCallback) {
        ao->onCallback();
    }
}
#endif
void AudioOutputOpenSL::bufferQueueCallback(SLBufferQueueItf bufferQueue, void *context)
{
#if 0
    SLBufferQueueState state;
    (*bufferQueue)->GetState(bufferQueue, &state);
    qDebug(">>>>>>>>>>>>>>bufferQueueCallback state.count=%lu .playIndex=%lu", state.count, state.playIndex);
#endif
    AudioOutputOpenSL *ao = reinterpret_cast<AudioOutputOpenSL*>(context);
    if (ao->bufferControl() & AudioOutputBackend::CountCallback) {
        ao->onCallback();
    }
}

void AudioOutputOpenSL::playCallback(SLPlayItf player, void *ctx, SLuint32 event)
{
    Q_UNUSED(player);
    Q_UNUSED(ctx);
    Q_UNUSED(event);
    //qDebug("---------%s  event=%lu", __FUNCTION__, event);
}

AudioOutputOpenSL::AudioOutputOpenSL(QObject *parent)
    : AudioOutputBackend(AudioOutput::DeviceFeatures()
                         |AudioOutput::SetVolume
                         |AudioOutput::SetMute, parent)
    , m_outputMixObject(0)
    , m_playerObject(0)
    , m_playItf(0)
    , m_volumeItf(0)
    , m_bufferQueueItf(0)
    , m_bufferQueueItf_android(0)
    , m_android(false)
    , m_android_api_level(0)
    , m_sl_major(0)
    , m_sl_minor(0)
    , m_sl_step(0)
    , m_streamType(-1)
    , buffers_queued(0)
    , queue_data_write(0)
{
#ifdef Q_OS_ANDROID
    char v[PROP_VALUE_MAX+1];
    __system_property_get("ro.build.version.sdk", v);
    m_android_api_level = atoi(v);
#endif
    available = false;
    SLEngineOption opt[] = {(SLuint32) SL_ENGINEOPTION_THREADSAFE, (SLuint32) SL_BOOLEAN_TRUE};
    SL_ENSURE(slCreateEngine(&engineObject, 1, opt, 0, NULL, NULL)); // SLEngineOption is ignored by android
    SL_ENSURE((*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE));
    SL_ENSURE((*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engine));
    available = true;

    if ((*engineObject)->GetInterface(engineObject, SL_IID_ENGINECAPABILITIES, &m_cap) == SL_RESULT_SUCCESS) { // SL_RESULT_FEATURE_UNSUPPORTED
        if ((*m_cap)->QueryAPIVersion(m_cap, &m_sl_major, &m_sl_minor, &m_sl_step) == SL_RESULT_SUCCESS) {
            printf("OpenSL version: %d.%d.%d\n", m_sl_major, m_sl_minor, m_sl_step);
        }
    }
}

AudioOutputOpenSL::~AudioOutputOpenSL()
{
    if (engineObject)
        (*engineObject)->Destroy(engineObject);
}

bool AudioOutputOpenSL::isSupported(const AudioFormat& format) const
{
    return isSupported(format.sampleFormat()) && isSupported(format.channelLayout());
}

bool AudioOutputOpenSL::isSupported(AudioFormat::SampleFormat sampleFormat) const
{
    if (sampleFormat == AudioFormat::SampleFormat_Unsigned8 || sampleFormat == AudioFormat::SampleFormat_Signed16) // TODO: android api 21 supports s32?
            return true;
    if (m_android_api_level < 21 || (engineVersion() > 0 && engineVersion() < 110))
        return false;
    if (!IsFloat(sampleFormat) || IsPlanar(sampleFormat))
        return false;
    return RawSampleSize(sampleFormat) == sizeof(float); // TODO: test s32 etc
}

bool AudioOutputOpenSL::isSupported(AudioFormat::ChannelLayout channelLayout) const
{
    return channelLayout == AudioFormat::ChannelLayout_Mono || channelLayout == AudioFormat::ChannelLayout_Stereo;
}

AudioOutputBackend::BufferControl AudioOutputOpenSL::bufferControl() const
{
    return CountCallback;//BufferControl(Callback | PlayedCount);
}

void AudioOutputOpenSL::onCallback()
{
    if (bufferControl() & CountCallback)
        sem.release();
}

void AudioOutputOpenSL::acquireNextBuffer()
{
    if (bufferControl() & CountCallback)
        sem.acquire();
}

bool AudioOutputOpenSL::open()
{
    queue_data.resize(buffer_size*buffer_count);
    SLDataLocator_BufferQueue bufferQueueLocator = { SL_DATALOCATOR_BUFFERQUEUE, (SLuint32)buffer_count };
    SLDataFormat_PCM_EX pcmFormat = audioFormatToSL(format);
    SLDataSource audioSrc = { &bufferQueueLocator, &pcmFormat };
#ifdef Q_OS_ANDROID
    SLDataLocator_AndroidSimpleBufferQueue bufferQueueLocator_android = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, (SLuint32)buffer_count };
    if (m_android)
        audioSrc.pLocator = &bufferQueueLocator_android;
#endif
    // OutputMix
    SL_ENSURE((*engine)->CreateOutputMix(engine, &m_outputMixObject, 0, NULL, NULL), false);
    SL_ENSURE((*m_outputMixObject)->Realize(m_outputMixObject, SL_BOOLEAN_FALSE), false);
    SLDataLocator_OutputMix outputMixLocator = { SL_DATALOCATOR_OUTPUTMIX, m_outputMixObject };
    SLDataSink audioSink = { &outputMixLocator, NULL };

    const SLInterfaceID ids[] = { SL_IID_BUFFERQUEUE, SL_IID_VOLUME
#ifdef Q_OS_ANDROID
                                  , SL_IID_ANDROIDCONFIGURATION
#endif
                                };
    const SLboolean req[] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE
#ifdef Q_OS_ANDROID
                              , SL_BOOLEAN_TRUE
#endif
                            };
    // AudioPlayer
    SL_ENSURE((*engine)->CreateAudioPlayer(engine, &m_playerObject, &audioSrc, &audioSink, sizeof(ids)/sizeof(ids[0]), ids, req), false);
#ifdef Q_OS_ANDROID
    if (m_android) {
        m_streamType = SL_ANDROID_STREAM_MEDIA;
        SLAndroidConfigurationItf cfg;
        if ((*m_playerObject)->GetInterface(m_playerObject, SL_IID_ANDROIDCONFIGURATION, &cfg)) {
            (*cfg)->SetConfiguration(cfg, SL_ANDROID_KEY_STREAM_TYPE, &m_streamType, sizeof(SLint32));
        }
    }
#endif
    SL_ENSURE((*m_playerObject)->Realize(m_playerObject, SL_BOOLEAN_FALSE), false);
    // Buffer interface
#ifdef Q_OS_ANDROID
    if (m_android) {
        SL_ENSURE((*m_playerObject)->GetInterface(m_playerObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &m_bufferQueueItf_android), false);
        SL_ENSURE((*m_bufferQueueItf_android)->RegisterCallback(m_bufferQueueItf_android, AudioOutputOpenSL::bufferQueueCallbackAndroid, this), false);
    } else
#endif
    {
        SL_ENSURE((*m_playerObject)->GetInterface(m_playerObject, SL_IID_BUFFERQUEUE, &m_bufferQueueItf), false);
        SL_ENSURE((*m_bufferQueueItf)->RegisterCallback(m_bufferQueueItf, AudioOutputOpenSL::bufferQueueCallback, this), false);
    }
    // Play interface
    SL_ENSURE((*m_playerObject)->GetInterface(m_playerObject, SL_IID_PLAY, &m_playItf), false);
    // call when SL_PLAYSTATE_STOPPED
    SL_ENSURE((*m_playItf)->RegisterCallback(m_playItf, AudioOutputOpenSL::playCallback, this), false);
    SL_ENSURE((*m_playerObject)->GetInterface(m_playerObject, SL_IID_VOLUME, &m_volumeItf), false);
#if 0
    SLuint32 mask = SL_PLAYEVENT_HEADATEND;
    // TODO: what does this do?
    SL_ENSURE((*m_playItf)->SetPositionUpdatePeriod(m_playItf, 100), false);
    SL_ENSURE((*m_playItf)->SetCallbackEventsMask(m_playItf, mask), false);
#endif
    // Volume interface
    //SL_ENSURE((*m_playerObject)->GetInterface(m_playerObject, SL_IID_VOLUME, &m_volumeItf), false);

    /*
     * DO NOT call sem.release(buffer_count - sem.available()) because playInitialData() will enqueue buffers and then sem.release() is called in callback.
     * Otherwise, Enqueue() the 1st(or more) real buffer may report SL_RESULT_BUFFER_INSUFFICIENT and noise will be played.
     * Can not Enqueue() internally here because buffers are managed in AudioOutput to ensure 0-copy
     */
    return true;
}

bool AudioOutputOpenSL::close()
{
    if (m_playItf)
        (*m_playItf)->SetPlayState(m_playItf, SL_PLAYSTATE_STOPPED);

#ifdef Q_OS_ANDROID
    if (m_android) {
        if (m_bufferQueueItf_android && SL_RESULT_SUCCESS != (*m_bufferQueueItf_android)->Clear(m_bufferQueueItf_android))
            qWarning("Unable to clear buffer");
        m_bufferQueueItf_android = NULL;
    }
#endif
    if (m_bufferQueueItf && SL_RESULT_SUCCESS != (*m_bufferQueueItf)->Clear(m_bufferQueueItf))
        qWarning("Unable to clear buffer");

    if (m_playerObject) {
        (*m_playerObject)->Destroy(m_playerObject);
        m_playerObject = NULL;
    }
    if (m_outputMixObject) {
        (*m_outputMixObject)->Destroy(m_outputMixObject);
        m_outputMixObject = NULL;
    }

    m_playItf = NULL;
    m_volumeItf = NULL;
    m_bufferQueueItf = NULL;
    queue_data.clear();
    queue_data_write = 0;
    return true;
}

bool AudioOutputOpenSL::write(const QByteArray& data)
{
    // assume data.size() <= buffer_size. It's true in QtAV
    const int s = qMin(queue_data.size() - queue_data_write, data.size());
    // assume data.size() <= buffer_size. It's true in QtAV
    if (s < data.size())
        queue_data_write = 0;
    memcpy((char*)queue_data.constData() + queue_data_write, data.constData(), data.size());
    //qDebug("enqueue %p, queue_data_write: %d/%d available:%d", data.constData(), queue_data_write, queue_data.size(), sem.available());
#ifdef Q_OS_ANDROID
    if (m_android)
        SL_ENSURE((*m_bufferQueueItf_android)->Enqueue(m_bufferQueueItf_android, queue_data.constData() + queue_data_write, data.size()), false);
    else
#endif
    SL_ENSURE((*m_bufferQueueItf)->Enqueue(m_bufferQueueItf, queue_data.constData() + queue_data_write, data.size()), false);
    buffers_queued++;
    queue_data_write += data.size();
    if (queue_data_write == queue_data.size())
        queue_data_write = 0;
    return true;
}

bool AudioOutputOpenSL::play()
{
    SLuint32 state = SL_PLAYSTATE_PLAYING;
    (*m_playItf)->GetPlayState(m_playItf, &state);
    if (state == SL_PLAYSTATE_PLAYING)
        return true;
    SL_ENSURE((*m_playItf)->SetPlayState(m_playItf, SL_PLAYSTATE_PLAYING), false);
    return true;
}

int AudioOutputOpenSL::getPlayedCount()
{
    int processed = buffers_queued;
    SLuint32 count = 0;
#ifdef Q_OS_ANDROID
    if (m_android) {
        SLAndroidSimpleBufferQueueState state;
        (*m_bufferQueueItf_android)->GetState(m_bufferQueueItf_android, &state);
        count = state.count;
    } else
#endif
    {
        SLBufferQueueState state;
        (*m_bufferQueueItf)->GetState(m_bufferQueueItf, &state);
        count = state.count;
    }
    buffers_queued = count;
    processed -= count;
    return processed;
}

bool AudioOutputOpenSL::setVolume(qreal value)
{
    if (!m_volumeItf)
        return false;
    SLmillibel v = 0;
    if (qFuzzyIsNull(value))
        v = SL_MILLIBEL_MIN;
    else if (!qFuzzyCompare(value, 1.0))
        v = 20.0*log10(value)*100.0;
    SLmillibel vmax = SL_MILLIBEL_MAX;
    SL_ENSURE((*m_volumeItf)->GetMaxVolumeLevel(m_volumeItf, &vmax), false);
    if (vmax < v) {
        qDebug("OpenSL does not support volume: %f %d/%d. sw scale will be used", value, v, vmax);
        return false;
    }
    SL_ENSURE((*m_volumeItf)->SetVolumeLevel(m_volumeItf, v), false);
    return true;
}

qreal AudioOutputOpenSL::getVolume() const
{
    if (!m_volumeItf)
        return false;
    SLmillibel v = 0;
    SL_ENSURE((*m_volumeItf)->GetVolumeLevel(m_volumeItf, &v), 1.0);
    if (v == SL_MILLIBEL_MIN)
        return 0;
    return pow(10.0, qreal(v)/2000.0);
}

bool AudioOutputOpenSL::setMute(bool value)
{
    if (!m_volumeItf)
        return false;
    SL_ENSURE((*m_volumeItf)->SetMute(m_volumeItf, value), false);
    return true;
}

} //namespace QtAV
