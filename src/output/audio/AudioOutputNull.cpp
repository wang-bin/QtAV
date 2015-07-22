/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2015 Wang Bin <wbsecg1@gmail.com>

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
#include "QtAV/private/prepost.h"

namespace QtAV {

static const char kName[] = "null";
class AudioOutputNull : public AudioOutputBackend
{
public:
    AudioOutputNull(QObject *parent = 0);
    QString name() const Q_DECL_OVERRIDE { return kName;}
    bool open() Q_DECL_OVERRIDE { return false;}
    bool close() Q_DECL_OVERRIDE { return false;}
    // TODO: check channel layout. Null supports channels>2
    BufferControl bufferControl() const Q_DECL_OVERRIDE { return Blocking;}
    bool write(const QByteArray& data) Q_DECL_OVERRIDE { return false;}
    bool play() Q_DECL_OVERRIDE { return false;}

};

typedef AudioOutputNull AudioOutputBackendNull;
static const AudioOutputBackendId AudioOutputBackendId_Null = mkid::id32base36_4<'n', 'u', 'l', 'l'>::value;
FACTORY_REGISTER_ID_AUTO(AudioOutputBackend, Null, kName)

void RegisterAudioOutputNull_Man()
{
    FACTORY_REGISTER_ID_MAN(AudioOutputBackend, Null, kName)
}

AudioOutputNull::AudioOutputNull(QObject *parent)
    : AudioOutputBackend(AudioOutput::DeviceFeatures(), parent)
{}

} //namespace QtAV
