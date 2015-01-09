/******************************************************************************
    VideoRendererTypes: type id and manually id register function
    Copyright (C) 2012-2015 Wang Bin <wbsecg1@gmail.com>

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

#include "QtAV/VideoRendererTypes.h"
#include <cstdio>
#include <cstdlib>
#include "QtAV/private/prepost.h"
#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
#include "QtAV/OpenGLWindowRenderer.h"
#endif
#include "QtAV/private/factory.h"
#include "QtAV/private/mkid.h"

namespace QtAV {

FACTORY_DEFINE(VideoRenderer)

VideoRendererId VideoRendererId_Widget = mkid::id32base36_6<'W', 'i', 'd', 'g', 'e', 't'>::value;
VideoRendererId VideoRendererId_GraphicsItem = mkid::id32base36_6<'Q', 'G', 'r', 'a', 'p', 'h'>::value;
VideoRendererId VideoRendererId_GLWidget = mkid::id32base36_6<'Q', 'G', 'L', 'W', 't', '1'>::value;
VideoRendererId VideoRendererId_GDI = mkid::id32base36_3<'G', 'D', 'I'>::value;
VideoRendererId VideoRendererId_Direct2D = mkid::id32base36_3<'D', '2', 'D'>::value;
VideoRendererId VideoRendererId_XV = mkid::id32base36_6<'X', 'V', 'i', 'd', 'e', 'o'>::value;
VideoRendererId VideoRendererId_GLWidget2 = mkid::id32base36_6<'Q', 'G', 'L', 'W', 't', '2'>::value;
VideoRendererId VideoRendererId_OpenGLWindow = mkid::id32base36_6<'Q', 'O', 'G', 'L', 'W', 'w'>::value;
VideoRendererId VideoRendererId_OpenGLWidget = mkid::id32base36_6<'Q', 'O', 'G', 'L', 'W', 't'>::value;

#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
FACTORY_REGISTER_ID_AUTO(VideoRenderer, OpenGLWindow, "OpenGLWindow")

void RegisterVideoRendererOpenGLWindow_Man()
{
    FACTORY_REGISTER_ID_MAN(VideoRenderer, OpenGLWindow, "OpenGLWindow")
}

VideoRendererId OpenGLWindowRenderer::id() const
{
    return VideoRendererId_OpenGLWindow;
}
#endif

void VideoRenderer_RegisterAll()
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
    RegisterVideoRendererOpenGLWindow_Man();
#endif
}
}//namespace QtAV
