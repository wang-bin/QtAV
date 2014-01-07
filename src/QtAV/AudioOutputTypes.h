/******************************************************************************
    VideoRendererTypes: type id and manually id register function
    Copyright (C) 2014 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_AUDIOOUTPUTTYPES_H
#define QTAV_AUDIOOUTPUTTYPES_H

#include <QtAV/AudioOutput.h>
#include <QtCore/QVector>

namespace QtAV {

/*!
 * \brief GetRegisted
 * \return count of available id
 *  if pass a null ids, only return the count. otherwise regitered ids will be stored in ids
 */
Q_AV_EXPORT QVector<AudioOutputId> GetRegistedAudioOutputIds();

extern Q_AV_EXPORT AudioOutputId AudioOutputId_PortAudio;
extern Q_AV_EXPORT AudioOutputId AudioOutputId_OpenAL;
extern Q_AV_EXPORT AudioOutputId AudioOutputId_OpenSL;


Q_AV_EXPORT void AudioOutput_RegisterAll();

} //namespace QtAV
#endif // QTAV_AUDIOOUTPUTTYPES_H
