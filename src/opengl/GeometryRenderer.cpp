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
    : program(NULL)
    , try_vbo(true)
    , try_vao(true)
{
    static bool disable_vbo = qgetenv("QTAV_NO_VBO").toInt() > 0;
    try_vbo = !disable_vbo;
    static bool disable_vao = qgetenv("QTAV_NO_VAO").toInt() > 0;
    try_vao = !disable_vao;
}

void GeometryRenderer::setShaderProgram(QOpenGLShaderProgram *sp)
{
    program = sp;
}

bool GeometryRenderer::updateBuffers(Geometry *g)
{
    if (!g) {
        vbo.destroy();
#if QT_VAO
        vao.destroy();
#endif
        return true;
    }
    if (!try_vbo)
        return false;
#if QT_VAO
    if (try_vao) {
        qDebug("updating vao...");
        if (!vao.isCreated()) {
            if (!vao.create()) {
                try_vao = false;
                qDebug("VAO is not supported");
            }
        }
    }
    QOpenGLVertexArrayObject::Binder vao_bind(&vao);
    Q_UNUSED(vao_bind);
#endif
    if (!vbo.isCreated()) {
        if (!vbo.create()) {
            try_vbo = false; // not supported by OpenGL
            try_vao = false; // also disable VAO. destroy?
            qWarning("VBO is not supported");
            return false;
        }
    }
    qDebug("updating vbo...");
    vbo.bind(); //check here
    vbo.allocate(g->vertexData(), g->vertexCount()*g->stride());
#if QT_VAO
    if (try_vao) {
        for (int an = 0; an < g->attributes().size(); ++an) {
            // FIXME: assume bind order is 0,1,2...
            const Attribute& a = g->attributes().at(an);
            program->setAttributeBuffer(an, a.type(), a.offset(), a.tupleSize(), g->stride());
            program->enableAttributeArray(an); //TODO: in setActiveShader
        }
    }
#endif
    vbo.release();
    return true;
}

void GeometryRenderer::bindBuffers(Geometry *g)
{
#if QT_VAO
    if (try_vao && vao.isCreated()) {
        vao.bind();
        return;
    }
#endif
    if (try_vbo && vbo.isCreated()) {
        vbo.bind();
        // normalized
        for (int an = 0; an < g->attributes().size(); ++an) {
            const Attribute& a = g->attributes().at(an);
            program->setAttributeBuffer(an, a.type(), a.offset(), a.tupleSize(), g->stride());
            program->enableAttributeArray(an); //TODO: in setActiveShader
        }
    } else {
        for (int an = 0; an < g->attributes().size(); ++an) {
            const Attribute& a = g->attributes().at(an);
            program->setAttributeArray(an, a.type(), (const char*)g->vertexData() + a.offset(), a.tupleSize(), g->stride());
            program->enableAttributeArray(an); //TODO: in setActiveShader
        }
    }
}

void GeometryRenderer::unbindBuffers(Geometry *g)
{
#if QT_VAO
    if (try_vao && vao.isCreated()) {
        vao.release();
        return;
    }
#endif //QT_VAO
    for (int an = 0; an < g->attributes().size(); ++an) {
        program->disableAttributeArray(an); //TODO: in setActiveShader
    }
    // release vbo. qpainter is affected if vbo is bound
    if (try_vbo && vbo.isCreated()) {
        vbo.release();
    }
}

void GeometryRenderer::render(Geometry *g)
{
    if (g->indexCount() > 0) {
        // IBO

    } else {
        DYGL(glDrawArrays(g->primitiveType(), 0, g->vertexCount()));
    }
}
} //namespace QtAV
