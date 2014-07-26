/******************************************************************************
    ImageConverter: Base class for image resizing & color model convertion
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


#include "QtAV/private/ImageConverter_p.h"
#include "QtAV/private/AVCompat.h"
#include "QtAV/private/factory.h"
#include "QtAV/ImageConverter.h"

namespace QtAV {

FACTORY_DEFINE(ImageConverter)

extern void RegisterImageConverterFF_Man();
extern void RegisterImageConverterIPP_Man();

void ImageConverter_RegisterAll()
{
    RegisterImageConverterFF_Man();
#if QTAV_HAVE(IPP)
    RegisterImageConverterIPP_Man();
#endif //QTAV_HAVE(IPP)
}


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
    return d_func().data_out;
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
    prepareData();
}

void ImageConverter::setOutSize(int width, int height)
{
    DPTR_D(ImageConverter);
    if (d.w_out == width && d.h_out == height)
        return;
    d.w_out = width;
    d.h_out = height;
    prepareData();
}

void ImageConverter::setInFormat(const VideoFormat& format)
{
    d_func().fmt_in = format.pixelFormatFFmpeg();
}

void ImageConverter::setInFormat(VideoFormat::PixelFormat format)
{
    d_func().fmt_in = VideoFormat::pixelFormatToFFmpeg(format);
}

void ImageConverter::setInFormat(int format)
{
    d_func().fmt_in = format;
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
    d.fmt_out = format;
    prepareData();
}

void ImageConverter::setInterlaced(bool interlaced)
{
    d_func().interlaced = interlaced;
}

bool ImageConverter::isInterlaced() const
{
    return d_func().interlaced;
}

void ImageConverter::setBrightness(int value)
{
    DPTR_D(ImageConverter);
    if (d.brightness == value)
        return;
    d.brightness = value;
    setupColorspaceDetails();
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
    setupColorspaceDetails();
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
    setupColorspaceDetails();
}

int ImageConverter::saturation() const
{
    return d_func().saturation;
}

QVector<quint8*> ImageConverter::outPlanes() const
{
    DPTR_D(const ImageConverter);
    QVector<quint8*> planes(4, 0); //TODO: only real plane count?
    planes[0] = d.picture.data[0];
    planes[1] = d.picture.data[1];
    planes[2] = d.picture.data[2];
    planes[3] = d.picture.data[3];
    return planes;
}

QVector<int> ImageConverter::outLineSizes() const
{
    DPTR_D(const ImageConverter);
    QVector<int> lineSizes(4, 0);
    lineSizes[0] = d.picture.linesize[0];
    lineSizes[1] = d.picture.linesize[1];
    lineSizes[2] = d.picture.linesize[2];
    lineSizes[3] = d.picture.linesize[3];
    return lineSizes;
}

bool ImageConverter::setupColorspaceDetails()
{
    return true;
}

bool ImageConverter::prepareData()
{
    DPTR_D(ImageConverter);
    if (d.fmt_out == QTAV_PIX_FMT_C(NONE) || d.w_out <=0 || d.h_out <= 0)
        return false;
    int bytes = avpicture_get_size((AVPixelFormat)d.fmt_out, d.w_out, d.h_out);
    //if (d.data_out.size() < bytes) {
        d.data_out.resize(bytes);
    //}
    //picture的数据按PIX_FMT格式自动"关联"到 data
    avpicture_fill(
            &d.picture,
            (uint8_t*)d.data_out.constData(),
            (AVPixelFormat)d.fmt_out,
            d.w_out,
            d.h_out
            );
    return true;
}

} //namespace QtAV
