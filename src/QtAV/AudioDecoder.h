/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2013 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QAV_AUDIODECODER_H
#define QAV_AUDIODECODER_H

#include <QtAV/AVDecoder.h>

namespace QtAV {

class AudioResampler;
class AudioDecoderPrivate;
class Q_EXPORT AudioDecoder : public AVDecoder
{
    DPTR_DECLARE_PRIVATE(AudioDecoder)
public:
    AudioDecoder();
    virtual bool prepare();
    virtual bool decode(const QByteArray &encoded);
    AudioResampler *resampler();
};

} //namespace QtAV
#endif // QAV_AUDIODECODER_H
