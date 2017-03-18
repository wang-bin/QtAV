/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2017 Wang Bin <wbsecg1@gmail.com>

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
#include "QtAV/GeometryRenderer.h"
#include "opengl/OpenGLHelper.h"
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#define QGLF(f) QOpenGLContext::currentContext()->functions()->f
#else
#define QGLF(f) QGLFunctions(NULL).f
#endif
namespace QtAV {

GeometryRenderer::GeometryRenderer()
    : g(NULL)
    , features_(kVBO|kIBO|kVAO|kMapBuffer)
    , vbo_size(0)
    , ibo_size(0)
    , ibo(QOpenGLBuffer::IndexBuffer)
    , stride(0)
{
    static bool disable_ibo = qgetenv("QTAV_NO_IBO").toInt() > 0;
    setFeature(kIBO, !disable_ibo);
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

int GeometryRenderer::actualFeatures() const
{
    int f = 0;
    if (vbo.isCreated())
        f |= kVBO;
    if (ibo.isCreated())
        f |= kIBO;
#if QT_VAO
    if (vao.isCreated())
        f |= kVAO;
#endif
    return f;
}

bool GeometryRenderer::testFeatures(int value) const
{
    return !!(features() & value);
}

void GeometryRenderer::updateGeometry(Geometry *geo)
{
    g = geo;
    if (!g) {
        ibo.destroy();
        vbo.destroy();
#if QT_VAO
        vao.destroy();
#endif
        vbo_size = 0;
        ibo_size = 0;
        return;
    }
    static int support_map = -1;
    if (support_map < 0) {
        static const char* ext[] = { "GL_OES_mapbuffer", NULL};
        if (OpenGLHelper::isOpenGLES()) {
            support_map = QOpenGLContext::currentContext()->format().majorVersion() > 2 ||
                    OpenGLHelper::hasExtension(ext);
        } else {
            support_map = 1;
        }
    }
    if (testFeatures(kIBO) && !ibo.isCreated()) {
        if (g->indexCount() > 0) {
            qDebug("creating IBO...");
            if (!ibo.create())
                qDebug("IBO create error");
        }
    }
    if (ibo.isCreated()) {
        ibo.bind();
        const int bs = g->indexDataSize();
        if (bs == ibo_size) {
            void * p = NULL;
            if (support_map && testFeatures(kMapBuffer))
                p = ibo.map(QOpenGLBuffer::WriteOnly);
            if (p) {
                memcpy(p, g->constIndexData(), bs);
                ibo.unmap();
            } else {
                ibo.write(0, g->constIndexData(), bs);
            }
        } else {
            ibo.allocate(g->indexData(), bs); // TODO: allocate NULL and then map or BufferSubData?
            ibo_size = bs;
        }
        ibo.release();
    }
    if (testFeatures(kVBO) && !vbo.isCreated()) {
        qDebug("creating VBO...");
        if (!vbo.create())
            qWarning("VBO create error");
    }
    if (vbo.isCreated()) {
        vbo.bind();
        const int bs = g->vertexCount()*g->stride();
        /* Notes from https://www.opengl.org/sdk/docs/man/html/glBufferSubData.xhtml
           When replacing the entire data store, consider using glBufferSubData rather than completely recreating the data store with glBufferData. This avoids the cost of reallocating the data store.
         */
        if (bs == vbo_size) { // vbo.size() error 0x501 on rpi, and query gl value can be slow
            void* p = NULL;
            if (support_map && testFeatures(kMapBuffer))
                p = vbo.map(QOpenGLBuffer::WriteOnly);
            if (p) {
                memcpy(p, g->constVertexData(), bs);
                vbo.unmap();
            } else {
                vbo.write(0, g->constVertexData(), bs);
                vbo_size = bs;
            }
        } else {
            vbo.allocate(g->vertexData(), bs);
        }
        vbo.release();
    }
#if QT_VAO
    if (stride == g->stride() && attrib == g->attributes())
        return;
    stride = g->stride();
    attrib = g->attributes();

    if (testFeatures(kVAO) && !vao.isCreated()) {
        qDebug("creating VAO...");
        if (!vao.create())
            qDebug("VAO create error");
    }
    qDebug("vao updated");
    if (vao.isCreated()) // can not use vao binder because it will create a vao if necessary
        vao.bind();
// can set data before vao bind
    if (!vao.isCreated())
        return;
    qDebug("geometry attributes changed, rebind vao...");
    // call once is enough if no feature and no geometry attribute is changed
    if (vbo.isCreated()) {
        vbo.bind();
        for (int an = 0; an < g->attributes().size(); ++an) {
            // FIXME: assume bind order is 0,1,2...
            const Attribute& a = g->attributes().at(an);
            QGLF(glVertexAttribPointer(an, a.tupleSize(), a.type(), a.normalize(), g->stride(), reinterpret_cast<const void *>(qptrdiff(a.offset())))); //TODO: in setActiveShader
            QGLF(glEnableVertexAttribArray(an));
        }
        vbo.release(); // unbind after vao unbind? http://www.zwqxin.com/archives/opengl/vao-and-vbo-stuff.html
    } // TODO: bind pointers if vbo is disabled
    // bind ibo to vao thus no bind is required later
    if (ibo.isCreated())// if not bind here, glDrawElements(...,NULL) crashes and must use ibo data ptr, why?
        ibo.bind();
    vao.release();
    if (ibo.isCreated())
        ibo.release();
#endif
    qDebug("geometry updated");
}

void GeometryRenderer::bindBuffers()
{
    bool bind_vbo = vbo.isCreated();
    bool bind_ibo = ibo.isCreated();
    bool setv_skip = false;
#if QT_VAO
    if (vao.isCreated()) {
        vao.bind(); // vbo, ibo is ok now
        setv_skip = bind_vbo;
        bind_vbo = false;
        bind_ibo = false;
    }
#endif
    //qDebug("bind ibo: %d vbo: %d; set v: %d", bind_ibo, bind_vbo, !setv_skip);
    if (bind_ibo)
        ibo.bind();
    // no vbo: set vertex attributes
    // has vbo, no vao: bind vbo & set vertex attributes
    // has vbo, has vao: skip
    if (setv_skip)
        return;
    if (!g)
        return;
    const char* vdata = static_cast<const char*>(g->vertexData());
    if (bind_vbo) {
        vbo.bind();
        vdata = NULL;
    }
    for (int an = 0; an < g->attributes().size(); ++an) {
        const Attribute& a = g->attributes().at(an);
        QGLF(glVertexAttribPointer(an, a.tupleSize(), a.type(), a.normalize(), g->stride(), vdata + a.offset()));
        QGLF(glEnableVertexAttribArray(an)); //TODO: in setActiveShader
    }
}

void GeometryRenderer::unbindBuffers()
{
    bool unbind_vbo = vbo.isCreated();
    bool unbind_ibo = ibo.isCreated();
    bool unsetv_skip = false;
#if QT_VAO
    if (vao.isCreated()) {
        vao.release();
        unsetv_skip = unbind_vbo;
        unbind_vbo = false;
        unbind_ibo = false;
    }
#endif //QT_VAO
    //qDebug("unbind ibo: %d vbo: %d; unset v: %d", unbind_ibo, unbind_vbo, !unsetv_skip);
    if (unbind_ibo)
        ibo.release();
    // release vbo. qpainter is affected if vbo is bound
    if (unbind_vbo)
        vbo.release();
    // no vbo: disable vertex attributes
    // has vbo, no vao: unbind vbo & set vertex attributes
    // has vbo, has vao: skip
    if (unsetv_skip)
        return;
    if (!g)
        return;
    for (int an = 0; an < g->attributes().size(); ++an) {
        QGLF(glDisableVertexAttribArray(an));
    }
}

void GeometryRenderer::render()
{
    if (!g)
        return;
    bindBuffers();
    if (g->indexCount() > 0) {
        DYGL(glDrawElements(g->primitive(), g->indexCount(), g->indexType(), ibo.isCreated() ? NULL : g->indexData())); // null: data in vao or ibo. not null: data in memory
    } else {
        DYGL(glDrawArrays(g->primitive(), 0, g->vertexCount()));
    }
    unbindBuffers();
}
} //namespace QtAV
