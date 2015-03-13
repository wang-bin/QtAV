/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2015 Wang Bin <wbsecg1@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "QtAV/AudioOutput.h"
#include "QtAV/private/AudioOutput_p.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QMetaObject>
#include <pulse/pulseaudio.h>
#include "QtAV/private/prepost.h"
#include "utils/Logger.h"

namespace QtAV {

class AudioOutputPulsePrivate;
class AudioOutputPulse : public AudioOutput
{
    DPTR_DECLARE_PRIVATE(AudioOutputPulse)
public:
    AudioOutputPulse(QObject *parent = 0);
    ~AudioOutputPulse() Q_DECL_FINAL;

    bool isSupported(AudioFormat::SampleFormat sampleFormat) const Q_DECL_FINAL;
    bool isSupported(AudioFormat::ChannelLayout channelLayout) const Q_DECL_FINAL;
    bool open() Q_DECL_FINAL;
    bool close() Q_DECL_FINAL;

protected:
    bool write(const QByteArray& data) Q_DECL_FINAL;
    bool play() Q_DECL_FINAL;
    BufferControl bufferControl() const Q_DECL_FINAL;
    int getWritableBytes() Q_DECL_FINAL;

    bool deviceSetVolume(qreal value) Q_DECL_FINAL;
    qreal deviceGetVolume() const Q_DECL_FINAL;
    bool deviceSetMute(bool value = true) Q_DECL_FINAL;
};

extern AudioOutputId AudioOutputId_Pulse;
FACTORY_REGISTER_ID_AUTO(AudioOutput, Pulse, "Pulse")

void RegisterAudioOutputPulse_Man()
{
    FACTORY_REGISTER_ID_MAN(AudioOutput, Pulse, "Pulse")
}

#define PA_ENSURE_TRUE(expr, ...) \
    do { \
        if (!(expr)) { \
            qWarning("PulseAudio error @%d. " #expr ": %s", __LINE__, pa_strerror(pa_context_errno(d.ctx))); \
            return __VA_ARGS__; \
        } \
    } while(0)

static const struct format_entry {
    AudioFormat::SampleFormat spfmt;
    pa_sample_format_t pa;
} format_map[] = {
    {AudioFormat::SampleFormat_Signed16, PA_SAMPLE_S16NE},
    {AudioFormat::SampleFormat_Signed32, PA_SAMPLE_S32NE},
    {AudioFormat::SampleFormat_Float, PA_SAMPLE_FLOAT32NE},
    {AudioFormat::SampleFormat_Unsigned8, PA_SAMPLE_U8},
    {AudioFormat::SampleFormat_Unknown, PA_SAMPLE_INVALID}
};

AudioFormat::SampleFormat sampleFormatFromPulse(pa_sample_format pa)
{
    for (int i = 0; format_map[i].spfmt != AudioFormat::SampleFormat_Unknown; ++i) {
        if (format_map[i].pa == pa)
            return format_map[i].spfmt;
    }
    return AudioFormat::SampleFormat_Unknown;
}

static pa_sample_format sampleFormatToPulse(AudioFormat::SampleFormat fmt)
{
    for (int i = 0; format_map[i].spfmt != AudioFormat::SampleFormat_Unknown; ++i) {
        if (format_map[i].spfmt == fmt)
            return format_map[i].pa;
    }
    return PA_SAMPLE_INVALID;
}

class ScopedPALocker {
    pa_threaded_mainloop *ml;
public:
    ScopedPALocker(pa_threaded_mainloop *loop) : ml(loop) {
        pa_threaded_mainloop_lock(ml);
    }
    ~ScopedPALocker() {
        pa_threaded_mainloop_unlock(ml);
    }
};

class AudioOutputPulsePrivate : public AudioOutputPrivate
{
public:
    AudioOutputPulsePrivate()
        : AudioOutputPrivate()
        , loop(0)
        , ctx(0)
        , stream(0)
        , writable_size(0)
    {}
    bool init(AudioOutputPulse *ao);
    static void contextStateCallback(pa_context *c, void *userdata);
    static void contextSubscribeCallback(pa_context *c, pa_subscription_event_type_t t, uint32_t idx, void *userdata);
    static void stateCallback(pa_stream *s, void *userdata);
    static void latencyUpdateCallback(pa_stream *s, void *userdata);
    static void underflowCallback(pa_stream *s, void *userdata);
    static void writeCallback(pa_stream *s, size_t length, void *userdata);
    static void sinkInfoCallback(struct pa_context *c, const struct pa_sink_input_info *i, int is_last, void *userdata);

    bool waitPAOperation(pa_operation *op) const {
        if (!op) {
            return false;
        }
        pa_operation_state_t state = pa_operation_get_state(op);
        while (state == PA_OPERATION_RUNNING) {
            pa_threaded_mainloop_wait(loop);
            state = pa_operation_get_state(op);
        }
        pa_operation_unref(op);
        return state == PA_OPERATION_DONE;
    }

    pa_threaded_mainloop *loop;
    pa_context *ctx;
    pa_stream *stream;
    pa_sink_input_info info;
    size_t writable_size; //has the same effect as pa_stream_writable_size
};

void AudioOutputPulsePrivate::contextStateCallback(pa_context *c, void *userdata)
{
    AudioOutputPulsePrivate *p = reinterpret_cast<AudioOutputPulsePrivate*>(userdata);
    switch (pa_context_get_state(c)) {
    case PA_CONTEXT_FAILED:
    case PA_CONTEXT_TERMINATED:
    case PA_CONTEXT_READY:
        pa_threaded_mainloop_signal(p->loop, 0);
        break;
    default:
        break;
    }
}

static void sink_input_info_cb(pa_context *c, const pa_sink_input_info *i, int eol, void *userdata)
{
    Q_UNUSED(c);
    if (eol)
        return;
    AudioOutputPulse *ao = reinterpret_cast<AudioOutputPulse*>(userdata);
    //signal may be protected in Qt4
    QMetaObject::invokeMethod(ao,  "volumeReported", Q_ARG(qreal, (qreal)pa_cvolume_avg(&i->volume)/qreal(PA_VOLUME_NORM)));
    QMetaObject::invokeMethod(ao, "muteReported", Q_ARG(bool, i->mute));
}

static void sink_input_event(pa_context* c, pa_subscription_event_type_t t, uint32_t idx, AudioOutputPulse* ao)
{
    switch (t) {
    case PA_SUBSCRIPTION_EVENT_REMOVE:
        qWarning("PulseAudio sink killed");
        break;
    default:
        pa_operation *op = pa_context_get_sink_input_info(c, idx, sink_input_info_cb, ao);
        if (Q_LIKELY(!!op))
            pa_operation_unref(op);
        break;
    }
}

void AudioOutputPulsePrivate::contextSubscribeCallback(pa_context *c, pa_subscription_event_type_t type, uint32_t idx, void *userdata)
{
    AudioOutputPulse *ao = reinterpret_cast<AudioOutputPulse*>(userdata);
    AudioOutputPulsePrivate *p = &ao->d_func();
    unsigned facility = type & PA_SUBSCRIPTION_EVENT_FACILITY_MASK;
    pa_subscription_event_type_t t = pa_subscription_event_type_t(type & PA_SUBSCRIPTION_EVENT_TYPE_MASK);
    switch (facility) {
    case PA_SUBSCRIPTION_EVENT_SINK:
        break;
    case PA_SUBSCRIPTION_EVENT_SINK_INPUT:
        if (p->stream && idx == pa_stream_get_index(p->stream))
            sink_input_event(c, t, idx, ao);
        break;
    case  PA_SUBSCRIPTION_EVENT_CARD:
        qDebug("PA_SUBSCRIPTION_EVENT_CARD");
        break;
    default:
        break;
    }
}

void AudioOutputPulsePrivate::stateCallback(pa_stream *s, void *userdata)
{
    AudioOutputPulsePrivate *p = reinterpret_cast<AudioOutputPulsePrivate*>(userdata);
    switch (pa_stream_get_state(s)) {
    case PA_STREAM_FAILED:
        qWarning("PA_STREAM_FAILED");
        pa_threaded_mainloop_signal(p->loop, 0);
        break;
    case PA_STREAM_READY:
    case PA_STREAM_TERMINATED:
        pa_threaded_mainloop_signal(p->loop, 0);
        break;
    default:
        break;
    }
}

void AudioOutputPulsePrivate::latencyUpdateCallback(pa_stream *s, void *userdata)
{
    Q_UNUSED(s);
    Q_UNUSED(userdata);
}

void AudioOutputPulsePrivate::writeCallback(pa_stream *s, size_t length, void *userdata)
{
    Q_UNUSED(s);
    // length: writable bytes. callback is called pirioddically
    AudioOutputPulsePrivate *p = reinterpret_cast<AudioOutputPulsePrivate*>(userdata);
    //qDebug("write callback: %d + %d", p->writable_size, length);
    p->writable_size = length;
    p->onCallback();
}

void AudioOutputPulsePrivate::sinkInfoCallback(pa_context *c, const pa_sink_input_info *i, int is_last, void *userdata)
{
    Q_UNUSED(c);
    AudioOutputPulsePrivate *p = reinterpret_cast<AudioOutputPulsePrivate*>(userdata);
    if (is_last < 0) {
        qWarning("Failed to get sink input info");
        return;
    }
    if (!i)
        return;
    p->info = *i;
    pa_threaded_mainloop_signal(p->loop, 0);
}

bool AudioOutputPulsePrivate::init(AudioOutputPulse *ao)
{
    resetStatus();
    writable_size = 0;
    loop = pa_threaded_mainloop_new();
    if (pa_threaded_mainloop_start(loop) < 0) {
        qWarning("PulseAudio failed to start mainloop");
        return false;
    }

    ScopedPALocker lock(loop);
    Q_UNUSED(lock);
    pa_mainloop_api *api = pa_threaded_mainloop_get_api(loop);
    ctx = pa_context_new(api, qApp->applicationName().append(" (QtAV)").toUtf8().constData());
    if (!ctx) {
        qWarning("PulseAudio failed to allocate a context");
        return false;
    }
    qDebug() << QString("PulseAudio %1, protocol: %2, server protocol: %3").arg(pa_get_library_version()).arg(pa_context_get_protocol_version(ctx)).arg(pa_context_get_server_protocol_version(ctx));
    // TODO: host property
    pa_context_connect(ctx, NULL, PA_CONTEXT_NOFLAGS, NULL);
    pa_context_set_state_callback(ctx, AudioOutputPulsePrivate::contextStateCallback, this);
    while (true) {
        const pa_context_state_t st = pa_context_get_state(ctx);
        if (st == PA_CONTEXT_READY)
            break;
        if (!PA_CONTEXT_IS_GOOD(st)) {
            qWarning("PulseAudio context init failed");
            return false;
        }
        pa_threaded_mainloop_wait(loop);
    }

    pa_context_set_subscribe_callback(ctx, AudioOutputPulsePrivate::contextSubscribeCallback, ao);
    pa_context_subscribe(ctx, pa_subscription_mask_t(
                                 PA_SUBSCRIPTION_MASK_CARD |
                                 PA_SUBSCRIPTION_MASK_SINK |
                                 PA_SUBSCRIPTION_MASK_SINK_INPUT),
                         NULL, NULL);
    //pa_sample_spec
    // setup format
    pa_format_info *fi = pa_format_info_new();
    fi->encoding = PA_ENCODING_PCM;
    pa_format_info_set_sample_format(fi, sampleFormatToPulse(format.sampleFormat()));
    pa_format_info_set_channels(fi, format.channels());
    pa_format_info_set_rate(fi, format.sampleRate());
   // pa_format_info_set_channel_map(fi, NULL); // TODO
    if (!pa_format_info_valid(fi)) {
        qWarning("PulseAudio: invalid format");
        return false;
    }
    pa_proplist *pl = pa_proplist_new();
    if (pl) {
        pa_proplist_sets(pl, PA_PROP_MEDIA_ROLE, "video");
        pa_proplist_sets(pl, PA_PROP_MEDIA_ICON_NAME, qApp->applicationName().append(" (QtAV)").toUtf8().constData());
    }
    stream = pa_stream_new_extended(ctx, "audio stream", &fi, 1, pl);
    if (!stream) {
        pa_format_info_free(fi);
        pa_proplist_free(pl);
        qWarning("PulseAudio: failed to create a stream");
        return false;
    }
    pa_format_info_free(fi);
    pa_proplist_free(pl);
    pa_stream_set_write_callback(stream, AudioOutputPulsePrivate::writeCallback, this);
    pa_stream_set_state_callback(stream, AudioOutputPulsePrivate::stateCallback, this);
    pa_stream_set_latency_update_callback(stream, AudioOutputPulsePrivate::latencyUpdateCallback, this);

    pa_buffer_attr ba;
    ba.maxlength = bufferSizeTotal(); // max buffer size on the server
    ba.tlength = (uint32_t)-1; // ?
    ba.prebuf = 1;//(uint32_t)-1; // play as soon as possible
    ba.minreq = (uint32_t)-1;
    ba.fragsize = (uint32_t)-1;
    // PA_STREAM_NOT_MONOTONIC?
    pa_stream_flags_t flags = pa_stream_flags_t(PA_STREAM_NOT_MONOTONIC|PA_STREAM_INTERPOLATE_TIMING|PA_STREAM_AUTO_TIMING_UPDATE);
    if (pa_stream_connect_playback(stream, NULL /*sink*/, &ba, flags, NULL, NULL) < 0) {
        qWarning("PulseAudio failed: pa_stream_connect_playback");
        return false;
    }

    while (true) {
        const pa_stream_state_t st = pa_stream_get_state(stream);
        if (st == PA_STREAM_READY)
            break;
        if (!PA_STREAM_IS_GOOD(st)) {
            qWarning("PulseAudio stream init failed");
            return false;
        }
        pa_threaded_mainloop_wait(loop);
    }
    if (pa_stream_is_suspended(stream)) {
        qWarning("PulseAudio stream is suspended.");
        return false;
    }
    available = true;
    return true;
}

AudioOutputPulse::AudioOutputPulse(QObject *parent)
    : AudioOutput(DeviceFeatures()|SetVolume|SetMute|SetSampleRate, *new AudioOutputPulsePrivate(), parent)
{
    setDeviceFeatures(DeviceFeatures()|SetVolume|SetMute);
}

AudioOutputPulse::~AudioOutputPulse()
{
    close();
}

bool AudioOutputPulse::isSupported(AudioFormat::SampleFormat spfmt) const
{
    for (int i = 0; format_map[i].spfmt != AudioFormat::SampleFormat_Unknown; ++i) {
        if (format_map[i].spfmt == spfmt)
            return true;
    }
    return false;
}

bool AudioOutputPulse::isSupported(AudioFormat::ChannelLayout channelLayout) const
{
    Q_UNUSED(channelLayout);
    return true;
}

bool AudioOutputPulse::open()
{
    DPTR_D(AudioOutputPulse);
    if (!d.init(this)) {
        if (d.ctx)
            qWarning("%s", pa_strerror(pa_context_errno(d.ctx)));
        close();
        return false;
    }
    for (int i = 0; i < bufferCount(); ++i) {
        write(QByteArray(bufferSize(), 0));
        d.bufferAdded();
    }
    return true;
}

bool AudioOutputPulse::close()
{
    DPTR_D(AudioOutputPulse);
    resetStatus();
    if (d.loop) {
        pa_threaded_mainloop_stop(d.loop);
    }
    if (d.stream) {
        pa_stream_disconnect(d.stream);
        pa_stream_unref(d.stream);
        d.stream = NULL;
    }
    if (d.ctx) {
        pa_context_disconnect(d.ctx);
        pa_context_unref(d.ctx);
        d.ctx = NULL;
    }
    if (d.loop) {
        pa_threaded_mainloop_free(d.loop);
        d.loop = NULL;
    }
    return true;
}

AudioOutput::BufferControl AudioOutputPulse::bufferControl() const
{
    return Callback;
}

int AudioOutputPulse::getWritableBytes()
{
    DPTR_D(AudioOutputPulse);
    //return d.writable_size;
    ScopedPALocker palock(d.loop);
    Q_UNUSED(palock);
    return pa_stream_writable_size(d.stream);
}

bool AudioOutputPulse::write(const QByteArray &data)
{
    DPTR_D(AudioOutputPulse);
    ScopedPALocker palock(d.loop);
    Q_UNUSED(palock);
    PA_ENSURE_TRUE(pa_stream_write(d.stream, data.constData(), data.size(), NULL, 0LL, PA_SEEK_RELATIVE) >= 0, false);
    d.writable_size -= data.size();
    return true;
}

bool AudioOutputPulse::play()
{
    return true;
}

bool AudioOutputPulse::deviceSetVolume(qreal value)
{
    DPTR_D(AudioOutputPulse);
    ScopedPALocker palock(d.loop);
    Q_UNUSED(palock);
    uint32_t stream_idx = pa_stream_get_index(d.stream);
    struct pa_cvolume vol; // TODO: per-channel volume
    pa_cvolume_reset(&vol, channels());
    pa_cvolume_set(&vol, channels(), pa_volume_t(value*qreal(PA_VOLUME_NORM)));
    pa_operation *o = 0;
    PA_ENSURE_TRUE((o = pa_context_set_sink_input_volume(d.ctx, stream_idx, &vol, NULL, NULL)) != NULL, false);
    pa_operation_unref(o);
    return true;
}

qreal AudioOutputPulse::deviceGetVolume() const
{
    DPTR_D(const AudioOutputPulse);
    ScopedPALocker palock(d.loop);
    Q_UNUSED(palock);
    uint32_t stream_idx = pa_stream_get_index(d.stream);
    PA_ENSURE_TRUE(d.waitPAOperation(pa_context_get_sink_input_info(d.ctx, stream_idx, AudioOutputPulsePrivate::sinkInfoCallback, (void*)&d)), 0.0);
    return (qreal)pa_cvolume_avg(&d.info.volume)/qreal(PA_VOLUME_NORM);
}

bool AudioOutputPulse::deviceSetMute(bool value)
{
    DPTR_D(AudioOutputPulse);
    ScopedPALocker palock(d.loop);
    Q_UNUSED(palock);
    uint32_t stream_idx = pa_stream_get_index(d.stream);
    pa_operation *o = 0;
    PA_ENSURE_TRUE((o = pa_context_set_sink_input_mute(d.ctx, stream_idx, value, NULL, NULL)) != NULL, false);
    pa_operation_unref(o);
    return true;
}
} //namespace QtAV
