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
#include "QtAV/private/prepost.h"

// TODO: move to an internal header
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0) || defined(QT_WIDGETS_LIB)
#ifndef QTAV_HAVE_WIDGETS
#define QTAV_HAVE_WIDGETS 1
#endif //QTAV_HAVE_WIDGETS
#endif

#if QTAV_HAVE(WIDGETS)
#include "QtAV/WidgetRenderer.h"
#include "QtAV/GraphicsItemRenderer.h"
#endif
#if QTAV_HAVE(GL)
#include "QtAV/GLWidgetRenderer2.h"
#endif //QTAV_HAVE(GL)
#if QTAV_HAVE(GL1)
#include "QtAV/GLWidgetRenderer.h"
#endif //QTAV_HAVE(GL1)
#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
#include "QtAV/OpenGLWindowRenderer.h"
#ifdef QT_WIDGETS_LIB
#include "QtAV/OpenGLWidgetRenderer.h"
#endif
#endif
#include "QtAV/private/factory.h"
#include "QtAV/private/mkid.h"

namespace QtAV {

FACTORY_DEFINE(VideoRenderer)

VideoRendererId VideoRendererId_Widget = mkid32base36_6<'W', 'i', 'd', 'g', 'e', 't'>::value;
VideoRendererId VideoRendererId_GraphicsItem = mkid32base36_6<'Q', 'G', 'r', 'a', 'p', 'h'>::value;
VideoRendererId VideoRendererId_GLWidget = mkid32base36_6<'Q', 'G', 'L', 'W', 't', '1'>::value;
VideoRendererId VideoRendererId_GDI = mkid32base36_3<'G', 'D', 'I'>::value;
VideoRendererId VideoRendererId_Direct2D = mkid32base36_3<'D', '2', 'D'>::value;
VideoRendererId VideoRendererId_XV = mkid32base36_6<'X', 'V', 'i', 'd', 'e', 'o'>::value;
VideoRendererId VideoRendererId_GLWidget2 = mkid32base36_6<'Q', 'G', 'L', 'W', 't', '2'>::value;
VideoRendererId VideoRendererId_OpenGLWindow = mkid32base36_6<'Q', 'O', 'G', 'L', 'W', 'w'>::value;
VideoRendererId VideoRendererId_OpenGLWidget = mkid32base36_6<'Q', 'O', 'G', 'L', 'W', 't'>::value;

#if QTAV_HAVE(WIDGETS)
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
#endif //QTAV_HAVE(WIDGETS)

#if QTAV_HAVE(GL)
#if QTAV_HAVE(GL1)
FACTORY_REGISTER_ID_AUTO(VideoRenderer, GLWidget, "QGLWidegt")

void RegisterVideoRendererGLWidget_Man()
{
    FACTORY_REGISTER_ID_MAN(VideoRenderer, GLWidget, "QGLWidegt")
}
VideoRendererId GLWidgetRenderer::id() const
{
    return VideoRendererId_GLWidget;
}
#endif //QTAV_HAVE(GL1)
#if QTAV_HAVE(WIDGETS)
FACTORY_REGISTER_ID_AUTO(VideoRenderer, GLWidget2, "QGLWidegt2")

void RegisterVideoRendererGLWidget2_Man()
{
    FACTORY_REGISTER_ID_MAN(VideoRenderer, GLWidget2, "QGLWidegt2")
}

VideoRendererId GLWidgetRenderer2::id() const
{
    return VideoRendererId_GLWidget2;
}
#endif
#endif //QTAV_HAVE(GL)
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
#ifdef QT_WIDGETS_LIB
FACTORY_REGISTER_ID_AUTO(VideoRenderer, OpenGLWidget, "OpenGLWidget")

void RegisterVideoRendererOpenGLWidget_Man()
{
    FACTORY_REGISTER_ID_MAN(VideoRenderer, OpenGLWidget, "OpenGLWidget")
}

VideoRendererId OpenGLWidgetRenderer::id() const
{
    return VideoRendererId_OpenGLWidget;
}
#endif //QT_WIDGETS_LIB
#endif

extern void RegisterVideoRendererGDI_Man();
extern void RegisterVideoRendererDirect2D_Man();
extern void RegisterVideoRendererXV_Man();

void VideoRenderer_RegisterAll()
{
#if QTAV_HAVE(WIDGETS)
    RegisterVideoRendererWidget_Man();
    RegisterVideoRendererGraphicsItem_Man();
#endif //QTAV_HAVE(WIDGETS)
#if QTAV_HAVE(GL)
    RegisterVideoRendererGLWidget2_Man();
#endif //QTAV_HAVE(GL)
#if QTAV_HAVE(GL1)
    RegisterVideoRendererGLWidget_Man();
#endif //QTAV_HAVE(GL1)
#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
    RegisterVideoRendererOpenGLWindow_Man();
#ifdef QT_WIDGETS_LIB

#endif //QT_WIDGETS_LIB
#endif
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
