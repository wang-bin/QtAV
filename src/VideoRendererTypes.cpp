/******************************************************************************
    VideoRendererTypes: type id and manually id register function
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>

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
#include "QtAV/prepost.h"
#include "QtAV/WidgetRenderer.h"
#include "QtAV/GraphicsItemRenderer.h"
#if QTAV_HAVE(GL)
#include "QtAV/GLWidgetRenderer2.h"
#include "QtAV/GLWidgetRenderer.h"
#endif //QTAV_HAVE(GL)
#include "QtAV/private/factory.h"

namespace QtAV {

FACTORY_DEFINE(VideoRenderer)

VideoRendererId VideoRendererId_QPainter = 1;
VideoRendererId VideoRendererId_Widget = 2;
VideoRendererId VideoRendererId_GraphicsItem = 3;
VideoRendererId VideoRendererId_GLWidget = 4;
VideoRendererId VideoRendererId_GDI = 5;
VideoRendererId VideoRendererId_Direct2D = 6;
VideoRendererId VideoRendererId_XV = 7;
VideoRendererId VideoRendererId_GLWidget2 = 8;

//QPainterRenderer is abstract. So can not register(operator new will needed)
FACTORY_REGISTER_ID_AUTO(VideoRenderer, Widget, "QWidegt")

void RegisterVideoRendererWidget_Man()
{
    FACTORY_REGISTER_ID_MAN(VideoRenderer, Widget, "QWidegt")
}

FACTORY_REGISTER_ID_AUTO(VideoRenderer, GraphicsItem, "QGraphicsItem")

void RegisterVideoRendererGraphicsItem_Man()
{
    FACTORY_REGISTER_ID_MAN(VideoRenderer, GraphicsItem, "QGraphicsItem")
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

#if QTAV_HAVE(GL)
FACTORY_REGISTER_ID_AUTO(VideoRenderer, GLWidget, "QGLWidegt")

void RegisterVideoRendererGLWidget_Man()
{
    FACTORY_REGISTER_ID_MAN(VideoRenderer, GLWidget, "QGLWidegt")
}

VideoRendererId GLWidgetRenderer::id() const
{
    return VideoRendererId_GLWidget;
}

FACTORY_REGISTER_ID_AUTO(VideoRenderer, GLWidget2, "QGLWidegt2")

void RegisterVideoRendererGLWidget2_Man()
{
    FACTORY_REGISTER_ID_MAN(VideoRenderer, GLWidget2, "QGLWidegt2")
}

VideoRendererId GLWidgetRenderer2::id() const
{
    return VideoRendererId_GLWidget2;
}
#endif //QTAV_HAVE(GL)

extern void RegisterVideoRendererGDI_Man();
extern void RegisterVideoRendererDirect2D_Man();
extern void RegisterVideoRendererXV_Man();

void VideoRenderer_RegisterAll()
{
    RegisterVideoRendererWidget_Man();
#if QTAV_HAVE(GL)
    RegisterVideoRendererGLWidget_Man();
#endif //QTAV_HAVE(GL)
#if QTAV_HAVE(GDIPLUS)
    RegisterVideoRendererGDI_Man();
#endif //QTAV_HAVE(GDIPLUS)
#if QTAV_HAVE(DIRECT2D)
    RegisterVideoRendererDirect2D_Man();
#endif //QTAV_HAVE(DIRECT2D)
#if QTAV_HAVE(XV)
    RegisterVideoRendererXV_Man();
#endif //QTAV_HAVE(XV)
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
