/******************************************************************************
    AudioOutputOpenAL.cpp: description
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
#include "QtAV/private/prepost.h"
#include <QtCore/QVector>

#if defined(HEADER_OPENAL_PREFIX)
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif
#include "utils/Logger.h"

#define UNQUEUE_QUICK 0

namespace QtAV {

class AudioOutputOpenALPrivate;
class AudioOutputOpenAL : public AudioOutput
{
    DPTR_DECLARE_PRIVATE(AudioOutputOpenAL)
public:
    AudioOutputOpenAL();
    ~AudioOutputOpenAL();

    QString name() const;
    virtual bool open();
    virtual bool close();
    virtual bool isSupported(const AudioFormat& format) const;
    virtual bool isSupported(AudioFormat::SampleFormat sampleFormat) const;
    virtual bool isSupported(AudioFormat::ChannelLayout channelLayout) const;
    virtual AudioFormat::SampleFormat preferredSampleFormat() const;
    virtual AudioFormat::ChannelLayout preferredChannelLayout() const;

    virtual BufferControl supportedBufferControl() const;
    virtual bool play();
protected:
    virtual bool write(const QByteArray& data);
    virtual int getPlayedCount();
    int getQueued();
};

extern AudioOutputId AudioOutputId_OpenAL;
FACTORY_REGISTER_ID_AUTO(AudioOutput, OpenAL, "OpenAL")

void RegisterAudioOutputOpenAL_Man()
{
    FACTORY_REGISTER_ID_MAN(AudioOutput, OpenAL, "OpenAL")
}


#define AL_CHECK_RETURN_VALUE(RET) \
    do { \
        ALenum err = alGetError(); \
        if (err != AL_NO_ERROR) { \
            qWarning("AudioOutputOpenAL Error @%d  (%d) : %s", __LINE__, err, alGetString(err)); \
            return RET; \
        } \
    } while(0)

#define AL_CHECK_RETURN() AL_CHECK_RETURN_VALUE()
#define AL_CHECK() AL_CHECK_RETURN_VALUE(false)

#define AL_RUN_CHECK(FUNC) \
    do { \
        FUNC; \
        ALenum err = alGetError(); \
        if (err != AL_NO_ERROR) { \
            qWarning("AudioOutputOpenAL Error>>> " #FUNC " (%d) : %s", err, alGetString(err)); \
            return false; \
        } \
    } while(0)

#define SCOPE_LOCK_CONTEXT() \
    QMutexLocker ctx_lock(&d_func().global_mutex); \
    Q_UNUSED(ctx_lock); \
    alcMakeContextCurrent(d_func().context)

// TODO: planar
static ALenum audioFormatToAL(const AudioFormat& fmt)
{
    ALenum format = 0;
    ALCcontext *ctx = alcGetCurrentContext(); //a context is required for al functions!
    if (fmt.sampleFormat() == AudioFormat::SampleFormat_Unsigned8) {
        if (fmt.channels() == 1)
            format = AL_FORMAT_MONO8;
        else if (fmt.channels() == 2)
            format = AL_FORMAT_STEREO8;
        else if (ctx) {
            if (alIsExtensionPresent("AL_EXT_MCFORMATS")) {
                if (fmt.channels() == 4)
                    format = alGetEnumValue("AL_FORMAT_QUAD8");
                else if (fmt.channels() == 6)
                    format = alGetEnumValue("AL_FORMAT_51CHN8");
                else if (fmt.channels() == 7)
                    format = alGetEnumValue("AL_FORMAT_61CHN8");
                else if (fmt.channels() == 8)
                    format = alGetEnumValue("AL_FORMAT_71CHN8");
            }
        }
    } else if (fmt.sampleFormat() == AudioFormat::SampleFormat_Signed16) {
        if (fmt.channels() == 1)
            format = AL_FORMAT_MONO16;
        else if (fmt.channels() == 2)
            format = AL_FORMAT_STEREO16;
        else if (ctx) {
            if (alIsExtensionPresent("AL_EXT_MCFORMATS")) {
                if (fmt.channels() == 4)
                    format = alGetEnumValue("AL_FORMAT_QUAD16");
                else if (fmt.channels() == 6)
                    format = alGetEnumValue("AL_FORMAT_51CHN16");
                else if (fmt.channels() == 7)
                    format = alGetEnumValue("AL_FORMAT_61CHN16");
                else if (fmt.channels() == 8)
                    format = alGetEnumValue("AL_FORMAT_71CHN16");
            }
        }
    } else if (ctx) {
        if (fmt.sampleFormat() == AudioFormat::SampleFormat_Float) {
            if (alIsExtensionPresent("AL_EXT_float32")) {
                if (fmt.channels() == 1)
                    format = alGetEnumValue("AL_FORMAT_MONO_FLOAT32");
                else if (fmt.channels() == 2)
                    format = alGetEnumValue("AL_FORMAT_STEREO_FLOAT32");
                else if (alIsExtensionPresent("AL_EXT_MCFORMATS")) {
                    if (fmt.channels() == 4)
                        format = alGetEnumValue("AL_FORMAT_QUAD32");
                    else if (fmt.channels() == 6)
                        format = alGetEnumValue("AL_FORMAT_51CHN32");
                    else if (fmt.channels() == 7)
                        format = alGetEnumValue("AL_FORMAT_61CHN32");
                    else if (fmt.channels() == 8)
                        format = alGetEnumValue("AL_FORMAT_71CHN32");
                }
            }
        } else if (fmt.sampleFormat() == AudioFormat::SampleFormat_Double) {
            if (alIsExtensionPresent("AL_EXT_double")) {
                if (fmt.channels() == 1)
                    format = alGetEnumValue("AL_FORMAT_MONO_DOUBLE_EXT");
                else if (fmt.channels() == 2)
                    format = alGetEnumValue("AL_FORMAT_STEREO_DOUBLE_EXT");
            }
        }
    }
    ALCenum err = alGetError();
    if (err != AL_NO_ERROR) {
        qWarning("OpenAL audioFormatToAL error: %s", alGetString(err));
    }
    if (format == 0) {
        qWarning("AudioOutputOpenAL Error: No OpenAL format available for audio data format %s %s."
                 , qPrintable(fmt.sampleFormatName())
                 , qPrintable(fmt.channelLayoutName()));
    }
    qDebug("OpenAL audio format: %#x ch:%d, sample format: %s", format, fmt.channels(), qPrintable(fmt.sampleFormatName()));
    return format;
}

class  AudioOutputOpenALPrivate : public AudioOutputPrivate
{
public:
    AudioOutputOpenALPrivate()
        : AudioOutputPrivate()
        , device(0)
        , context(0)
        , format_al(AL_FORMAT_STEREO16)
        , state(0)
    {
    }
    ~AudioOutputOpenALPrivate() {
    }

    ALCdevice *device;
    ALCcontext *context;
    ALenum format_al;
    ALuint buffer[kBufferCount];
    ALuint source;
    ALint state;
    QMutex mutex;
    QWaitCondition cond;

    // used for 1 context per instance. lock when makeCurrent
    static QMutex global_mutex;
};

QMutex AudioOutputOpenALPrivate::global_mutex;

AudioOutputOpenAL::AudioOutputOpenAL()
    :AudioOutput(*new AudioOutputOpenALPrivate())
{
    setBufferControl(PlayedCount); //TODO: AL_BYTE_OFFSET
}

AudioOutputOpenAL::~AudioOutputOpenAL()
{
    close();
}

bool AudioOutputOpenAL::open()
{
    DPTR_D(AudioOutputOpenAL);
    d.available = false; // TODO: d.reset()
    resetStatus();
    QVector<QByteArray> _devices;
    const char *p = NULL;
    // maybe defined in alext.h
#ifdef ALC_ALL_DEVICES_SPECIFIER
    p = alcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);
#else
    if (alcIsExtensionPresent(NULL, "ALC_ENUMERATE_ALL_EXT")) {
        // avoid using enum ALC_ALL_DEVICES_SPECIFIER directly
        ALenum param = alcGetEnumValue(NULL, "ALC_ALL_DEVICES_SPECIFIER");
        p = alcGetString(NULL, param);
    } else {
        //alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT")
        //ALenum param = alcGetEnumValue(NULL, "ALC_DEVICE_SPECIFIER");
        p = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
    }
#endif
    while (p && *p) {
        _devices.push_back(p);
        p += _devices.last().size() + 1;
    }
    qDebug("OpenAL devices available: %d", _devices.size());
    for (int i = 0; i < _devices.size(); i++) {
        qDebug("device %d: %s", i, _devices[i].constData());
    }
    const ALCchar *default_device = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
    qDebug("AudioOutputOpenAL Opening default device: %s", default_device);
    d.device = alcOpenDevice(NULL); //parameter: NULL or default_device
    if (!d.device) {
        qWarning("AudioOutputOpenAL Failed to open sound device: %s", alcGetString(0, alcGetError(0)));
        return false;
    }
    qDebug("AudioOutputOpenAL creating context...");
    d.context = alcCreateContext(d.device, NULL);
    alcMakeContextCurrent(d.context);
    SCOPE_LOCK_CONTEXT();
    //alcProcessContext(ctx); //used when dealing witg multiple contexts
    ALCenum err = alcGetError(d.device);
    if (err != ALC_NO_ERROR) {
        qWarning("AudioOutputOpenAL Error: %s", alcGetString(d.device, err));
        return false;
    }
    qDebug("device: %p, context: %p", d.device, d.context);
    //init params. move to another func?
    d.format_al = audioFormatToAL(audioFormat());

    alGenBuffers(kBufferCount, d.buffer);
    err = alGetError();
    if (err != AL_NO_ERROR) {
        qWarning("Failed to generate OpenAL buffers: %s", alGetString(err));
        goto fail;
    }
    alGenSources(1, &d.source);
    err = alGetError();
    if (err != AL_NO_ERROR) {
        qWarning("Failed to generate OpenAL source: %s", alGetString(err));
        alDeleteBuffers(kBufferCount, d.buffer);
        goto fail;
    }

    alSourcei(d.source, AL_LOOPING, AL_FALSE);
    alSourcei(d.source, AL_SOURCE_RELATIVE, AL_TRUE);
    alSourcei(d.source, AL_ROLLOFF_FACTOR, 0);
    alSource3f(d.source, AL_POSITION, 0.0, 0.0, 0.0);
    alSource3f(d.source, AL_VELOCITY, 0.0, 0.0, 0.0);
    alListener3f(AL_POSITION, 0.0, 0.0, 0.0);

    //// Initial all buffers. TODO: move to open?
    //alSourcef(d.source, AL_GAIN, d.vol);

    static char init_data[kBufferSize];
    memset(init_data, 0, sizeof(init_data));
    for (int i = 1; i < kBufferCount; ++i) {
        AL_RUN_CHECK(alBufferData(d.buffer[i], d.format_al, init_data, sizeof(init_data), audioFormat().sampleRate()));
        AL_RUN_CHECK(alSourceQueueBuffers(d.source, 1, &d.buffer[i]));
        d.nextEnqueueInfo().data_size = sizeof(init_data);
        d.nextEnqueueInfo().timestamp = 0;
        d.bufferAdded();
    }
    // FIXME: Invalid Operation
    //AL_RUN_CHECK(alSourceQueueBuffers(d.source, sizeof(d.buffer)/sizeof(d.buffer[0]), d.buffer));
    alSourcePlay(d.source);

    d.state = 0;
    d.available = true;
    qDebug("AudioOutputOpenAL open ok...");
    return true;
fail:
    alcMakeContextCurrent(NULL);
    alcDestroyContext(d.context);
    alcCloseDevice(d.device);
    d.context = 0;
    d.device = 0;
    return false;
}

bool AudioOutputOpenAL::close()
{
    if (!isAvailable())
        return true;
    DPTR_D(AudioOutputOpenAL);
    d.state = 0;
    d.available = false;
    resetStatus();
    SCOPE_LOCK_CONTEXT();
    alSourceStop(d.source);
    do {
        alGetSourcei(d.source, AL_SOURCE_STATE, &d.state);
    } while (alGetError() == AL_NO_ERROR && d.state == AL_PLAYING);
    ALint processed = 0; //android need this!! otherwise the value may be undefined
    alGetSourcei(d.source, AL_BUFFERS_PROCESSED, &processed);
    ALuint buf;
    while (processed-- > 0) { alSourceUnqueueBuffers(d.source, 1, &buf); }
    alDeleteSources(1, &d.source);
    alDeleteBuffers(kBufferCount, d.buffer);

    alcMakeContextCurrent(NULL);
    if (d.context) {
        qDebug("alcDestroyContext(%p)", d.context);
        alcDestroyContext(d.context);
        ALCenum err = alcGetError(d.device);
        if (err != ALC_NO_ERROR) { //ALC_INVALID_CONTEXT
            qWarning("AudioOutputOpenAL Failed to destroy context: %s", alcGetString(d.device, err));
            return false;
        }
        d.context = 0;
    }
    if (d.device) {
        qDebug("alcCloseDevice(%p)", d.device);
        alcCloseDevice(d.device);
        // ALC_INVALID_DEVICE now
        d.device = 0;
    }
    return true;
}

bool AudioOutputOpenAL::isSupported(const AudioFormat& format) const
{
    SCOPE_LOCK_CONTEXT();
    return !!audioFormatToAL(format);
}

bool AudioOutputOpenAL::isSupported(AudioFormat::SampleFormat sampleFormat) const
{
    return sampleFormat == AudioFormat::SampleFormat_Unsigned8 || sampleFormat == AudioFormat::SampleFormat_Signed16;
}

bool AudioOutputOpenAL::isSupported(AudioFormat::ChannelLayout channelLayout) const
{
    return channelLayout == AudioFormat::ChannelLayout_Mono || channelLayout == AudioFormat::ChannelLayout_Stero;
}

AudioFormat::SampleFormat AudioOutputOpenAL::preferredSampleFormat() const
{
    return AudioFormat::SampleFormat_Signed16;
}

AudioFormat::ChannelLayout AudioOutputOpenAL::preferredChannelLayout() const
{
    return AudioFormat::ChannelLayout_Stero;
}

QString AudioOutputOpenAL::name() const
{
    DPTR_D(const AudioOutputOpenAL);
    if (!d.device)
        return QString();
    const ALCchar *name = alcGetString(d.device, ALC_DEVICE_SPECIFIER);
    return name;
}

AudioOutput::BufferControl AudioOutputOpenAL::supportedBufferControl() const
{
    return PlayedCount;
}

// http://kcat.strangesoft.net/openal-tutorial.html
bool AudioOutputOpenAL::write(const QByteArray& data)
{
    DPTR_D(AudioOutputOpenAL);
    if (data.isEmpty())
        return false;
    SCOPE_LOCK_CONTEXT();
    ALuint buf;
    //unqueues a set of buffers attached to a source
    AL_RUN_CHECK(alSourceUnqueueBuffers(d.source, 1, &buf));
    AL_RUN_CHECK(alBufferData(buf, d.format_al, data.constData(), data.size(), audioFormat().sampleRate()));
    AL_RUN_CHECK(alSourceQueueBuffers(d.source, 1, &buf));
    return true;
}

bool AudioOutputOpenAL::play()
{
    SCOPE_LOCK_CONTEXT();
    DPTR_D(AudioOutputOpenAL);
    alGetSourcei(d.source, AL_SOURCE_STATE, &d.state);
    if (d.state != AL_PLAYING) {
        qDebug("AudioOutputOpenAL: !AL_PLAYING alSourcePlay");
        alSourcePlay(d.source);
    }
    return true;
}

int AudioOutputOpenAL::getPlayedCount()
{
    SCOPE_LOCK_CONTEXT();
    DPTR_D(AudioOutputOpenAL);
    ALint processed = 0;
    alGetSourcei(d.source, AL_BUFFERS_PROCESSED, &processed);
    return processed;
}

int AudioOutputOpenAL::getQueued()
{
    SCOPE_LOCK_CONTEXT();
    ALint queued = 0;
    alGetSourcei(d_func().source, AL_BUFFERS_QUEUED, &queued);
    return queued;
}

} //namespace QtAV
