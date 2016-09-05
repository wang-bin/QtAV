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

#include "QtAV/AudioOutput.h"
#include "QtAV/private/AVOutput_p.h"
#include "QtAV/private/AudioOutputBackend.h"
#include "QtAV/private/AVCompat.h"
#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
#include <QtCore/QElapsedTimer>
#else
#include <QtCore/QTime>
typedef QTime QElapsedTimer;
#endif
#include "utils/ring.h"
#include "utils/Logger.h"

#define AO_USE_TIMER 1

namespace QtAV {

// chunk
static const int kBufferSamples = 512;
static const int kBufferCount = 8*2; // may wait too long at the beginning (oal) if too large. if buffer count is too small, can not play for high sample rate audio.

typedef void (*scale_samples_func)(quint8 *dst, const quint8 *src, int nb_samples, int volume, float volumef);

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

class AudioOutputPrivate : public AVOutputPrivate
{
public:
    AudioOutputPrivate():
        mute(false)
      , sw_volume(true)
      , sw_mute(true)
      , volume_i(256)
      , vol(1)
      , speed(1.0)
      , nb_buffers(kBufferCount)
      , buffer_samples(kBufferSamples)
      , features(0)
      , play_pos(0)
      , processed_remain(0)
      , msecs_ahead(0)
      , scale_samples(0)
      , backend(0)
      , update_backend(true)
      , index_enqueue(-1)
      , index_deuqueue(-1)
      , frame_infos(ring<FrameInfo>(nb_buffers))
    {
        available = false;
    }
    virtual ~AudioOutputPrivate();

    void playInitialData(); //required by some backends, e.g. openal
    void onCallback() { cond.wakeAll();}
    virtual void uwait(qint64 us) {
        QMutexLocker lock(&mutex);
        Q_UNUSED(lock);
        cond.wait(&mutex, (us+500LL)/1000LL);
    }

    struct FrameInfo {
        FrameInfo(const QByteArray& d = QByteArray(), qreal t = 0, int us = 0) : timestamp(t), duration(us), data(d) {}
        qreal timestamp;
        int duration; // in us
        QByteArray data;
    };

    void resetStatus() {
        play_pos = 0;
        processed_remain = 0;
        msecs_ahead = 0;
#if AO_USE_TIMER
        timer.invalidate();
#endif
        frame_infos = ring<FrameInfo>(nb_buffers);
    }
    /// call this if sample format or volume is changed
    void updateSampleScaleFunc();
    void tryVolume(qreal value);
    void tryMute(bool value);

    bool mute;
    bool sw_volume, sw_mute;
    int volume_i;
    qreal vol;
    qreal speed;
    AudioFormat format;
    AudioFormat requested;
    //AudioFrame audio_frame;
    quint32 nb_buffers;
    qint32 buffer_samples;
    int features;
    int play_pos; // index or bytes
    int processed_remain;
    int msecs_ahead;
#if AO_USE_TIMER
    QElapsedTimer timer;
#endif
    scale_samples_func scale_samples;
    AudioOutputBackend *backend;
    bool update_backend;
    QStringList backends;
//private:
    // the index of current enqueue/dequeue
    int index_enqueue, index_deuqueue;
    ring<FrameInfo> frame_infos;
};

void AudioOutputPrivate::updateSampleScaleFunc()
{
    scale_samples = get_scaler(format.sampleFormat(), vol, &volume_i);
}

AudioOutputPrivate::~AudioOutputPrivate()
{
    if (backend) {
        backend->close();
        delete backend;
    }
}

void AudioOutputPrivate::playInitialData()
{
    const char c = (format.sampleFormat() == AudioFormat::SampleFormat_Unsigned8
                    || format.sampleFormat() == AudioFormat::SampleFormat_Unsigned8Planar)
            ? 0x80 : 0;
    for (quint32 i = 0; i < nb_buffers; ++i) {
        const QByteArray data(backend->buffer_size, c);
        backend->write(data); // fill silence byte, not always 0. AudioFormat.silenceByte
        frame_infos.push_back(FrameInfo(data, 0, 0)); // initial data can be small (1 instead of buffer_samples)
    }
    backend->play();
}

void AudioOutputPrivate::tryVolume(qreal value)
{
    // if not open, try later
    if (!available)
        return;
    if (features & AudioOutput::SetVolume) {
        sw_volume = !backend->setVolume(value);
        //if (!qFuzzyCompare(backend->volume(), value))
        //    sw_volume = true;
        if (sw_volume)
            backend->setVolume(1.0); // TODO: partial software?
    } else {
        sw_volume = true;
    }
}

void AudioOutputPrivate::tryMute(bool value)
{
    // if not open, try later
    if (!available)
        return;
    if ((features & AudioOutput::SetMute) && backend)
        sw_mute = !backend->setMute(value);
    else
        sw_mute = true;
}

AudioOutput::AudioOutput(QObject* parent)
    : QObject(parent)
    , AVOutput(*new AudioOutputPrivate())
{
    qDebug() << "Registered audio backends: " << AudioOutput::backendsAvailable(); // call this to register
    setBackends(AudioOutputBackend::defaultPriority()); //ensure a backend is available
}

AudioOutput::~AudioOutput()
{
    close();
}

extern void AudioOutput_RegisterAll(); //why vc link error if put in the following a exported class member function?
QStringList AudioOutput::backendsAvailable()
{
    AudioOutput_RegisterAll();
    static QStringList all;
    if (!all.isEmpty())
        return all;
    AudioOutputBackendId* i = NULL;
    while ((i = AudioOutputBackend::next(i)) != NULL) {
        all.append(AudioOutputBackend::name(*i));
    }
    all = AudioOutputBackend::defaultPriority() << all;
    all.removeDuplicates();
    return all;
}

void AudioOutput::setBackends(const QStringList &backendNames)
{
    DPTR_D(AudioOutput);
    if (d.backends == backendNames)
        return;
    d.update_backend = true;
    d.backends = backendNames;
    // create backend here because we have to check format support before open which needs a backend
    d.update_backend = false;
    if (d.backend) {
        d.backend->close();
        delete d.backend;
        d.backend = 0;
    }
    // TODO: empty backends use dummy backend
    if (!d.backends.isEmpty()) {
        foreach (const QString& b, d.backends) {
            d.backend = AudioOutputBackend::create(b.toLatin1().constData());
            if (!d.backend)
                continue;
            if (d.backend->available)
                break;
            delete d.backend;
            d.backend = NULL;
        }
    }
    if (d.backend) {
        // default: set all features when backend is ready
        setDeviceFeatures(d.backend->supportedFeatures());
        // connect volumeReported
        connect(d.backend, SIGNAL(volumeReported(qreal)), SLOT(reportVolume(qreal)));
        connect(d.backend, SIGNAL(muteReported(bool)), SLOT(reportMute(bool)));
    }

    Q_EMIT backendsChanged();
}

QStringList AudioOutput::backends() const
{
    return d_func().backends;
}

QString AudioOutput::backend() const
{
    DPTR_D(const AudioOutput);
    if (d.backend)
        return d.backend->name();
    return QString();
}

void AudioOutput::flush()
{
    DPTR_D(AudioOutput);
    while (!d.frame_infos.empty()) {
        if (d.backend)
            d.backend->flush();
        waitForNextBuffer();
    }
}

void AudioOutput::clear()
{
    DPTR_D(AudioOutput);
    if (!d.backend || !d.backend->clear())
        flush();
    d.resetStatus();
}

bool AudioOutput::open()
{
    DPTR_D(AudioOutput);
    QMutexLocker lock(&d.mutex);
    Q_UNUSED(lock);
    d.available = false;
    d.paused = false;
    d.resetStatus();
    if (!d.backend)
        return false;
    d.backend->audio = this;
    d.backend->buffer_size = bufferSize();
    d.backend->buffer_count = bufferCount();
    d.backend->format = audioFormat();
    // TODO: open next backend if fail and emit backendChanged()
    if (!d.backend->open())
        return false;
    d.available = true;
    d.tryVolume(volume());
    d.tryMute(isMute());
    d.playInitialData();
    return true;
}

bool AudioOutput::close()
{
    DPTR_D(AudioOutput);
    QMutexLocker lock(&d.mutex);
    Q_UNUSED(lock);
    d.available = false;
    d.paused = false;
    d.resetStatus();
    if (!d.backend)
        return false;
    // TODO: drain() before close
    d.backend->audio = 0;
    return d.backend->close();
}

bool AudioOutput::isOpen() const
{
    return d_func().available;
}

bool AudioOutput::play(const QByteArray &data, qreal pts)
{
    DPTR_D(AudioOutput);
    if (!d.backend)
        return false;
    if (!receiveData(data, pts))
        return false;
    return d.backend->play();
}

void AudioOutput::pause(bool value)
{
    DPTR_D(AudioOutput);
    d.paused = value;
    // backend pause? Without backend pause, the buffered data will be played
}

bool AudioOutput::isPaused() const
{
    return d_func().paused;
}

bool AudioOutput::receiveData(const QByteArray &data, qreal pts)
{
    DPTR_D(AudioOutput);
    if (isPaused())
        return false;
    QByteArray queue_data(data);
    if (isMute() && d.sw_mute) {
        char s = 0;
        if (d.format.isUnsigned() && !d.format.isFloat())
            s = 1<<((d.format.bytesPerSample() << 3)-1);
        queue_data.fill(s);
    } else {
        if (!qFuzzyCompare(volume(), (qreal)1.0)
                && d.sw_volume
                && d.scale_samples
                ) {
            // TODO: af_volume needs samples_align to get nb_samples
            const int nb_samples = queue_data.size()/d.format.bytesPerSample();
            quint8 *dst = (quint8*)queue_data.constData();
            d.scale_samples(dst, dst, nb_samples, d.volume_i, volume());
        }
    }
    // wait after all data processing finished to reduce time error
    if (!waitForNextBuffer()) { // TODO: wait or not parameter, set by user (async)
        qWarning("ao backend maybe not open");
        d.resetStatus();
        return false;
    }
    d.frame_infos.push_back(AudioOutputPrivate::FrameInfo(queue_data, pts, d.format.durationForBytes(queue_data.size())));
    return d.backend->write(queue_data); // backend is not null here
}

AudioFormat AudioOutput::setAudioFormat(const AudioFormat& format)
{
    DPTR_D(AudioOutput);
    // no support check because that may require an open device(AL) while this function is called before ao.open()
    if (d.format == format)
        return format;
    d.requested = format;
    if (!d.backend) {
        d.format = AudioFormat();
        d.scale_samples = NULL;
        return AudioFormat();
    }
    if (d.backend->isSupported(format)) {
        d.format = format;
        d.updateSampleScaleFunc();
        return format;
    }
    AudioFormat af(format);
    // set channel layout first so that isSupported(AudioFormat) will not always false
    if (!d.backend->isSupported(format.channelLayout()))
        af.setChannelLayout(AudioFormat::ChannelLayout_Stereo); // assume stereo is supported
    bool check_up = af.bytesPerSample() == 1;
    while (!d.backend->isSupported(af) && !d.backend->isSupported(af.sampleFormat())) {
        if (af.isPlanar()) {
            af.setSampleFormat(ToPacked(af.sampleFormat()));
            continue;
        }
        if (af.isFloat()) {
            if (af.bytesPerSample() == 8)
                af.setSampleFormat(AudioFormat::SampleFormat_Float);
            else
                af.setSampleFormat(AudioFormat::SampleFormat_Signed32);
        } else {
            af.setSampleFormat(AudioFormat::make(af.bytesPerSample()/2, false, (af.bytesPerSample() == 2) | af.isUnsigned() /* U8, no S8 */, false));
        }
        if (af.bytesPerSample() < 1) {
            if (!check_up) {
                qWarning("No sample format found");
                break;
            }
            af.setSampleFormat(AudioFormat::SampleFormat_Float);
            check_up = false;
            continue;
        }
    }
    d.format = af;
    d.updateSampleScaleFunc();
    return af;
}

const AudioFormat& AudioOutput::requestedFormat() const
{
    return d_func().requested;
}

const AudioFormat& AudioOutput::audioFormat() const
{
    return d_func().format;
}

void AudioOutput::setVolume(qreal value)
{
    DPTR_D(AudioOutput);
    if (value < 0.0)
        return;
    if (d.vol == value) //fuzzy compare?
        return;
    d.vol = value;
    Q_EMIT volumeChanged(value);
    d.updateSampleScaleFunc();
    d.tryVolume(value);
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
    Q_EMIT muteChanged(value);
    d.tryMute(value);
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
    DPTR_D(const AudioOutput);
    if (!d.backend)
        return false;
    return d.backend->isSupported(format);
}

int AudioOutput::bufferSize() const
{
    return bufferSamples() * d_func().format.bytesPerSample();
}

int AudioOutput::bufferSamples() const
{
    return d_func().buffer_samples;
}

void AudioOutput::setBufferSamples(int value)
{
    d_func().buffer_samples = value;
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
    DPTR_D(const AudioOutput);
    if (!d.backend)
        return NoFeature;
    return d.backend->supportedFeatures();
}

bool AudioOutput::waitForNextBuffer() // parameter bool wait: if no wait and no next buffer, return false
{
    DPTR_D(AudioOutput);
    if (d.frame_infos.empty())
        return true;
    //don't return even if we can add buffer because we don't know when a buffer is processed and we have /to update dequeue index
    // openal need enqueue to a dequeued buffer! why sl crash
    bool no_wait = false;//d.canAddBuffer();
    const AudioOutputBackend::BufferControl f = d.backend->bufferControl();
    int remove = 0;
    const AudioOutputPrivate::FrameInfo &fi(d.frame_infos.front());
    if (f & AudioOutputBackend::Blocking) {
        remove = 1;
    } else if (f & AudioOutputBackend::CountCallback) {
        d.backend->acquireNextBuffer();
        remove = 1;
    } else if (f & AudioOutputBackend::BytesCallback) {
#if AO_USE_TIMER
        d.timer.restart();
#endif //AO_USE_TIMER
        int processed = d.processed_remain;
        d.processed_remain = d.backend->getWritableBytes();
        if (d.processed_remain < 0)
            return false;
        const int next = fi.data.size();
        //qDebug("remain: %d-%d, size: %d, next: %d", processed, d.processed_remain, d.data.size(), next);
        qint64 last_wait = 0LL;
        while (d.processed_remain - processed < next || d.processed_remain < fi.data.size()) { //implies next > 0
            const qint64 us = d.format.durationForBytes(next - (d.processed_remain - processed));
            d.uwait(us);
            d.processed_remain = d.backend->getWritableBytes();
            if (d.processed_remain < 0)
                return false;
#if AO_USE_TIMER
            if (!d.timer.isValid()) {
                qWarning("invalid timer. closed in another thread");
                return false;
            }
#endif
            if (us >= last_wait
#if AO_USE_TIMER
                    && d.timer.elapsed() > 1000
#endif //AO_USE_TIMER
                    ) {
                return false;
            }
            last_wait = us;
        }
        processed = d.processed_remain - processed;
        d.processed_remain -= fi.data.size(); //ensure d.processed_remain later is greater
        remove = -processed; // processed_this_period
    } else if (f & AudioOutputBackend::PlayedBytes) {
        d.processed_remain = d.backend->getPlayedBytes();
        const int next = fi.data.size();
        // TODO: avoid always 0
        // TODO: compare processed_remain with fi.data.size because input chuncks can be in different sizes
        while (!no_wait && d.processed_remain < next) {
            const qint64 us = d.format.durationForBytes(next - d.processed_remain);
            if (us < 1000LL)
                d.uwait(1000LL);
            else
                d.uwait(us);
            d.processed_remain = d.backend->getPlayedBytes();
            // what if s always 0?
        }
        remove = -d.processed_remain;
    } else if (f & AudioOutputBackend::PlayedCount) {
#if AO_USE_TIMER
        if (!d.timer.isValid())
            d.timer.start();
        qint64 elapsed = 0;
#endif //AO_USE_TIMER
        int c = d.backend->getPlayedCount();
        // TODO: avoid always 0
        qint64 us = 0;
        while (!no_wait && c < 1) {
            if (us <= 0)
                us = fi.duration;
#if AO_USE_TIMER
            elapsed = d.timer.restart();
            if (elapsed > 0 && us > elapsed*1000LL)
                us -= elapsed*1000LL;
            if (us < 1000LL)
                us = 1000LL; //opensl crash if 1ms
#endif //AO_USE_TIMER
            d.uwait(us);
            c = d.backend->getPlayedCount();
        }
        // what if c always 0?
        remove = c;
    } else if (f & AudioOutputBackend::OffsetBytes) { //TODO: similar to Callback+getWritableBytes()
        int s = d.backend->getOffsetByBytes();
        int processed = s - d.play_pos;
        //qDebug("s: %d, play_pos: %d, processed: %d, bufferSizeTotal: %d", s, d.play_pos, processed, bufferSizeTotal());
        if (processed < 0)
            processed += bufferSizeTotal();
        d.play_pos = s;
        const int next = fi.data.size();
        int writable_size = d.processed_remain + processed;
        while (!no_wait && (/*processed < next ||*/ writable_size < fi.data.size()) && next > 0) {
            const qint64 us = d.format.durationForBytes(next - writable_size);
            d.uwait(us);
            s = d.backend->getOffsetByBytes();
            processed += s - d.play_pos;
            if (processed < 0)
                processed += bufferSizeTotal();
            writable_size = d.processed_remain + processed;
            d.play_pos = s;
        }
        d.processed_remain += processed;
        d.processed_remain -= fi.data.size(); //ensure d.processed_remain later is greater
        remove = -processed;
    } else if (f & AudioOutputBackend::OffsetIndex) {
        int n = d.backend->getOffset();
        int processed = n - d.play_pos;
        if (processed < 0)
            processed += bufferCount();
        d.play_pos = n;
        // TODO: timer
        // TODO: avoid always 0
        while (!no_wait && processed < 1) {
            d.uwait(fi.duration);
            n = d.backend->getOffset();
            processed = n - d.play_pos;
            if (processed < 0)
                processed += bufferCount();
            d.play_pos = n;
        }
        remove = processed;
    } else {
        qFatal("User defined waitForNextBuffer() not implemented!");
        return false;
    }
    if (remove < 0) {
        int next = fi.data.size();
        int free_bytes = -remove;//d.processed_remain;
        while (free_bytes >= next && next > 0) {
            free_bytes -= next;
            if (d.frame_infos.empty()) {
//                qWarning("buffer queue empty");
                break;
            }
            d.frame_infos.pop_front();
            next = d.frame_infos.front().data.size();
        }
        //qDebug("remove: %d, unremoved bytes < %d, writable_bytes: %d", remove, free_bytes, d.processed_remain);
        return true;
    }
    //qDebug("remove count: %d", remove);
    while (remove-- > 0) {
        if (d.frame_infos.empty()) {
//            qWarning("empty. can not pop!");
            break;
        }
        d.frame_infos.pop_front();
    }
    return true;
}


qreal AudioOutput::timestamp() const
{
    DPTR_D(const AudioOutput);
    return d.frame_infos.front().timestamp;
}

void AudioOutput::reportVolume(qreal value)
{
    if (qFuzzyCompare(value + 1.0, volume() + 1.0))
        return;
    DPTR_D(AudioOutput);
    d.vol = value;
    Q_EMIT volumeChanged(value);
    // skip sw sample scale
    d.sw_volume = false;
}

void AudioOutput::reportMute(bool value)
{
    if (value == isMute())
        return;
    DPTR_D(AudioOutput);
    d.mute = value;
    Q_EMIT muteChanged(value);
    // skip sw sample scale
    d.sw_mute = false;
}

void AudioOutput::onCallback()
{
    d_func().onCallback();
}
} //namespace QtAV
