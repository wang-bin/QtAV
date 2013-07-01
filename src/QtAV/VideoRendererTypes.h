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

#include <QtAV/VideoRenderer.h>
#include <QtAV/FactoryDefine.h>

namespace QtAV {

class VideoRenderer;
FACTORY_DECLARE(VideoRenderer)

//Q_EXPORT(dllexport/import) is needed if used out of the library
//TODO graphics item?
extern Q_EXPORT VideoRendererId VideoRendererId_QPainter;
extern Q_EXPORT VideoRendererId VideoRendererId_Widget;
extern Q_EXPORT VideoRendererId VideoRendererId_GraphicsItem;
extern Q_EXPORT VideoRendererId VideoRendererId_GLWidget;
extern Q_EXPORT VideoRendererId VideoRendererId_GDI;
extern Q_EXPORT VideoRendererId VideoRendererId_Direct2D;
extern Q_EXPORT VideoRendererId VideoRendererId_XV;

Q_EXPORT void VideoRenderer_RegisterAll();

} //namespace QtAV

#endif // QTAV_VIDEORENDERERTYPES_H
