/******************************************************************************
    ImageConverterIPP: Image resizing & color model convertion using Intel IPP
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

#include <QtAV/ImageConverterIPP.h>
#include <private/ImageConverter_p.h>
#include <QtAV/QtAV_Compat.h>

#define LINK_IPP 1
#if LINK_IPP
#include <ipp.h>
#else

#endif

namespace QtAV {

class ImageConverterIPPPrivate : public ImageConverterPrivate
{
public:

};

ImageConverterIPP::ImageConverterIPP()
    :ImageConverter(*new ImageConverterIPPPrivate())
{
}

bool ImageConverterIPP::convert(const quint8 *const srcSlice[], const int srcStride[])
{
    DPTR_D(ImageConverterIPP);
    ippiYUV420ToRGB_8u_P3C3(const_cast<const quint8 **>(srcSlice), (Ipp8u*)(d.data_out.data()), (IppiSize){d.w_in, d.h_in});
    return true;
}

bool ImageConverterIPP::prepareData()
{
    DPTR_D(ImageConverterIPP);
    int bytes = avpicture_get_size((PixelFormat)d.fmt_out, d.w_out, d.h_out);
    if(d.data_out.size() < bytes) {
        d.data_out.resize(bytes);
    }
    return true;
}

} //namespace QtAV
