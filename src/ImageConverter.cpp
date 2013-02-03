/******************************************************************************
    ImageConverter: Base class for image resizing & color model convertion
    Copyright (C) 2012-2013 Wang Bin <wbsecg1@gmail.com>
    
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


#include "ImageConverter.h"
#include <private/ImageConverter_p.h>
#include <QtAV/QtAV_Compat.h>

namespace QtAV {

ImageConverter::ImageConverter()
{
}

ImageConverter::ImageConverter(ImageConverterPrivate& d):DPTR_INIT(&d)
{
}

QByteArray ImageConverter::outData() const
{
    return d_func().data_out;
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

void ImageConverter::setInFormat(int format)
{
    d_func().fmt_in = format;
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

bool ImageConverter::prepareData()
{
    return false;
}

} //namespace QtAV
