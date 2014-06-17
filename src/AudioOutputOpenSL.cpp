/******************************************************************************
    AudioOutputOpenSL.cpp: description
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>

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

#include "QtAV/AudioOutput.h"
#include "QtAV/private/AudioOutput_p.h"
#include <QtCore/QThread>
#include <SLES/OpenSLES.h>
#include "prepost.h"

namespace QtAV {

class AudioOutputOpenSLPrivate;
class AudioOutputOpenSL : public AudioOutput
{
    DPTR_DECLARE_PRIVATE(AudioOutputOpenSL)
public:
    AudioOutputOpenSL();
    ~AudioOutputOpenSL();

    virtual bool isSupported(const AudioFormat& format) const;
    virtual bool isSupported(AudioFormat::SampleFormat sampleFormat) const;
    virtual bool isSupported(AudioFormat::ChannelLayout channelLayout) const;
    virtual AudioFormat::SampleFormat preferredSampleFormat() const;
    virtual AudioFormat::ChannelLayout preferredChannelLayout() const;

    virtual bool open();
    virtual bool close();

    QString name() const;
    void waitForNextBuffer();

protected:
    virtual bool write();
};

extern AudioOutputId AudioOutputId_OpenSL;
FACTORY_REGISTER_ID_AUTO(AudioOutput, OpenSL, "OpenSL")

void RegisterAudioOutputOpenSL_Man()
{
    FACTORY_REGISTER_ID_MAN(AudioOutput, OpenSL, "OpenSL")
}

#define CheckError(message) if (result != SL_RESULT_SUCCESS) { qWarning(message); return; }

static SLDataFormat_PCM audioFormatToSL(const AudioFormat &format)
{
    SLDataFormat_PCM format_pcm;
    format_pcm.formatType = SL_DATAFORMAT_PCM;
    format_pcm.numChannels = format.channels();
    format_pcm.samplesPerSec = format.sampleRate() * 1000;
    format_pcm.bitsPerSample = format.bytesPerSample()*8;
    format_pcm.containerSize = format.bytesPerSample()*8;
    format_pcm.channelMask = (format.channels() == 1 ?
                                  SL_SPEAKER_FRONT_CENTER :
                                  SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT);
    format_pcm.endianness = SL_BYTEORDER_LITTLEENDIAN; //FIXME
    return format_pcm;
}

static void bufferQueueCallback(SLBufferQueueItf bufferQueue, void *context)
{
    SLBufferQueueState state;
    (*bufferQueue)->GetState(bufferQueue, &state);
    qDebug(">>>>>>>>>>>>>>bufferQueueCallback state.count=%d .playIndex=%d", state.count, state.playIndex);
    //QWaitCondition *cond = (QWaitCondition *)context;
    //cond->wakeAll();
}
void playCallback(SLPlayItf player, void *ctx, SLuint32 event)
{
    Q_UNUSED(player);
    qDebug("---------%s  event=%p", __FUNCTION__, event);
}

class  AudioOutputOpenSLPrivate : public AudioOutputPrivate
{
public:
    AudioOutputOpenSLPrivate()
        : AudioOutputPrivate()
        //, format(AL_FORMAT_STEREO16)
        , m_outputMixObject(0)
        , m_playerObject(0)
        , m_playItf(0)
        , m_volumeItf(0)
        , m_bufferQueueItf(0)
        , m_notifyInterval(1000)
        , buffers_queued(0)
        , init_buffers(true)
    {
        SLresult result = slCreateEngine(&engineObject, 0, 0, 0, 0, 0);
        CheckError("Failed to create engine");

        result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
        CheckError("Failed to realize engine");
        result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engine);
        CheckError("Failed to get engine interface");
        available = true;
    }
    ~AudioOutputOpenSLPrivate() {
    }

    SLObjectItf engineObject;
    SLEngineItf engine;
    SLObjectItf m_outputMixObject;
    SLObjectItf m_playerObject;
    SLPlayItf m_playItf;
    SLVolumeItf m_volumeItf;
    SLBufferQueueItf m_bufferQueueItf;
    int m_notifyInterval;
    int buffers_queued;
    bool init_buffers;
};

AudioOutputOpenSL::AudioOutputOpenSL()
    :AudioOutput(*new AudioOutputOpenSLPrivate())
{
}

AudioOutputOpenSL::~AudioOutputOpenSL()
{
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

bool AudioOutputOpenSL::open()
{
    DPTR_D(AudioOutputOpenSL);
    d.init_buffers = true;
    SLDataLocator_BufferQueue bufferQueueLocator = { SL_DATALOCATOR_BUFFERQUEUE, d.nb_buffers };
    SLDataFormat_PCM pcmFormat = audioFormatToSL(audioFormat());
    SLDataSource audioSrc = { &bufferQueueLocator, &pcmFormat };
    // OutputMix
    if (SL_RESULT_SUCCESS != (*d.engine)->CreateOutputMix(d.engine, &d.m_outputMixObject, 0, NULL, NULL)) {
        qWarning("Unable to create output mix");
        //setError(QAudio::FatalError);
        return false;
    }

    if (SL_RESULT_SUCCESS != (*d.m_outputMixObject)->Realize(d.m_outputMixObject, SL_BOOLEAN_FALSE)) {
        qWarning("Unable to initialize output mix");
        //setError(QAudio::FatalError);
        return false;
    }
    SLDataLocator_OutputMix outputMixLocator = { SL_DATALOCATOR_OUTPUTMIX, d.m_outputMixObject };
    SLDataSink audioSink = { &outputMixLocator, Q_NULLPTR };

    const int iids = 1;//2;
    const SLInterfaceID ids[iids] = { SL_IID_BUFFERQUEUE};//, SL_IID_VOLUME };
    const SLboolean req[iids] = { SL_BOOLEAN_TRUE};//, SL_BOOLEAN_TRUE };

    // AudioPlayer
    if (SL_RESULT_SUCCESS != (*d.engine)->CreateAudioPlayer(d.engine, &d.m_playerObject, &audioSrc, &audioSink, iids, ids, req)) {
        qWarning("Unable to create AudioPlayer");
        //setError(QAudio::OpenError);
        return false;
    }
    if (SL_RESULT_SUCCESS != (*d.m_playerObject)->Realize(d.m_playerObject, SL_BOOLEAN_FALSE)) {
        qWarning("Unable to initialize AudioPlayer");
        //setError(QAudio::OpenError);
        return false;
    }
    // Buffer interface
    if (SL_RESULT_SUCCESS != (*d.m_playerObject)->GetInterface(d.m_playerObject, SL_IID_BUFFERQUEUE, &d.m_bufferQueueItf)) {
        //setError(QAudio::FatalError);
        return false;
    }
    if (SL_RESULT_SUCCESS != (*d.m_bufferQueueItf)->RegisterCallback(d.m_bufferQueueItf, bufferQueueCallback, &d.cond)) {
        //setError(QAudio::FatalError);
        return false;
    }

    // Play interface

    if (SL_RESULT_SUCCESS != (*d.m_playerObject)->GetInterface(d.m_playerObject, SL_IID_PLAY, &d.m_playItf)) {
        //setError(QAudio::FatalError);
        return false;
    }

    if (SL_RESULT_SUCCESS != (*d.m_playItf)->RegisterCallback(d.m_playItf, playCallback, this)) {
        //setError(QAudio::FatalError);
        return false;
    }

    SLuint32 mask = SL_PLAYEVENT_HEADATEND;
    if (d.m_notifyInterval && SL_RESULT_SUCCESS == (*d.m_playItf)->SetPositionUpdatePeriod(d.m_playItf, 100)) {
        mask |= SL_PLAYEVENT_HEADATNEWPOS;
    }

    if (SL_RESULT_SUCCESS != (*d.m_playItf)->SetCallbackEventsMask(d.m_playItf, mask)) {
        //setError(QAudio::FatalError);
        return false;
    }
    // Volume interface
    if (SL_RESULT_SUCCESS != (*d.m_playerObject)->GetInterface(d.m_playerObject, SL_IID_VOLUME, &d.m_volumeItf)) {
        //setError(QAudio::FatalError);
        return false;
    }

    //setVolume(d.m_volume);

    return true;
}

bool AudioOutputOpenSL::close()
{
    DPTR_D(AudioOutputOpenSL);
    d.available = false;
    d.init_buffers = true;
    if (d.m_playItf)
        (*d.m_playItf)->SetPlayState(d.m_playItf, SL_PLAYSTATE_STOPPED);

    if (d.m_bufferQueueItf && SL_RESULT_SUCCESS != (*d.m_bufferQueueItf)->Clear(d.m_bufferQueueItf))
        qWarning("Unable to clear buffer");

    if (d.m_playerObject) {
        (*d.m_playerObject)->Destroy(d.m_playerObject);
        d.m_playerObject = Q_NULLPTR;
    }
    if (d.m_outputMixObject) {
        (*d.m_outputMixObject)->Destroy(d.m_outputMixObject);
        d.m_outputMixObject = Q_NULLPTR;
    }

    d.m_playItf = Q_NULLPTR;
    d.m_volumeItf = Q_NULLPTR;
    d.m_bufferQueueItf = Q_NULLPTR;
}

bool AudioOutputOpenSL::write()
{
    DPTR_D(AudioOutputOpenSL);
    if (d.init_buffers) {
        d.init_buffers = false;
        for (int i = 0; i < d.nb_buffers; ++i) {
            if (SL_RESULT_SUCCESS != (*d.m_bufferQueueItf)->Enqueue(d.m_bufferQueueItf, d.data.constData(), d.data.size())) {
                qWarning("failed to enqueue");
                return false;
            }
            d.buffers_queued++;
        }
        if (SL_RESULT_SUCCESS != (*d.m_playItf)->SetPlayState(d.m_playItf, SL_PLAYSTATE_PLAYING)) {
            //destroyPlayer();
        }
        return true;
    }
    if (SL_RESULT_SUCCESS != (*d.m_bufferQueueItf)->Enqueue(d.m_bufferQueueItf, d.data.constData(), d.data.size())) {
        qWarning("failed to enqueue");
        return false;
    }
    d.buffers_queued++;
    return true;
}

void AudioOutputOpenSL::waitForNextBuffer()
{
    DPTR_D(AudioOutputOpenSL);
    if (!d.canRemoveBuffer()) {
        return;
    }
    SLBufferQueueState state;
    (*d.m_bufferQueueItf)->GetState(d.m_bufferQueueItf, &state);
    qDebug(">>>>>>>>>>>>>>bufferQueueCallback state.count=%d .playIndex=%d", state.count, state.playIndex);
    // number of buffers in queue
    if (state.count <= 0) {
        return;
    }
    int processed = d.buffers_queued;
    while (state.count >= d.buffers_queued) {
        unsigned long duration = d.format.durationForBytes(d.nextDequeueInfo().data_size)/1000LL;
        QMutexLocker lock(&d.mutex);
        Q_UNUSED(lock);
        d.cond.wait(&d.mutex, duration);
        (*d.m_bufferQueueItf)->GetState(d.m_bufferQueueItf, &state);
    }
    d.buffers_queued = state.count;
    processed -= state.count;
    while (processed--) {
        d.bufferRemoved();
    }
}

} //namespace QtAV
