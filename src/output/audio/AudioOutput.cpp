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
#include "utils/Logger.h"

namespace QtAV {
AudioOutput::AudioOutput()
    :AVOutput(*new AudioOutputPrivate())
{
    d_func().format.setSampleFormat(AudioFormat::SampleFormat_Signed16);
    d_func().format.setChannelLayout(AudioFormat::ChannelLayout_Stero);
}

AudioOutput::AudioOutput(AudioOutputPrivate &d)
    :AVOutput(d)
{
    d_func().format.setSampleFormat(AudioFormat::SampleFormat_Signed16);
    d_func().format.setChannelLayout(AudioFormat::ChannelLayout_Stero);
}

AudioOutput::~AudioOutput()
{
}

bool AudioOutput::play(const QByteArray &data, qreal pts)
{
    waitForNextBuffer();
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

void AudioOutput::setBufferControl(BufferControl value)
{
    // check supported? it's virtual, but called in ctor
    d_func().control = value;
}

AudioOutput::BufferControl AudioOutput::bufferControl() const
{
    return (AudioOutput::BufferControl)d_func().control;
}

AudioOutput::BufferControl AudioOutput::supportedBufferControl() const
{
    return AudioOutput::User;
}

void AudioOutput::setFeatures(Feature value)
{
    if (!onSetFeatures(value))
        return;
    if (hasFeatures(value))
        return;
    d_func().features = value;
    emit featuresChanged();
}

AudioOutput::Feature AudioOutput::features() const
{
    return (Feature)d_func().features;
}

void AudioOutput::setFeature(Feature value, bool on)
{
    if (!onSetFeatures(value, on))
        return;
    if (on) {
        if (hasFeatures(value))
            return;
        d_func().features |= value;
    } else {
        if (!hasFeatures(value))
            return;
        d_func().features &= ~value;
    }
    emit featuresChanged();
}

bool AudioOutput::hasFeatures(Feature value) const
{
    return ((Feature)d_func().features & value) == value;
}

bool AudioOutput::onSetFeatures(Feature value, bool set)
{
    Q_UNUSED(value);
    Q_UNUSED(set);
    return false;
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
        if (processed < 0)
            processed += bufferSizeTotal();
        d.play_pos = s;
        d.processed_remain += processed;
        const int next = d.nextDequeueInfo().data_size;
        // TODO: avoid always 0
        while (!no_wait && d.processed_remain < next && next > 0) {
            const qint64 us = d.format.durationForBytes(next - d.processed_remain);
            if (us < 1000LL)
                d.uwait(10000LL);
            else
                d.uwait(us);
            s = getOffsetByBytes();
            processed += s - d.play_pos;
            if (processed < 0)
                processed += bufferSizeTotal();
            d.play_pos = s;
            d.processed_remain += processed;
        }
        remove = -d.processed_remain;
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

} //namespace QtAV
