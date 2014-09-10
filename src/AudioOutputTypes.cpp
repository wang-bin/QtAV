/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>

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

#include "QtAV/AudioOutputTypes.h"
#include "QtAV/FactoryDefine.h"
#include "QtAV/private/factory.h"
#include "QtAV/private/mkid.h"
namespace QtAV {

AudioOutputId AudioOutputId_PortAudio = mkid32base36_5<'P', 'o', 'r', 't', 'A'>::value;
AudioOutputId AudioOutputId_OpenAL = mkid32base36_6<'O', 'p', 'e', 'n', 'A', 'L'>::value;
AudioOutputId AudioOutputId_OpenSL = mkid32base36_6<'O', 'p', 'e', 'n', 'S', 'L'>::value;
AudioOutputId AudioOutputId_DSound = mkid32base36_6<'D', 'S', 'o', 'u', 'n', 'd'>::value;

QVector<AudioOutputId> GetRegistedAudioOutputIds()
{
    return QVector<AudioOutputId>::fromStdVector(AudioOutputFactory::registeredIds());
}


FACTORY_DEFINE(AudioOutput)

extern void RegisterAudioOutputPortAudio_Man();
extern void RegisterAudioOutputOpenAL_Man();
extern void RegisterAudioOutputOpenSL_Man();
extern void RegisterAudioOutputDSound_Man();

void AudioOutput_RegisterAll()
{
#if QTAV_HAVE(PORTAUDIO)
    RegisterAudioOutputPortAudio_Man();
#endif //QTAV_HAVE(PORTAUDIO)
#if QTAV_HAVE(OPENAL)
    RegisterAudioOutputOpenAL_Man();
#endif //QTAV_HAVE(OPENAL)
#if QTAV_HAVE(OPENSL)
    RegisterAudioOutputOpenSL_Man();
#endif //QTAV_HAVE(OPENSL)
#if QTAV_HAVE(DSOUND)
    extern void RegisterAudioOutputDSound_Man();
#endif
}

} //namespace QtAV
