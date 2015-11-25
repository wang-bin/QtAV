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

#include "QtAV/VideoDecoderTypes.h"
#include "QtAV/private/mkid.h"

namespace QtAV {

VideoDecoderId VideoDecoderId_FFmpeg = mkid::id32base36_6<'F', 'F', 'm', 'p', 'e', 'g'>::value;
VideoDecoderId VideoDecoderId_CUDA = mkid::id32base36_4<'C', 'U', 'D', 'A'>::value;
VideoDecoderId VideoDecoderId_DXVA = mkid::id32base36_4<'D', 'X', 'V', 'A'>::value;
VideoDecoderId VideoDecoderId_VAAPI = mkid::id32base36_5<'V', 'A', 'A', 'P', 'I'>::value;
VideoDecoderId VideoDecoderId_Cedarv = mkid::id32base36_6<'C', 'e', 'd', 'a', 'r', 'V'>::value;
VideoDecoderId VideoDecoderId_VDA = mkid::id32base36_3<'V', 'D', 'A'>::value;
VideoDecoderId VideoDecoderId_VideoToolbox = mkid::id32base36_5<'V', 'T', 'B', 'o', 'x'>::value;
VideoDecoderId VideoDecoderId_VPU = mkid::id32base36_3<'V', 'P', 'U'>::value;

} //namespace QtAV
