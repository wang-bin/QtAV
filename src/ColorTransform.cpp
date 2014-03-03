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

#include "QtAV/ColorTransform.h"
#include <QtCore/qmath.h>

namespace QtAV {

// TODO: type bt601 etc
QMatrix4x4 ColorTransform::YUV2RGB()
{
    return QMatrix4x4(
                1.0f,  0.000f,  1.402f, 0.0f,
                1.0f, -0.344f, -0.714f, 0.0f,
                1.0f,  1.772f,  0.000f, 0.0f,
                0.0f,  0.000f,  0.000f, 1.0f)
            *
            QMatrix4x4(
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, -0.5f,
                0.0f, 0.0f, 1.0f, -0.5f,
                0.0f, 0.0f, 0.0f, 1.0f);
}


class ColorTransform::Private : public QSharedData
{
public:
    Private()
        : recompute(true)
        , hue(0)
        , saturation(0)
        , contrast(0)
        , brightness(0)
    {}
    Private(const Private& other)
        : QSharedData(other)
        , recompute(true)
        , hue(0)
        , saturation(0)
        , contrast(0)
        , brightness(0)
    {}
    ~Private() {}

    void reset() {
        recompute = true;
        hue = 0;
        saturation = 0;
        contrast = 0;
        brightness = 0;
        M.setToIdentity();
    }
    void compute() const {
        recompute = false;
        // ??
        float b = (brightness < 0.0f ? brightness : 4.0f * brightness) + 1.0f;
        // brightness R,G,B
        QMatrix4x4 B(b, 0, 0, 0,
                     0, b, 0, 0,
                     0, 0, b, 0,
                     0, 0, 0, 1);
        // ??
        float c = -contrast;
        // Contrast (offset) R,G,B
        QMatrix4x4 C(1, 0, 0, 0,
                     0, 1, 0, 0,
                     0, 0, 0, 1,
                     c, c, c, 1);

        // Saturation
        const float wr = 0.3086f;
        const float wg = 0.6094f;
        const float wb = 0.0820f;
        float s = saturation + 1.0f;
        QMatrix4x4 S(
            (1.0f - s)*wr + s, (1.0f - s)*wg    , (1.0f - s)*wb    , 0.0f,
            (1.0f - s)*wr    , (1.0f - s)*wg + s, (1.0f - s)*wb    , 0.0f,
            (1.0f - s)*wr    , (1.0f - s)*wg    , (1.0f - s)*wb + s, 0.0f,
                           0.0f,                0.0f,                0.0f, 1.0f
        );
        // Hue
        const float n = 1.0f / sqrtf(3.0f);       // normalized hue rotation axis: sqrt(3)*(1 1 1)
        const float h = hue*M_PI;               // hue rotation angle
        const float hc = cosf(h);
        const float hs = sinf(h);
        QMatrix4x4 H(     // conversion of angle/axis representation to matrix representation
            n*n*(1.0f - hc) + hc  , n*n*(1.0f - hc) - n*hs, n*n*(1.0f - hc) + n*hs, 0.0f,
            n*n*(1.0f - hc) + n*hs, n*n*(1.0f - hc) + hc  , n*n*(1.0f - hc) - n*hs, 0.0f,
            n*n*(1.0f - hc) - n*hs, n*n*(1.0f - hc) + n*hs, n*n*(1.0f - hc) + hc  , 0.0f,
                              0.0f,                   0.0f,                   0.0f, 1.0f
        );

        M = B*C*S*H;
    }

    mutable bool recompute;
    qreal hue, saturation, contrast, brightness;
    mutable QMatrix4x4 M;
};

ColorTransform::ColorTransform()
    : d(new Private())
{
}

QMatrix4x4 ColorTransform::matrix() const
{
    if (d->recompute)
        d->compute();
    return d->M;
}


/*!
 * \brief reset
 *   set to identity
 */
void ColorTransform::reset()
{
    d->reset();
}

void ColorTransform::setBrightness(qreal brightness)
{
    if (d->brightness == brightness)
        return;
    d->brightness = brightness;
    d->recompute = true;
}

qreal ColorTransform::brightness() const
{
    return d->brightness;
}

// -1~1
void ColorTransform::setHue(qreal hue)
{
    if (d->hue == hue)
        return;
    d->hue = hue;
    d->recompute = true;
}

qreal ColorTransform::hue() const
{
    return d->hue;
}

void ColorTransform::setContrast(qreal contrast)
{
    if (d->contrast == contrast)
        return;
    d->contrast = contrast;
    d->recompute = true;
}

qreal ColorTransform::contrast() const
{
    return d->contrast;
}

void ColorTransform::setSaturation(qreal saturation)
{
    if (d->saturation == saturation)
        return;
    d->saturation = saturation;
    d->recompute = true;
}

qreal ColorTransform::saturation() const
{
    return d->saturation;
}

} //namespace QtAV
