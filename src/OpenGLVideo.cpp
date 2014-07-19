/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2014 Wang Bin <wbsecg1@gmail.com>

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

#include "QtAV/OpenGLVideo.h"
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLFunctions>
#else
#include <QtOpenGL/QGLShaderProgram>
#include <QtOpenGL/QGLFunctions>
typedef QGLShaderProgram QOpenGLShaderProgram;
typedef QGLShader QOpenGLShader;
typedef QGLFunctions QOpenGLFunctions;
#endif
#include "QtAV/SurfaceInterop.h"
#include "QtAV/VideoShader.h"
#include "QtAV/private/ShaderManager.h"

namespace QtAV {

class OpenGLVideoPrivate : public DPtrPrivate<OpenGLVideo>
{
public:
    OpenGLVideoPrivate()
        : material(new VideoMaterial())
    {}
    ~OpenGLVideoPrivate() {
        if (material) {
            delete material;
            material = 0;
        }
    }

    ShaderManager manager;
    VideoMaterial *material;
    QRect viewport;
    QRect out_rect;
    QMatrix4x4 matrix;
};

OpenGLVideo::OpenGLVideo() {}

void OpenGLVideo::setCurrentFrame(const VideoFrame &frame)
{
    d_func().material->setCurrentFrame(frame);
}

void OpenGLVideo::setViewport(const QRect& rect)
{
    DPTR_D(OpenGLVideo);
    d.viewport = rect;
    //qDebug() << "out_rect: " << d.out_rect << " viewport: " << d.viewport;
    d.matrix(0, 0) = (GLfloat)d.out_rect.width()/(GLfloat)d.viewport.width();
    d.matrix(1, 1) = (GLfloat)d.out_rect.height()/(GLfloat)d.viewport.height();
}

void OpenGLVideo::setVideoRect(const QRect &rect)
{
    DPTR_D(OpenGLVideo);
    d.out_rect = rect;
    //qDebug() << "out_rect: " << d.out_rect << " viewport: " << d.viewport;
    if (!d.viewport.isValid())
        return;
    d.matrix(0, 0) = (GLfloat)d.out_rect.width()/(GLfloat)d.viewport.width();
    d.matrix(1, 1) = (GLfloat)d.out_rect.height()/(GLfloat)d.viewport.height();
}

void OpenGLVideo::setBrightness(qreal value)
{
    d_func().material->setBrightness(value);
}

void OpenGLVideo::setContrast(qreal value)
{
    d_func().material->setContrast(value);
}

void OpenGLVideo::setHue(qreal value)
{
    d_func().material->setHue(value);
}

void OpenGLVideo::setSaturation(qreal value)
{
    d_func().material->setSaturation(value);
}

void OpenGLVideo::render(const QRect &roi)
{
    DPTR_D(OpenGLVideo);
    glViewport(d.viewport.x(), d.viewport.y(), d.viewport.width(), d.viewport.height());
    VideoShader *shader = d.manager.prepareMaterial(d.material);
    shader->update(d.material);
    shader->program()->setUniformValue(shader->matrixLocation(), d.matrix);
    // uniform end. attribute begin
    const int kTupleSize = 2;
    GLfloat texCoords[kTupleSize*4];
    d.material->getTextureCoordinates(roi, texCoords);
    const GLfloat kVertices[] = {
        -1, 1,
        1, 1,
        1, -1,
        -1, -1,
    };
    shader->program()->setAttributeArray(0, GL_FLOAT, kVertices, kTupleSize);
    shader->program()->setAttributeArray(1, GL_FLOAT, texCoords, kTupleSize);

    char const *const *attr = shader->attributeNames();
    for (int i = 0; attr[i]; ++i) {
        shader->program()->enableAttributeArray(i); //TODO: in setActiveShader
    }

   glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

   // d.shader->program()->release(); //glUseProgram(0)
   for (int i = 0; attr[i]; ++i) {
       shader->program()->disableAttributeArray(i); //TODO: in setActiveShader
   }

   d.material->unbind();
}

} //namespace QtAV
