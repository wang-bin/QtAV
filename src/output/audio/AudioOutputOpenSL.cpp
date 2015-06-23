/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2014-2015 Wang Bin <wbsecg1@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "QtAV/private/AudioOutputBackend.h"
#include <QtCore/QSemaphore>
#include <QtCore/QThread>
#include <SLES/OpenSLES.h>
#ifdef Q_OS_ANDROID
#include <SLES/OpenSLES_Android.h>
#endif
#include "QtAV/private/mkid.h"
#include "QtAV/private/prepost.h"
#include "utils/Logger.h"

namespace QtAV {

static const char kName[] = "OpenSL";
class AudioOutputOpenSL Q_DECL_FINAL: public AudioOutputBackend
{
public:
    AudioOutputOpenSL(QObject *parent = 0);
    ~AudioOutputOpenSL();

    QString name() const Q_DECL_OVERRIDE { return kName;}
    bool isSupported(const AudioFormat& format) const Q_DECL_OVERRIDE;
    bool isSupported(AudioFormat::SampleFormat sampleFormat) const Q_DECL_OVERRIDE;
    bool isSupported(AudioFormat::ChannelLayout channelLayout) const Q_DECL_OVERRIDE;
    AudioFormat::SampleFormat preferredSampleFormat() const Q_DECL_OVERRIDE;
    AudioFormat::ChannelLayout preferredChannelLayout() const Q_DECL_OVERRIDE;
    bool open() Q_DECL_OVERRIDE;
    bool close() Q_DECL_OVERRIDE;
    BufferControl bufferControl() const Q_DECL_OVERRIDE;
    void onCallback() Q_DECL_OVERRIDE;
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
    SLObjectItf engineObject;
    SLEngineItf engine;
    SLObjectItf m_outputMixObject;
    SLObjectItf m_playerObject;
    SLPlayItf m_playItf;
    SLVolumeItf m_volumeItf;
    SLBufferQueueItf m_bufferQueueItf;
#ifdef Q_OS_ANDROID
    SLAndroidSimpleBufferQueueItf m_bufferQueueItf_android;
#endif
    bool m_android;
    int m_notifyInterval;
    quint32 buffers_queued;
    QSemaphore sem;
};

typedef AudioOutputOpenSL AudioOutputBackendOpenSL;
static const AudioOutputBackendId AudioOutputBackendId_OpenSL = mkid::id32base36_6<'O', 'p', 'e', 'n', 'S', 'L'>::value;
FACTORY_REGISTER_ID_AUTO(AudioOutputBackend, OpenSL, kName)

void RegisterAudioOutputOpenSL_Man()
{
    FACTORY_REGISTER_ID_MAN(AudioOutputBackend, OpenSL, kName)
}

#define SL_ENSURE_OK(FUNC, ...) \
    do { \
        SLresult ret = FUNC; \
        if (ret != SL_RESULT_SUCCESS) { \
            qWarning("AudioOutputOpenSL Error>>> " #FUNC " (%lu)", ret); \
            return __VA_ARGS__; \
        } \
    } while(0)

static SLDataFormat_PCM audioFormatToSL(const AudioFormat &format)
{
    SLDataFormat_PCM format_pcm;
    format_pcm.formatType = SL_DATAFORMAT_PCM;
    format_pcm.numChannels = format.channels();
    format_pcm.samplesPerSec = format.sampleRate() * 1000;
    format_pcm.bitsPerSample = format.bytesPerSample()*8;
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
    SLAndroidSimpleBufferQueueState state;
    (*bufferQueue)->GetState(bufferQueue, &state);
    //qDebug(">>>>>>>>>>>>>>bufferQueueCallback state.count=%lu .playIndex=%lu", state.count, state.playIndex);
    AudioOutputOpenSL *ao = reinterpret_cast<AudioOutputOpenSL*>(context);
    if (ao->bufferControl() & AudioOutputBackend::CountCallback) {
        ao->onCallback();
    }
}
#endif
void AudioOutputOpenSL::bufferQueueCallback(SLBufferQueueItf bufferQueue, void *context)
{
    SLBufferQueueState state;
    (*bufferQueue)->GetState(bufferQueue, &state);
    //qDebug(">>>>>>>>>>>>>>bufferQueueCallback state.count=%lu .playIndex=%lu", state.count, state.playIndex);
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
    , m_android(true)
    , m_notifyInterval(1000)
    , buffers_queued(0)
{
    SL_ENSURE_OK(slCreateEngine(&engineObject, 0, 0, 0, 0, 0));
    SL_ENSURE_OK((*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE));
    SL_ENSURE_OK((*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engine));
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
    return sampleFormat == AudioFormat::SampleFormat_Unsigned8 || sampleFormat == AudioFormat::SampleFormat_Signed16;
}

bool AudioOutputOpenSL::isSupported(AudioFormat::ChannelLayout channelLayout) const
{
    return channelLayout == AudioFormat::ChannelLayout_Mono || channelLayout == AudioFormat::ChannelLayout_Stero;
}

AudioFormat::SampleFormat AudioOutputOpenSL::preferredSampleFormat() const
{
    return AudioFormat::SampleFormat_Signed16;
}

AudioFormat::ChannelLayout AudioOutputOpenSL::preferredChannelLayout() const
{
    return AudioFormat::ChannelLayout_Stero;
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

bool AudioOutputOpenSL::open()
{

    SLDataLocator_BufferQueue bufferQueueLocator = { SL_DATALOCATOR_BUFFERQUEUE, (SLuint32)buffer_count };
    SLDataFormat_PCM pcmFormat = audioFormatToSL(format);
    SLDataSource audioSrc = { &bufferQueueLocator, &pcmFormat };
#ifdef Q_OS_ANDROID
    SLDataLocator_AndroidSimpleBufferQueue bufferQueueLocator_android = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, (SLuint32)buffer_count };
    if (m_android)
        audioSrc.pLocator = &bufferQueueLocator_android;
#endif
    // OutputMix
    SL_ENSURE_OK((*engine)->CreateOutputMix(engine, &m_outputMixObject, 0, NULL, NULL), false);
    SL_ENSURE_OK((*m_outputMixObject)->Realize(m_outputMixObject, SL_BOOLEAN_FALSE), false);
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
    SL_ENSURE_OK((*engine)->CreateAudioPlayer(engine, &m_playerObject, &audioSrc, &audioSink, sizeof(ids)/sizeof(ids[0]), ids, req), false);
    SL_ENSURE_OK((*m_playerObject)->Realize(m_playerObject, SL_BOOLEAN_FALSE), false);
    // Buffer interface
#ifdef Q_OS_ANDROID
    if (m_android) {
        SL_ENSURE_OK((*m_playerObject)->GetInterface(m_playerObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &m_bufferQueueItf_android), false);
        SL_ENSURE_OK((*m_bufferQueueItf_android)->RegisterCallback(m_bufferQueueItf_android, AudioOutputOpenSL::bufferQueueCallbackAndroid, this), false);
    } else {
        SL_ENSURE_OK((*m_playerObject)->GetInterface(m_playerObject, SL_IID_BUFFERQUEUE, &m_bufferQueueItf), false);
        SL_ENSURE_OK((*m_bufferQueueItf)->RegisterCallback(m_bufferQueueItf, AudioOutputOpenSL::bufferQueueCallback, this), false);
    }
#endif
    SL_ENSURE_OK((*m_playerObject)->GetInterface(m_playerObject, SL_IID_BUFFERQUEUE, &m_bufferQueueItf), false);
    SL_ENSURE_OK((*m_bufferQueueItf)->RegisterCallback(m_bufferQueueItf, AudioOutputOpenSL::bufferQueueCallback, this), false);
    // Play interface
    SL_ENSURE_OK((*m_playerObject)->GetInterface(m_playerObject, SL_IID_PLAY, &m_playItf), false);
    // call when SL_PLAYSTATE_STOPPED
    SL_ENSURE_OK((*m_playItf)->RegisterCallback(m_playItf, AudioOutputOpenSL::playCallback, this), false);
    SL_ENSURE_OK((*m_playerObject)->GetInterface(m_playerObject, SL_IID_VOLUME, &m_volumeItf), false);
#if 0
    SLuint32 mask = SL_PLAYEVENT_HEADATEND;
    // TODO: what does this do?
    SL_ENSURE_OK((*m_playItf)->SetPositionUpdatePeriod(m_playItf, 100), false);
    SL_ENSURE_OK((*m_playItf)->SetCallbackEventsMask(m_playItf, mask), false);
#endif
    // Volume interface
    //SL_ENSURE_OK((*m_playerObject)->GetInterface(m_playerObject, SL_IID_VOLUME, &m_volumeItf), false);

    sem.release(buffer_count - sem.available());
    return true;
}

bool AudioOutputOpenSL::close()
{
    if (m_playItf)
        (*m_playItf)->SetPlayState(m_playItf, SL_PLAYSTATE_STOPPED);

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
    return true;
}

bool AudioOutputOpenSL::write(const QByteArray& data)
{
    if (bufferControl() & CountCallback)
        sem.acquire();
    SL_ENSURE_OK((*m_bufferQueueItf)->Enqueue(m_bufferQueueItf, data.constData(), data.size()), false);
    buffers_queued++;
    return true;
}

bool AudioOutputOpenSL::play()
{
    SL_ENSURE_OK((*m_playItf)->SetPlayState(m_playItf, SL_PLAYSTATE_PLAYING), false);
    return true;
}

int AudioOutputOpenSL::getPlayedCount()
{
    int processed = buffers_queued;
    SLBufferQueueState state;
    (*m_bufferQueueItf)->GetState(m_bufferQueueItf, &state);
    buffers_queued = state.count;
    processed -= state.count;
    return processed;
}

bool AudioOutputOpenSL::setVolume(qreal value)
{
    if (!m_volumeItf)
        return false;
    SLmillibel v = 0;
    if (!qFuzzyCompare(value, 1.0))
        v = 20*log10(value)*100; // I.e., 20 * LOG10(SL_MILLIBEL_MAX * vol / SL_MILLIBEL_MIN)
    SLmillibel vmax = SL_MILLIBEL_MAX;
    SL_ENSURE_OK((*m_volumeItf)->GetMaxVolumeLevel(m_volumeItf, &vmax), false);
    if (vmax < v)
        return false;
    SL_ENSURE_OK((*m_volumeItf)->SetVolumeLevel(m_volumeItf, v), false);
    return true;
}

qreal AudioOutputOpenSL::getVolume() const
{
    if (!m_volumeItf)
        return false;
    SLmillibel v = 0;
    SL_ENSURE_OK((*m_volumeItf)->GetVolumeLevel(m_volumeItf, &v), 1.0);
    return pow(10.0, qreal(v)/2000.0);
}

bool AudioOutputOpenSL::setMute(bool value)
{
    if (!m_volumeItf)
        return false;
    SL_ENSURE_OK((*m_volumeItf)->SetMute(m_volumeItf, value), false);
    return true;
}

} //namespace QtAV
