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
    SGVideoMaterial() {}
    ~SGVideoMaterial() {}

    virtual QSGMaterialType *type() const {
        return reinterpret_cast<QSGMaterialType*>(m_material.type());
    }

    virtual QSGMaterialShader *createShader() const {
        //TODO: setVideoFormat?
        return new SGVideoMaterialShader(m_material.createShader());
    }

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


SGVideoNode::SGVideoNode()
    : m_material(new SGVideoMaterial())
{
    setFlag(QSGNode::OwnsGeometry);
    setFlag(QSGNode::OwnsMaterial);
    setMaterial(m_material);
}

SGVideoNode::~SGVideoNode() {}

void SGVideoNode::setCurrentFrame(const VideoFrame &frame)
{
    m_material->setCurrentFrame(frame);
    markDirty(DirtyMaterial);
}

/* Helpers */
static inline void qSetGeom(QSGGeometry::TexturedPoint2D *v, const QPointF &p)
{
    v->x = p.x();
    v->y = p.y();
}

static inline void qSetTex(QSGGeometry::TexturedPoint2D *v, const QPointF &p)
{
    v->tx = p.x();
    v->ty = p.y();
}

void SGVideoNode::setTexturedRectGeometry(const QRectF &rect, const QRectF &textureRect, int orientation)
{
    if (rect == m_rect && textureRect == m_textureRect && orientation == m_orientation)
        return;

    m_rect = rect;
    m_textureRect = textureRect;
    m_orientation = orientation;

    QSGGeometry *g = geometry();

    if (g == 0)
        g = new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4);

    QSGGeometry::TexturedPoint2D *v = g->vertexDataAsTexturedPoint2D();

    // Set geometry first
    qSetGeom(v + 0, rect.topLeft());
    qSetGeom(v + 1, rect.bottomLeft());
    qSetGeom(v + 2, rect.topRight());
    qSetGeom(v + 3, rect.bottomRight());

    // and then texture coordinates
    switch (orientation) {
        default:
            // tl, bl, tr, br
            qSetTex(v + 0, textureRect.topLeft());
            qSetTex(v + 1, textureRect.bottomLeft());
            qSetTex(v + 2, textureRect.topRight());
            qSetTex(v + 3, textureRect.bottomRight());
            break;

        case 90:
            // tr, tl, br, bl
            qSetTex(v + 0, textureRect.topRight());
            qSetTex(v + 1, textureRect.topLeft());
            qSetTex(v + 2, textureRect.bottomRight());
            qSetTex(v + 3, textureRect.bottomLeft());
            break;

        case 180:
            // br, tr, bl, tl
            qSetTex(v + 0, textureRect.bottomRight());
            qSetTex(v + 1, textureRect.topRight());
            qSetTex(v + 2, textureRect.bottomLeft());
            qSetTex(v + 3, textureRect.topLeft());
            break;

        case 270:
            // bl, br, tl, tr
            qSetTex(v + 0, textureRect.bottomLeft());
            qSetTex(v + 1, textureRect.bottomRight());
            qSetTex(v + 2, textureRect.topLeft());
            qSetTex(v + 3, textureRect.topRight());
            break;
    }

    if (!geometry())
        setGeometry(g);

    markDirty(DirtyGeometry);
}

} //namespace QtAV
