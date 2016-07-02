/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
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

#include "QtAV/private/AVCompat.h"
#include "QtAV/version.h"

#if !FFMPEG_MODULE_CHECK(LIBAVFORMAT, 56, 4, 101)
int avio_feof(AVIOContext *s)
{
#if QTAV_USE_FFMPEG(LIBAVFORMAT)
    return url_feof(s);
#else
    return s && s->eof_reached;
#endif
}
#endif
#if QTAV_USE_LIBAV(LIBAVFORMAT)
int avformat_alloc_output_context2(AVFormatContext **avctx, AVOutputFormat *oformat, const char *format, const char *filename)
{
    AVFormatContext *s = avformat_alloc_context();
    int ret = 0;

    *avctx = NULL;
    if (!s)
        goto nomem;

    if (!oformat) {
        if (format) {
            oformat = av_guess_format(format, NULL, NULL);
            if (!oformat) {
                av_log(s, AV_LOG_ERROR, "Requested output format '%s' is not a suitable output format\n", format);
                ret = AVERROR(EINVAL);
                goto error;
            }
        } else {
            oformat = av_guess_format(NULL, filename, NULL);
            if (!oformat) {
                ret = AVERROR(EINVAL);
                av_log(s, AV_LOG_ERROR, "Unable to find a suitable output format for '%s'\n",
                       filename);
                goto error;
            }
        }
    }

    s->oformat = oformat;
    if (s->oformat->priv_data_size > 0) {
        s->priv_data = av_mallocz(s->oformat->priv_data_size);
        if (!s->priv_data)
            goto nomem;
        if (s->oformat->priv_class) {
            *(const AVClass**)s->priv_data= s->oformat->priv_class;
            av_opt_set_defaults(s->priv_data);
        }
    } else
        s->priv_data = NULL;

    if (filename)
        av_strlcpy(s->filename, filename, sizeof(s->filename));
    *avctx = s;
    return 0;
nomem:
    av_log(s, AV_LOG_ERROR, "Out of memory\n");
    ret = AVERROR(ENOMEM);
error:
    avformat_free_context(s);
    return ret;
}
#endif //QTAV_USE_LIBAV(LIBAVFORMAT)
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

#if !AV_MODULE_CHECK(LIBAVUTIL, 52, 3, 0, 13, 100)
extern const AVPixFmtDescriptor av_pix_fmt_descriptors[];
const AVPixFmtDescriptor *av_pix_fmt_desc_get(AVPixelFormat pix_fmt)
{
    if (pix_fmt < 0 || pix_fmt >= QTAV_PIX_FMT_C(NB))
        return NULL;
    return &av_pix_fmt_descriptors[pix_fmt];
}

const AVPixFmtDescriptor *av_pix_fmt_desc_next(const AVPixFmtDescriptor *prev)
{
    if (!prev)
        return &av_pix_fmt_descriptors[0];
    // can not use sizeof(av_pix_fmt_descriptors)
    while (prev - av_pix_fmt_descriptors < QTAV_PIX_FMT_C(NB) - 1) {
        prev++;
        if (prev->name)
            return prev;
    }
    return NULL;
}

AVPixelFormat av_pix_fmt_desc_get_id(const AVPixFmtDescriptor *desc)
{
    if (desc < av_pix_fmt_descriptors ||
        desc >= av_pix_fmt_descriptors + QTAV_PIX_FMT_C(NB))
        return QTAV_PIX_FMT_C(NONE);

    return AVPixelFormat(desc - av_pix_fmt_descriptors);
}
#endif // !AV_MODULE_CHECK(LIBAVUTIL, 52, 3, 0, 13, 100)
#if !FFMPEG_MODULE_CHECK(LIBAVUTIL, 52, 48, 101)
enum AVColorSpace av_frame_get_colorspace(const AVFrame *frame)
{
    if (!frame)
        return AVCOL_SPC_NB;
#if LIBAV_MODULE_CHECK(LIBAVUTIL, 53, 16, 0) //8c02adc
    return frame->colorspace;
#endif
    return AVCOL_SPC_NB;
}

enum AVColorRange av_frame_get_color_range(const AVFrame *frame)
{
    if (!frame)
        return AVCOL_RANGE_UNSPECIFIED;
#if LIBAV_MODULE_CHECK(LIBAVUTIL, 53, 16, 0) //8c02adc
    return frame->color_range;
#endif
    return AVCOL_RANGE_UNSPECIFIED;
}
#endif //!FFMPEG_MODULE_CHECK(LIBAVUTIL, 52, 28, 101)
#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(52, 38, 100)
int av_pix_fmt_count_planes(AVPixelFormat pix_fmt)
{
    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(pix_fmt);
    int i, planes[4] = { 0 }, ret = 0;

    if (!desc)
        return AVERROR(EINVAL);

    for (i = 0; i < desc->nb_components; i++)
        planes[desc->comp[i].plane] = 1;
    for (i = 0; i < (int)FF_ARRAY_ELEMS(planes); i++)
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

#if !AV_MODULE_CHECK(LIBAVCODEC, 55, 55, 0, 68, 100)
void av_packet_rescale_ts(AVPacket *pkt, AVRational src_tb, AVRational dst_tb)
{
    if (pkt->pts != (int64_t)AV_NOPTS_VALUE)
        pkt->pts = av_rescale_q(pkt->pts, src_tb, dst_tb);
    if (pkt->dts != (int64_t)AV_NOPTS_VALUE)
        pkt->dts = av_rescale_q(pkt->dts, src_tb, dst_tb);
    if (pkt->duration > 0)
        pkt->duration = av_rescale_q(pkt->duration, src_tb, dst_tb);
    if (pkt->convergence_duration > 0)
        pkt->convergence_duration = av_rescale_q(pkt->convergence_duration, src_tb, dst_tb);
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
#if !AV_MODULE_CHECK(LIBAVCODEC, 55, 34, 1, 39, 101)
int av_packet_ref(AVPacket *dst, const AVPacket *src)
{
#if QTAV_USE_FFMPEG(LIBAVCODEC)
    return av_copy_packet(dst, const_cast<AVPacket*>(src)); // not const in these versions
#else // libav <=11 has no av_copy_packet
#define DUP_DATA(dst, src, size, padding)                               \
    do {                                                                \
        void *data;                                                     \
        if (padding) {                                                  \
            if ((unsigned)(size) >                                      \
                (unsigned)(size) + FF_INPUT_BUFFER_PADDING_SIZE)        \
                goto failed_alloc;                                      \
            data = av_malloc(size + FF_INPUT_BUFFER_PADDING_SIZE);      \
        } else {                                                        \
            data = av_malloc(size);                                     \
        }                                                               \
        if (!data)                                                      \
            goto failed_alloc;                                          \
        memcpy(data, src, size);                                        \
        if (padding)                                                    \
            memset((uint8_t*)data + size, 0,                           \
                   FF_INPUT_BUFFER_PADDING_SIZE);                       \
        *((void**)&dst) = data;                                                     \
    } while (0)

    *dst = *src;
    dst->data      = NULL;
    dst->side_data = NULL;
    DUP_DATA(dst->data, src->data, dst->size, 1);
    dst->destruct = av_destruct_packet;
    if (dst->side_data_elems) {
        int i;
        DUP_DATA(dst->side_data, src->side_data,
                dst->side_data_elems * sizeof(*dst->side_data), 0);
        memset(dst->side_data, 0,
                dst->side_data_elems * sizeof(*dst->side_data));
        for (i = 0; i < dst->side_data_elems; i++) {
            DUP_DATA(dst->side_data[i].data, src->side_data[i].data, src->side_data[i].size, 1);
            dst->side_data[i].size = src->side_data[i].size;
            dst->side_data[i].type = src->side_data[i].type;
        }
    }
    return 0;
failed_alloc:
    av_destruct_packet(dst);
    return AVERROR(ENOMEM);
#endif
}
#endif
#if !AV_MODULE_CHECK(LIBAVCODEC, 55, 52, 0, 63, 100)
void avcodec_free_context(AVCodecContext **pavctx)
{

    AVCodecContext *avctx = *pavctx;
    if (!avctx)
        return;
    avcodec_close(avctx);
    av_freep(&avctx->extradata);
    av_freep(&avctx->subtitle_header);
    av_freep(&avctx->intra_matrix);
    av_freep(&avctx->inter_matrix);
    av_freep(&avctx->rc_override);
    av_freep(pavctx);
}
#endif

const char *get_codec_long_name(enum AVCodecID id)
{
    if (id == AV_CODEC_ID_NONE)
        return "none";
    const AVCodecDescriptor *cd = avcodec_descriptor_get(id);
    if (cd)
        return cd->long_name;
    av_log(NULL, AV_LOG_WARNING, "Codec 0x%x is not in the full list.\n", id);
    AVCodec *codec = avcodec_find_decoder(id);
    if (codec)
        return codec->long_name;
    codec = avcodec_find_encoder(id);
    if (codec)
        return codec->long_name;
    return "unknown_codec";
}

#if QTAV_HAVE(AVFILTER)
#if !AV_MODULE_CHECK(LIBAVFILTER, 2, 22, 0, 79, 100) //FF_API_AVFILTERPAD_PUBLIC
const char *avfilter_pad_get_name(const AVFilterPad *pads, int pad_idx)
{
    return pads[pad_idx].name;
}

enum AVMediaType avfilter_pad_get_type(const AVFilterPad *pads, int pad_idx)
{
    return pads[pad_idx].type;
}
#endif
#endif //QTAV_HAVE(AVFILTER)
