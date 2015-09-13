/******************************************************************************
    VideoRendererTypes: type id and manually id register function
    Copyright (C) 2013-2015 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_VIDEODECODERTYPES_H
#define QTAV_VIDEODECODERTYPES_H

#include <QtAV/VideoDecoder.h>

namespace QtAV {

extern Q_AV_EXPORT VideoDecoderId VideoDecoderId_FFmpeg;
extern Q_AV_EXPORT VideoDecoderId VideoDecoderId_CUDA;
extern Q_AV_EXPORT VideoDecoderId VideoDecoderId_DXVA;
extern Q_AV_EXPORT VideoDecoderId VideoDecoderId_VAAPI;
extern Q_AV_EXPORT VideoDecoderId VideoDecoderId_Cedarv;
extern Q_AV_EXPORT VideoDecoderId VideoDecoderId_FFmpeg_VDPAU;
extern Q_AV_EXPORT VideoDecoderId VideoDecoderId_VDA;
extern Q_AV_EXPORT VideoDecoderId VideoDecoderId_VideoToolbox;


Q_AV_EXPORT void VideoDecoder_RegisterAll();

} //namespace QtAV
#endif // QTAV_VIDEODECODERTYPES_H
