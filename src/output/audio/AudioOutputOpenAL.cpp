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


#include "QtAV/private/AudioOutputBackend.h"
#include "QtAV/private/mkid.h"
#include "QtAV/private/factory.h"
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/QVector>

#if QTAV_HAVE(CAPI)
#define OPENAL_CAPI_NS // CAPI_LINK_OPENAL will override it
#include "capi/openal_api.h"
#else
#if defined(HEADER_OPENAL_PREFIX)
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif
#endif //QTAV_HAVE(CAPI)
#include "utils/Logger.h"

#define UNQUEUE_QUICK 0

namespace QtAV {

static const char kName[] = "OpenAL";
class AudioOutputOpenAL Q_DECL_FINAL: public AudioOutputBackend
{
public:
    AudioOutputOpenAL(QObject* parent = 0);
    QString name() const Q_DECL_FINAL { return QLatin1String(kName);}
    QString deviceName() const;
    bool open() Q_DECL_FINAL;
    bool close() Q_DECL_FINAL;
    bool isSupported(const AudioFormat& format) const Q_DECL_FINAL;
    bool isSupported(AudioFormat::SampleFormat sampleFormat) const Q_DECL_FINAL;
    bool isSupported(AudioFormat::ChannelLayout channelLayout) const Q_DECL_FINAL;
protected:
    BufferControl bufferControl() const Q_DECL_FINAL;
    bool write(const QByteArray& data) Q_DECL_FINAL;
    bool play() Q_DECL_FINAL;
    int getPlayedCount() Q_DECL_FINAL;
    bool setVolume(qreal value) Q_DECL_FINAL;
    qreal getVolume() const Q_DECL_FINAL;
    int getQueued();

    bool openDevice() {
        if (context)
            return true;
        const ALCchar *default_device = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
        qDebug("OpenAL opening default device: %s", default_device);
        device = alcOpenDevice(NULL); //parameter: NULL or default_device
        if (!device) {
            qWarning("OpenAL failed to open sound device: %s", alcGetString(0, alcGetError(0)));
            return false;
        }
        qDebug("AudioOutputOpenAL creating context...");
        context = alcCreateContext(device, NULL);
        alcMakeContextCurrent(context);
        return true;
    }

    ALCdevice *device;
    ALCcontext *context;
    ALenum format_al;
    QVector<ALuint> buffer;
    ALuint source;
    ALint state;
    QMutex mutex;
    QWaitCondition cond;

    // used for 1 context per instance. lock when makeCurrent
    static QMutex global_mutex;
};

typedef AudioOutputOpenAL AudioOutputBackendOpenAL;
static const AudioOutputBackendId AudioOutputBackendId_OpenAL = mkid::id32base36_6<'O', 'p', 'e', 'n', 'A', 'L'>::value;
FACTORY_REGISTER(AudioOutputBackend, OpenAL, kName)

#define AL_ENSURE(expr, ...) \
    do { \
        expr; \
        const ALenum err = alGetError(); \
        if (err != AL_NO_ERROR) { \
            qWarning("AudioOutputOpenAL Error>>> " #expr " (%d) : %s", err, alGetString(err)); \
            return __VA_ARGS__; \
        } \
    } while(0)

#define SCOPE_LOCK_CONTEXT() \
    QMutexLocker ctx_lock(&global_mutex); \
    Q_UNUSED(ctx_lock); \
    if (context) \
        alcMakeContextCurrent(context)

static ALenum audioFormatToAL(const AudioFormat& fmt)
{
    if (fmt.isPlanar())
        return 0;
    typedef union {
        const char* ext;
        ALenum fmt;
    } al_fmt_t;
    ALenum format = 0;
    // al functions need a context
    ALCcontext *ctx = alcGetCurrentContext(); //a context is required for al functions!
    const int c = fmt.channels();
    const AudioFormat::SampleFormat spfmt = fmt.sampleFormat(); //TODO: planar formats are fine too
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
        if (AudioFormat::SampleFormat_Float == spfmt) {
            static const al_fmt_t f32fmt[] = {
                {"AL_FORMAT_MONO_FLOAT32"},
                {"AL_FORMAT_STEREO_FLOAT32"},
                {0},
                // AL_EXT_MCFORMATS
                {"AL_FORMAT_QUAD32"},
                {"AL_FORMAT_REAR32"},
                {"AL_FORMAT_51CHN32"},
                {"AL_FORMAT_61CHN32"},
                {"AL_FORMAT_71CHN32"}
            };
            if (c <=8 && f32fmt[c-1].ext) {
                format = alGetEnumValue(f32fmt[c-1].ext);
            }
        } else if (AudioFormat::SampleFormat_Double == spfmt) {
            if (c < 3) {
                if (alIsExtensionPresent("AL_EXT_double")) {
                    static const al_fmt_t d64fmt[] = {
                        {"AL_FORMAT_MONO_DOUBLE_EXT"},
                        {"AL_FORMAT_STEREO_DOUBLE_EXT"}
                    };
                    format = alGetEnumValue(d64fmt[c-1].ext);
                }
            }
        }
    }
    ALCenum err = alGetError();
    if (err != AL_NO_ERROR) {
        if (ctx)
            qWarning("OpenAL audioFormatToAL error: %s", alGetString(err));
        else
            qWarning("OpenAL audioFormatToAL error (null context): %#x", err);
    }
    if (format == 0) {
        qWarning("AudioOutputOpenAL Error: No OpenAL format available for audio data format %s %s."
                 , qPrintable(fmt.sampleFormatName())
                 , qPrintable(fmt.channelLayoutName()));
    }
    qDebug("OpenAL audio format: %#x ch:%d, sample format: %s", format, fmt.channels(), qPrintable(fmt.sampleFormatName()));
    return format;
}

QMutex AudioOutputOpenAL::global_mutex;

AudioOutputOpenAL::AudioOutputOpenAL(QObject *parent)
    : AudioOutputBackend(AudioOutput::SetVolume, parent)
    , device(0)
    , context(0)
    , format_al(AL_FORMAT_STEREO16)
    , state(0)
{
#if QTAV_HAVE(CAPI)
#ifndef CAPI_LINK_OPENAL
    if (!openal::capi::loaded()) {
        available = false;
        return;
    }
#endif //CAPI_LINK_OPENAL
#endif
    //setDeviceFeatures(AudioOutput::SetVolume);
    // ensure we have a context to check format support
    // TODO: AudioOutput::getDevices() => ao.setDevice() => ao.open
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
    qDebug() << _devices;
    available = openDevice(); //ensure isSupported(AudioFormat) works correctly
}

bool AudioOutputOpenAL::open()
{
    if (!openDevice())
        return false;
    {
    SCOPE_LOCK_CONTEXT();
    // alGetString: alsoft needs a context. apple does not
    qDebug("OpenAL %s vendor: %s; renderer: %s", alGetString(AL_VERSION), alGetString(AL_VENDOR), alGetString(AL_RENDERER));
    //alcProcessContext(ctx); //used when dealing witg multiple contexts
    ALCenum err = alcGetError(device);
    if (err != ALC_NO_ERROR) {
        qWarning("AudioOutputOpenAL Error: %s", alcGetString(device, err));
        return false;
    }
    qDebug("device: %p, context: %p", device, context);
    //init params. move to another func?
    format_al = audioFormatToAL(format);

    buffer.resize(buffer_count);
    alGenBuffers(buffer.size(), buffer.data());
    err = alGetError();
    if (err != AL_NO_ERROR) {
        qWarning("Failed to generate OpenAL buffers: %s", alGetString(err));
        goto fail;
    }
    alGenSources(1, &source);
    err = alGetError();
    if (err != AL_NO_ERROR) {
        qWarning("Failed to generate OpenAL source: %s", alGetString(err));
        alDeleteBuffers(buffer.size(), buffer.constData());
        goto fail;
    }
    alSourcei(source, AL_LOOPING, AL_FALSE);
    alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
    alSourcei(source, AL_ROLLOFF_FACTOR, 0);
    alSource3f(source, AL_POSITION, 0.0, 0.0, 0.0);
    alSource3f(source, AL_VELOCITY, 0.0, 0.0, 0.0);
    alListener3f(AL_POSITION, 0.0, 0.0, 0.0);
    state = 0;
    qDebug("AudioOutputOpenAL open ok...");
    }
    return true;
fail:
    alcMakeContextCurrent(NULL);
    alcDestroyContext(context);
    alcCloseDevice(device);
    context = 0;
    device = 0;
    return false;
}

bool AudioOutputOpenAL::close()
{
    state = 0;
    if (!context)
        return true;
    SCOPE_LOCK_CONTEXT();
    alSourceStop(source);
    do {
        alGetSourcei(source, AL_SOURCE_STATE, &state);
    } while (alGetError() == AL_NO_ERROR && state == AL_PLAYING);
    ALint processed = 0; //android need this!! otherwise the value may be undefined
    alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);
    ALuint buf;
    while (processed-- > 0) { alSourceUnqueueBuffers(source, 1, &buf); }
    alDeleteSources(1, &source);
    alDeleteBuffers(buffer.size(), buffer.constData());

    alcMakeContextCurrent(NULL);
    qDebug("alcDestroyContext(%p)", context);
    alcDestroyContext(context);
    ALCenum err = alcGetError(device);
    if (err != ALC_NO_ERROR) { //ALC_INVALID_CONTEXT
        qWarning("AudioOutputOpenAL Failed to destroy context: %s", alcGetString(device, err));
        return false;
    }
    context = 0;
    if (device) {
        qDebug("alcCloseDevice(%p)", device);
        alcCloseDevice(device);
        // ALC_INVALID_DEVICE now
        device = 0;
    }
    return true;
}

bool AudioOutputOpenAL::isSupported(const AudioFormat& format) const
{
    //if (!context)
      //  openDevice(); //not const
    SCOPE_LOCK_CONTEXT();
    return !!audioFormatToAL(format);
}

bool AudioOutputOpenAL::isSupported(AudioFormat::SampleFormat sampleFormat) const
{
    if (sampleFormat == AudioFormat::SampleFormat_Unsigned8 || sampleFormat == AudioFormat::SampleFormat_Signed16)
        return true;
    if (IsPlanar(sampleFormat))
        return false;
    SCOPE_LOCK_CONTEXT();
    if (sampleFormat == AudioFormat::SampleFormat_Float)
        return alIsExtensionPresent("AL_EXT_float32");
    if (sampleFormat == AudioFormat::SampleFormat_Double)
        return alIsExtensionPresent("AL_EXT_double");
    // because preferredChannelLayout() is stereo while s32 only supports >3 channels, so always false
    return false;
}

bool AudioOutputOpenAL::isSupported(AudioFormat::ChannelLayout channelLayout) const // FIXME: check
{
    return channelLayout == AudioFormat::ChannelLayout_Mono || channelLayout == AudioFormat::ChannelLayout_Stereo;
}

QString AudioOutputOpenAL::deviceName() const
{
    if (!device)
        return QString();
    const ALCchar *name = alcGetString(device, ALC_DEVICE_SPECIFIER);
    return QString::fromUtf8(name);
}

AudioOutputBackend::BufferControl AudioOutputOpenAL::bufferControl() const
{
    return PlayedCount; //TODO: AL_BYTE_OFFSET
}

// http://kcat.strangesoft.net/openal-tutorial.html
bool AudioOutputOpenAL::write(const QByteArray& data)
{
    if (data.isEmpty())
        return false;
    SCOPE_LOCK_CONTEXT();
    ALuint buf = 0;
    if (state <= 0) { //state used for filling initial data
        buf = buffer[(-state)%buffer_count];
        --state;
    } else {
        AL_ENSURE(alSourceUnqueueBuffers(source, 1, &buf), false);
    }
    AL_ENSURE(alBufferData(buf, format_al, data.constData(), data.size(), format.sampleRate()), false);
    AL_ENSURE(alSourceQueueBuffers(source, 1, &buf), false);
    return true;
}

bool AudioOutputOpenAL::play()
{
    SCOPE_LOCK_CONTEXT();
    alGetSourcei(source, AL_SOURCE_STATE, &state);
    if (state != AL_PLAYING) {
        qDebug("AudioOutputOpenAL: !AL_PLAYING alSourcePlay");
        alSourcePlay(source);
    }
    return true;
}

int AudioOutputOpenAL::getPlayedCount()
{
    SCOPE_LOCK_CONTEXT();
    ALint processed = 0;
    alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);
    return processed;
}

bool AudioOutputOpenAL::setVolume(qreal value)
{
    SCOPE_LOCK_CONTEXT();
    AL_ENSURE(alListenerf(AL_GAIN, value), false);
    return true;
}

qreal AudioOutputOpenAL::getVolume() const
{
    SCOPE_LOCK_CONTEXT();
    ALfloat v = 1.0;
    alGetListenerf(AL_GAIN, &v);
    ALenum err = alGetError();
    if (err != AL_NO_ERROR) {
        qWarning("AudioOutputOpenAL Error>>> getVolume (%d) : %s", err, alGetString(err));
    }
    return v;
}

int AudioOutputOpenAL::getQueued()
{
    SCOPE_LOCK_CONTEXT();
    ALint queued = 0;
    alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);
    return queued;
}

} //namespace QtAV
