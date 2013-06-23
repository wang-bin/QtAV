
#include "AudioResampler.h"
#include "private/AudioResampler_p.h"
#include "QtAV/QtAV_Compat.h"
#include "prepost.h"

namespace QtAV {

class AudioResamplerFFPrivate;
class AudioResamplerFF : public AudioResampler //Q_EXPORT is not needed
{
    DPTR_DECLARE_PRIVATE(AudioResampler)
public:
    AudioResamplerFF();
    virtual bool convert(const quint8** data);
    virtual bool prepare();
};

AudioResamplerId AudioResamplerId_FF = 0;
FACTORY_REGISTER_ID_AUTO(AudioResampler, FF, "FFmpeg")

void RegisterAudioResamplerFF_Man()
{
    FACTORY_REGISTER_ID_MAN(AudioResampler, FF, "FFmpeg")
}


class AudioResamplerFFPrivate : public AudioResamplerPrivate
{
public:
    AudioResamplerFFPrivate():
        context(0)
    {
    }
    ~AudioResamplerFFPrivate() {
        if (context) {
            swr_free(&context);
            context = 0;
        }
    }
    SwrContext *context;
};

AudioResamplerFF::AudioResamplerFF():
    AudioResampler(*new AudioResamplerFFPrivate())
{
}

bool AudioResamplerFF::convert(const quint8 **data)
{
    DPTR_D(AudioResamplerFF);
    /*
     * swr_get_delay: Especially when downsampling by a large value, the output sample rate may be a poor choice to represent
     * the delay, similarly  upsampling and the input sample rate.
     */
    int64_t a = qMax(d.in_sample_rate, d.out_sample_rate);
    int in_samples_per_channel = d.in_samples/d.in_nb_channels;
    int out_samples_per_channel = av_rescale_rnd(
                swr_get_delay(d.context, a) + 
		in_samples_per_channel
                , d.out_sample_rate, d.in_sample_rate, AV_ROUND_UP);
    int out_size = av_samples_get_buffer_size(NULL/*out linesize*/, d.out_nb_channels, out_samples_per_channel, (AVSampleFormat)d.out_sample_format, 0/*alignment default*/);
    if (out_size > d.data_out.size())
        d.data_out.resize(out_size);
    uint8_t *out[] = { ( uint8_t * )d.data_out.data() };
    //number of input/output samples available in one channel
    int converted_samplers_per_channel = swr_convert(d.context, out, out_samples_per_channel, data, in_samples_per_channel);
    if (converted_samplers_per_channel < 0) {
        qWarning("[AudioResamplerFF] %s", av_err2str(converted_samplers_per_channel));
        return false;
    }
    //converted_samplers_per_channel*d.out_nb_channels*av_get_bytes_per_sample(d.out_sample_format)
    //av_samples_get_buffer_size(0, d.out_nb_channels, converted_samplers_per_channel, d.out_sample_format, 0)
    //if (converted_samplers_per_channel != out_size)
    d.data_out.resize(converted_samplers_per_channel*d.out_nb_channels*av_get_bytes_per_sample((AVSampleFormat)d.out_sample_format));
    return true;
}

/*
 *TODO: broken sample rate(AAC), see mplayer
 */
bool AudioResamplerFF::prepare()
{
    DPTR_D(AudioResamplerFF);
    if (!d.in_channel_layout || !d.in_sample_rate || d.in_sample_format == AV_SAMPLE_FMT_NONE) {
        qWarning("src audio parameters in_channel_layout, in_sample_rate, in_sample_format must be set before initialize resampler");
        return false;
    }
    if (!d.out_channel_layout)
        d.out_channel_layout = d.in_channel_layout;
    if (!d.out_sample_rate)
        d.out_sample_rate = d.in_sample_rate;
    if (d.speed <= 0)
        d.speed = 1.0;
    if (d.speed != 1.0)
        d.out_sample_rate = qreal(d.out_sample_rate)/d.speed;
    //d.in_nb_channels = av_get_channel_layout_nb_channels(d.in_channel_layout);
    //d.out_nb_channels = av_get_channel_layout_nb_channels(d.out_channel_layout);
    if (!d.out_nb_channels)
	d.out_nb_channels = d.in_nb_channels;
    d.in_planes = av_sample_fmt_is_planar((enum AVSampleFormat)d.in_sample_format) ? d.in_nb_channels : 1;
    d.out_planes = av_sample_fmt_is_planar((enum AVSampleFormat)d.out_sample_format) ? d.out_nb_channels : 1;

    //If use swr_alloc() need to set the parameters (av_opt_set_xxx() manually or with swr_alloc_set_opts()) before calling swr_init()
    d.context = swr_alloc_set_opts(d.context
                                   , d.out_channel_layout
                                   , (enum AVSampleFormat)d.out_sample_format, d.out_sample_rate
                                   , d.in_channel_layout
                                   , (enum AVSampleFormat)d.in_sample_format, d.in_sample_rate
                                   , 0 /*log_offset*/, 0 /*log_ctx*/);
    if (d.context) {
        int ret = swr_init(d.context);
        if (!ret) {
            qWarning("swr_init failed: %s", av_err2str(ret));
            return false;
        }
    } else {
        qWarning("Allocat swr context failed!");
        return false;
    }
    return true;
}

} //namespace QtAV
