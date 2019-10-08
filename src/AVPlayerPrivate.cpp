/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2017 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2014)

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

#include "AVPlayerPrivate.h"
#include "filter/FilterManager.h"
#include "output/OutputSet.h"
#include "QtAV/AudioDecoder.h"
#include "QtAV/AudioFormat.h"
#include "QtAV/AudioResampler.h"
#include "QtAV/MediaIO.h"
#include "QtAV/VideoCapture.h"
#include "QtAV/private/AVCompat.h"
#if AV_MODULE_CHECK(LIBAVFORMAT, 55, 18, 0, 39, 100)
extern "C" {
#include <libavutil/display.h>
}
#endif
#include "utils/Logger.h"

namespace QtAV {

namespace Internal {

int computeNotifyPrecision(qint64 duration, qreal fps)
{
    if (duration <= 0 || duration > 60*1000) // no duration or 10min
        return 500;
    if (duration > 20*1000)
        return 250;
    int dt = 500;
    if (fps > 1) {
        dt = qMin(250, int(qreal(dt*2)/fps));
    } else {
        dt = duration / 80; //<= 250
    }
    return qMax(20, dt);
}
} // namespace Internal

static bool correct_audio_channels(AVCodecContext *ctx) {
    if (ctx->channels <= 0) {
        if (ctx->channel_layout) {
            ctx->channels = av_get_channel_layout_nb_channels(ctx->channel_layout);
        }
    } else {
        if (!ctx->channel_layout) {
            ctx->channel_layout = av_get_default_channel_layout(ctx->channels);
        }
    }
    return ctx->channel_layout > 0 && ctx->channels > 0;
}

AVPlayer::Private::Private()
    : auto_load(false)
    , async_load(true)
    , loaded(false)
    , relative_time_mode(true)
    , media_start_pts(0)
    , media_end(kInvalidPosition)
    , reset_state(true)
    , start_position(0)
    , stop_position(kInvalidPosition)
    , start_position_norm(0)
    , stop_position_norm(kInvalidPosition)
    , repeat_max(0)
    , repeat_current(-1)
    , timer_id(-1)
    , audio_track(0)
    , video_track(0)
    , subtitle_track(0)
    , buffer_mode(BufferPackets)
    , buffer_value(-1)
    , read_thread(0)
    , clock(new AVClock(AVClock::AudioClock))
    , vo(0)
    , ao(new AudioOutput())
    , adec(0)
    , vdec(0)
    , athread(0)
    , vthread(0)
    , vcapture(0)
    , speed(1.0)
    , vos(0)
    , aos(0)
    , brightness(0)
    , contrast(0)
    , saturation(0)
    , seeking(false)
    , seek_type(AccurateSeek)
    , interrupt_timeout(30000)
    , force_fps(0)
    , notify_interval(-500)
    , status(NoMedia)
    , state(AVPlayer::StoppedState)
    , end_action(MediaEndAction_Default)
    , last_known_good_pts(0)
    , was_stepping(false)
{
    demuxer.setInterruptTimeout(interrupt_timeout);
    /*
     * reset_state = true;
     * must be the same value at the end of stop(), and must be different from value in
     * stopFromDemuxerThread()(which is false), so the initial value must be true
     */

    vc_ids
#if QTAV_HAVE(DXVA)
            //<< VideoDecoderId_DXVA
#endif //QTAV_HAVE(DXVA)
#if QTAV_HAVE(VAAPI)
            //<< VideoDecoderId_VAAPI
#endif //QTAV_HAVE(VAAPI)
#if QTAV_HAVE(CEDARV)
            << VideoDecoderId_Cedarv
#endif //QTAV_HAVE(CEDARV)
            << VideoDecoderId_FFmpeg;
}
AVPlayer::Private::~Private() {
    // TODO: scoped ptr
    if (ao) {
        delete ao;
        ao = 0;
    }
    if (adec) {
        delete adec;
        adec = 0;
    }
    if (vdec) {
        delete vdec;
        vdec = 0;
    }
    if (vos) {
        vos->clearOutputs();
        delete vos;
        vos = 0;
    }
    if (aos) {
        aos->clearOutputs();
        delete aos;
        aos = 0;
    }
    if (vcapture) {
        delete vcapture;
        vcapture = 0;
    }
    if (clock) {
        delete clock;
        clock = 0;
    }
    if (read_thread) {
        delete read_thread;
        read_thread = 0;
    }
}

bool AVPlayer::Private::checkSourceChange()
{
    if (current_source.type() == QVariant::String)
        return demuxer.fileName() != current_source.toString();
    if (current_source.canConvert<QIODevice*>())
        return demuxer.ioDevice() != current_source.value<QIODevice*>();
    return demuxer.mediaIO() != current_source.value<QtAV::MediaIO*>();
}

void AVPlayer::Private::updateNotifyInterval()
{
    if (notify_interval <= 0) {
        notify_interval = -Internal::computeNotifyPrecision(demuxer.duration(), demuxer.frameRate());
    }
    qDebug("notify_interval: %d", qAbs(notify_interval));
}

void AVPlayer::Private::applyFrameRate()
{
    qreal vfps = force_fps;
    bool force = vfps > 0;
    const bool ao_null = ao && ao->backend().toLower() == QLatin1String("null");
    if (athread && !ao_null) { // TODO: no null ao check. null ao block internally
        force = vfps > 0 && !!vthread;
    } else if (!force) {
        force = !!vthread;
        vfps = statistics.video.frame_rate > 0 ? statistics.video.frame_rate : 25;
        // vfps<0: try to use pts (ExternalClock). if no pts (raw codec), try the default fps(VideoClock)
        vfps = -vfps;
    }
    qreal r = speed;
    if (force) {
        clock->setClockAuto(false);
        // vfps>0: force video fps to vfps. clock must be external
        clock->setClockType(vfps > 0 ? AVClock::VideoClock : AVClock::ExternalClock);
        vthread->setFrameRate(vfps);
        if (statistics.video.frame_rate > 0)
            r = qAbs(qreal(vfps))/statistics.video.frame_rate;
    } else {
        clock->setClockAuto(true);
        clock->setClockType(athread && ao->isOpen() ? AVClock::AudioClock : AVClock::ExternalClock);
        if (vthread)
            vthread->setFrameRate(0.0);
        ao->setSpeed(1);
        clock->setSpeed(1);
    }
    ao->setSpeed(r);
    clock->setSpeed(r);
}

void AVPlayer::Private::initStatistics()
{
    initBaseStatistics();
    initAudioStatistics(demuxer.audioStream());
    initVideoStatistics(demuxer.videoStream());
    //initSubtitleStatistics(demuxer.subtitleStream());
}

//TODO: av_guess_frame_rate in latest ffmpeg
void AVPlayer::Private::initBaseStatistics()
{
    statistics.reset();
    statistics.url = current_source.type() == QVariant::String ? current_source.toString() : QString();
    statistics.start_time = QTime(0, 0, 0).addMSecs(int(demuxer.startTime()));
    statistics.duration = QTime(0, 0, 0).addMSecs((int)demuxer.duration());
    AVFormatContext *fmt_ctx = demuxer.formatContext();
    if (!fmt_ctx) {
        qWarning("demuxer.formatContext()==null. internal error");
        updateNotifyInterval();
        return;
    }
    statistics.bit_rate = fmt_ctx->bit_rate;
    statistics.format = QString().sprintf("%s - %s", fmt_ctx->iformat->name, fmt_ctx->iformat->long_name);
    //AV_TIME_BASE_Q: msvc error C2143
    //fmt_ctx->duration may be AV_NOPTS_VALUE. AVDemuxer.duration deals with this case
    AVDictionaryEntry *tag = NULL;
    while ((tag = av_dict_get(fmt_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        statistics.metadata.insert(QString::fromUtf8(tag->key), QString::fromUtf8(tag->value));
    }
    updateNotifyInterval();
}

void AVPlayer::Private::initCommonStatistics(int s, Statistics::Common *st, AVCodecContext *avctx)
{
    AVFormatContext *fmt_ctx = demuxer.formatContext();
    if (!fmt_ctx) {
        qWarning("demuxer.formatContext()==null. internal error");
        return;
    }
    AVStream *stream = fmt_ctx->streams[s];
    qDebug("stream: %d, duration=%lld (%lld ms), time_base=%f", s, stream->duration, qint64(qreal(stream->duration)*av_q2d(stream->time_base)*1000.0), av_q2d(stream->time_base));
    // AVCodecContext.codec_name is deprecated. use avcodec_get_name. check null avctx->codec?
    st->codec = QLatin1String(avcodec_get_name(avctx->codec_id));
    st->codec_long = QLatin1String(get_codec_long_name(avctx->codec_id));
    st->total_time = QTime(0, 0, 0).addMSecs(stream->duration == (qint64)AV_NOPTS_VALUE ? 0 : int(qreal(stream->duration)*av_q2d(stream->time_base)*1000.0));
    st->start_time = QTime(0, 0, 0).addMSecs(stream->start_time == (qint64)AV_NOPTS_VALUE ? 0 : int(qreal(stream->start_time)*av_q2d(stream->time_base)*1000.0));
    qDebug("codec: %s(%s)", qPrintable(st->codec), qPrintable(st->codec_long));
    st->bit_rate = avctx->bit_rate; //fmt_ctx
    st->frames = stream->nb_frames;
    if (stream->avg_frame_rate.den && stream->avg_frame_rate.num)
        st->frame_rate = av_q2d(stream->avg_frame_rate);
#if (defined FF_API_R_FRAME_RATE && FF_API_R_FRAME_RATE) //removed in libav10
    //FIXME: which 1 should we choose? avg_frame_rate may be nan, r_frame_rate may be wrong(guessed value)
    else if (stream->r_frame_rate.den && stream->r_frame_rate.num) {
        st->frame_rate = av_q2d(stream->r_frame_rate);
        qDebug("%d/%d", stream->r_frame_rate.num, stream->r_frame_rate.den);
    }
#endif //FF_API_R_FRAME_RATE
    //http://ffmpeg.org/faq.html#AVStream_002er_005fframe_005frate-is-wrong_002c-it-is-much-larger-than-the-frame-rate_002e
    //http://libav-users.943685.n4.nabble.com/Libav-user-Reading-correct-frame-rate-fps-of-input-video-td4657666.html
    //qDebug("time: %f~%f, nb_frames=%lld", st->start_time, st->total_time, stream->nb_frames); //why crash on mac? av_q2d({0,0})?
    AVDictionaryEntry *tag = NULL;
    while ((tag = av_dict_get(stream->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        st->metadata.insert(QString::fromUtf8(tag->key), QString::fromUtf8(tag->value));
    }
}

void AVPlayer::Private::initAudioStatistics(int s)
{
    AVCodecContext *avctx = demuxer.audioCodecContext();
    statistics.audio = Statistics::Common();
    statistics.audio_only = Statistics::AudioOnly();
    if (!avctx)
        return;
    statistics.audio.available = s == demuxer.audioStream();
    initCommonStatistics(s, &statistics.audio, avctx);
    if (adec) {
        statistics.audio.decoder = adec->name();
        statistics.audio.decoder_detail = adec->description();
    }
    correct_audio_channels(avctx);
    statistics.audio_only.block_align = avctx->block_align;
    statistics.audio_only.channels = avctx->channels;
    char cl[128]; //
    // nb_channels -1: will use av_get_channel_layout_nb_channels
    av_get_channel_layout_string(cl, sizeof(cl), avctx->channels, avctx->channel_layout);
    statistics.audio_only.channel_layout = QLatin1String(cl);
    statistics.audio_only.sample_fmt = QLatin1String(av_get_sample_fmt_name(avctx->sample_fmt));
    statistics.audio_only.frame_size = avctx->frame_size;
    statistics.audio_only.sample_rate = avctx->sample_rate;
}

void AVPlayer::Private::initVideoStatistics(int s)
{
    AVCodecContext *avctx = demuxer.videoCodecContext();
    statistics.video = Statistics::Common();
    statistics.video_only = Statistics::VideoOnly();
    if (!avctx)
        return;
    statistics.video.available = s == demuxer.videoStream();
    initCommonStatistics(s, &statistics.video, avctx);
    if (vdec) {
        statistics.video.decoder = vdec->name();
        statistics.video.decoder_detail = vdec->description();
    }
    statistics.video_only.coded_height = avctx->coded_height;
    statistics.video_only.coded_width = avctx->coded_width;
    statistics.video_only.gop_size = avctx->gop_size;
    statistics.video_only.pix_fmt = QLatin1String(av_get_pix_fmt_name(avctx->pix_fmt));
    statistics.video_only.height = avctx->height;
    statistics.video_only.width = avctx->width;
    statistics.video_only.rotate = 0;
#if AV_MODULE_CHECK(LIBAVFORMAT, 55, 18, 0, 39, 100)
    quint8 *sd = av_stream_get_side_data(demuxer.formatContext()->streams[s], AV_PKT_DATA_DISPLAYMATRIX, NULL);
    if (sd) {
        double r = av_display_rotation_get((qint32*)sd);
        if (!qIsNaN(r))
            statistics.video_only.rotate = ((int)r + 360) % 360;
    }
#endif
}
// notify statistics change after audio/video thread is set
bool AVPlayer::Private::setupAudioThread(AVPlayer *player)
{
    AVDemuxer *ademuxer = &demuxer;
    if (!external_audio.isEmpty())
        ademuxer = &audio_demuxer;
    ademuxer->setStreamIndex(AVDemuxer::AudioStream, audio_track);
    // pause demuxer, clear queues, set demuxer stream, set decoder, set ao, resume
    // clear packets before stream changed
    if (athread) {
        athread->packetQueue()->clear();
        athread->setDecoder(0);
        athread->setOutput(0);
    }
    AVCodecContext *avctx = ademuxer->audioCodecContext();
    if (!avctx) {
        // TODO: close ao? //TODO: check pulseaudio perapp control if closed
        return false;
    }
    qDebug("has audio");
    // TODO: no delete, just reset avctx and reopen
    if (adec) {
        adec->disconnect();
        delete adec;
        adec = 0;
    }
    adec = AudioDecoder::create();
    if (!adec)
    {
        qWarning("failed to create audio decoder");
        return false;
    }
    QObject::connect(adec, SIGNAL(error(QtAV::AVError)), player, SIGNAL(error(QtAV::AVError)));
    adec->setCodecContext(avctx);
    adec->setOptions(ac_opt);
    if (!adec->open()) {
        AVError e(AVError::AudioCodecNotFound);
        qWarning() << e.string();
        emit player->error(e);
        return false;
    }
    correct_audio_channels(avctx);
    AudioFormat af;
    af.setSampleRate(avctx->sample_rate);
    af.setSampleFormatFFmpeg(avctx->sample_fmt);
    af.setChannelLayoutFFmpeg(avctx->channel_layout);
    if (!af.isValid()) {
        qWarning("invalid audio format. audio stream will be disabled");
        return false;
    }
    //af.setChannels(avctx->channels);
    // always reopen to ensure internal buffer queue inside audio backend(openal) is clear. also make it possible to change backend when replay.
    //if (ao->audioFormat() != af) {
        //qDebug("ao audio format is changed. reopen ao");
        ao->setAudioFormat(af); /// set before close to workaround OpenAL context lost
        ao->close();
        qDebug() << "AudioOutput format: " << ao->audioFormat() << "; requested: " << ao->requestedFormat();
        if (!ao->open()) {
            return false;
        }
    //}
    adec->resampler()->setOutAudioFormat(ao->audioFormat());
    // no need to set resampler if AudioFrame is used
#if !USE_AUDIO_FRAME
    adec->resampler()->inAudioFormat().setSampleFormatFFmpeg(avctx->sample_fmt);
    adec->resampler()->inAudioFormat().setSampleRate(avctx->sample_rate);
    adec->resampler()->inAudioFormat().setChannels(avctx->channels);
    adec->resampler()->inAudioFormat().setChannelLayoutFFmpeg(avctx->channel_layout);
#endif
    if (audio_track < 0)
        return true;
    if (!athread) {
        qDebug("new audio thread");
        athread = new AudioThread(player);
        athread->setClock(clock);
        athread->setStatistics(&statistics);
        athread->setOutputSet(aos);
        qDebug("demux thread setAudioThread");
        read_thread->setAudioThread(athread);
        //reconnect if disconnected
        QList<Filter*> filters = FilterManager::instance().audioFilters(player);
        //TODO: isEmpty()==false but size() == 0 in debug mode, it's a Qt bug? we can not just foreach without check empty in debug mode
        if (filters.size() > 0) {
            foreach (Filter *filter, filters) {
                athread->installFilter(filter);
            }
        }
    }

    // we set the thre state before the thread start,
    // as it maybe clear after by AVDemuxThread starting
    athread->resetState();
    athread->setDecoder(adec);
    setAVOutput(ao, ao, athread);
    updateBufferValue(athread->packetQueue());
    initAudioStatistics(ademuxer->audioStream());
    return true;
}

QVariantList AVPlayer::Private::getTracksInfo(AVDemuxer *demuxer, AVDemuxer::StreamType st)
{
    QVariantList info;
    if (!demuxer)
        return info;
    QList<int> streams;
    switch (st) {
    case AVDemuxer::AudioStream:
        streams = demuxer->audioStreams();
        break;
    case AVDemuxer::SubtitleStream:
        streams = demuxer->subtitleStreams();
        break;
    case AVDemuxer::VideoStream:
        streams = demuxer->videoStreams();
    default:
        break;
    }
    if (streams.isEmpty())
        return info;
    foreach (int s, streams) {
        QVariantMap t;
        t[QStringLiteral("id")] = info.size();
        t[QStringLiteral("file")] = demuxer->fileName();
        t[QStringLiteral("stream_index")] = QVariant(s);

        AVStream *stream = demuxer->formatContext()->streams[s];
        AVCodecContext *ctx = stream->codec;
        if (ctx) {
            t[QStringLiteral("codec")] = QByteArray(avcodec_descriptor_get(ctx->codec_id)->name);
            if (ctx->extradata)
                t[QStringLiteral("extra")] = QByteArray((const char*)ctx->extradata, ctx->extradata_size);
        }
        AVDictionaryEntry *tag = av_dict_get(stream->metadata, "language", NULL, 0);
        if (!tag)
            tag = av_dict_get(stream->metadata, "lang", NULL, 0);
        if (tag) {
            t[QStringLiteral("language")] = QString::fromUtf8(tag->value);
        }
        tag = av_dict_get(stream->metadata, "title", NULL, 0);
        if (tag) {
            t[QStringLiteral("title")] = QString::fromUtf8(tag->value);
        }
        info.push_back(t);
    }
    //QVariantMap t;
    //t[QStringLiteral("id")] = -1;
    //info.prepend(t);
    return info;
}

bool AVPlayer::Private::applySubtitleStream(int n, AVPlayer *player)
{
    if (!demuxer.setStreamIndex(AVDemuxer::SubtitleStream, n))
        return false;
    AVCodecContext *ctx = demuxer.subtitleCodecContext();
    if (!ctx)
        return false;
    // FIXME: AVCodecDescriptor.name and AVCodec.name are different!
    const AVCodecDescriptor *codec_desc = avcodec_descriptor_get(ctx->codec_id);
    QByteArray codec(codec_desc->name);
    if (ctx->extradata)
        Q_EMIT player->internalSubtitleHeaderRead(codec, QByteArray((const char*)ctx->extradata, ctx->extradata_size));
    else
        Q_EMIT player->internalSubtitleHeaderRead(codec, QByteArray());
    return true;
}

bool AVPlayer::Private::tryApplyDecoderPriority(AVPlayer *player)
{
    // TODO: add an option to apply the new decoder even if not available
    qint64 pos = player->position();
    VideoDecoder *vd = NULL;
    AVCodecContext *avctx = demuxer.videoCodecContext();
    foreach(VideoDecoderId vid, vc_ids) {
        qDebug("**********trying video decoder: %s...", VideoDecoder::name(vid));
        vd = VideoDecoder::create(vid);
        if (!vd)
            continue;
        vd->setCodecContext(avctx); // It's fine because AVDecoder copy the avctx properties
        vd->setOptions(vc_opt);
        if (vd->open()) {
            qDebug("**************Video decoder found:%p", vd);
            break;
        }
        delete vd;
        vd = 0;
    }
    qDebug("**************set new decoder:%p -> %p", vdec, vd);
    if (!vd) {
        Q_EMIT player->error(AVError(AVError::VideoCodecNotFound));
        return false;
    }
    if (vd->id() == vdec->id()
            && vd->options() == vdec->options()) {
        qDebug("Video decoder does not change");
        delete vd;
        return true;
    }
    vthread->packetQueue()->clear();
    vthread->setDecoder(vd);
    // MUST delete decoder after video thread set the decoder to ensure the deleted vdec will not be used in vthread!
    if (vdec)
        delete vdec;
    vdec = vd;
    QObject::connect(vdec, SIGNAL(error(QtAV::AVError)), player, SIGNAL(error(QtAV::AVError)));
    initVideoStatistics(demuxer.videoStream());
    // If no seek, drop packets until a key frame packet is found. But we may drop too many packets, and also a/v sync is a problem.
    player->setPosition(pos);
    return true;
}

bool AVPlayer::Private::setupVideoThread(AVPlayer *player)
{
    demuxer.setStreamIndex(AVDemuxer::VideoStream, video_track);
    // pause demuxer, clear queues, set demuxer stream, set decoder, set ao, resume
    // clear packets before stream changed
    if (vthread) {
        vthread->packetQueue()->clear();
        vthread->setDecoder(0);
    }
    AVCodecContext *avctx = demuxer.videoCodecContext();
    if (!avctx) {
        return false;
    }
    if (vdec) {
        vdec->disconnect();
        delete vdec;
        vdec = 0;
    }
    foreach(VideoDecoderId vid, vc_ids) {
        qDebug("**********trying video decoder: %s...", VideoDecoder::name(vid));
        VideoDecoder *vd = VideoDecoder::create(vid);
        if (!vd) {
            continue;
        }
        //vd->isAvailable() //TODO: the value is wrong now
        vd->setCodecContext(avctx);
        vd->setOptions(vc_opt);
        if (vd->open()) {
            vdec = vd;
            qDebug("**************Video decoder found:%p", vdec);
            break;
        }
        delete vd;
    }
    if (!vdec) {
        // DO NOT emit error signals in VideoDecoder::open(). 1 signal is enough
        AVError e(AVError::VideoCodecNotFound);
        qWarning() << e.string();
        emit player->error(e);
        return false;
    }
    QObject::connect(vdec, SIGNAL(error(QtAV::AVError)), player, SIGNAL(error(QtAV::AVError)));
    if (!vthread) {
        vthread = new VideoThread(player);
        vthread->setClock(clock);
        vthread->setStatistics(&statistics);
        vthread->setVideoCapture(vcapture);
        vthread->setOutputSet(vos);
        read_thread->setVideoThread(vthread);

        QList<Filter*> filters = FilterManager::instance().videoFilters(player);
        if (filters.size() > 0) {
            foreach (Filter *filter, filters) {
                vthread->installFilter(filter);
            }
        }
        QObject::connect(vthread, SIGNAL(finished()), player, SLOT(tryClearVideoRenderers()), Qt::DirectConnection);
    }

    // we set the thre state before the thread start
    // as it maybe clear after by AVDemuxThread starting
    vthread->resetState();
    vthread->setDecoder(vdec);

    vthread->setBrightness(brightness);
    vthread->setContrast(contrast);
    vthread->setSaturation(saturation);
    updateBufferValue(vthread->packetQueue());
    initVideoStatistics(demuxer.videoStream());

    return true;
}

// TODO: set to a lower value when buffering
void AVPlayer::Private::updateBufferValue(PacketBuffer* buf)
{
    const bool video = vthread && buf == vthread->packetQueue();
    const qreal fps = qMax<qreal>(24.0, statistics.video.frame_rate);
    qint64 bv = 0.5*fps;
    if (!video) {
        // if has video, then audio buffer should not block the video buffer (bufferValue == 1, modified in AVDemuxThread)
        bv = statistics.audio.frame_rate > 0 && statistics.audio.frame_rate < 60 ?
                        statistics.audio.frame_rate : 3LL;
    }
    if (buffer_mode == BufferTime)
        bv = 600LL; //ms
    else if (buffer_mode == BufferBytes)
        bv = 1024LL;
    // no block for music with cover
    if (video) {
        if (demuxer.hasAttacedPicture() || (statistics.video.frames > 0 && statistics.video.frames < bv))
            bv = qMax<qint64>(1LL, statistics.video.frames);
    }
    buf->setBufferMode(buffer_mode);
    buf->setBufferValue(buffer_value < 0LL ? bv : buffer_value);
}

void AVPlayer::Private::updateBufferValue()
{
    if (athread)
        updateBufferValue(athread->packetQueue());
    if (vthread)
        updateBufferValue(vthread->packetQueue());
}

} //namespace QtAV
