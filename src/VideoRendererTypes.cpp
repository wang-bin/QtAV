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
#include <QtAV/GraphicsItemRenderer.h>
#if HAVE_GL
#include <QtAV/GLWidgetRenderer.h>
#endif //HAVE_GL
#if HAVE_GDIPLUS
#include <QtAV/GDIRenderer.h>
#endif //HAVE_GDIPLUS
#if HAVE_DIRECT2D
#include <QtAV/Direct2DRenderer.h>
#endif //HAVE_DIRECT2D
#if HAVE_XV
#include <QtAV/XVRenderer.h>
#endif //HAVE_XV
#include <QtAV/factory.h>

namespace QtAV {

FACTORY_DEFINE(VideoRenderer)

VideoRendererId VideoRendererId_QPainter = 0;
VideoRendererId VideoRendererId_Widget = 1;
VideoRendererId VideoRendererId_GraphicsItem = 2;
VideoRendererId VideoRendererId_GLWidget = 3;
VideoRendererId VideoRendererId_GDI = 4;
VideoRendererId VideoRendererId_Direct2D = 5;
VideoRendererId VideoRendererId_XV = 6;

FACTORY_REGISTER_ID_AUTO(VideoRenderer, Widget, "QWidegt")

void RegisterVideoRendererWidget_Man()
{
    FACTORY_REGISTER_ID_MAN(VideoRenderer, Widget, "QWidegt")
}

VideoRendererId WidgetRenderer::id() const
{
    return VideoRendererId_Widget;
}

VideoRendererId GraphicsItemRenderer::id() const
{
    return VideoRendererId_GraphicsItem;
}

VideoRendererId QPainterRenderer::id() const
{
    return VideoRendererId_QPainter;
}

#if HAVE_GL
FACTORY_REGISTER_ID_AUTO(VideoRenderer, GLWidget, "QGLWidegt")

void RegisterVideoRendererGLWidget_Man()
{
    FACTORY_REGISTER_ID_MAN(VideoRenderer, GLWidget, "QGLWidegt")
}

VideoRendererId GLWidgetRenderer::id() const
{
    return VideoRendererId_GLWidget;
}
#endif //HAVE_GL
#if HAVE_GDIPLUS
FACTORY_REGISTER_ID_AUTO(VideoRenderer, GDI, "GDI")

void RegisterVideoRendererGDI_Man()
{
    FACTORY_REGISTER_ID_MAN(VideoRenderer, GDI, "GDI")
}

VideoRendererId GDIRenderer::id() const
{
    return VideoRendererId_GDI;
}
#endif //HAVE_GDIPLUS
#if HAVE_DIRECT2D
FACTORY_REGISTER_ID_AUTO(VideoRenderer, Direct2D, "Direct2D")

void RegisterVideoRendererDirect2D_Man()
{
    FACTORY_REGISTER_ID_MAN(VideoRenderer, Direct2D, "Direct2D")
}

VideoRendererId Direct2DRenderer::id() const
{
    return VideoRendererId_Direct2D;
}
#endif //HAVE_DIRECT2D
#if HAVE_XV
FACTORY_REGISTER_ID_AUTO(VideoRenderer, XV, "XVideo")

void RegisterVideoRendererXV_Man()
{
    FACTORY_REGISTER_ID_MAN(VideoRenderer, XV, "XVideo")
}

VideoRendererId XVRenderer::id() const
{
    return VideoRendererId_XV;
}
#endif //HAVE_XV

void VideoRenderer_RegisterAll()
{
    RegisterVideoRendererWidget_Man();
#if HAVE_GL
    RegisterVideoRendererGLWidget_Man();
#endif //HAVE_GL
#if HAVE_GDIPLUS
    RegisterVideoRendererGDI_Man();
#endif //HAVE_GDIPLUS
#if HAVE_DIRECT2D
    RegisterVideoRendererDirect2D_Man();
#endif //HAVE_DIRECT2D
#if HAVE_XV
    RegisterVideoRendererXV_Man();
#endif //HAVE_XV
}

#if ID_STATIC
namespace {
static void FixUnusedCompileWarning()
{
    FixUnusedCompileWarning(); //avoid warning about this function may not be used
    Q_UNUSED(VideoRendererId_GDI);
    Q_UNUSED(VideoRendererId_GLWidget);
    Q_UNUSED(VideoRendererId_Direct2D);
    Q_UNUSED(VideoRendererId_XV);
}
}//namespace
#endif //ID_STATIC
}//namespace QtAV
