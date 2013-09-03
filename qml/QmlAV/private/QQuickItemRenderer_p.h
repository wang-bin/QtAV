/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013 Wang Bin <wbsecg1@gmail.com>
    theoribeiro <theo@fictix.com.br>

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

#ifndef QTAV_QML_QQUICKRENDERER_P_H
#define QTAV_QML_QQUICKRENDERER_P_H

#include <QtAV/private/VideoRenderer_p.h>
#include <QtQuick/QSGTexture>


namespace QtAV
{

class QQuickItemRendererPrivate : public VideoRendererPrivate
{
public:
    QQuickItemRendererPrivate() : texture(NULL),
        image(NULL)
    {
    }

    QImage* image;
    QSGTexture* texture;
};

} //namespace QtAV

#endif // QTAV_QML_QQUICKRENDERER_P_H
