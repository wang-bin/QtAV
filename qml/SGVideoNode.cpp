/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2014 Wang Bin <wbsecg1@gmail.com>
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


#include "QmlAV/SGVideoNode.h"
#include "QtAV/VideoShader.h"
#include "QtAV/VideoFrame.h"
#include <QtCore/QMutexLocker>
#include <QtGui/QOpenGLFunctions>
#include <QtQuick/QSGMaterialShader>

namespace QtAV {

class SGVideoMaterialShader : public QSGMaterialShader
{
public:
    SGVideoMaterialShader(VideoShader* s) :
        m_shader(s)
    {
        setVideoFormat(s->videoFormat());
        //setColorSpace(s->colorSpace());
    }
    ~SGVideoMaterialShader() {
        delete m_shader;
    }

    virtual void updateState(const RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial);
    virtual char const *const *attributeNames() const { return m_shader->attributeNames();}
    void setVideoFormat(const VideoFormat& format) { m_shader->setVideoFormat(format);}
    //void setColorSpace(ColorTransform::ColorSpace cs) { m_shader->setColorSpace(cs);}
protected:
    virtual const char *vertexShader() const { return m_shader->vertexShader();}
    virtual const char *fragmentShader() const { return m_shader->fragmentShader();}
    virtual void initialize() { m_shader->initialize(program());}

    int textureLocationCount() const { return m_shader->textureLocationCount();}
    int textureLocation(int index) const { return m_shader->textureLocation(index);}
    int matrixLocation() const { return m_shader->matrixLocation();}
    int colorMatrixLocation() const { return m_shader->colorMatrixLocation();}

private:
    VideoShader *m_shader;
};

class SGVideoMaterial : public QSGMaterial
{
public:
    SGVideoMaterial(const VideoFormat &format);
    ~SGVideoMaterial() {}

    virtual QSGMaterialType *type() const {
        return reinterpret_cast<QSGMaterialType*>(m_material.type());
    }

    virtual QSGMaterialShader *createShader() const {
        //TODO: setVideoFormat?
        return new SGVideoMaterialShader(m_material.createShader());
    }

    //TODO: compare video format?
    virtual int compare(const QSGMaterial *other) const {
        Q_ASSERT(other && type() == other->type());
        const SGVideoMaterial *m = static_cast<const SGVideoMaterial*>(other);
        return m_material.compare(&m->m_material);
    }
/*
    void updateBlending() {
        setFlag(Blending, qFuzzyCompare(m_opacity, qreal(1.0)) ? false : true);
    }
*/
    void setCurrentFrame(const VideoFrame &frame) {
        QMutexLocker lock(&m_frameMutex);
        m_material.setCurrentFrame(frame);
    }

    void bind();
    //qreal m_opacity;
    QMutex m_frameMutex;
    VideoMaterial m_material;
};

void SGVideoMaterial::bind()
{
    m_material.bind();
}


SGVideoNode::SGVideoNode()
{
}


void SGVideoMaterialShader::updateState(const RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    Q_UNUSED(oldMaterial);
    SGVideoMaterial *mat = static_cast<SGVideoMaterial *>(newMaterial);
    m_shader->update(&mat->m_material);
#if 0
    if (state.isOpacityDirty()) {
        mat->m_opacity = state.opacity();
        program()->setUniformValue(m_id_opacity, GLfloat(mat->m_opacity));
    }
#endif
    if (state.isMatrixDirty())
        program()->setUniformValue(matrixLocation(), state.combinedMatrix());
}

} //namespace QtAV
