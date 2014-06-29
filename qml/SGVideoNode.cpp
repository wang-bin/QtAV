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
#include "QtAV/OpenGLVideo.h"
#include "QtAV/VideoFrame.h"
#include <QtGui/QOpenGLFunctions>
#include <QtQuick/QSGMaterialShader>

namespace QtAV {

class SGVideoMaterialShader : public QSGMaterialShader
{
public:
    SGVideoMaterialShader() :
        m_shader(new VideoShader())
    {
    }
    ~SGVideoMaterialShader() {
        delete m_shader;
    }

    virtual void updateState(const RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial);
    virtual char const *const *attributeNames() const { return m_shader->attributeNames();}
protected:
    virtual const char *vertexShader() const { return m_shader->vertexShader();}
    virtual const char *fragmentShader() const { return m_shader->fragmentShader();}
    virtual void initialize() { m_shader->initialize(program());}

    int textureCount() const { return m_shader->textureCount();}
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
    ~SGVideoMaterial() {
        if (!m_textureIds.isEmpty())
            glDeleteTextures(m_textureIds.size(), m_textureIds.data());
    }

    virtual QSGMaterialType *type() const {
        static QSGMaterialType rgbType;
        static QSGMaterialType yuv16leType;
        static QSGMaterialType yuv16beType;
        static QSGMaterialType yuv8Type;
        const VideoFormat &fmt = m_frame.format();
        if (fmt.isRGB() && !fmt.isPlanar())
            return &rgbType;
        if (fmt.bytesPerPixel(0) == 1)
            return &yuv8Type;
        if (fmt.isBigEndian())
            return &yuv16beType;
        return &yuv16leType;
    }

    virtual QSGMaterialShader *createShader() const {
        return new SGVideoMaterialShader();
    }

    //TODO: compare video format?
    virtual int compare(const QSGMaterial *other) const {
        Q_ASSERT(other && type() == other->type());
        const SGVideoMaterial *m = static_cast<const SGVideoMaterial*>(other);
        for (int i = 0; i < m_textureIds.size(); ++i) {
            const int diff = m_textureIds[i] - m->m_textureIds[i];
            if (diff)
                return diff;
        }
        return m_bpp - m->m_bpp;
    }
/*
    void updateBlending() {
        setFlag(Blending, qFuzzyCompare(m_opacity, qreal(1.0)) ? false : true);
    }
*/
    void setCurrentFrame(const VideoFrame &frame) {
        QMutexLocker lock(&m_frameMutex);
        m_frame = frame;
    }

    void bind();
    void bindTexture(int id, int w, int h, const uchar *bits);

    VideoFormat m_format;
    QSize m_textureSize;


    int m_bpp;
    //qreal m_opacity;
    //GLfloat m_yWidth;
    //GLfloat m_uvWidth;
    QVector<GLuint> m_textureIds;
    QMatrix4x4 m_colorMatrix;

    VideoFrame m_frame;
    QMutex m_frameMutex;
};

void SGVideoMaterial::bind()
{
    QOpenGLFunctions *functions = QOpenGLContext::currentContext()->functions();
    QMutexLocker lock(&m_frameMutex);
    if (!m_frame.isValid()) {
        if (m_textureIds.isEmpty())
            return;
        for (int i = 0; i < m_textureIds.size(); ++i) {
            functions->glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, m_textureIds[i]);
        }
    }
}

void SGVideoMaterial::bindTexture(int id, int w, int h, const uchar *bits)
{

}


SGVideoNode::SGVideoNode()
{
}


void SGVideoMaterialShader::updateState(const RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    Q_UNUSED(oldMaterial);
    SGVideoMaterial *mat = static_cast<SGVideoMaterial *>(newMaterial);
    for (int i = 0; i < textureCount(); ++i) {
        program()->setUniformValue(textureLocation(i), i);
    }
    mat->bind();
    program()->setUniformValue(colorMatrixLocation(), mat->m_colorMatrix);
    //program()->setUniformValue(m_id_yWidth, mat->m_yWidth);
    //program()->setUniformValue(m_id_uvWidth, mat->m_uvWidth);
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
