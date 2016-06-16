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
#ifndef QTAV_SUBIMAGESRENDERER_H
#define QTAV_SUBIMAGESRENDERER_H
#include <QtGui/QMatrix4x4>
#include <QtAV/SubImage.h>
#include <QtAV/OpenGLTypes.h>
#include "opengl/OpenGLHelper.h"
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLShader>
#else
#include <QtOpenGL/QGLShaderProgram>
#include <QtOpenGL/QGLShader>
#undef QOpenGLShaderProgram
#undef QOpenGLShader
#define QOpenGLShaderProgram QGLShaderProgram
#define QOpenGLShader QGLShader
#endif

namespace QtAV {
class SubImagesGeometry;
class GeometryRenderer;
class SubImagesRenderer
{
public:
    SubImagesRenderer();
    ~SubImagesRenderer();
    /*!
     * \brief render
     * \param ass
     * \param target
     * \param transform additional transform, e.g. aspect ratio
     */
    void render(const SubImageSet& ass, const QRect& target, const QMatrix4x4& transform = QMatrix4x4());
    /*!
     * \brief setProjectionMatrixToRect
     * the rect will be viewport
     */
    void setProjectionMatrixToRect(const QRectF& v);

private:
    void uploadTexture(SubImagesGeometry* g);

    SubImagesGeometry *m_geometry;
    GeometryRenderer *m_renderer;
    QMatrix4x4 m_mat;
    QRect m_rect;

    GLuint m_tex;
    QOpenGLShaderProgram m_program;
};
} //namespace QtAV
#endif //QTAV_SUBIMAGESRENDERER_H
