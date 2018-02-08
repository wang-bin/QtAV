/******************************************************************************
    ImageConverter: Base class for image resizing & color model convertion
    Copyright (C) 2012-2018 Wang Bin <wbsecg1@gmail.com>
    
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


#include "ImageConverter_p.h"
#include "QtAV/private/AVCompat.h"
#include "QtAV/private/factory.h"
#include "ImageConverter.h"
#include "utils/Logger.h"

namespace QtAV {

FACTORY_DEFINE(ImageConverter)

ImageConverter::ImageConverter()
{
}

ImageConverter::ImageConverter(ImageConverterPrivate& d)
    : DPTR_INIT(&d)
{
}

ImageConverter::~ImageConverter()
{
}

QByteArray ImageConverter::outData() const
{
    DPTR_D(const ImageConverter);
    return d.data_out;
}

bool ImageConverter::check() const
{
    DPTR_D(const ImageConverter);
    return d.w_in > 0 && d.w_out > 0 && d.h_in > 0 && d.h_out > 0
            && d.fmt_in != QTAV_PIX_FMT_C(NONE) && d.fmt_out != QTAV_PIX_FMT_C(NONE);
}

void ImageConverter::setInSize(int width, int height)
{
    DPTR_D(ImageConverter);
    if (d.w_in == width && d.h_in == height)
        return;
    d.w_in = width;
    d.h_in = height;
}

// TODO: default is in size
void ImageConverter::setOutSize(int width, int height)
{
    DPTR_D(ImageConverter);
    if (d.w_out == width && d.h_out == height)
        return;
    d.w_out = width;
    d.h_out = height;
    d.update_data = true;
    prepareData();
    d.update_data = false;
}

void ImageConverter::setInFormat(const VideoFormat& format)
{
    d_func().fmt_in = (AVPixelFormat)format.pixelFormatFFmpeg();
}

void ImageConverter::setInFormat(VideoFormat::PixelFormat format)
{
    d_func().fmt_in = (AVPixelFormat)VideoFormat::pixelFormatToFFmpeg(format);
}

void ImageConverter::setInFormat(int format)
{
    d_func().fmt_in = (AVPixelFormat)format;
}

void ImageConverter::setOutFormat(const VideoFormat& format)
{
    setOutFormat(format.pixelFormatFFmpeg());
}

void ImageConverter::setOutFormat(VideoFormat::PixelFormat format)
{
    setOutFormat(VideoFormat::pixelFormatToFFmpeg(format));
}

void ImageConverter::setOutFormat(int format)
{
    DPTR_D(ImageConverter);
    if (d.fmt_out == format)
        return;
    d.fmt_out = (AVPixelFormat)format;
    d.update_data = true;
    prepareData();
    d.update_data = false;
}

void ImageConverter::setInRange(ColorRange range)
{
    DPTR_D(ImageConverter);
    if (d.range_in == range)
        return;
    d.range_in = range;
    // TODO: do not call setupColorspaceDetails(). use a wrapper convert() func and call there
    d.setupColorspaceDetails();
}

ColorRange ImageConverter::inRange() const
{
    return d_func().range_in;
}

void ImageConverter::setOutRange(ColorRange range)
{
    DPTR_D(ImageConverter);
    if (d.range_out == range)
        return;
    d.range_out = range;
    d.setupColorspaceDetails();
}

ColorRange ImageConverter::outRange() const
{
    return d_func().range_out;
}

void ImageConverter::setBrightness(int value)
{
    DPTR_D(ImageConverter);
    if (d.brightness == value)
        return;
    d.brightness = value;
    d.setupColorspaceDetails();
}

int ImageConverter::brightness() const
{
    return d_func().brightness;
}

void ImageConverter::setContrast(int value)
{
    DPTR_D(ImageConverter);
    if (d.contrast == value)
        return;
    d.contrast = value;
    d.setupColorspaceDetails();
}

int ImageConverter::contrast() const
{
    return d_func().contrast;
}

void ImageConverter::setSaturation(int value)
{
    DPTR_D(ImageConverter);
    if (d.saturation == value)
        return;
    d.saturation = value;
    d.setupColorspaceDetails();
}

int ImageConverter::saturation() const
{
    return d_func().saturation;
}

QVector<quint8*> ImageConverter::outPlanes() const
{
    return d_func().bits;
}

QVector<int> ImageConverter::outLineSizes() const
{
    return d_func().pitchs;
}

bool ImageConverter::convert(const quint8 * const src[], const int srcStride[])
{
    DPTR_D(ImageConverter);
    if (d.update_data && !prepareData()) {
        qWarning("prepair output data error");
        return false;
    } else {
        d.update_data = false;
    }
    return convert(src, srcStride, (uint8_t**)d.bits.constData(), d.pitchs.constData());
}

bool ImageConverter::prepareData()
{
    DPTR_D(ImageConverter);
    if (d.fmt_out == QTAV_PIX_FMT_C(NONE) || d.w_out <=0 || d.h_out <= 0)
        return false;
    AV_ENSURE(av_image_check_size(d.w_out, d.h_out, 0, NULL), false);
    const int nb_planes = qMax(av_pix_fmt_count_planes(d.fmt_out), 0);
    d.bits.resize(nb_planes);
    d.pitchs.resize(nb_planes);
    // alignment is 16. sws in ffmpeg is 16, libav10 is 8. if not aligned sws will print warnings and go slow code paths
    const int kAlign = DataAlignment;
    AV_ENSURE(av_image_fill_linesizes((int*)d.pitchs.constData(), d.fmt_out, kAlign > 7 ? FFALIGN(d.w_out, 8) : d.w_out), false);
    for (int i = 0; i < d.pitchs.size(); ++i)
        d.pitchs[i] = FFALIGN(d.pitchs[i], kAlign);
    int s = av_image_fill_pointers((uint8_t**)d.bits.constData(), d.fmt_out, d.h_out, NULL, d.pitchs.constData());
    if (s < 0)
        return false;
    d.data_out.resize(s + kAlign-1);
    d.out_offset = (kAlign - ((uintptr_t)d.data_out.constData() & (kAlign-1))) & (kAlign-1);
    AV_ENSURE(av_image_fill_pointers((uint8_t**)d.bits.constData(), d.fmt_out, d.h_out, (uint8_t*)d.data_out.constData()+d.out_offset, d.pitchs.constData()), false);
    // TODO: special formats
    //if (desc->flags & AV_PIX_FMT_FLAG_PAL || desc->flags & AV_PIX_FMT_FLAG_PSEUDOPAL)
       //    avpriv_set_systematic_pal2((uint32_t*)pointers[1], pix_fmt);
    for (int i = 0; i < d.pitchs.size(); ++i) {
        Q_ASSERT(d.pitchs[i]%kAlign == 0);
        Q_ASSERT(qptrdiff(d.bits[i])%kAlign == 0);
    }
    return true;
}

} //namespace QtAV
