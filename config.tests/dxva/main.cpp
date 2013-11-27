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

extern "C" {
#include <libavcodec/dxva2.h> //will include d3d9.h, dxva2api.h
}
#define VA_DXVA2_MAX_SURFACE_COUNT (64)

#include <d3d9.h>
#include <dxva2api.h>

#define MS_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    static const GUID name = { l, w1, w2, {b1, b2, b3, b4, b5, b6, b7, b8}}
#ifdef __MINGW32__
# include <_mingw.h>
# if defined(__MINGW64_VERSION_MAJOR)
#  include <dxva.h>
# endif
#endif /* __MINGW32__ */

int main()
{
	return 0;
}
