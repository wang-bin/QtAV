/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2014)

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


#include "QtAV/CommonTypes.h"
#include "QtAV/private/AVCompat.h"
/*
 * AVColorSpace:
 * libav11 libavutil54.3.0 pixfmt.h, ffmpeg2.1*libavutil52.48.101 frame.h
 * ffmpeg2.5 pixfmt.h. AVFrame.colorspace
 * earlier versions: avcodec.h, avctx.colorspace
 */
namespace QtAV {

namespace
{
static const struct RegisterMetaTypes
{
    RegisterMetaTypes()
    {
        qRegisterMetaType<QtAV::MediaStatus>("QtAV::MediaStatus");
    }
} _registerMetaTypes;
}

ColorSpace colorSpaceFromFFmpeg(AVColorSpace cs)
{
    switch (cs) {
    // from ffmpeg: order of coefficients is actually GBR
    case AVCOL_SPC_RGB: return ColorSpace_GBR;
    case AVCOL_SPC_BT709: return ColorSpace_BT709;
    case AVCOL_SPC_BT470BG: return ColorSpace_BT601;
    case AVCOL_SPC_SMPTE170M: return ColorSpace_BT601;
    default: return ColorSpace_Unknown;
    }
}

}
