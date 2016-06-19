/******************************************************************************
    ImageConverterIPP: Image resizing & color model convertion using Intel IPP
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

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
#include "ImageConverter_p.h"
#include "QtAV/private/AVCompat.h"
#include "QtAV/private/factory.h"
#include "QtAV/private/mkid.h"
#if QTAV_HAVE(IPP)
#include <ipp.h>
#endif
#include "utils/Logger.h"

namespace QtAV {

class ImageConverterIPPPrivate;
class ImageConverterIPP : public ImageConverter //Q_AV_EXPORT is not needed
{
    DPTR_DECLARE_PRIVATE(ImageConverterIPP)
public:
    ImageConverterIPP();
    virtual bool convert(const quint8 *const srcSlice[], const int srcStride[]);
protected:
    virtual bool prepareData(); //Allocate memory for out data
};

ImageConverterId ImageConverterId_IPP = mkid::id32base36_3<'I', 'P', 'P'>::value;
FACTORY_REGISTER(ImageConverter, IPP, "IPP")

class ImageConverterIPPPrivate : public ImageConverterPrivate
{
public:
    ImageConverterIPPPrivate():need_scale(true) {}
    bool need_scale;
    QByteArray orig_ori_rgb;
};

ImageConverterIPP::ImageConverterIPP()
    :ImageConverter(*new ImageConverterIPPPrivate())
{
}

bool ImageConverterIPP::convert(const quint8 *const srcSlice[], const int srcStride[])
{
    DPTR_D(ImageConverterIPP);
    //color convertion, no scale
#if QTAV_HAVE(IPP)
    struct {
        const quint8 *data[3];
        int linesize[3];
    } yuv = {
        { srcSlice[0], srcSlice[2], srcSlice[1] },
        { srcStride[0], srcStride[2], srcStride[1] }
    };
    //ippiSwapChannels
    ippiYUV420ToRGB_8u_P3AC4R(const_cast<const quint8 **>(yuv.data), const_cast<int*>(yuv.linesize), (Ipp8u*)(d.orig_ori_rgb.data())
                           , 4*sizeof(quint8)*d.w_in, (IppiSize){d.w_in, d.h_in});
    d.data_out = d.orig_ori_rgb;
    return true;
    if (d.need_scale) {
        qDebug("rs");
        ippiResize_8u_AC4R((const Ipp8u*)d.orig_ori_rgb.data(), (IppiSize){d.w_in, d.h_in}, 4*sizeof(quint8)*d.w_in, (IppiRect){0, 0, d.w_in, d.h_in}
                  , (Ipp8u*)d.data_out.data(), 4*sizeof(quint8)*d.w_in, (IppiSize){d.w_out, d.h_out}
                  , (double)d.w_out/(double)d.w_in, (double)d.h_out/(double)d.h_in, IPPI_INTER_CUBIC);
    } else {
        d.data_out = d.orig_ori_rgb;
    }
#endif
    return true;
}

//TODO: call it when out format is setted. and avoid too much calls
bool ImageConverterIPP::prepareData()
{
    DPTR_D(ImageConverterIPP);
    //for color convertion
    if (d.w_in > 0 && d.h_in > 0) {
        qDebug("in size=%d x %d", d.w_in, d.h_in);
        int bytes = avpicture_get_size((PixelFormat)d.fmt_out, d.w_in, d.h_in);
        //if(d.orig_ori_rgb.size() < bytes) {
            d.orig_ori_rgb.resize(bytes);
        //}
    }
    return ImageConverter::prepareData();
}

} //namespace QtAV
