/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012 Wang Bin <wbsecg1@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include <QtAV/AVDecoder.h>
#include <private/AVDecoder_p.h>

namespace QtAV {
AVDecoder::AVDecoder()
{
}

AVDecoder::AVDecoder(AVDecoderPrivate &d)
    :DPTR_INIT(&d)
{

}

AVDecoder::~AVDecoder()
{
}

void AVDecoder::setCodecContext(AVCodecContext *codecCtx)
{
    DPTR_D(AVDecoder);
    d.codec_ctx = codecCtx;
}

AVCodecContext* AVDecoder::codecContext() const
{
    return d_func().codec_ctx;
}

bool AVDecoder::decode(const QByteArray &encoded)
{
    return true;
}

QByteArray AVDecoder::data() const
{
    return d_func().decoded;
}

} //namespace QtAV
