/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
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

#ifndef QTAV_GLWIDGETRENDERER_P_H
#define QTAV_GLWIDGETRENDERER_P_H

#include "private/VideoRenderer_p.h"

namespace QtAV {

class GLWidgetRendererPrivate : public VideoRendererPrivate
{
public:
    GLWidgetRendererPrivate():
        texture(0)
    {
        if (QGLFormat::openGLVersionFlags() == QGLFormat::OpenGL_Version_None) {
            available = false;
            return;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
    }
    ~GLWidgetRendererPrivate() {
        glDeleteTextures(1, &texture);
    }

    QRect out_rect_old;
    GLuint texture;
};

} //namespace QtAV
#endif // QTAV_GLWIDGETRENDERER_P_H
