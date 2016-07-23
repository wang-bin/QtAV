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
#include "GeometryRenderer.h"

namespace QtAV {

GeometryRenderer::GeometryRenderer()
    : g(NULL)
    , features_(kVBO|kIBO|kVAO)
    , ibo(QOpenGLBuffer::IndexBuffer)
{
    static bool disable_vbo = qgetenv("QTAV_NO_VBO").toInt() > 0;
    setFeature(kVBO, !disable_vbo);
    static bool disable_vao = qgetenv("QTAV_NO_VAO").toInt() > 0;
    setFeature(kVAO, !disable_vao);
}

void GeometryRenderer::setFeature(int f, bool on)
{
    if (on)
        features_ |= f;
    else
        features_ ^= f;
}

void GeometryRenderer::setFeatures(int value)
{
    features_ = value;
}

int GeometryRenderer::features() const
{
    return features_;
}

bool GeometryRenderer::testFeatures(int value) const
{
    return !!(features() & value);
}

bool GeometryRenderer::updateGeometry(Geometry *geo)
{
    g = geo;
    if (!g) {
        vbo.destroy();
#if QT_VAO
        vao.destroy();
#endif
        ibo.destroy();
        return true;
    }
    if (testFeatures(kIBO) && g->indexCount() > 0) {
        if (!ibo.isCreated()) {
            if (!ibo.create()) {
                setFeature(kIBO, false);
                qDebug("IBO is not supported");
            }
        }
        if (ibo.isCreated()) {
            ibo.bind();
            ibo.allocate(g->indexData(), g->indexDataSize());
            ibo.release();
        }
    }
    if (!testFeatures(kVBO))
        return false;
#if QT_VAO
    if (testFeatures(kVAO)) {
        //qDebug("updating vao...");
        if (!vao.isCreated()) {
            if (!vao.create()) {
                setFeature(kVAO, false);
                qDebug("VAO is not supported");
            }
        }
    }
    QOpenGLVertexArrayObject::Binder vao_bind(&vao);
    Q_UNUSED(vao_bind);
#endif
    if (!vbo.isCreated()) {
        if (!vbo.create()) {
            setFeature(kVBO, false);
            setFeature(kVAO, false);// also disable Vao_. destroy?
            qWarning("VBO is not supported");
            return false;
        }
    }
    //qDebug("updating vbo...");
    vbo.bind(); //check here
    vbo.allocate(g->vertexData(), g->vertexCount()*g->stride());
    //qDebug("allocate(%p, %d*%d)", g->vertexData(), g->vertexCount(), g->stride());
#if QT_VAO
    if (testFeatures(kVAO)) {
        for (int an = 0; an < g->attributes().size(); ++an) {
            // FIXME: assume bind order is 0,1,2...
            const Attribute& a = g->attributes().at(an);
            DYGL(glVertexAttribPointer(an, a.tupleSize(), a.type(), a.normalize(), g->stride(), reinterpret_cast<const void *>(qptrdiff(a.offset())))); //TODO: in setActiveShader
            /// FIXME: why QOpenGLShaderProgram function crash?
            DYGL(glEnableVertexAttribArray(an));
        }
    }
#endif
    vbo.release();
    return true;
}

void GeometryRenderer::bindBuffers()
{
    if (!g)
        return;
    if (testFeatures(kIBO) && ibo.isCreated())
        ibo.bind();
#if QT_VAO
    if (testFeatures(kVAO) && vao.isCreated()) {
        vao.bind();
        return;
    }
#endif
    const char* vdata = static_cast<const char*>(g->vertexData());
    if (testFeatures(kVBO) && vbo.isCreated()) {
        vbo.bind();
        vdata = NULL;
    }
    for (int an = 0; an < g->attributes().size(); ++an) {
        const Attribute& a = g->attributes().at(an);
        DYGL(glVertexAttribPointer(an, a.tupleSize(), a.type(), a.normalize(), g->stride(), vdata + a.offset()));
        DYGL(glEnableVertexAttribArray(an)); //TODO: in setActiveShader
    }
}

void GeometryRenderer::unbindBuffers()
{
    if (!g)
        return;
    if (testFeatures(kIBO) && ibo.isCreated())
        ibo.release();
#if QT_VAO
    if (testFeatures(kVAO) && vao.isCreated()) {
        vao.release();
        return;
    }
#endif //QT_VAO
    for (int an = 0; an < g->attributes().size(); ++an) {
        DYGL(glDisableVertexAttribArray(an));
    }
    // release vbo. qpainter is affected if vbo is bound
    if (testFeatures(kVBO) && vbo.isCreated()) {
        vbo.release();
    }
}

void GeometryRenderer::render()
{
    if (!g)
        return;
    bindBuffers();
    if (g->indexCount() > 0) {
        DYGL(glDrawElements(g->primitive(), g->indexCount(), g->indexType(), g->indexData()));
    } else {
        DYGL(glDrawArrays(g->primitive(), 0, g->vertexCount()));
    }
    unbindBuffers();
}
} //namespace QtAV
