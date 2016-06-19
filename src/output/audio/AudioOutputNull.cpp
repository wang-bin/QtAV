/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

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
#include "QtAV/private/mkid.h"
#include "QtAV/private/factory.h"

namespace QtAV {
//TODO: block internally
static const char kName[] = "null";
class AudioOutputNull : public AudioOutputBackend
{
public:
    AudioOutputNull(QObject *parent = 0);
    QString name() const Q_DECL_OVERRIDE { return QLatin1String(kName);}
    bool open() Q_DECL_OVERRIDE { return true;}
    bool close() Q_DECL_OVERRIDE { return true;}
    // TODO: check channel layout. Null supports channels>2
    BufferControl bufferControl() const Q_DECL_OVERRIDE { return Blocking;}
    bool write(const QByteArray&) Q_DECL_OVERRIDE { return true;}
    bool play() Q_DECL_OVERRIDE { return true;}

};

typedef AudioOutputNull AudioOutputBackendNull;
static const AudioOutputBackendId AudioOutputBackendId_Null = mkid::id32base36_4<'n', 'u', 'l', 'l'>::value;
FACTORY_REGISTER(AudioOutputBackend, Null, kName)

AudioOutputNull::AudioOutputNull(QObject *parent)
    : AudioOutputBackend(AudioOutput::DeviceFeatures(), parent)
{}

} //namespace QtAV
