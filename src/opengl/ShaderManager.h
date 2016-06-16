/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2014)

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

#ifndef QTAV_SHADERMANAGER_H
#define QTAV_SHADERMANAGER_H

#include <QtCore/QObject>

namespace QtAV {
class VideoShader;
class VideoMaterial;
/*!
 * \brief The ShaderManager class
 * Cache VideoShader and shader programes for different video material type.
 * TODO: ShaderManager does not change for a given vo, so we can expose VideoRenderer.shaderManager() to set custom shader. It's better than VideoRenderer.opengl() because OpenGLVideo exposes too many apis that may confuse user.
 */
class ShaderManager : public QObject
{
    Q_OBJECT
public:
    ShaderManager(QObject *parent = 0);
    ~ShaderManager();
    VideoShader* prepareMaterial(VideoMaterial *material, qint32 materialType = -1);
//    void setCacheSize(int value);

private:
    class Private;
    Private* d;
};
} //namespace QtAV
#endif // QTAV_SHADERMANAGER_H
