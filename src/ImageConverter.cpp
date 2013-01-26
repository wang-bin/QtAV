/******************************************************************************
    ImageConverter: Base class for image resizing & color model convertion
    Copyright (C) 2012-2013 Wang Bin <wbsecg1@gmail.com>
    
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
    d_func().fmt_out = format;
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
