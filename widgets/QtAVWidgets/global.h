/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2015 Wang Bin <wbsecg1@gmail.com>

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


#ifndef QTAVWIDGETS_GLOBAL_H
#define QTAVWIDGETS_GLOBAL_H

#include <QtAV/VideoRendererTypes.h>

#if defined(BUILD_QTAVWIDGETS_LIB)
#  undef Q_AVWIDGETS_EXPORT
#  define Q_AVWIDGETS_EXPORT Q_DECL_EXPORT
#else
#  undef Q_AVWIDGETS_EXPORT
#  define Q_AVWIDGETS_EXPORT Q_DECL_IMPORT //only for vc?
#endif
#define Q_AVWIDGETS_PRIVATE_EXPORT Q_AVWIDGETS_EXPORT

namespace QtAV {
extern Q_AVWIDGETS_EXPORT VideoRendererId VideoRendererId_Widget;
extern Q_AVWIDGETS_EXPORT VideoRendererId VideoRendererId_GraphicsItem;
extern Q_AVWIDGETS_EXPORT VideoRendererId VideoRendererId_GLWidget;
extern Q_AVWIDGETS_EXPORT VideoRendererId VideoRendererId_GDI;
extern Q_AVWIDGETS_EXPORT VideoRendererId VideoRendererId_Direct2D;
extern Q_AVWIDGETS_EXPORT VideoRendererId VideoRendererId_XV;
extern Q_AVWIDGETS_EXPORT VideoRendererId VideoRendererId_GLWidget2;
extern Q_AVWIDGETS_EXPORT VideoRendererId VideoRendererId_OpenGLWidget;

//popup a dialog
Q_AVWIDGETS_EXPORT void about();
Q_AVWIDGETS_EXPORT void aboutFFmpeg();
Q_AVWIDGETS_EXPORT void aboutQtAV();
namespace Widgets {
/*!
 * \brief registerRenderers
 * register built-in renderers.
 * If you do not explicitly use any var, function or class in this module in your code,
 * QtAVWidgets module maybe not linked to your program and renderers will not be available.
 * Then you have to call registerRenderers() to ensure QtAVWidgets module is linked.
 */
Q_AVWIDGETS_EXPORT void registerRenderers();
} // namespace Widgets
} // namespace QtAV
#endif //QTAVWIDGETS_GLOBAL_H
