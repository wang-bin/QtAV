#define QTAV_HAVE_SWR_AVR_MAP 1
#include "AudioResampler.h"
#include "private/AudioResampler_p.h"
#include "QtAV/QtAV_Compat.h"
#include "prepost.h"

namespace QtAV {

class AudioResamplerLibavPrivate;
class AudioResamplerLibav : public AudioResampler //Q_EXPORT is not needed
{
    DPTR_DECLARE_PRIVATE(AudioResampler)
public:
    AudioResamplerLibav();
    virtual bool convert(const quint8** data);
    virtual bool prepare();
};

extern AudioResamplerId AudioResamplerId_Libav;
FACTORY_REGISTER_ID_AUTO(AudioResampler, Libav, "Libav")

void RegisterAudioResamplerLibav_Man()
{
    FACTORY_REGISTER_ID_MAN(AudioResampler, Libav, "Libav")
}


class AudioResamplerLibavPrivate : public AudioResamplerPrivate
{
public:
    AudioResamplerLibavPrivate():
        context(0)
    {
    }
    ~AudioResamplerLibavPrivate() {
        if (context) {
            swr_free(&context);
            context = 0;
        }
    }
    SwrContext *context;
    int channel_map[SWR_CH_MAX];
};

AudioResamplerLibav::AudioResamplerLibav():
    AudioResampler(*new AudioResamplerLibavPrivate())
{
}

bool AudioResamplerLibav::convert(const quint8 **data)
{
    DPTR_D(AudioResamplerLibav);
    /*
     * swr_get_delay: Especially when downsampling by a large value, the output sample rate may be a poor choice to represent
     * the delay, similarly  upsampling and the input sample rate.
     */
    d.out_samples_per_channel = av_rescale_rnd(
#if HAVE_SWR_GET_DELAY
                swr_get_delay(d.context, qMax(d.in_format.sampleRate(), d.out_format.sampleRate())) +
#else
                128 + //TODO: QtAV_Compat
#endif //HAVE_SWR_GET_DELAY
                d.in_samples_per_channel //TODO: wanted_samples(ffplay mplayer2)
                , d.out_format.sampleRate(), d.in_format.sampleRate(), AV_ROUND_UP);
    //TODO: why crash for swr 0.5?
    //int out_size = av_samples_get_buffer_size(NULL/*out linesize*/, d.out_channels, d.out_samples_per_channel, (AVSampleFormat)d.out_sample_format, 0/*alignment default*/);
    int size_per_sample_with_channels = d.out_format.channels()*d.out_format.bytesPerSample();
    int out_size = d.out_samples_per_channel*size_per_sample_with_channels;
    if (out_size > d.data_out.size())
        d.data_out.resize(out_size);
    uint8_t *out[] = {(uint8_t*)d.data_out.data()};
    //number of input/output samples available in one channel
    int converted_samplers_per_channel = swr_convert(d.context, out, d.out_samples_per_channel, data, d.in_samples_per_channel);
    if (converted_samplers_per_channel < 0) {
        qWarning("[AudioResamplerLibav] %s", av_err2str(converted_samplers_per_channel));
        return false;
    }
    //TODO: converted_samplers_per_channel==out_samples_per_channel means out_size is too small, see mplayer2
    //converted_samplers_per_channel*d.out_channels*av_get_bytes_per_sample(d.out_sample_format)
    //av_samples_get_buffer_size(0, d.out_channels, converted_samplers_per_channel, d.out_sample_format, 0)
    //if (converted_samplers_per_channel != out_size)
    d.data_out.resize(converted_samplers_per_channel*size_per_sample_with_channels);
    return true;
}

/*
 *TODO: broken sample rate(AAC), see mplayer
 */
bool AudioResamplerLibav::prepare()
{
    DPTR_D(AudioResamplerLibav);
    if (!d.in_format.isValid()) {
        qWarning("src audio parameters 'channel layout(or channels), sample rate and sample format must be set before initialize resampler");
        return false;
    }
    //TODO: also in do this statistics
    qDebug("in cs: %d, cl: %lld", d.in_format.channels(), d.in_format.channelLayout());
    if (!d.in_format.channels()) {
        if (!d.in_format.channelLayout()) { //FIXME: already return
            d.in_format.setChannels(2);
            d.in_format.setChannelLayoutFFmpeg(av_get_default_channel_layout(d.in_format.channels())); //from mplayer2
            qWarning("both channels and channel layout are not available, assume channels=%d, channel layout=%lld", d.in_format.channels(), d.in_format.channelLayout());
        } else {
            d.in_format.setChannels(av_get_channel_layout_nb_channels(d.in_format.channelLayout()));
        }
    }
    if (!d.in_format.channels())
        d.in_format.setChannels(2); //TODO: why av_get_channel_layout_nb_channels() may return 0?
    if (!d.in_format.channelLayout()) {
        qWarning("channel layout not available, use default layout");
        d.in_format.setChannelLayoutFFmpeg(av_get_default_channel_layout(d.in_format.channels()));
    }
    qDebug("in cs: %d, cl: %lld", d.in_format.channels(), d.in_format.channelLayout());

    if (!d.out_format.channels()) {
        if (d.out_format.channelLayout())
            d.out_format.setChannels(av_get_channel_layout_nb_channels(d.out_format.channelLayout()));
        else
            d.out_format.setChannels(d.in_format.channels());
    }
    //now we have out channels
    if (!d.out_format.channelLayout())
        d.out_format.setChannelLayoutFFmpeg(av_get_default_channel_layout(d.out_format.channels()));
    if (!d.out_format.sampleRate())
        d.out_format.setSampleRate(inAudioFormat().sampleRate());
    if (d.speed <= 0)
        d.speed = 1.0;
    //DO NOT set sample rate here, we should keep the original and multiply 1/speed when needed
    //if (d.speed != 1.0)
    //    d.out_format.setSampleRate(int(qreal(d.out_format.sampleFormat())/d.speed));
    qDebug("swr speed=%.2f", d.speed);

    //d.in_planes = av_sample_fmt_is_planar((enum AVSampleFormat)d.in_sample_format) ? d.in_channels : 1;
    //d.out_planes = av_sample_fmt_is_planar((enum AVSampleFormat)d.out_sample_format) ? d.out_channels : 1;
    //swr_free(&d.context); //ffplay
    //If use swr_alloc() need to set the parameters (av_opt_set_xxx() manually or with swr_alloc_set_opts()) before calling swr_init()
    d.context = swr_alloc_set_opts(d.context
                                   , d.out_format.channelLayout()
                                   , (enum AVSampleFormat)outAudioFormat().sampleFormatFFmpeg()
                                   , qreal(outAudioFormat().sampleRate())/d.speed
                                   , d.in_format.channelLayout()
                                   , (enum AVSampleFormat)inAudioFormat().sampleFormatFFmpeg()
                                   , inAudioFormat().sampleRate()
                                   , 0 /*log_offset*/, 0 /*log_ctx*/);
    /*
    av_opt_set_int(d.context, "in_channel_layout",    d.in_channel_layout, 0);
    av_opt_set_int(d.context, "in_sample_rate",       d.in_format.sampleRate(), 0);
    av_opt_set_sample_fmt(d.context, "in_sample_fmt", (enum AVSampleFormat)in_format.sampleFormatFFmpeg(), 0);
    av_opt_set_int(d.context, "out_channel_layout",    d.out_channel_layout, 0);
    av_opt_set_int(d.context, "out_sample_rate",       d.out_format.sampleRate(), 0);
    av_opt_set_sample_fmt(d.context, "out_sample_fmt", (enum AVSampleFormat)out_format.sampleFormatFFmpeg(), 0);
    */
    av_log(NULL, AV_LOG_INFO, "out cl: %lld\n", d.out_format.channelLayout());
    av_log(NULL, AV_LOG_INFO, "out fmt: %d\n", d.out_format.sampleFormat());
    av_log(NULL, AV_LOG_INFO, "out freq: %d\n", d.out_format.sampleRate());
    av_log(NULL, AV_LOG_INFO, "in cl: %lld\n", d.in_format.channelLayout());
    av_log(NULL, AV_LOG_INFO, "in fmt: %d\n", d.in_format.sampleFormat());
    av_log(NULL, AV_LOG_INFO, "in freq: %d\n",  d.in_format.sampleRate());

    if (!d.context) {
        qWarning("Allocat swr context failed!");
        return false;
    }
    if (d.in_format.channels() < d.out_format.channels()) {
        memset(d.channel_map, 0, sizeof(d.channel_map));
        for (int i = 0; i < d.out_format.channels(); ++i) {
            d.channel_map[i] = i % d.in_format.channels();
        }
        av_opt_set_int(d.context, "icl", d.out_format.channelLayout(), 0);
        av_opt_set_int(d.context, "uch", d.out_format.channels() , 0);
        swr_set_channel_mapping(d.context, d.channel_map);
    }
    int ret = swr_init(d.context);
    if (ret < 0) {
        qWarning("swr_init failed: %s", av_err2str(ret));
        return false;
    }
    return true;
}

} //namespace QtAV
