/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2013)

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

#ifndef QTAV_AUDIORESAMPLER_P_H
#define QTAV_AUDIORESAMPLER_P_H

#include "QtAV/AudioFormat.h"
#include "QtAV/private/AVCompat.h"
#include <QtCore/QByteArray>

namespace QtAV {

class AudioResampler;
class Q_AV_PRIVATE_EXPORT AudioResamplerPrivate : public DPtrPrivate<AudioResampler>
{
public:
    AudioResamplerPrivate():
        in_samples_per_channel(0)
      , out_samples_per_channel(0)
      , speed(1.0)
    {
        in_format.setSampleFormat(AudioFormat::SampleFormat_Unknown);
        out_format.setSampleFormat(AudioFormat::SampleFormat_Float);
    }

    int in_samples_per_channel, out_samples_per_channel;
    qreal speed;
    AudioFormat in_format, out_format;
    QByteArray data_out;
};

} //namespace QtAV

#endif // QTAV_AUDIORESAMPLER_P_H
