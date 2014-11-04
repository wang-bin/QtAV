/******************************************************************************
    ImageConverterFF: Image resizing & color model convertion using FFmpeg swscale
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

#include "QtAV/ImageConverter.h"
#include "QtAV/private/ImageConverter_p.h"
#include "QtAV/private/AVCompat.h"
#include "QtAV/private/mkid.h"
#include "QtAV/private/prepost.h"
#include "utils/Logger.h"

namespace QtAV {

class ImageConverterFFPrivate;
class ImageConverterFF : public ImageConverter //Q_AV_EXPORT is not needed
{
    DPTR_DECLARE_PRIVATE(ImageConverterFF)
public:
    ImageConverterFF();
    virtual bool check() const;
    virtual bool convert(const quint8 *const srcSlice[], const int srcStride[]);
protected:
    virtual bool setupColorspaceDetails();
};


ImageConverterId ImageConverterId_FF = mkid32base36_6<'F', 'F', 'm', 'p', 'e', 'g'>::value;
FACTORY_REGISTER_ID_AUTO(ImageConverter, FF, "FFmpeg")

void RegisterImageConverterFF_Man()
{
    FACTORY_REGISTER_ID_MAN(ImageConverter, FF, "FFmpeg")
}

class ImageConverterFFPrivate : public ImageConverterPrivate
{
public:
    ImageConverterFFPrivate()
        : sws_ctx(0)
        , update_eq(true)
    {}
    ~ImageConverterFFPrivate() {
        if (sws_ctx) {
            sws_freeContext(sws_ctx);
            sws_ctx = 0;
        }
    }

    SwsContext *sws_ctx;
    bool update_eq;
};

ImageConverterFF::ImageConverterFF()
    :ImageConverter(*new ImageConverterFFPrivate())
{
}

bool ImageConverterFF::check() const
{
    if (!ImageConverter::check())
        return false;
    DPTR_D(const ImageConverterFF);
    if (sws_isSupportedInput((AVPixelFormat)d.fmt_in) <= 0) {
        qWarning("Input pixel format not supported (%s)", av_get_pix_fmt_name((AVPixelFormat)d.fmt_in));
        return false;
    }
    if (sws_isSupportedOutput((AVPixelFormat)d.fmt_out) <= 0) {
        qWarning("Output pixel format not supported (%s)", av_get_pix_fmt_name((AVPixelFormat)d.fmt_out));
        return false;
    }
    return true;
}

bool ImageConverterFF::convert(const quint8 *const srcSlice[], const int srcStride[])
{
    DPTR_D(ImageConverterFF);
    //Check out dimension. equals to in dimension if not setted. TODO: move to another common func
    if (d.w_out == 0 || d.h_out == 0) {
        if (d.w_in == 0 || d.h_in == 0)
            return false;
        setOutSize(d.w_in, d.h_in);
    }
//TODO: move those code to prepare()
    d.sws_ctx = sws_getCachedContext(d.sws_ctx
            , d.w_in, d.h_in, (AVPixelFormat)d.fmt_in
            , d.w_out, d.h_out, (AVPixelFormat)d.fmt_out
            , (d.w_in == d.w_out && d.h_in == d.h_out) ? SWS_POINT : SWS_FAST_BILINEAR //SWS_BICUBIC
            , NULL, NULL, NULL
            );
    //int64_t flags = SWS_CPU_CAPS_SSE2 | SWS_CPU_CAPS_MMX | SWS_CPU_CAPS_MMX2;
    //av_opt_set_int(d.sws_ctx, "sws_flags", flags, 0);
    if (!d.sws_ctx)
        return false;
    setupColorspaceDetails();
#if PREPAREDATA_NO_PICTURE //for YUV420 <=> RGB
#if 0
    struct
    {
        uint8_t *data[4]; //AV_NUM_DATA_POINTERS
        int linesize[4];  //AV_NUM_DATA_POINTERS
    }
#else
    AVPicture
#endif
            pic_in, pic_out;

    if ((AVPixelFormat)fmt_in == PIX_FMT_YUV420P) {
        pic_in.data[0] = (uint8_t*)in;
        pic_in.data[2] = (uint8_t*)pic_in.data[0] + (w_in * h_in);
        pic_in.data[1] = (uint8_t*)pic_in.data[2] + (w_in * h_in) / 4;
        pic_in.linesize[0] = w_in;
        pic_in.linesize[1] = w_in / 2;
        pic_in.linesize[2] = w_in / 2;
        //pic_in.linesize[3] = 0; //not used
    } else {
        pic_in.data[0] = (uint8_t*)in;
        pic_in.linesize[0] = w_in * 4; //TODO: not 0
    }
    if ((AVPixelFormat)fmt_out == PIX_FMT_YUV420P) {
        pic_out.data[0] = (uint8_t*)out;
        pic_out.data[2] = (uint8_t*)pic_out.data[0] + (w_out * h_in);
        pic_out.data[1] = (uint8_t*)pic_out.data[2] + (w_out * h_in) / 4;
        //pic_out.data[3] = (uint8_t*)pic_out.data[0] - 1;
        pic_out.linesize[0] = w_out;
        pic_out.linesize[1] = w_out / 2;
        pic_out.linesize[2] = w_out / 2;
        //3 not used
    } else {
        pic_out.data[0] = (uint8_t*)out;
        pic_out.linesize[0] = w_out * 4;
    }
#endif //PREPAREDATA_NO_PICTURE
    int result_h = sws_scale(d.sws_ctx, srcSlice, srcStride, 0, d.h_in, d.picture.data, d.picture.linesize);
    if (result_h != d.h_out) {
        qDebug("convert failed: %d, %d", result_h, d.h_out);
        return false;
    }
#if 0
    if (isInterlaced()) {
        //deprecated
        avpicture_deinterlace(&d.picture, &d.picture, (AVPixelFormat)d.fmt_out, d.w_out, d.h_out);
    }
#endif //0
    Q_UNUSED(result_h);
    return true;
}

bool ImageConverterFF::setupColorspaceDetails()
{
    DPTR_D(ImageConverterFF);
    if (!d.sws_ctx) {
        d.update_eq = true;
        return false;
    }
    //if (!d.update_eq)
    //    return true;
    // FIXME: how to fill the ranges?
    const int srcRange = 1;
    const int dstRange = 0;
    // TODO: SWS_CS_DEFAULT?
    sws_setColorspaceDetails(d.sws_ctx, sws_getCoefficients(SWS_CS_DEFAULT)
                             , srcRange, sws_getCoefficients(SWS_CS_DEFAULT)
                             , dstRange
                             , ((d.brightness << 16) + 50)/100
                             , (((d.contrast + 100) << 16) + 50)/100
                             , (((d.saturation + 100) << 16) + 50)/100
                             );
    // TODO: b, c, s map function?
    //sws_init_context(d.sws_ctx, NULL, NULL);
    d.update_eq = false;
    return true;
}

} //namespace QtAV
