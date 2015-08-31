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
#ifndef QT_NO_OPENGL
#include "QtAV/OpenGLWindowRenderer.h"
#endif //QT_NO_OPENGL
#endif
#include "QtAV/private/factory.h"
#include "QtAV/private/mkid.h"

namespace QtAV {

FACTORY_DEFINE(VideoRenderer)

VideoRendererId VideoRendererId_OpenGLWindow = mkid::id32base36_6<'Q', 'O', 'G', 'L', 'W', 'w'>::value;

#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
#ifndef QT_NO_OPENGL
FACTORY_REGISTER_ID_AUTO(VideoRenderer, OpenGLWindow, "OpenGLWindow")

void RegisterVideoRendererOpenGLWindow_Man()
{
    FACTORY_REGISTER_ID_MAN(VideoRenderer, OpenGLWindow, "OpenGLWindow")
}

VideoRendererId OpenGLWindowRenderer::id() const
{
    return VideoRendererId_OpenGLWindow;
}
#endif //QT_NO_OPENGL
#endif //qt5.4.0

void VideoRenderer_RegisterAll()
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
#ifndef QT_NO_OPENGL
    RegisterVideoRendererOpenGLWindow_Man();
#endif //QT_NO_OPENGL
#endif
}
}//namespace QtAV
