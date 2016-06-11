/******************************************************************************
    mkapi dynamic load code generation for capi template
    Copyright (C) 2014-2016 Wang Bin <wbsecg1@gmail.com>
    https://github.com/wang-bin/mkapi
    https://github.com/wang-bin/capi

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

#ifndef OPENAL_API_H
#define OPENAL_API_H

// no need to include the C header if only functions declared there
#ifndef CAPI_LINK_OPENAL
namespace openal {
namespace capi {
#define AL_LIBTYPE_STATIC // openal-soft AL_API dllimport error. mac's macro is AL_BUILD_LIBRARY
#else
extern "C" {
#endif
// the following line will be replaced by the content of config/OPENAL/include if exists
#ifdef __APPLE__
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif
#ifndef CAPI_LINK_OPENAL
}
#endif
}

namespace openal {
#ifndef CAPI_LINK_OPENAL
using namespace capi; // original header is in namespace capi, types are changed
#endif // CAPI_LINK_OPENAL
namespace capi { bool loaded();} // For link or NS style. Or load test for class style. api.loaded for class style.
class api_dll;
class api
{
    api_dll *dll;
public:
    api();
    virtual ~api();
    virtual bool loaded() const; // user may inherits multiple api classes: final::loaded() { return base1::loaded() && base2::loaded();}
#if !defined(CAPI_LINK_OPENAL) && !defined(OPENAL_CAPI_NS)
// mkapi code generation BEGIN
    void alDopplerFactor(ALfloat value);
    void alDopplerVelocity(ALfloat value);
    void alSpeedOfSound(ALfloat value);
    void alDistanceModel(ALenum distanceModel);
    void alEnable(ALenum capability);
    void alDisable(ALenum capability);
    ALboolean alIsEnabled(ALenum capability);
    const ALchar * alGetString(ALenum param);
    void alGetBooleanv(ALenum param, ALboolean * values);
    void alGetIntegerv(ALenum param, ALint * values);
    void alGetFloatv(ALenum param, ALfloat * values);
    void alGetDoublev(ALenum param, ALdouble * values);
    ALboolean alGetBoolean(ALenum param);
    ALint alGetInteger(ALenum param);
    ALfloat alGetFloat(ALenum param);
    ALdouble alGetDouble(ALenum param);
    ALenum alGetError();
    ALboolean alIsExtensionPresent(const ALchar * extname);
    void * alGetProcAddress(const ALchar * fname);
    ALenum alGetEnumValue(const ALchar * ename);
    void alListenerf(ALenum param, ALfloat value);
    void alListener3f(ALenum param, ALfloat value1, ALfloat value2, ALfloat value3);
    void alListenerfv(ALenum param, const ALfloat * values);
    void alListeneri(ALenum param, ALint value);
    void alListener3i(ALenum param, ALint value1, ALint value2, ALint value3);
    void alListeneriv(ALenum param, const ALint * values);
    void alGetListenerf(ALenum param, ALfloat * value);
    void alGetListener3f(ALenum param, ALfloat * value1, ALfloat * value2, ALfloat * value3);
    void alGetListenerfv(ALenum param, ALfloat * values);
    void alGetListeneri(ALenum param, ALint * value);
    void alGetListener3i(ALenum param, ALint * value1, ALint * value2, ALint * value3);
    void alGetListeneriv(ALenum param, ALint * values);
    void alGenSources(ALsizei n, ALuint * sources);
    void alDeleteSources(ALsizei n, const ALuint * sources);
    ALboolean alIsSource(ALuint source);
    void alSourcef(ALuint source, ALenum param, ALfloat value);
    void alSource3f(ALuint source, ALenum param, ALfloat value1, ALfloat value2, ALfloat value3);
    void alSourcefv(ALuint source, ALenum param, const ALfloat * values);
    void alSourcei(ALuint source, ALenum param, ALint value);
    void alSource3i(ALuint source, ALenum param, ALint value1, ALint value2, ALint value3);
    void alSourceiv(ALuint source, ALenum param, const ALint * values);
    void alGetSourcef(ALuint source, ALenum param, ALfloat * value);
    void alGetSource3f(ALuint source, ALenum param, ALfloat * value1, ALfloat * value2, ALfloat * value3);
    void alGetSourcefv(ALuint source, ALenum param, ALfloat * values);
    void alGetSourcei(ALuint source, ALenum param, ALint * value);
    void alGetSource3i(ALuint source, ALenum param, ALint * value1, ALint * value2, ALint * value3);
    void alGetSourceiv(ALuint source, ALenum param, ALint * values);
    void alSourcePlayv(ALsizei n, const ALuint * sources);
    void alSourceStopv(ALsizei n, const ALuint * sources);
    void alSourceRewindv(ALsizei n, const ALuint * sources);
    void alSourcePausev(ALsizei n, const ALuint * sources);
    void alSourcePlay(ALuint source);
    void alSourceStop(ALuint source);
    void alSourceRewind(ALuint source);
    void alSourcePause(ALuint source);
    void alSourceQueueBuffers(ALuint source, ALsizei nb, const ALuint * buffers);
    void alSourceUnqueueBuffers(ALuint source, ALsizei nb, ALuint * buffers);
    void alGenBuffers(ALsizei n, ALuint * buffers);
    void alDeleteBuffers(ALsizei n, const ALuint * buffers);
    ALboolean alIsBuffer(ALuint buffer);
    void alBufferData(ALuint buffer, ALenum format, const ALvoid * data, ALsizei size, ALsizei freq);
    void alBufferf(ALuint buffer, ALenum param, ALfloat value);
    void alBuffer3f(ALuint buffer, ALenum param, ALfloat value1, ALfloat value2, ALfloat value3);
    void alBufferfv(ALuint buffer, ALenum param, const ALfloat * values);
    void alBufferi(ALuint buffer, ALenum param, ALint value);
    void alBuffer3i(ALuint buffer, ALenum param, ALint value1, ALint value2, ALint value3);
    void alBufferiv(ALuint buffer, ALenum param, const ALint * values);
    void alGetBufferf(ALuint buffer, ALenum param, ALfloat * value);
    void alGetBuffer3f(ALuint buffer, ALenum param, ALfloat * value1, ALfloat * value2, ALfloat * value3);
    void alGetBufferfv(ALuint buffer, ALenum param, ALfloat * values);
    void alGetBufferi(ALuint buffer, ALenum param, ALint * value);
    void alGetBuffer3i(ALuint buffer, ALenum param, ALint * value1, ALint * value2, ALint * value3);
    void alGetBufferiv(ALuint buffer, ALenum param, ALint * values);
    ALCcontext * alcCreateContext(ALCdevice * device, const ALCint* attrlist);
    ALCboolean alcMakeContextCurrent(ALCcontext * context);
    void alcProcessContext(ALCcontext * context);
    void alcSuspendContext(ALCcontext * context);
    void alcDestroyContext(ALCcontext * context);
    ALCcontext * alcGetCurrentContext();
    ALCdevice * alcGetContextsDevice(ALCcontext * context);
    ALCdevice * alcOpenDevice(const ALCchar * devicename);
    ALCboolean alcCloseDevice(ALCdevice * device);
    ALCenum alcGetError(ALCdevice * device);
    ALCboolean alcIsExtensionPresent(ALCdevice * device, const ALCchar * extname);
    void * alcGetProcAddress(ALCdevice * device, const ALCchar * funcname);
    ALCenum alcGetEnumValue(ALCdevice * device, const ALCchar * enumname);
    const ALCchar * alcGetString(ALCdevice * device, ALCenum param);
    void alcGetIntegerv(ALCdevice * device, ALCenum param, ALCsizei size, ALCint * values);
    ALCdevice * alcCaptureOpenDevice(const ALCchar * devicename, ALCuint frequency, ALCenum format, ALCsizei buffersize);
    ALCboolean alcCaptureCloseDevice(ALCdevice * device);
    void alcCaptureStart(ALCdevice * device);
    void alcCaptureStop(ALCdevice * device);
    void alcCaptureSamples(ALCdevice * device, ALCvoid * buffer, ALCsizei samples);
// mkapi code generation END
#endif // !defined(CAPI_LINK_OPENAL) && !defined(OPENAL_CAPI_NS)
};
} //namespace openal

#ifndef OPENAL_CAPI_BUILD // avoid ambiguous in openal_api.cpp
#if defined(OPENAL_CAPI_NS) && !defined(CAPI_LINK_OPENAL)
using namespace openal::capi;
#else
using namespace openal;
#endif
#endif //OPENAL_CAPI_BUILD
#endif // OPENAL_API_H
//this file is generated by "mkapi.sh -name openal /Users/wangbin/dev/openal-soft/include/AL/al.h /Users/wangbin/dev/openal-soft/include/AL/alc.h"
