/******************************************************************************
    ImageConverterFF: Image resizing & color model convertion using FFmpeg swscale
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

#include <QtAV/ImageConverterFF.h>
#include <private/ImageConverter_p.h>
#include <QtAV/QtAV_Compat.h>

namespace QtAV {

class ImageConverterFFPrivate : public ImageConverterPrivate
{
public:
    ImageConverterFFPrivate():sws_ctx(0){}
    ~ImageConverterFFPrivate() {
        if (sws_ctx) {
            sws_freeContext(sws_ctx);
            sws_ctx = 0;
        }
    }

    SwsContext *sws_ctx;
    AVPicture picture;
};

ImageConverterFF::ImageConverterFF()
    :ImageConverter(*new ImageConverterFFPrivate())
{
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

    d.sws_ctx = sws_getCachedContext(d.sws_ctx
            , d.w_in, d.h_in, (PixelFormat)d.fmt_in
            , d.w_out, d.h_out, (PixelFormat)d.fmt_out
            , (d.w_in == d.w_out && d.h_in == d.h_out) ? SWS_POINT : SWS_FAST_BILINEAR //SWS_BICUBIC
            , NULL, NULL, NULL
            );
    //int64_t flags = SWS_CPU_CAPS_SSE2 | SWS_CPU_CAPS_MMX | SWS_CPU_CAPS_MMX2;
    //av_opt_set_int(d.sws_ctx, "sws_flags", flags, 0);
    if (!d.sws_ctx)
        return false;
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

    if ((PixelFormat)fmt_in == PIX_FMT_YUV420P) {
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
    if ((PixelFormat)fmt_out == PIX_FMT_YUV420P) {
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
    if (isInterlaced()) {
        avpicture_deinterlace(&d.picture, &d.picture, (PixelFormat)d.fmt_out, d.w_out, d.h_out);
    }
    Q_UNUSED(result_h);
    return true;
}

bool ImageConverterFF::prepareData()
{
    DPTR_D(ImageConverterFF);
    int bytes = avpicture_get_size((PixelFormat)d.fmt_out, d.w_out, d.h_out);
    //if (d.data_out.size() < bytes) {
        d.data_out.resize(bytes);
    //}
    //picture的数据按PIX_FMT格式自动"关联"到 data
    avpicture_fill(
            &d.picture,
            reinterpret_cast<uint8_t*>(d.data_out.data()),
            (PixelFormat)d.fmt_out,
            d.w_out,
            d.h_out
            );
    return true;
}

} //namespace QtAV
