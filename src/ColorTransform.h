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

#ifndef QTAV_COLORTRANSFORM_H
#define QTAV_COLORTRANSFORM_H

#include <QtCore/QSharedDataPointer>
#include <QtGui/QMatrix4x4>
#include <QtAV/QtAV_Global.h>
// TODO: kernel QGenericMatrix<M,N>
//http://www.graficaobscura.com/matrix/index.html

namespace QtAV {
/*!
 * \brief The ColorTransform class
 *  A convenience class to get color transformation matrix.
 *  Implicitly shared.
 */
class ColorTransform
{
public:
    //http://msdn.microsoft.com/en-us/library/dd206750.aspx
    // cs: BT601 or BT709
    static const QMatrix4x4& YUV2RGB(ColorSpace cs);

    ColorTransform();
    ~ColorTransform(); //required by QSharedDataPointer if Private is forward declared
    /*!
     * \brief inputColorSpace
     * if inputColorSpace is different from outputColorSpace, then the result matrix(), matrixRef() and
     * matrixData() will count the transformation between in/out color space.
     * default in/output color space is rgb
     * \param cs
     */
    ColorSpace inputColorSpace() const;
    void setInputColorSpace(ColorSpace cs);
    ColorSpace outputColorSpace() const;
    void setOutputColorSpace(ColorSpace cs);
    /// Currently assume input is yuv, output is rgb
    void setInputColorRange(ColorRange value);
    ColorRange inputColorRange() const;
    void setOutputColorRange(ColorRange value);
    ColorRange outputColorRange() const;
    /*!
     * \brief matrix
     * \return result matrix to transform from inputColorSpace to outputColorSpace with given brightness,
     * contrast, saturation and hue
     */
    QMatrix4x4 matrix() const;
    const QMatrix4x4& matrixRef() const;

    /*!
     * Get the matrix in column-major order. Used by OpenGL
     */
    template<typename T> void matrixData(T* M) const {
        const QMatrix4x4 &m = matrixRef();
        M[0] = m(0,0), M[4] = m(0,1), M[8] = m(0,2), M[12] = m(0,3),
        M[1] = m(1,0), M[5] = m(1,1), M[9] = m(1,2), M[13] = m(1,3),
        M[2] = m(2,0), M[6] = m(2,1), M[10] = m(2,2), M[14] = m(2,3),
        M[3] = m(3,0), M[7] = m(3,1), M[11] = m(3,2), M[15] = m(3,3);
    }

    /*!
     * \brief reset
     *   only set in-space transform to identity. other parameters such as in/out color space does not change
     */
    void reset();
    // -1~1
    void setBrightness(qreal brightness);
    qreal brightness() const;
    // -1~1
    void setHue(qreal hue);
    qreal hue() const;
    // -1~1
    void setContrast(qreal contrast);
    qreal contrast() const;
    // -1~1
    void setSaturation(qreal saturation);
    qreal saturation() const;

    void setChannelDepthScale(qreal value, bool scaleAlpha = false);

private:
    class Private;
    QSharedDataPointer<ColorTransform::Private> d;
};
} //namespace QtAV
#endif // QTAV_COLORTRANSFORM_H
