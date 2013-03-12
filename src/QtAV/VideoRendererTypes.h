/******************************************************************************
    VideoRendererTypes: type id and manually id register function
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

#ifndef QTAV_VIDEORENDERERTYPES_H
#define QTAV_VIDEORENDERERTYPES_H

#include "VideoRenderer.h"
#include <QtAV/FactoryDefine.h>

namespace QtAV {

typedef int VideoRendererId;
class VideoRenderer;
FACTORY_DECLARE(VideoRenderer)

static VideoRendererId VideoRendererId_Widget = 0;
static VideoRendererId VideoRendererId_GLWidget = 1;
static VideoRendererId VideoRendererId_GDI = 2;
static VideoRendererId VideoRendererId_Direct2D = 3;

Q_EXPORT void VideoRenderer_RegisterAll();

} //namespace QtAV

#endif // QTAV_VIDEORENDERERTYPES_H
