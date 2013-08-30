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

#ifdef QT_QUICK_LIB
#include <QtAV/QQuickItemRenderer.h>
#endif //QT_QUICK_LIB
#include <QtAV/WidgetRenderer.h>
#include <QtAV/GraphicsItemRenderer.h>
#if QTAV_HAVE(GL)
#include <QtAV/GLWidgetRenderer.h>
#endif //QTAV_HAVE(GL)
#if QTAV_HAVE(GDIPLUS)
#include <QtAV/GDIRenderer.h>
#endif //QTAV_HAVE(GDIPLUS)
#if QTAV_HAVE(DIRECT2D)
#include <QtAV/Direct2DRenderer.h>
#endif //QTAV_HAVE(DIRECT2D)
#if QTAV_HAVE(XV)
#include <QtAV/XVRenderer.h>
#endif //QTAV_HAVE(XV)
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
VideoRendererId VideoRendererId_QQuickItem = 7;

#ifdef QT_QUICK_LIB
FACTORY_REGISTER_ID_AUTO(VideoRenderer, QQuickItem, "QQuickItem")

void RegisterVideoRendererQQuickItem_Man()
{
    FACTORY_REGISTER_ID_MAN(VideoRenderer, QQuickItem, "QQuickItem")
}
#endif //QT_QUICK_LIB
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
#ifdef QT_QUICK_LIB
VideoRendererId QQuickItemRenderer::id() const
{
    return VideoRendererId_QQuickItem;
}
#endif //QT_QUICK_LIB

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
#endif //QTAV_HAVE(GL)
#if QTAV_HAVE(GDIPLUS)
FACTORY_REGISTER_ID_AUTO(VideoRenderer, GDI, "GDI")

void RegisterVideoRendererGDI_Man()
{
    FACTORY_REGISTER_ID_MAN(VideoRenderer, GDI, "GDI")
}

VideoRendererId GDIRenderer::id() const
{
    return VideoRendererId_GDI;
}
#endif //QTAV_HAVE(GDIPLUS)
#if QTAV_HAVE(DIRECT2D)
FACTORY_REGISTER_ID_AUTO(VideoRenderer, Direct2D, "Direct2D")

void RegisterVideoRendererDirect2D_Man()
{
    FACTORY_REGISTER_ID_MAN(VideoRenderer, Direct2D, "Direct2D")
}

VideoRendererId Direct2DRenderer::id() const
{
    return VideoRendererId_Direct2D;
}
#endif //QTAV_HAVE(DIRECT2D)
#if QTAV_HAVE(XV)
FACTORY_REGISTER_ID_AUTO(VideoRenderer, XV, "XVideo")

void RegisterVideoRendererXV_Man()
{
    FACTORY_REGISTER_ID_MAN(VideoRenderer, XV, "XVideo")
}

VideoRendererId XVRenderer::id() const
{
    return VideoRendererId_XV;
}
#endif //QTAV_HAVE(XV)

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
#ifdef QT_QUICK_LIB
    RegisterVideoRendererQQuickItem_Man();
#endif //QT_QUICK_LIB
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
