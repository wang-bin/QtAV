/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2015 Wang Bin <wbsecg1@gmail.com>
    
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
    AudioOutputOpenAL(QObject* parent = 0);
    ~AudioOutputOpenAL();

    QString name() const;
    virtual bool open();
    virtual bool close();
    virtual bool isSupported(const AudioFormat& format) const;
    virtual bool isSupported(AudioFormat::SampleFormat sampleFormat) const;
    virtual bool isSupported(AudioFormat::ChannelLayout channelLayout) const;
    virtual AudioFormat::SampleFormat preferredSampleFormat() const;
    virtual AudioFormat::ChannelLayout preferredChannelLayout() const;
protected:
    virtual BufferControl bufferControl() const;
    virtual bool write(const QByteArray& data);
    virtual bool play();
    virtual int getPlayedCount();
    virtual bool deviceSetVolume(qreal value);
    virtual qreal deviceGetVolume() const;
    int getQueued();
};

extern AudioOutputId AudioOutputId_OpenAL;
FACTORY_REGISTER_ID_AUTO(AudioOutput, OpenAL, "OpenAL")

void RegisterAudioOutputOpenAL_Man()
{
    FACTORY_REGISTER_ID_MAN(AudioOutput, OpenAL, "OpenAL")
}

#define AL_ENSURE_OK(expr, ...) \
    do { \
        expr; \
        const ALenum err = alGetError(); \
        if (err != AL_NO_ERROR) { \
            qWarning("AudioOutputOpenAL Error>>> " #expr " (%d) : %s", err, alGetString(err)); \
            return __VA_ARGS__; \
        } \
    } while(0)

#define SCOPE_LOCK_CONTEXT() \
    QMutexLocker ctx_lock(&d_func().global_mutex); \
    Q_UNUSED(ctx_lock); \
    if (d_func().context) \
        alcMakeContextCurrent(d_func().context)

// TODO: planar
static ALenum audioFormatToAL(const AudioFormat& fmt)
{
    typedef union {
        const char* ext;
        ALenum fmt;
    } al_fmt_t;
    ALenum format = 0;
    // al functions need a context
    ALCcontext *ctx = alcGetCurrentContext(); //a context is required for al functions!
    const int c = fmt.channels();
    const AudioFormat::SampleFormat spfmt = fmt.sampleFormat();
    if (AudioFormat::SampleFormat_Unsigned8 == spfmt) {
        static const al_fmt_t u8fmt[] = {
            {(const char*)AL_FORMAT_MONO8},
            {(const char*)AL_FORMAT_STEREO8},
            {(const char*)0},
            {"AL_FORMAT_QUAD8"},
            {"AL_FORMAT_REAR8"},
            {"AL_FORMAT_51CHN8"},
            {"AL_FORMAT_61CHN8"},
            {"AL_FORMAT_71CHN8"}
        };
        if (c < 3) {
            format = u8fmt[c-1].fmt;
        } else if (c > 3 && c <= 8 && ctx) {
            if (alIsExtensionPresent("AL_EXT_MCFORMATS"))
                format = alGetEnumValue(u8fmt[c-1].ext);
        }
    } else if (AudioFormat::SampleFormat_Signed16 == spfmt) {
        static const al_fmt_t s16fmt[] = {
            {(const char*)AL_FORMAT_MONO16},
            {(const char*)AL_FORMAT_STEREO16},
            {(const char*)0},
            {"AL_FORMAT_QUAD16"},
            {"AL_FORMAT_REAR16"},
            {"AL_FORMAT_51CHN16"},
            {"AL_FORMAT_61CHN16"},
            {"AL_FORMAT_71CHN16"}
        };
        if (c < 3) {
            format = s16fmt[c-1].fmt;
        } else if (c > 3 && c <= 8 && ctx) {
            if (alIsExtensionPresent("AL_EXT_MCFORMATS"))
                format = alGetEnumValue(s16fmt[c-1].ext);
        }
    } else if (ctx) {
        if (AudioFormat::SampleFormat_Signed32 == spfmt) {
            if (c > 3 && c <= 8) {
                if (alIsExtensionPresent("AL_EXT_MCFORMATS")) {
                    static const al_fmt_t s32fmt[] = {
                        {"AL_FORMAT_QUAD32"},
                        {"AL_FORMAT_REAR32"},
                        {"AL_FORMAT_51CHN32"},
                        {"AL_FORMAT_61CHN32"},
                        {"AL_FORMAT_71CHN32"}
                    };
                    format = alGetEnumValue(s32fmt[c-4].ext);
                }
            }
        } else if (AudioFormat::SampleFormat_Float == spfmt) {
            if (c < 3) {
                if (alIsExtensionPresent("AL_EXT_float32")) {
                    static const al_fmt_t f32fmt[] = {
                        {"AL_FORMAT_MONO_FLOAT32"},
                        {"AL_FORMAT_STEREO_FLOAT32"}
                    };
                    format = alGetEnumValue(f32fmt[c].ext);
                }
            }
        } else if (AudioFormat::SampleFormat_Double == spfmt) {
            if (c < 3) {
                if (alIsExtensionPresent("AL_EXT_double")) {
                    static const al_fmt_t d64fmt[] = {
                        {"AL_FORMAT_MONO_DOUBLE_EXT"},
                        {"AL_FORMAT_STEREO_DOUBLE_EXT"}
                    };
                    format = alGetEnumValue(d64fmt[c].ext);
                }
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

AudioOutputOpenAL::AudioOutputOpenAL(QObject *parent)
    : AudioOutput(SetVolume, *new AudioOutputOpenALPrivate(), parent)
{
    setDeviceFeatures(SetVolume);
}

AudioOutputOpenAL::~AudioOutputOpenAL()
{
    close();
}

bool AudioOutputOpenAL::open()
{
    DPTR_D(AudioOutputOpenAL);
    resetStatus();
    QVector<QByteArray> _devices;
    const char *p = NULL;
    if (alcIsExtensionPresent(NULL, "ALC_ENUMERATE_ALL_EXT")) {
        // ALC_ALL_DEVICES_SPECIFIER maybe not defined
        p = alcGetString(NULL, alcGetEnumValue(NULL, "ALC_ALL_DEVICES_SPECIFIER"));
    } else {
        p = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
    }
    while (p && *p) {
        _devices.push_back(p);
        p += _devices.last().size() + 1;
    }
    qDebug("OpenAL devices available: %d", _devices.size());
    qDebug() << _devices;
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
    // alGetString: alsoft needs a context. apple does not
    qDebug("OpenAL %s vendor: %s; renderer: %s", alGetString(AL_VERSION), alGetString(AL_VENDOR), alGetString(AL_RENDERER));
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
    for (int i = 1; i < bufferCount(); ++i) {
        AL_ENSURE_OK(alBufferData(d.buffer[i], d.format_al, init_data, sizeof(init_data), audioFormat().sampleRate()), false);
        AL_ENSURE_OK(alSourceQueueBuffers(d.source, 1, &d.buffer[i]), false);
        d.nextEnqueueInfo().data_size = sizeof(init_data);
        d.nextEnqueueInfo().timestamp = 0;
        d.bufferAdded();
    }
    // FIXME: Invalid Operation
    //AL_ENSURE_OK(alSourceQueueBuffers(d.source, sizeof(d.buffer)/sizeof(d.buffer[0]), d.buffer), false);
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
    DPTR_D(AudioOutputOpenAL);
    d.state = 0;
    resetStatus();
    if (!d.context)
        return true;
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
    qDebug("alcDestroyContext(%p)", d.context);
    alcDestroyContext(d.context);
    ALCenum err = alcGetError(d.device);
    if (err != ALC_NO_ERROR) { //ALC_INVALID_CONTEXT
        qWarning("AudioOutputOpenAL Failed to destroy context: %s", alcGetString(d.device, err));
        return false;
    }
    d.context = 0;
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

AudioOutput::BufferControl AudioOutputOpenAL::bufferControl() const
{
    return PlayedCount; //TODO: AL_BYTE_OFFSET
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
    AL_ENSURE_OK(alSourceUnqueueBuffers(d.source, 1, &buf), false);
    AL_ENSURE_OK(alBufferData(buf, d.format_al, data.constData(), data.size(), audioFormat().sampleRate()), false);
    AL_ENSURE_OK(alSourceQueueBuffers(d.source, 1, &buf), false);
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

bool AudioOutputOpenAL::deviceSetVolume(qreal value)
{
    SCOPE_LOCK_CONTEXT();
    AL_ENSURE_OK(alListenerf(AL_GAIN, value), false);
    return true;
}

qreal AudioOutputOpenAL::deviceGetVolume() const
{
    SCOPE_LOCK_CONTEXT();
    ALfloat v = 1.0;
    alGetListenerf(AL_GAIN, &v);
    ALenum err = alGetError();
    if (err != AL_NO_ERROR) {
        qWarning("AudioOutputOpenAL Error>>> deviceGetVolume (%d) : %s", err, alGetString(err));
    }
    return v;
}

int AudioOutputOpenAL::getQueued()
{
    SCOPE_LOCK_CONTEXT();
    ALint queued = 0;
    alGetSourcei(d_func().source, AL_BUFFERS_QUEUED, &queued);
    return queued;
}

} //namespace QtAV
