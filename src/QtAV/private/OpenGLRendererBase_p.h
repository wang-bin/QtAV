/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2014-2016 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_OPENGLRENDERERBASE_P_H
#define QTAV_OPENGLRENDERERBASE_P_H

#include "private/VideoRenderer_p.h"
#include "QtAV/OpenGLVideo.h"

namespace QtAV {

class Q_AV_PRIVATE_EXPORT OpenGLRendererBasePrivate : public VideoRendererPrivate
{
public:
    OpenGLRendererBasePrivate(QPaintDevice *pd);
    virtual ~OpenGLRendererBasePrivate();
    void setupAspectRatio();

    QPainter *painter;
    OpenGLVideo glv;
    QMatrix4x4 matrix;
    bool frame_changed;
};

} //namespace QtAV

#endif // QTAV_OpenGLRendererBase_P_H
