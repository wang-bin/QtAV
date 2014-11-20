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

#include "QtAV/private/AVCompat.h"
#include "QtAV/private/prepost.h"
#include "QtAV/version.h"

void ffmpeg_version_print()
{
    struct _component {
        const char* lib;
        unsigned build_version;
        unsigned rt_version;
    } components[] = {
        { "avcodec", LIBAVCODEC_VERSION_INT, avcodec_version()},
        { "avformat", LIBAVFORMAT_VERSION_INT, avformat_version()},
        { "avutil", LIBAVUTIL_VERSION_INT, avutil_version()},
        { "swscale", LIBSWSCALE_VERSION_INT, swscale_version()},
#if QTAV_HAVE(SWRESAMPLE)
        { "swresample", LIBSWRESAMPLE_VERSION_INT, swresample_version()}, //swresample_version not declared in 0.9
#endif //QTAV_HAVE(SWRESAMPLE)
#if QTAV_HAVE(AVRESAMPLE)
        { "avresample", LIBAVRESAMPLE_VERSION_INT, avresample_version()},
#endif //QTAV_HAVE(AVRESAMPLE)
#if QTAV_HAVE(AVFILTER)
        { "avfilter", LIBAVFILTER_VERSION_INT, avfilter_version() },
#endif //QTAV_HAVE(AVFILTER)
#if QTAV_HAVE(AVDEVICE)
        { "avdevice", LIBAVDEVICE_VERSION_INT, avdevice_version() },
#endif //QTAV_HAVE(AVDEVICE)
        { 0, 0, 0}
    };
    for (int i = 0; components[i].lib != 0; ++i) {
        printf("Build with lib%s-%u.%u.%u\n"
               , components[i].lib
               , QTAV_VERSION_MAJOR(components[i].build_version)
               , QTAV_VERSION_MINOR(components[i].build_version)
               , QTAV_VERSION_PATCH(components[i].build_version)
               );
        unsigned rt_version = components[i].rt_version;
        if (components[i].build_version != rt_version) {
            fprintf(stderr, "Warning: %s runtime version %u.%u.%u mismatch!\n"
                    , components[i].lib
                    , QTAV_VERSION_MAJOR(rt_version)
                    , QTAV_VERSION_MINOR(rt_version)
                    , QTAV_VERSION_PATCH(rt_version)
                    );
        }
    }
    fflush(0);
}

//PRE_FUNC_ADD(ffmpeg_version_print); //move to Internal::Logger

#ifndef av_err2str

#endif //av_err2str

#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(51, 32, 0)
static const struct {
    const char *name;
    int         nb_channels;
    uint64_t     layout;
} channel_layout_map[] = {
    { "mono",        1,  AV_CH_LAYOUT_MONO },
    { "stereo",      2,  AV_CH_LAYOUT_STEREO },
    { "4.0",         4,  AV_CH_LAYOUT_4POINT0 },
    { "quad",        4,  AV_CH_LAYOUT_QUAD },
    { "5.0",         5,  AV_CH_LAYOUT_5POINT0 },
    { "5.0",         5,  AV_CH_LAYOUT_5POINT0_BACK },
    { "5.1",         6,  AV_CH_LAYOUT_5POINT1 },
    { "5.1",         6,  AV_CH_LAYOUT_5POINT1_BACK },
    { "5.1+downmix", 8,  AV_CH_LAYOUT_5POINT1|AV_CH_LAYOUT_STEREO_DOWNMIX, },
    { "7.1",         8,  AV_CH_LAYOUT_7POINT1 },
    { "7.1(wide)",   8,  AV_CH_LAYOUT_7POINT1_WIDE },
    { "7.1+downmix", 10, AV_CH_LAYOUT_7POINT1|AV_CH_LAYOUT_STEREO_DOWNMIX, },
    { 0 }
};
int64_t av_get_default_channel_layout(int nb_channels) {
    int i;
    for (i = 0; i < FF_ARRAY_ELEMS(channel_layout_map); i++)
        if (nb_channels == channel_layout_map[i].nb_channels)
            return channel_layout_map[i].layout;
    return 0;
}
#endif

/*
 * always need this function if avresample available
 * use AVAudioResampleContext to avoid func type confliction when swr is also available
 */
#if QTAV_HAVE(AVRESAMPLE)
AVAudioResampleContext *swr_alloc_set_opts(AVAudioResampleContext *s
                                      , int64_t out_ch_layout
                                      , enum AVSampleFormat out_sample_fmt
                                      , int out_sample_rate
                                      , int64_t in_ch_layout
                                      , enum AVSampleFormat in_sample_fmt
                                      , int in_sample_rate
                                      , int log_offset, void *log_ctx)
{
    //DO NOT use swr_alloc() because it's not defined as a macro in QtAV_Compat.h
    if (!s)
        s = avresample_alloc_context();
    if (!s)
        return 0;

    Q_UNUSED(log_offset);
    Q_UNUSED(log_ctx);

    av_opt_set_int(s, "out_channel_layout", out_ch_layout  , 0);
    av_opt_set_int(s, "out_sample_fmt"    , out_sample_fmt , 0);
    av_opt_set_int(s, "out_sample_rate"   , out_sample_rate, 0);
    av_opt_set_int(s, "in_channel_layout" , in_ch_layout   , 0);
    av_opt_set_int(s, "in_sample_fmt"     , in_sample_fmt  , 0);
    av_opt_set_int(s, "in_sample_rate"    , in_sample_rate , 0);
    return s;
}
#endif

#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(52, 13, 100)
extern const AVPixFmtDescriptor av_pix_fmt_descriptors[];
const AVPixFmtDescriptor *av_pix_fmt_desc_get(AVPixelFormat pix_fmt)
{
    if (pix_fmt < 0 || pix_fmt >= PIX_FMT_NB)
        return NULL;
    return &av_pix_fmt_descriptors[pix_fmt];
}

#endif //AV_VERSION_INT(52, 13, 100)

#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(52, 38, 100)
int av_pix_fmt_count_planes(AVPixelFormat pix_fmt)
{
    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(pix_fmt);
    int i, planes[4] = { 0 }, ret = 0;

    if (!desc)
        return AVERROR(EINVAL);

    for (i = 0; i < desc->nb_components; i++)
        planes[desc->comp[i].plane] = 1;
    for (i = 0; i < FF_ARRAY_ELEMS(planes); i++)
        ret += planes[i];
    return ret;
}
#endif //AV_VERSION_INT(52, 38, 100)

#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(51, 73, 101)
int av_samples_copy(uint8_t **dst, uint8_t * const *src, int dst_offset,
                    int src_offset, int nb_samples, int nb_channels,
                    enum AVSampleFormat sample_fmt)
{
    int planar      = av_sample_fmt_is_planar(sample_fmt);
    int planes      = planar ? nb_channels : 1;
    int block_align = av_get_bytes_per_sample(sample_fmt) * (planar ? 1 : nb_channels);
    int data_size   = nb_samples * block_align;
    int i;

    dst_offset *= block_align;
    src_offset *= block_align;

    if((dst[0] < src[0] ? src[0] - dst[0] : dst[0] - src[0]) >= data_size) {
        for (i = 0; i < planes; i++)
            memcpy(dst[i] + dst_offset, src[i] + src_offset, data_size);
    } else {
        for (i = 0; i < planes; i++)
            memmove(dst[i] + dst_offset, src[i] + src_offset, data_size);
    }

    return 0;
}
#endif //AV_VERSION_INT(51, 73, 101)

#if QTAV_USE_LIBAV(LIBAVCODEC)
const char *avcodec_get_name(enum AVCodecID id)
{
    const AVCodecDescriptor *cd;
    AVCodec *codec;

    if (id == AV_CODEC_ID_NONE)
        return "none";
    cd = avcodec_descriptor_get(id);
    if (cd)
        return cd->name;
    av_log(NULL, AV_LOG_WARNING, "Codec 0x%x is not in the full list.\n", id);
    codec = avcodec_find_decoder(id);
    if (codec)
        return codec->name;
    codec = avcodec_find_encoder(id);
    if (codec)
        return codec->name;
    return "unknown_codec";
}
#endif

// since libav-11, ffmpeg-2.1
#if !LIBAV_MODULE_CHECK(LIBAVCODEC, 56, 1, 0) && !FFMPEG_MODULE_CHECK(LIBAVCODEC, 55, 39, 100)
int av_packet_copy_props(AVPacket *dst, const AVPacket *src)
{
    dst->pts                  = src->pts;
    dst->dts                  = src->dts;
    dst->pos                  = src->pos;
    dst->duration             = src->duration;
    dst->convergence_duration = src->convergence_duration;
    dst->flags                = src->flags;
    dst->stream_index         = src->stream_index;

    for (int i = 0; i < src->side_data_elems; i++) {
         enum AVPacketSideDataType type = src->side_data[i].type;
         int size          = src->side_data[i].size;
         uint8_t *src_data = src->side_data[i].data;
         uint8_t *dst_data = av_packet_new_side_data(dst, type, size);

        if (!dst_data) {
            av_packet_free_side_data(dst);
            return AVERROR(ENOMEM);
        }
        memcpy(dst_data, src_data, size);
    }

    return 0;
}

#endif
// since libav-10, ffmpeg-2.1
#if !LIBAV_MODULE_CHECK(LIBAVCODEC, 55, 34, 1) && !FFMPEG_MODULE_CHECK(LIBAVCODEC, 55, 39, 100)
void av_packet_free_side_data(AVPacket *pkt)
{
    for (int i = 0; i < pkt->side_data_elems; ++i)
        av_freep(&pkt->side_data[i].data);
    av_freep(&pkt->side_data);
    pkt->side_data_elems = 0;
}
#endif

