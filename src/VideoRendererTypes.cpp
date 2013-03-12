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

#include "VideoRendererTypes.h"
#include <cstdio>
#include <cstdlib>
#include "prepost.h"

#include <QtAV/WidgetRenderer.h>
#include <QtAV/GLWidgetRenderer.h>
#if HAVE_GDIPLUS
#include <QtAV/GDIRenderer.h>
#endif //HAVE_GDIPLUS
#if HAVE_DIRECT2D
#include <QtAV/Direct2DRenderer.h>
#endif //HAVE_DIRECT2D
#include <QtAV/factory.h>

namespace QtAV {

FACTORY_DEFINE(VideoRenderer)


FACTORY_REGISTER_ID_AUTO(VideoRenderer, Widget, "QWidegt")

void RegisterVideoRendererWidget_Man()
{
    FACTORY_REGISTER_ID_MAN(VideoRenderer, Widget, "QWidegt")
}

FACTORY_REGISTER_ID_AUTO(VideoRenderer, GLWidget, "QGLWidegt")

void RegisterVideoRendererGLWidget_Man()
{
    FACTORY_REGISTER_ID_MAN(VideoRenderer, GLWidget, "QGLWidegt")
}

#if HAVE_GDIPLUS
FACTORY_REGISTER_ID_AUTO(VideoRenderer, GDI, "GDI")

void RegisterVideoRendererGDI_Man()
{
    FACTORY_REGISTER_ID_MAN(VideoRenderer, GDI, "GDI")
}
#endif //HAVE_GDIPLUS
#if HAVE_DIRECT2D
FACTORY_REGISTER_ID_AUTO(VideoRenderer, Direct2D, "Direct2D")

void RegisterVideoRendererDirect2D_Man()
{
    FACTORY_REGISTER_ID_MAN(VideoRenderer, Direct2D, "Direct2D")
}
#endif //HAVE_DIRECT2D

void VideoRenderer_RegisterAll()
{
    RegisterVideoRendererWidget_Man();
    RegisterVideoRendererGLWidget_Man();
#if HAVE_GDIPLUS
    RegisterVideoRendererGDI_Man();
#endif //HAVE_GDIPLUS
#if HAVE_DIRECT2D
    RegisterVideoRendererDirect2D_Man();
#endif //HAVE_DIRECT2D
}


}
