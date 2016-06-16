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
#ifndef QTAV_CONVOLUTIONSHADER_H
#define QTAV_CONVOLUTIONSHADER_H
#include <QtAV/VideoShader.h>

namespace QtAV {
class ConvolutionShaderPrivate;
/*!
 * \brief The ConvolutionShader class
 * Uniform u_Kernel is used
 */
class Q_AV_EXPORT ConvolutionShader : public VideoShader
{
    DPTR_DECLARE_PRIVATE(ConvolutionShader)
public:
    ConvolutionShader();
    /*!
     * \brief kernelRadius
     * Default is 1, i.e. 3x3 kernel
     * kernelSize is (2*kernelRadius()+1)^2
     * \return
     */
    int kernelRadius() const;
    /// TODO: update shader program if radius is changed. mark dirty program
    void setKernelRadius(int value);
    int kernelSize() const;
protected:
    virtual const float* kernel() const = 0;
    const QByteArray& kernelUniformHeader() const; //can be used in your userFragmentShaderHeader();
    const QByteArray& kernelSample() const; //can be  in your userSample();
    void setKernelUniformValue(); // can be used in your setUserUniformValues()
private:
    /// default implementions
    const char* userShaderHeader(QOpenGLShader::ShaderType t) const Q_DECL_OVERRIDE;
    const char* userSample() const Q_DECL_OVERRIDE;
    bool setUserUniformValues() Q_DECL_OVERRIDE;
protected:
    ConvolutionShader(ConvolutionShaderPrivate &d);
};
} //namespace QtAV
#endif // QTAV_CONVOLUTIONSHADER_H
