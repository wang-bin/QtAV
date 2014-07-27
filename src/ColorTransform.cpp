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

static const QMatrix4x4 kGBR2RGB = QMatrix4x4(0, 0, 1, 0,
                                              1, 0, 0, 0,
                                              0, 1, 0, 0,
                                              0, 0, 0, 1);

static const QMatrix4x4 yuv2rgb_bt601 =
           QMatrix4x4(
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

static const QMatrix4x4 yuv2rgb_bt709 =
           QMatrix4x4(
                1.0f,  0.000f,  1.5701f, 0.0f,
                1.0f, -0.187f, -0.4664f, 0.0f,
                1.0f,  1.8556f, 0.000f,  0.0f,
                0.0f,  0.000f,  0.000f,  1.0f)
            *
            QMatrix4x4(
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, -0.5f,
                0.0f, 0.0f, 1.0f, -0.5f,
                0.0f, 0.0f, 0.0f, 1.0f);

const QMatrix4x4& ColorTransform::YUV2RGB(ColorSpace cs)
{
    switch (cs) {
    case BT601:
        return yuv2rgb_bt601;
    case BT709:
        return yuv2rgb_bt709;
    default:
        return yuv2rgb_bt601;
    }
    return yuv2rgb_bt601;
}

class ColorTransform::Private : public QSharedData
{
public:
    Private()
        : recompute(true)
        , in(ColorTransform::RGB)
        , out(ColorTransform::RGB)
        , hue(0)
        , saturation(0)
        , contrast(0)
        , brightness(0)
    {}
    Private(const Private& other)
        : QSharedData(other)
        , recompute(true)
        , in(ColorTransform::RGB)
        , out(ColorTransform::RGB)
        , hue(0)
        , saturation(0)
        , contrast(0)
        , brightness(0)
    {}
    ~Private() {}

    void reset() {
        recompute = true;
        //in = out = ColorTransform::RGB; ///
        hue = 0;
        saturation = 0;
        contrast = 0;
        brightness = 0;
        M.setToIdentity();
    }
    // TODO: optimize for other color spaces
    void compute() const {
        recompute = false;
        //http://docs.rainmeter.net/tips/colormatrix-guide
        //http://www.graficaobscura.com/matrix/index.html
        //http://beesbuzz.biz/code/hsv_color_transforms.php
        // ??
        float b = brightness;
        // brightness R,G,B
        QMatrix4x4 B(1, 0, 0, b,
                     0, 1, 0, b,
                     0, 0, 1, b,
                     0, 0, 0, 1);
        float c = contrast+1.0;
        // Contrast (offset) R,G,B
        QMatrix4x4 C(c, 0, 0, 0,
                     0, c, 0, 0,
                     0, 0, c, 0,
                     0, 0, 0, 1);
        // Saturation
        const float wr = 0.3086f;
        const float wg = 0.6094f;
        const float wb = 0.0820f;
        float s = saturation + 1.0f;
        QMatrix4x4 S(
            (1.0f - s)*wr + s, (1.0f - s)*wg    , (1.0f - s)*wb    , 0.0f,
            (1.0f - s)*wr    , (1.0f - s)*wg + s, (1.0f - s)*wb    , 0.0f,
            (1.0f - s)*wr    , (1.0f - s)*wg    , (1.0f - s)*wb + s, 0.0f,
                         0.0f,              0.0f,              0.0f, 1.0f
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
        // TODO: transform to output color space other than RGB
        switch (in) {
        case ColorTransform::RGB:
            break;
        case ColorTransform::GBR:
            M *= kGBR2RGB;
            break;
        default:
            M *= YUV2RGB(in);
            break;
        }
        switch (out) {
        case ColorTransform::RGB:
            break;
        case ColorTransform::GBR:
            M = kGBR2RGB.inverted() * M;
            break;
        default:
            M = YUV2RGB(out).inverted() * M;
            break;
        }
    }

    mutable bool recompute;
    ColorTransform::ColorSpace in, out;
    qreal hue, saturation, contrast, brightness;
    mutable QMatrix4x4 M; // count the transformations between spaces
};

ColorTransform::ColorTransform()
    : d(new Private())
{
}

ColorTransform::~ColorTransform()
{
}

ColorTransform::ColorSpace ColorTransform::inputColorSpace() const
{
    return d->in;
}

void ColorTransform::setInputColorSpace(ColorSpace cs)
{
    if (d->in == cs)
        return;
    d->in = cs;
    d->recompute = true; //TODO: only recompute color space transform
}

ColorTransform::ColorSpace ColorTransform::outputColorSpace() const
{
    return d->out;
}

void ColorTransform::setOutputColorSpace(ColorSpace cs)
{
    if (d->out == cs)
        return;
    d->out = cs;
    d->recompute = true; //TODO: only recompute color space transform
}

QMatrix4x4 ColorTransform::matrix() const
{
    if (d->recompute)
        d->compute();
    return d->M;
}

const QMatrix4x4& ColorTransform::matrixRef() const
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
