/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2017 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2015)

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
#include "QtAV/private/factory.h"
#include "utils/Logger.h"

namespace QtAV {

QStringList AudioOutputBackend::defaultPriority()
{
    static const QStringList sBackends = QStringList()
#ifdef Q_OS_MAC
            << QStringLiteral("AudioToolbox")
#endif
#if QTAV_HAVE(XAUDIO2)
            << QStringLiteral("XAudio2")
#endif
#if QTAV_HAVE(OPENSL)
            << QStringLiteral("OpenSL")
#endif
#if QTAV_HAVE(OPENAL)
            << QStringLiteral("OpenAL")
#endif
#if QTAV_HAVE(PORTAUDIO)
            << QStringLiteral("PortAudio")
#endif
#if QTAV_HAVE(PULSEAUDIO)&& !defined(Q_OS_MAC)
            << QStringLiteral("Pulse")
#endif
#if QTAV_HAVE(DSOUND)
            << QStringLiteral("DirectSound")
#endif
              ;
    return sBackends;
}

AudioOutputBackend::AudioOutputBackend(AudioOutput::DeviceFeatures f, QObject *parent)
    : QObject(parent)
    , audio(0)
    , available(true)
    , buffer_size(0)
    , buffer_count(0)
    , m_features(f)
{}

void AudioOutputBackend::onCallback()
{
    if (!audio)
        return;
    audio->onCallback();
}


FACTORY_DEFINE(AudioOutputBackend)

void AudioOutput_RegisterAll()
{
    static bool initialized = false;
    if (initialized)
        return;
    initialized = true;
    // check whether ids are registered automatically
    if (!AudioOutputBackendFactory::Instance().registeredIds().empty())
        return;
    extern bool RegisterAudioOutputBackendNull_Man();
    RegisterAudioOutputBackendNull_Man();
#ifdef Q_OS_DARWIN
    extern bool RegisterAudioOutputBackendAudioToolbox_Man();
    RegisterAudioOutputBackendAudioToolbox_Man();
#endif
#if QTAV_HAVE(OPENSL)
    extern bool RegisterAudioOutputBackendOpenSL_Man();
    RegisterAudioOutputBackendOpenSL_Man();
#endif //QTAV_HAVE(OPENSL)
#if QTAV_HAVE(XAUDIO2)
    extern bool RegisterAudioOutputBackendXAudio2_Man();
    RegisterAudioOutputBackendXAudio2_Man();
#endif
#if QTAV_HAVE(OPENAL)
    extern bool RegisterAudioOutputBackendOpenAL_Man();
    RegisterAudioOutputBackendOpenAL_Man();
#endif //QTAV_HAVE(OPENAL)
#if QTAV_HAVE(PULSEAUDIO)
    extern bool RegisterAudioOutputBackendPulse_Man();
    RegisterAudioOutputBackendPulse_Man();
#endif
#if QTAV_HAVE(PORTAUDIO)
    extern bool RegisterAudioOutputBackendPortAudio_Man();
    RegisterAudioOutputBackendPortAudio_Man();
#endif //QTAV_HAVE(PORTAUDIO)
#if QTAV_HAVE(DSOUND)
    extern bool RegisterAudioOutputBackendDSound_Man();
    RegisterAudioOutputBackendDSound_Man();
#endif
}

} //namespace QtAV
