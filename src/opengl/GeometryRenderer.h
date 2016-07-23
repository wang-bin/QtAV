/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2016)

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
#ifndef QTAV_GEOMETRYRENDERER_H
#define QTAV_GEOMETRYRENDERER_H
#include "opengl/Geometry.h"
#include "opengl/OpenGLHelper.h"
#define QT_VAO (QT_VERSION >= QT_VERSION_CHECK(5, 1, 0))
#if QT_VAO
#include <QtGui/QOpenGLVertexArrayObject>
#endif //QT_VAO

QT_BEGIN_NAMESPACE
class QOpenGLShaderProgram;
QT_END_NAMESPACE

namespace QtAV {
class GeometryRenderer
{
public:
    // rendering features. Use all possible features as the default.
    static const int kVBO = 0x01;
    static const int kIBO = 0x02;
    static const int kVAO = 0x04;

    GeometryRenderer();
    // call updateBuffer internally in bindBuffer if feature is changed
    void setFeature(int f, bool on);
    void setFeatures(int value);
    int features() const;
    bool testFeatures(int value) const;
    /// assume attributes are bound in the order 0, 1, 2,....
    /// null geometry: release vao/vbo
    bool updateGeometry(Geometry* geo = NULL);
    void render();
protected:
    void bindBuffers();
    void unbindBuffers();
private:
    Geometry *g;
    int features_;
    QOpenGLBuffer vbo; //VertexBuffer
#if QT_VAO
    QOpenGLVertexArrayObject vao;
#endif //QT_VAO
    QOpenGLBuffer ibo;
};

} //namespace QtAV
#endif //QTAV_GEOMETRYRENDERER_H
