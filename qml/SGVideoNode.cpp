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


#include "QmlAV/SGVideoNode.h"
#include "QtAV/VideoShader.h"
#include "QtAV/VideoFrame.h"
#include <QtCore/QScopedPointer>
#include <QtGui/QOpenGLFunctions>
#include <QtQuick/QSGMaterial>
#include <QtQuick/QSGMaterialShader>

// all in QSGRenderThread
namespace QtAV {

class SGVideoMaterialShader : public QSGMaterialShader
{
public:
    SGVideoMaterialShader(VideoShader* s) :
        QSGMaterialShader()
        , m_shader(s)
    {}
    virtual void updateState(const RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial);
    virtual char const *const *attributeNames() const { return m_shader->attributeNames();}
protected:
    virtual const char *vertexShader() const { return m_shader->vertexShader();}
    virtual const char *fragmentShader() const { return m_shader->fragmentShader();}
    virtual void initialize() { m_shader->initialize(program());}

    int textureLocationCount() const { return m_shader->textureLocationCount();}
    int textureLocation(int index) const { return m_shader->textureLocation(index);}
    int matrixLocation() const { return m_shader->matrixLocation();}
    int colorMatrixLocation() const { return m_shader->colorMatrixLocation();}
    int opacityLocation() const { return m_shader->opacityLocation();}
private:
    QScopedPointer<VideoShader> m_shader;
};

class SGVideoMaterial : public QSGMaterial
{
public:
    SGVideoMaterial() : QSGMaterial(), m_opacity(1.0) {}

    virtual QSGMaterialType *type() const {
        return reinterpret_cast<QSGMaterialType*>((quintptr)m_material.type());
    }

    virtual QSGMaterialShader *createShader() const {
        return new SGVideoMaterialShader(m_material.createShader());
    }

    virtual int compare(const QSGMaterial *other) const {
        Q_ASSERT(other && type() == other->type());
        const SGVideoMaterial *m = static_cast<const SGVideoMaterial*>(other);
        return m_material.compare(&m->m_material);
    }
/*
    void updateBlending() {
        setFlag(Blending, m_material.hasAlpha() || !qFuzzyCompare(m_opacity, qreal(1.0)));
    }
*/
    void setCurrentFrame(const VideoFrame &frame) {
        m_material.setCurrentFrame(frame);
        setFlag(Blending, frame.format().hasAlpha());
    }

    VideoMaterial* videoMaterial() { return &m_material;}
    qreal m_opacity;
    VideoMaterial m_material;
};

void SGVideoMaterialShader::updateState(const RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    Q_UNUSED(oldMaterial);
    SGVideoMaterial *mat = static_cast<SGVideoMaterial *>(newMaterial);
    if (!m_shader->update(&mat->m_material)) //material not ready. e.g. video item have not got a frame
        return;
    //mat->updateBlending();
    if (state.isOpacityDirty()) {
        mat->m_opacity = state.opacity();
        program()->setUniformValue(opacityLocation(), GLfloat(mat->m_opacity));
    }
    if (state.isMatrixDirty())
        program()->setUniformValue(matrixLocation(), state.combinedMatrix());
}


SGVideoNode::SGVideoNode()
    : QSGGeometryNode()
    , m_material(new SGVideoMaterial())
    , m_validWidth(1.0)
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
    if (m_validWidth == m_material->videoMaterial()->validTextureWidth()
            && rect == m_rect && textureRect == m_textureRect && orientation == m_orientation)
        return;
    QRectF validTexRect = m_material->videoMaterial()->normalizedROI(textureRect);
    if (!validTexRect.isEmpty()) {
        m_validWidth = m_material->videoMaterial()->validTextureWidth();
        m_rect = rect;
        m_textureRect = textureRect;
        m_orientation = orientation;
    }
    //qDebug() << ">>>>>>>valid: " << m_validWidth << "  roi: " << validTexRect;
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
            qSetTex(v + 0, validTexRect.topLeft());
            qSetTex(v + 1, validTexRect.bottomLeft());
            qSetTex(v + 2, validTexRect.topRight());
            qSetTex(v + 3, validTexRect.bottomRight());
            break;

        case 90:
            // tr, tl, br, bl
            qSetTex(v + 0, validTexRect.topRight());
            qSetTex(v + 1, validTexRect.topLeft());
            qSetTex(v + 2, validTexRect.bottomRight());
            qSetTex(v + 3, validTexRect.bottomLeft());
            break;

        case 180:
            // br, tr, bl, tl
            qSetTex(v + 0, validTexRect.bottomRight());
            qSetTex(v + 1, validTexRect.topRight());
            qSetTex(v + 2, validTexRect.bottomLeft());
            qSetTex(v + 3, validTexRect.topLeft());
            break;

        case 270:
            // bl, br, tl, tr
            qSetTex(v + 0, validTexRect.bottomLeft());
            qSetTex(v + 1, validTexRect.bottomRight());
            qSetTex(v + 2, validTexRect.topLeft());
            qSetTex(v + 3, validTexRect.topRight());
            break;
    }

    if (!geometry())
        setGeometry(g);

    markDirty(DirtyGeometry);
}

} //namespace QtAV
