/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_AUDIORESAMPLER_P_H
#define QTAV_AUDIORESAMPLER_P_H


#include <QtAV/QtAV_Compat.h>
#include <QtCore/QByteArray>

namespace QtAV {

class AudioResampler;
class AudioResamplerPrivate : public DPtrPrivate<AudioResampler>
{
public:
    AudioResamplerPrivate():
        in_channel_layout(0)
      , out_channel_layout(0)
      , in_nb_channels(0)
      , out_nb_channels(0)
      , in_samples_per_channel(0)
      , in_sample_rate(0)
      , out_sample_rate(0)
      , in_sample_format(AV_SAMPLE_FMT_NONE)
      , out_sample_format(AV_SAMPLE_FMT_FLT)
      , in_planes(0)
      , out_planes(0)
      , speed(1.0)
    {

    }

    int in_channel_layout, out_channel_layout;
    int in_nb_channels, out_nb_channels;
    int in_samples_per_channel;
    int in_sample_rate, out_sample_rate;
    int in_sample_format, out_sample_format; //AVSampleFormat
    int in_planes, out_planes;
    qreal speed;
    QByteArray data_out;
};

} //namespace QtAV

#endif // QTAV_AUDIORESAMPLER_P_H
