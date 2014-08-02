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

#include "QtAV/AudioOutput.h"
#include "QtAV/private/AudioOutput_p.h"

namespace QtAV {
AudioOutput::AudioOutput()
    :AVOutput(*new AudioOutputPrivate())
{
    DPTR_D(AudioOutput);
    d_func().format.setSampleFormat(preferredSampleFormat());
    d_func().format.setChannelLayout(preferredChannelLayout());
}

AudioOutput::AudioOutput(AudioOutputPrivate &d)
    :AVOutput(d)
{
    d_func().format.setSampleFormat(preferredSampleFormat());
    d_func().format.setChannelLayout(preferredChannelLayout());
}

AudioOutput::~AudioOutput()
{
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
    d.nextEnqueueInfo().data_size = data.size();
    d.nextEnqueueInfo().timestamp = pts;
    d.bufferAdded();
    return write(d.data);
}

int AudioOutput::maxChannels() const
{
    return d_func().max_channels;
}

void AudioOutput::setAudioFormat(const AudioFormat& format)
{
    DPTR_D(AudioOutput);
    if (!isSupported(format)) {
        return;
    }
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
    if (channels > d.max_channels) {
        qWarning("not support. max channels is %d", d.max_channels);
        return;
    }
    d.format.setChannels(channels);
}

int AudioOutput::channels() const
{
    return d_func().format.channels();
}

void AudioOutput::setVolume(qreal volume)
{
    DPTR_D(AudioOutput);
    d.vol = qMax<qreal>(volume, 0);
    d.mute = d.vol == 0;
}

qreal AudioOutput::volume() const
{
    return qMax<qreal>(d_func().vol, 0);
}

void AudioOutput::setMute(bool yes)
{
    d_func().mute = yes;
}

bool AudioOutput::isMute() const
{
    return !isAvailable() || d_func().mute;
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
    return AudioFormat::SampleFormat_Float;
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

void AudioOutput::setFeature(Feature value)
{
    // check supported? it's virtual, but called in ctor
    d_func().feature = value;
}

AudioOutput::Feature AudioOutput::feature() const
{
    return (AudioOutput::Feature)d_func().feature;
}

AudioOutput::Feature AudioOutput::supportedFeatures() const
{
    return AudioOutput::User;
}

void AudioOutput::waitForNextBuffer()
{
    DPTR_D(AudioOutput);
    //don't return even if we can add buffer because we have /to update dequeue index
    // openal need enqueue to a dequeued buffer! why sl crash
    bool no_wait = false;//d.canAddBuffer();
    const Feature f = feature();
    int remove = 0;
    if (f & Blocking) {
        remove = 1;
    } else if (f & Callback) {
        QMutexLocker lock(&d.mutex);
        Q_UNUSED(lock);
        d.cond.wait(&d.mutex);
        remove = 1;
    } else if (f & PlayedBytes) {
        d.processed_remain = getPlayedBytes();
        const int next = d.nextDequeueInfo().data_size;
        // TODO: avoid always 0
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
    } else if (f & GetPlayedIndices) {
#if AO_USE_TIMER
        if (!d.timer.isValid())
            d.timer.start();
        qint64 elapsed = 0;
#endif //AO_USE_TIMER
        int c = getProcessed();
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
            c = getProcessed();
        }
        // what if c always 0?
        remove = c;
    } else if (f & GetPlayingBytes) {
        int s = getPlayingBytes();
        int processed = s - d.play_pos;
        if (processed < 0)
            processed += bufferSizeTotal();
        d.play_pos = s;
        d.processed_remain += processed;
        const int next = d.nextDequeueInfo().data_size;
        // TODO: avoid always 0
        while (!no_wait && d.processed_remain < next) {
            const qint64 us = d.format.durationForBytes(next - d.processed_remain);
            if (us < 1000LL)
                d.uwait(1000LL);
            else
                d.uwait(us);
            s = getPlayingBytes();
            processed += s - d.play_pos;
            if (processed < 0)
                processed += bufferSizeTotal();
            d.play_pos = s;
            d.processed_remain += processed;
        }
        remove = -d.processed_remain;
    } else if (f & GetPlayingIndex) {
        int n = getPlayingIndex();
        int processed = n - d.play_pos;
        if (processed < 0)
            processed += bufferCount();
        d.play_pos = n;
        // TODO: timer
        // TODO: avoid always 0
        while (!no_wait && processed < 1) {
            d.uwait(d.format.durationForBytes(d.nextDequeueInfo().data_size));
            n = getPlayingIndex();
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
        //qDebug("remove bytes < %d", -remove);
        int next = d.nextDequeueInfo().data_size;
        while (d.processed_remain > next && next > 0) {
            d.processed_remain -= next;
            d.bufferRemoved();
            next = d.nextDequeueInfo().data_size;
        }
        return;
    }
    //qDebug("remove count: %d", remove);
    while (remove-- > 0) {
        d.bufferRemoved();
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

//default return -1. means not the feature
int AudioOutput::getProcessed()
{
    return -1;
}

int AudioOutput::getPlayedBytes()
{
    return -1;
}

int AudioOutput::getPlayingIndex()
{
    return -1;
}

int AudioOutput::getPlayingBytes()
{
    return -1;
}

} //namespace QtAV
