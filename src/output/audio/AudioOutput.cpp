/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2015 Wang Bin <wbsecg1@gmail.com>

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

#include "QtAV/AudioOutput.h"
#include "QtAV/private/AudioOutput_p.h"
#include "QtAV/private/AVCompat.h"
#include "utils/Logger.h"

namespace QtAV {

/// from libavfilter/af_volume begin
static inline void scale_samples_u8(quint8 *dst, const quint8 *src, int nb_samples, int volume, float)
{
    for (int i = 0; i < nb_samples; i++)
        dst[i] = av_clip_uint8(((((qint64)src[i] - 128) * volume + 128) >> 8) + 128);
}

static inline void scale_samples_u8_small(quint8 *dst, const quint8 *src, int nb_samples, int volume, float)
{
    for (int i = 0; i < nb_samples; i++)
        dst[i] = av_clip_uint8((((src[i] - 128) * volume + 128) >> 8) + 128);
}

static inline void scale_samples_s16(quint8 *dst, const quint8 *src, int nb_samples, int volume, float)
{
    int16_t *smp_dst       = (int16_t *)dst;
    const int16_t *smp_src = (const int16_t *)src;
    for (int i = 0; i < nb_samples; i++)
        smp_dst[i] = av_clip_int16(((qint64)smp_src[i] * volume + 128) >> 8);
}

static inline void scale_samples_s16_small(quint8 *dst, const quint8 *src, int nb_samples, int volume, float)
{
    int16_t *smp_dst       = (int16_t *)dst;
    const int16_t *smp_src = (const int16_t *)src;
    for (int i = 0; i < nb_samples; i++)
        smp_dst[i] = av_clip_int16((smp_src[i] * volume + 128) >> 8);
}

static inline void scale_samples_s32(quint8 *dst, const quint8 *src, int nb_samples, int volume, float)
{
    qint32 *smp_dst       = (qint32 *)dst;
    const qint32 *smp_src = (const qint32 *)src;
    for (int i = 0; i < nb_samples; i++)
        smp_dst[i] = av_clipl_int32((((qint64)smp_src[i] * volume + 128) >> 8));
}
/// from libavfilter/af_volume end

//TODO: simd
template<typename T>
static inline void scale_samples(quint8 *dst, const quint8 *src, int nb_samples, int, float volume)
{
    T *smp_dst = (T *)dst;
    const T *smp_src = (const T *)src;
    for (int i = 0; i < nb_samples; ++i)
        smp_dst[i] = smp_src[i] * (T)volume;
}

scale_samples_func get_scaler(AudioFormat::SampleFormat fmt, qreal vol, int* voli)
{
    int v = (int)(vol * 256.0 + 0.5);
    if (voli)
        *voli = v;
    switch (fmt) {
    case AudioFormat::SampleFormat_Unsigned8:
    case AudioFormat::SampleFormat_Unsigned8Planar:
        return v < 0x1000000 ? scale_samples_u8_small : scale_samples_u8;
    case AudioFormat::SampleFormat_Signed16:
    case AudioFormat::SampleFormat_Signed16Planar:
        return v < 0x10000 ? scale_samples_s16_small : scale_samples_s16;
    case AudioFormat::SampleFormat_Signed32:
    case AudioFormat::SampleFormat_Signed32Planar:
        return scale_samples_s32;
    case AudioFormat::SampleFormat_Float:
    case AudioFormat::SampleFormat_FloatPlanar:
        return scale_samples<float>;
    case AudioFormat::SampleFormat_Double:
    case AudioFormat::SampleFormat_DoublePlanar:
        return scale_samples<double>;
    default:
        return 0;
    }
}

void AudioOutputPrivate::updateSampleScaleFunc()
{
    scale_samples = get_scaler(format.sampleFormat(), vol, &volume_i);
}

AudioOutput::AudioOutput(QObject* parent)
    : QObject(parent)
    , AVOutput(*new AudioOutputPrivate())
{
    d_func().format.setSampleFormat(AudioFormat::SampleFormat_Signed16);
    d_func().format.setChannelLayout(AudioFormat::ChannelLayout_Stero);
}

AudioOutput::AudioOutput(DeviceFeatures featuresSupported, AudioOutputPrivate& d, QObject *parent)
    : QObject(parent)
    , AVOutput(d)
{
    d_func().format.setSampleFormat(AudioFormat::SampleFormat_Signed16);
    d_func().format.setChannelLayout(AudioFormat::ChannelLayout_Stero);
    d_func().supported_features = featuresSupported;
}

AudioOutput::~AudioOutput()
{
}

bool AudioOutput::play(const QByteArray &data, qreal pts)
{
    receiveData(data, pts);
    return play();
}

bool AudioOutput::receiveData(const QByteArray &data, qreal pts)
{
    DPTR_D(AudioOutput);
    //DPTR_D(AVOutput);
    //locker(&mutex)
    //TODO: make sure d.data thread safe. lock around here? for audio and video(main thread problem)?
    /* you can use d.data directly in AVThread. In other thread, it's not safe, you must do something
     * to make sure the data is not be modified in AVThread when using it*/
    if (d.paused)
        return false;
    d.data = data;
    if (isMute() && d.sw_mute) {
        d.data.fill(0);
    } else {
        if (!qFuzzyCompare(volume(), (qreal)1.0)
                && d.sw_volume
                && d.scale_samples
                ) {
            // TODO: af_volume needs samples_align to get nb_samples
            const int nb_samples = d.data.size()/d.format.bytesPerSample();
            quint8 *dst = (quint8*)d.data.constData();
            d.scale_samples(dst, dst, nb_samples, d.volume_i, volume());
        }
    }
    // wait after all data processing finished to reduce time error
    waitForNextBuffer();
    d.nextEnqueueInfo().data_size = data.size();
    d.nextEnqueueInfo().timestamp = pts;
    d.bufferAdded();
    return write(d.data);
}

void AudioOutput::setAudioFormat(const AudioFormat& format)
{
    DPTR_D(AudioOutput);
    if (!isSupported(format)) {
        return;
    }
    if (d.format == format)
        return;
    d.updateSampleScaleFunc();
    d.format = format;
}

AudioFormat& AudioOutput::audioFormat()
{
    return d_func().format;
}

const AudioFormat& AudioOutput::audioFormat() const
{
    return d_func().format;
}

void AudioOutput::setSampleRate(int rate)
{
    d_func().format.setSampleRate(rate);
}

int AudioOutput::sampleRate() const
{
    return d_func().format.sampleRate();
}

// TODO: check isSupported
void AudioOutput::setChannels(int channels)
{
    DPTR_D(AudioOutput);
    d.format.setChannels(channels);
}

int AudioOutput::channels() const
{
    return d_func().format.channels();
}

void AudioOutput::setVolume(qreal volume)
{
    DPTR_D(AudioOutput);
    if (volume < 0.0)
        return;
    if (d.vol == volume) //fuzzy compare?
        return;
    d.vol = volume;
    emit volumeChanged(d.vol);
    d.updateSampleScaleFunc();
    if (deviceFeatures() & SetVolume) {
        d.sw_volume = !deviceSetVolume(d.vol);
        //if (!qFuzzyCompare(deviceGetVolume(), d.vol))
        //    d.sw_volume = true;
        if (d.sw_volume)
            deviceSetVolume(1.0); // TODO: partial software?
    } else {
        d.sw_volume = true;
    }
}

qreal AudioOutput::volume() const
{
    return qMax<qreal>(d_func().vol, 0);
}

void AudioOutput::setMute(bool value)
{
    DPTR_D(AudioOutput);
    if (d.mute == value)
        return;
    d.mute = value;
    emit muteChanged(value);
    if (deviceFeatures() & SetMute)
        d.sw_mute = !deviceSetMute(value);
    else
        d.sw_mute = true;
}

bool AudioOutput::isMute() const
{
    return d_func().mute;
}

void AudioOutput::setSpeed(qreal speed)
{
    d_func().speed = speed;
}

qreal AudioOutput::speed() const
{
    return d_func().speed;
}

bool AudioOutput::isSupported(const AudioFormat &format) const
{
    Q_UNUSED(format);
    return isSupported(format.sampleFormat()) && isSupported(format.channelLayout());
}

bool AudioOutput::isSupported(AudioFormat::SampleFormat sampleFormat) const
{
    Q_UNUSED(sampleFormat);
    return true;
}

bool AudioOutput::isSupported(AudioFormat::ChannelLayout channelLayout) const
{
    Q_UNUSED(channelLayout);
    return true;
}

AudioFormat::SampleFormat AudioOutput::preferredSampleFormat() const
{
    return AudioFormat::SampleFormat_Signed16;
}

AudioFormat::ChannelLayout AudioOutput::preferredChannelLayout() const
{
    return AudioFormat::ChannelLayout_Stero;
}

int AudioOutput::bufferSize() const
{
    return d_func().buffer_size;
}

void AudioOutput::setBufferSize(int value)
{
    d_func().buffer_size = value;
}

int AudioOutput::bufferCount() const
{
    return d_func().nb_buffers;
}

void AudioOutput::setBufferCount(int value)
{
    d_func().nb_buffers = value;
}

// no virtual functions inside because it can be called in ctor
void AudioOutput::setDeviceFeatures(DeviceFeatures value)
{
    DPTR_D(AudioOutput);
    //Qt5: QFlags::Int (int or uint)
    const int s(supportedDeviceFeatures());
    const int f(value);
    if (d.features == (f & s))
        return;
    d.features = (f & s);
    emit deviceFeaturesChanged();
}

AudioOutput::DeviceFeatures AudioOutput::deviceFeatures() const
{
    return (DeviceFeature)d_func().features;
}

AudioOutput::DeviceFeatures AudioOutput::supportedDeviceFeatures() const
{
    return (DeviceFeature)d_func().supported_features;
}

void AudioOutput::resetStatus()
{
    d_func().resetStatus();
}

void AudioOutput::waitForNextBuffer()
{
    DPTR_D(AudioOutput);
    //don't return even if we can add buffer because we don't know when a buffer is processed and we have /to update dequeue index
    // openal need enqueue to a dequeued buffer! why sl crash
    bool no_wait = false;//d.canAddBuffer();
    const BufferControl f = bufferControl();
    int remove = 0;
    if (f & Blocking) {
        remove = 1;
    } else if (f & Callback) {
        int processed = d.processed_remain;
        d.processed_remain = getWritableBytes();
        const int next = d.nextDequeueInfo().data_size;
        //qDebug("remain: %d-%d, size: %d, next: %d", processed, d.processed_remain, d.data.size(), next);
        while (d.processed_remain - processed < next || d.processed_remain < d.data.size()) {
            const qint64 us = d.format.durationForBytes(next - (d.processed_remain - processed));
            QMutexLocker lock(&d.mutex);
            Q_UNUSED(lock);
            d.cond.wait(&d.mutex, us/1000LL);
            d.processed_remain = getWritableBytes();
        }
        processed = d.processed_remain - processed;
        //if ()
        d.processed_remain -= d.data.size(); //ensure d.processed_remain later is greater
        remove = -processed; // processed_this_period
    } else if (f & PlayedBytes) {
        d.processed_remain = getPlayedBytes();
        const int next = d.nextDequeueInfo().data_size;
        // TODO: avoid always 0
        // TODO: timer
        // TODO: compare processed_remain with d.data.size because input chuncks can be in different sizes
        while (!no_wait && d.processed_remain < next) {
            const qint64 us = d.format.durationForBytes(next - d.processed_remain);
            if (us < 1000LL)
                d.uwait(10000LL);
            else
                d.uwait(us);
            d.processed_remain = getPlayedBytes();
            // what if s always 0?
        }
        remove = -d.processed_remain;
    } else if (f & PlayedCount) {
#if AO_USE_TIMER
        if (!d.timer.isValid())
            d.timer.start();
        qint64 elapsed = 0;
#endif //AO_USE_TIMER
        int c = getPlayedCount();
        // TODO: avoid always 0
        qint64 us = 0;
        while (!no_wait && c < 1) {
            if (us <= 0)
                us = d.format.durationForBytes(d.nextDequeueInfo().data_size);
#if AO_USE_TIMER
            elapsed = d.timer.restart();
            if (elapsed > 0 && us > elapsed*1000LL)
                us -= elapsed*1000LL;
            if (us < 1000LL)
                us = 10000LL; //opensl crash if 1
#endif //AO_USE_TIMER
            d.uwait(us);
            c = getPlayedCount();
        }
        // what if c always 0?
        remove = c;
    } else if (f & OffsetBytes) {
        int s = getOffsetByBytes();
        int processed = s - d.play_pos;
        qDebug("s: %d, play_pos: %d, processed: %d, bufferSizeTotal: %d", s, d.play_pos, processed, bufferSizeTotal());
        if (processed < 0)
            processed += bufferSizeTotal();
        d.play_pos = s;
        d.processed_remain += processed;
        const int next = d.nextDequeueInfo().data_size;
        // TODO: avoid always 0
        //qDebug("d.processed_remain: %d", d.processed_remain);
        int writable_size = d.processed_remain;
        while (!no_wait && writable_size < next && next > 0) {
            const qint64 us = d.format.durationForBytes(next - d.processed_remain);
            // TODO: timer
            if (us < 1000LL)
                d.uwait(10000LL);
            else
                d.uwait(us);
            s = getOffsetByBytes();
            processed += s - d.play_pos;
            if (processed < 0)
                processed += bufferSizeTotal();
            writable_size = d.processed_remain + processed;
            d.play_pos = s;
            //qDebug("writable_size: %d", writable_size);
        }
        d.processed_remain += processed;
        remove = -processed;
    } else if (f & OffsetIndex) {
        int n = getOffset();
        int processed = n - d.play_pos;
        if (processed < 0)
            processed += bufferCount();
        d.play_pos = n;
        // TODO: timer
        // TODO: avoid always 0
        while (!no_wait && processed < 1) {
            d.uwait(d.format.durationForBytes(d.nextDequeueInfo().data_size));
            n = getOffset();
            processed = n - d.play_pos;
            if (processed < 0)
                processed += bufferCount();
            d.play_pos = n;
        }
        remove = processed;
    } else {
        qFatal("User defined waitForNextBuffer() not implemented!");
        return;
    }
    if (remove < 0) {
        int next = d.nextDequeueInfo().data_size;
        int free_bytes = -remove;//d.processed_remain;
        while (free_bytes >= next && next > 0) {
            free_bytes -= next;
            if (!d.bufferRemoved()) {//|| d.processed_remain <= 0
                qWarning("buffer queue empty");
                break;
            }
            next = d.nextDequeueInfo().data_size;
        }
        //qDebug("remove: %d, unremoved bytes < %d, writable_bytes: %d", remove, free_bytes, d.processed_remain);
        return;
    }
    //qDebug("remove count: %d", remove);
    while (remove-- > 0) {
        if (!d.bufferRemoved())
            break;
    }
}


qreal AudioOutput::timestamp() const
{
    DPTR_D(const AudioOutput);
    return d.nextDequeueInfo().timestamp;
}

void AudioOutput::onCallback()
{
    d_func().onCallback();
}

//default return -1. means not the control
int AudioOutput::getPlayedCount()
{
    return -1;
}

int AudioOutput::getPlayedBytes()
{
    return -1;
}

int AudioOutput::getOffset()
{
    return -1;
}

int AudioOutput::getOffsetByBytes()
{
    return -1;
}

int AudioOutput::getWritableBytes()
{
    return -1;
}

bool AudioOutput::deviceSetVolume(qreal value)
{
    Q_UNUSED(value)
    return false;
}

qreal AudioOutput::deviceGetVolume() const
{
    return 1.0;
}

bool AudioOutput::deviceSetMute(bool value)
{
    Q_UNUSED(value)
    return false;
}

} //namespace QtAV
