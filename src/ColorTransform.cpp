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

#define RGB_INTERMEDIATE 0
namespace QtAV {

static const QMatrix4x4 kGBR2RGB = QMatrix4x4(0, 0, 1, 0,
                                              1, 0, 0, 0,
                                              0, 1, 0, 0,
                                              0, 0, 0, 1);
#if 1
// from mpv
static QMatrix4x4 luma_coeffs(qreal lr, qreal lg, qreal lb)
{
    Q_ASSERT(qFuzzyCompare(lr + lg + lb, 1.0));
    QMatrix4x4 m;
    m(0, 0) = m(1, 0) = m(2, 0) = 1;
    m(1, 1) = -2*(1-lb)*lb/lg;
    m(2, 1) = 2*(1-lb);
    m(0, 2) = 2*(1-lr);
    m(1, 2) = -2*(1-lr)*lr/lg;
    m(2, 2) = 0;
    // YUV [0,1]x[-0.5,0.5]x[-0.5,0.5] -> [0,1]x[0,1]x[0,1]
    return m * QMatrix4x4(
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, -0.5f,
                0.0f, 0.0f, 1.0f, -0.5f,
                0.0f, 0.0f, 0.0f, 1.0f);
}
#else
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
#endif
const QMatrix4x4& ColorTransform::YUV2RGB(ColorSpace cs)
{
    static const QMatrix4x4 yuv2rgb_bt601 = luma_coeffs(0.299,  0.587,  0.114);
    static const QMatrix4x4 yuv2rgb_bt709 = luma_coeffs(0.2126, 0.7152, 0.0722);
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
    // TODO: transform to output color space other than RGB
    // ??? => RGB
    void compute() const {
        recompute = false;
        const float b = brightness;
        const float c = contrast + 1.0;
        const float s = saturation + 1.0f;
        const float h = hue * M_PI;               // hue rotation angle
        //http://docs.rainmeter.net/tips/colormatrix-guide
        //http://www.graficaobscura.com/matrix/index.html
        //http://beesbuzz.biz/code/hsv_color_transforms.php
        // ??
        // brightness translate R,G,B by a const
        QMatrix4x4 B;
        B.translate(b, b, b);
#ifndef CONTRAST_YUV
        // Contrast (offset) R,G,B
        QMatrix4x4 C;
        C.scale(c, c, c);
#endif
        if (RGB_INTERMEDIATE || in == ColorTransform::RGB || in == ColorTransform::GBR) {
            // Saturation
            const float wr = 0.3086f;
            const float wg = 0.6094f;
            const float wb = 0.0820f;
            const QMatrix4x4 S(
                (1.0f - s)*wr + s, (1.0f - s)*wg    , (1.0f - s)*wb    , 0.0f,
                (1.0f - s)*wr    , (1.0f - s)*wg + s, (1.0f - s)*wb    , 0.0f,
                (1.0f - s)*wr    , (1.0f - s)*wg    , (1.0f - s)*wb + s, 0.0f,
                             0.0f,              0.0f,              0.0f, 1.0f
            );
            // Hue
            const float n = 1.0f / sqrtf(3.0f);       // normalized hue rotation axis: sqrt(3)*(1 1 1)
            const float hc = cosf(h);
            const float hs = sinf(h);
            const QMatrix4x4 H(     // conversion of angle/axis representation to matrix representation
                n*n*(1.0f - hc) + hc  , n*n*(1.0f - hc) - n*hs, n*n*(1.0f - hc) + n*hs, 0.0f,
                n*n*(1.0f - hc) + n*hs, n*n*(1.0f - hc) + hc  , n*n*(1.0f - hc) - n*hs, 0.0f,
                n*n*(1.0f - hc) - n*hs, n*n*(1.0f - hc) + n*hs, n*n*(1.0f - hc) + hc  , 0.0f,
                                  0.0f,                   0.0f,                   0.0f, 1.0f
            );

            M = B*C*S*H;
            if (in == ColorTransform::GBR) {
                M *= kGBR2RGB;
            }
#if RGB_INTERMEDIATE
            else if (in != ColorTransform::RGB) {
                M *= YUV2RGB(in);
            }
#endif
            switch (out) {
            case ColorTransform::RGB:
                break;
            case ColorTransform::GBR:
                M = kGBR2RGB.inverted() * M;
                break;
            default: //yuv
                M = YUV2RGB(out).inverted() * M;
                break;
            }
        } else { // YUV
            QMatrix4x4 H, S;
            // rotate UV
            H.rotate(h/M_PI*180.0, 1, 0, 0); //degree
            // scale U, V
            S.scale(1, s, s);
            // TODO: contrast
#ifdef CONTRAST_YUV
            // scale Y around 1/2
            //C.scale(c, 1, 1);
            //B.translate(0.5*(1.0-c), 0.5*(1.0-c), 0.5*(1.0-c));
            //M = B * YUV2RGB(in) * H * S * C;
#else
            //M = B * C * YUV2RGB(in) * H * S;
#endif
            switch (out) {
            case ColorTransform::RGB:
                M = B * C * YUV2RGB(in) * H * S;
                break;
            case ColorTransform::GBR:
                M = B * C * YUV2RGB(in) * H * S;
                M = kGBR2RGB.inverted() * M;
                break;
            default:
                M = YUV2RGB(out).inverted() * B * C * YUV2RGB(in) * H * S; // TODO: no yuv->rgb
                break;
            }
        }
        //qDebug() << YUV2RGB(in) << M;
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
