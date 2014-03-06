/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_COLORTRANSFORM_H
#define QTAV_COLORTRANSFORM_H

#include <QtCore/QSharedDataPointer>
#include <QMatrix4x4>

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
    ColorTransform();
    ~ColorTransform(); //required by QSharedDataPointer if Private is forward declared
    // TODO: type bt601 etc
    static QMatrix4x4 YUV2RGB();

    QMatrix4x4 matrix() const;
    const QMatrix4x4& matrixRef() const;
    template<typename T> void matrixData(T* M) {
        const QMatrix4x4 &m = matrixRef();
        M[0] = m(0,0), M[4] = m(0,1), M[8] = m(0,2), M[12] = m(0,3),
        M[1] = m(1,0), M[5] = m(1,1), M[9] = m(1,2), M[13] = m(1,3),
        M[2] = m(2,0), M[6] = m(2,1), M[10] = m(2,2), M[14] = m(2,3),
        M[3] = m(3,0), M[7] = m(3,1), M[11] = m(3,2), M[15] = m(3,3);
    }

    /*!
     * \brief reset
     *   set to identity
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

private:
    class Private;
    QSharedDataPointer<ColorTransform::Private> d;
};

} //namespace QtAV

#endif // QTAV_COLORTRANSFORM_H
