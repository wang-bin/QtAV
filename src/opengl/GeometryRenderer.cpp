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
    , g(NULL)
    , try_vbo(true)
    , try_vao(true)
    , try_ibo(true)
    , ibo(QOpenGLBuffer::IndexBuffer)
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

bool GeometryRenderer::updateBuffers(Geometry *geo)
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
    if (try_ibo && g->indexCount() > 0) {
        if (!ibo.isCreated()) {
            if (!ibo.create()) {
                try_ibo = false;
                qDebug("IBO is not supported");
            }
        }
        if (ibo.isCreated()) {
            ibo.bind();
            ibo.allocate(g->indexData(), g->indexDataSize());
            ibo.release();
        }
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
    //qDebug("updating vbo...");
    vbo.bind(); //check here
    vbo.allocate(g->vertexData(), g->vertexCount()*g->stride());
    //qDebug("allocate(%p, %d*%d)", g->vertexData(), g->vertexCount(), g->stride());
#if QT_VAO
    if (try_vao) {
        for (int an = 0; an < g->attributes().size(); ++an) {
            // FIXME: assume bind order is 0,1,2...
            const Attribute& a = g->attributes().at(an);
            //DYGL(glVertexAttribPointer(an, a.tupleSize(), a.type(), a.normalize(), g->stride(), reinterpret_cast<const void *>(qptrdiff(a.offset()))));
            /// FIXME: why qopengl function crash?
            program->enableAttributeArray(an); //TODO: in setActiveShader
            program->setAttributeBuffer(an, a.type(), a.offset(), a.tupleSize(), g->stride());
            //DYGL(glEnableVertexAttribArray(an));
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
    if (try_ibo && ibo.isCreated())
        ibo.bind();
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
            //DYGL(glVertexAttribPointer(an, a.tupleSize(), a.type(), a.normalize(), g->stride(), reinterpret_cast<const void *>(qptrdiff(a.offset()))));
            program->setAttributeBuffer(an, a.type(), a.offset(), a.tupleSize(), g->stride());
            program->enableAttributeArray(an); //TODO: in setActiveShader
            //DYGL(glEnableVertexAttribArray(an));
        }
    } else {
        for (int an = 0; an < g->attributes().size(); ++an) {
            const Attribute& a = g->attributes().at(an);
            program->setAttributeArray(an, a.type(), (const char*)g->vertexData() + a.offset(), a.tupleSize(), g->stride());
            //DYGL(glVertexAttribPointer(an, a.tupleSize(), a.type(), a.normalize(), g->stride(), (const char*)g->vertexData() + a.offset()));
            program->enableAttributeArray(an); //TODO: in setActiveShader
            //DYGL(glEnableVertexAttribArray(an));
        }
    }
}

void GeometryRenderer::unbindBuffers()
{
    if (!g)
        return;
    if (try_ibo && ibo.isCreated())
        ibo.release();
#if QT_VAO
    if (try_vao && vao.isCreated()) {
        vao.release();
        return;
    }
#endif //QT_VAO
    // release vbo. qpainter is affected if vbo is bound
    if (try_vbo && vbo.isCreated()) {
        vbo.release();
    }
    for (int an = 0; an < g->attributes().size(); ++an) {
        program->disableAttributeArray(an); //TODO: in setActiveShader
    }
}

void GeometryRenderer::render()
{
    if (!g)
        return;
    if (g->indexCount() > 0) {
        DYGL(glDrawElements(g->primitive(), g->indexCount(), g->indexType(), g->indexData()));
    } else {
        DYGL(glDrawArrays(g->primitive(), 0, g->vertexCount()));
    }
}
} //namespace QtAV
