/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2018 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2015)

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

#include "QtAV/AVMuxer.h"
#include "QtAV/private/AVCompat.h"
#include "QtAV/MediaIO.h"
#include "QtAV/VideoEncoder.h"
#include "QtAV/AudioEncoder.h"
#include "utils/internal.h"
#include "utils/Logger.h"

namespace QtAV {
static const char kFileScheme[] = "file:";
#define CHAR_COUNT(s) (sizeof(s) - 1) // tail '\0'

// Packet::asAVPacket() assumes time base is 0.001
static const AVRational kTB = {1, 1000};

class AVMuxer::Private
{
public:
    Private()
        : seekable(false)
        , network(false)
        , started(false)
        , eof(false)
        , media_changed(true)
        , open(false)
        , format_ctx(0)
        , format(0)
        , io(0)
        , dict(0)
        , aenc(0)
        , venc(0)
    {
#if !AVFORMAT_STATIC_REGISTER
        av_register_all();
#endif
    }
    ~Private() {
        //delete interrupt_hanlder;
        if (dict) {
            av_dict_free(&dict);
            dict = 0;
        }
        if (io) {
            delete io;
            io = 0;
        }
    }
    AVStream* addStream(AVFormatContext* ctx, const QString& codecName, AVCodecID codecId);
    bool prepareStreams();
    void applyOptionsForDict();
    void applyOptionsForContext();

    bool seekable;
    bool network;
    bool started;
    bool eof;
    bool media_changed;
    bool open;
    AVFormatContext *format_ctx;
    //copy the info, not parse the file when constructed, then need member vars
    QString file;
    QString file_orig;
    AVOutputFormat *format;
    QString format_forced;
    MediaIO *io;

    AVDictionary *dict;
    QVariantHash options;
    QList<int> audio_streams, video_streams, subtitle_streams;
    AudioEncoder *aenc; // not owner
    VideoEncoder *venc; // not owner
};

AVStream *AVMuxer::Private::addStream(AVFormatContext* ctx, const QString &codecName, AVCodecID codecId)
{
    AVCodec *codec = NULL;
    if (!codecName.isEmpty()) {
        codec = avcodec_find_encoder_by_name(codecName.toUtf8().constData());
        if (!codec) {
            const AVCodecDescriptor* cd = avcodec_descriptor_get_by_name(codecName.toUtf8().constData());
            if (cd) {
                codec = avcodec_find_encoder(cd->id);
            }
        }
        if (!codec)
            qWarning("Can not find encoder for %s", codecName.toUtf8().constData());
    } else if (codecId != QTAV_CODEC_ID(NONE)) {
        codec = avcodec_find_encoder(codecId);
        if (!codec)
            qWarning("Can not find encoder for %s", avcodec_get_name(codecId));
    }
    if (!codec)
        return 0;
    AVStream *s = avformat_new_stream(ctx, codec);
    if (!s) {
        qWarning("Can not allocate stream");
        return 0;
    }
    // set by avformat if unset
    s->id = ctx->nb_streams - 1;
    s->time_base = kTB;
    AVCodecContext *c = s->codec;
    c->codec_id = codec->id;
    // Using codec->time_base is deprecated, but needed for older lavf.
    c->time_base = s->time_base;
    /* Some formats want stream headers to be separate. */
    if (ctx->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    // expose avctx to encoder and set properties in encoder?
    // list codecs for a given format in ui
    return s;
}

bool AVMuxer::Private::prepareStreams()
{
    audio_streams.clear();
    video_streams.clear();
    subtitle_streams.clear();
    AVOutputFormat* fmt = format_ctx->oformat;
    if (venc) {
        AVStream *s = addStream(format_ctx, venc->codecName(), fmt->video_codec);
        if (s) {
            AVCodecContext *c = s->codec;
            c->bit_rate = venc->bitRate();
            c->width = venc->width();
            c->height = venc->height();
            /// MUST set after encoder is open to ensure format is valid and the same
            c->pix_fmt = (AVPixelFormat)VideoFormat::pixelFormatToFFmpeg(venc->pixelFormat());

            // Set avg_frame_rate based on encoder frame_rate
            s->avg_frame_rate = av_d2q(venc->frameRate(), venc->frameRate()*1001.0+2);

            video_streams.push_back(s->id);
        }
    }
    if (aenc) {
        AVStream *s = addStream(format_ctx, aenc->codecName(), fmt->audio_codec);
        if (s) {
            AVCodecContext *c = s->codec;
            c->bit_rate = aenc->bitRate();
            /// MUST set after encoder is open to ensure format is valid and the same
            c->sample_rate = aenc->audioFormat().sampleRate();
            c->sample_fmt = (AVSampleFormat)aenc->audioFormat().sampleFormatFFmpeg();
            c->channel_layout = aenc->audioFormat().channelLayoutFFmpeg();
            c->channels = aenc->audioFormat().channels();
            c->bits_per_raw_sample = aenc->audioFormat().bytesPerSample()*8; // need??

            AVCodecContext *avctx = (AVCodecContext *) aenc->codecContext();
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(56,5,100)
            c->initial_padding = avctx->initial_padding;
#else
            c->delay = avctx->delay;
#endif
            if (avctx->extradata_size) {
                c->extradata = avctx->extradata;
                c->extradata_size = avctx->extradata_size;
            }

            audio_streams.push_back(s->id);
        }
    }
    return !(audio_streams.isEmpty() && video_streams.isEmpty() && subtitle_streams.isEmpty());
}

static void getFFmpegOutputFormats(QStringList* formats, QStringList* extensions)
{
    static QStringList exts;
    static QStringList fmts;
    if (exts.isEmpty() && fmts.isEmpty()) {
        QStringList e, f;
#if AVFORMAT_STATIC_REGISTER
        const AVOutputFormat *o = NULL;
        void* it = NULL;
        while ((o = av_muxer_iterate(&it))) {
#else
        av_register_all(); // MUST register all input/output formats
        AVOutputFormat *o = NULL;
        while ((o = av_oformat_next(o))) {
#endif
            if (o->extensions)
                e << QString::fromLatin1(o->extensions).split(QLatin1Char(','), QString::SkipEmptyParts);
            if (o->name)
                f << QString::fromLatin1(o->name).split(QLatin1Char(','), QString::SkipEmptyParts);
        }
        foreach (const QString& v, e) {
            exts.append(v.trimmed());
        }
        foreach (const QString& v, f) {
            fmts.append(v.trimmed());
        }
        exts.removeDuplicates();
        fmts.removeDuplicates();
    }
    if (formats)
        *formats = fmts;
    if (extensions)
        *extensions = exts;
}

const QStringList& AVMuxer::supportedFormats()
{
    static QStringList fmts;
    if (fmts.isEmpty())
        getFFmpegOutputFormats(&fmts, NULL);
    return fmts;
}

const QStringList& AVMuxer::supportedExtensions()
{
    static QStringList exts;
    if (exts.isEmpty())
        getFFmpegOutputFormats(NULL, &exts);
    return exts;
}

// TODO: move to QtAV::supportedFormats(bool out). custom protols?
const QStringList &AVMuxer::supportedProtocols()
{
    static bool called = false;
    static QStringList protocols;
    if (called)
        return protocols;
    called = true;
    if (!protocols.isEmpty())
        return protocols;
#if QTAV_HAVE(AVDEVICE)
    protocols << QStringLiteral("avdevice");
#endif
#if !AVFORMAT_STATIC_REGISTER
    av_register_all(); // MUST register all input/output formats
#endif
    void* opq = 0;
    const char* protocol = avio_enum_protocols(&opq, 1);
    while (protocol) {
        // static string, no deep copy needed. but QByteArray::fromRawData(data,size) assumes data is not null terminated and we must give a size
        protocols.append(QString::fromUtf8(protocol));
        protocol = avio_enum_protocols(&opq, 1);
    }
    return protocols;
}

AVMuxer::AVMuxer(QObject *parent)
    : QObject(parent)
    , d(new Private())
{
}

AVMuxer::~AVMuxer()
{
    close();
}

QString AVMuxer::fileName() const
{
    return d->file_orig;
}

QIODevice* AVMuxer::ioDevice() const
{
    if (!d->io)
        return 0;
    if (d->io->name() != QLatin1String("QIODevice"))
        return 0;
    return d->io->property("device").value<QIODevice*>();
}

MediaIO* AVMuxer::mediaIO() const
{
    return d->io;
}

bool AVMuxer::setMedia(const QString &fileName)
{
    if (d->io) {
        delete d->io;
        d->io = 0;
    }
    d->file_orig = fileName;
    const QString url_old(d->file);
    d->file = fileName.trimmed();
    if (d->file.startsWith(QLatin1String("mms:")))
        d->file.insert(3, QLatin1Char('h'));
    else if (d->file.startsWith(QLatin1String(kFileScheme)))
        d->file = Internal::Path::toLocal(d->file);
    int colon = d->file.indexOf(QLatin1Char(':'));
    if (colon == 1) {
#ifdef Q_OS_WINRT
        d->file.prepend(QStringLiteral("qfile:"));
#endif
    }
    d->media_changed = url_old != d->file;
    if (d->media_changed) {
        d->format_forced.clear();
    }
    // a local file. return here to avoid protocol checking. If path contains ":", protocol checking will fail
    if (d->file.startsWith(QLatin1Char('/')))
        return d->media_changed;
    // use MediaIO to support protocols not supported by ffmpeg
    colon = d->file.indexOf(QLatin1Char(':'));
    if (colon >= 0) {
#ifdef Q_OS_WIN
        if (colon == 1 && d->file.at(0).isLetter())
            return d->media_changed;
#endif
        const QString scheme = colon == 0 ? QStringLiteral("qrc") : d->file.left(colon);
        // supportedProtocols() is not complete. so try MediaIO 1st, if not found, fallback to libavformat
        d->io = MediaIO::createForProtocol(scheme);
        if (d->io) {
            d->io->setUrl(d->file);
        }
    }
    return d->media_changed;
}

bool AVMuxer::setMedia(QIODevice* device)
{
    d->file = QString();
    d->file_orig = QString();
    if (d->io) {
        if (d->io->name() != QLatin1String("QIODevice")) {
            delete d->io;
            d->io = 0;
        }
    }
    if (!d->io)
        d->io = MediaIO::create("QIODevice");
    QIODevice* old_dev = d->io->property("device").value<QIODevice*>();
    d->media_changed = old_dev != device;
    if (d->media_changed) {
        d->format_forced.clear();
    }
    d->io->setProperty("device", QVariant::fromValue(device)); //open outside?

    if (device->isWritable()) {
        d->io->setAccessMode(MediaIO::Write);
    }

    return d->media_changed;
}

bool AVMuxer::setMedia(MediaIO *in)
{
    d->media_changed = in != d->io;
    if (d->media_changed) {
        d->format_forced.clear();
    }
    d->file = QString();
    d->file_orig = QString();
    if (!d->io)
        d->io = in;
    if (d->io != in) {
        delete d->io;
        d->io = in;
    }
    return d->media_changed;
}

void AVMuxer::setFormat(const QString &fmt)
{
    d->format_forced = fmt;
}

QString AVMuxer::formatForced() const
{
    return d->format_forced;
}

bool AVMuxer::open()
{
    // avformatcontext will be allocated in avformat_alloc_output_context2()
    //d->format_ctx->interrupt_callback = *d->interrupt_hanlder;

    d->applyOptionsForDict();
    // check special dict keys
    // d->format_forced can be set from AVFormatContext.format_whitelist
    if (!d->format_forced.isEmpty()) {
        d->format = av_guess_format(d->format_forced.toUtf8().constData(), NULL, NULL);
        qDebug() << "force format: " << d->format_forced;
    }

    //d->interrupt_hanlder->begin(InterruptHandler::Open);
    if (d->io) {
        if (d->io->accessMode() == MediaIO::Read) {
            qWarning("wrong MediaIO accessMode. MUST be Write");
        }
        AV_ENSURE_OK(avformat_alloc_output_context2(&d->format_ctx, d->format, d->format_forced.isEmpty() ? 0 : d->format_forced.toUtf8().constData(), ""), false);
        d->format_ctx->pb = (AVIOContext*)d->io->avioContext();
        d->format_ctx->flags |= AVFMT_FLAG_CUSTOM_IO;
        //d->format_ctx->flags |= AVFMT_FLAG_GENPTS;
    } else {
        AV_ENSURE_OK(avformat_alloc_output_context2(&d->format_ctx, d->format, d->format_forced.isEmpty() ? 0 : d->format_forced.toUtf8().constData(), fileName().toUtf8().constData()), false);
    }
    //d->interrupt_hanlder->end();

    if (!d->prepareStreams()) {
        return false;
    }
    // TODO: AVFMT_NOFILE ? examples/muxing.c only check AVFMT_NOFILE
    // a custome io does not need avio_open. it open resource in it's own way, e.g. QIODevice.open
    if (!(d->format_ctx->oformat->flags & AVFMT_NOFILE) && !(d->format_ctx->flags & AVFMT_FLAG_CUSTOM_IO)) {
        // avio_open2?
        AV_ENSURE_OK(avio_open(&d->format_ctx->pb, fileName().toUtf8().constData(), AVIO_FLAG_WRITE), false);
    }
    // d->format_ctx->start_time_realtime
    AV_ENSURE_OK(avformat_write_header(d->format_ctx, &d->dict), false);
    d->started = false;
    d->open = true;

    return true;
}

bool AVMuxer::close()
{
    if (!isOpen())
        return true;
    d->open = false;
    av_write_trailer(d->format_ctx);
    // close AVCodecContext* in encoder
    // custom io will call avio_close in ~MediaIO()
    if (!(d->format_ctx->oformat->flags & AVFMT_NOFILE) && !(d->format_ctx->flags & AVFMT_FLAG_CUSTOM_IO)) {
        if (d->format_ctx->pb) {
            avio_flush(d->format_ctx->pb);
            avio_close(d->format_ctx->pb);
            d->format_ctx->pb = 0;
        }
    }
    avformat_free_context(d->format_ctx);
    d->format_ctx = 0;
    d->audio_streams.clear();
    d->video_streams.clear();
    d->subtitle_streams.clear();
    d->started = false;
    return true;
}

bool AVMuxer::isOpen() const
{
    return d->open;
}

bool AVMuxer::writeAudio(const QtAV::Packet& packet)
{
    AVPacket *pkt = (AVPacket*)packet.asAVPacket(); //FIXME
    pkt->stream_index = d->audio_streams[0]; //FIXME
    AVStream *s = d->format_ctx->streams[pkt->stream_index];
    // stream.time_base is set in avformat_write_header
    av_packet_rescale_ts(pkt, kTB, s->time_base);
    av_interleaved_write_frame(d->format_ctx, pkt);

    d->started = true;
    return true;
}

bool AVMuxer::writeVideo(const QtAV::Packet& packet)
{
    AVPacket *pkt = (AVPacket*)packet.asAVPacket();
    pkt->stream_index = d->video_streams[0];
    AVStream *s = d->format_ctx->streams[pkt->stream_index];
    // stream.time_base is set in avformat_write_header
    av_packet_rescale_ts(pkt, kTB, s->time_base);
    //av_write_frame
    av_interleaved_write_frame(d->format_ctx, pkt);
#if 0
    qDebug("mux packet.pts: %.3f dts:%.3f duration: %.3f, avpkt.pts: %lld,dts:%lld,duration:%lld"
           , packet.pts, packet.dts, packet.duration
           , pkt->pts, pkt->dts, pkt->duration);
    qDebug("stream: %d duration: %lld, end: %lld. tb:{%d/%d}"
           , pkt->stream_index, s->duration
            , av_stream_get_end_pts(s)
           , s->time_base.num, s->time_base.den
            );
#endif
    d->started = true;
    return true;
}

void AVMuxer::copyProperties(VideoEncoder *enc)
{
    d->venc = enc;
}

void AVMuxer::copyProperties(AudioEncoder *enc)
{
    d->aenc = enc;
}

void AVMuxer::setOptions(const QVariantHash &dict)
{
    d->options = dict;
    d->applyOptionsForContext(); // apply even if avformat context is open
}

QVariantHash AVMuxer::options() const
{
    return d->options;
}

void AVMuxer::Private::applyOptionsForDict()
{
    if (dict) {
        av_dict_free(&dict);
        dict = 0; //aready 0 in av_free
    }
    if (options.isEmpty())
        return;
    QVariant opt(options);
    if (options.contains(QStringLiteral("avformat")))
        opt = options.value(QStringLiteral("avformat"));
    Internal::setOptionsToDict(opt, &dict);

    if (opt.type() == QVariant::Map) {
        QVariantMap avformat_dict(opt.toMap());
        if (avformat_dict.contains(QStringLiteral("format_whitelist"))) {
            const QString fmts(avformat_dict[QStringLiteral("format_whitelist")].toString());
            if (!fmts.contains(QLatin1Char(',')) && !fmts.isEmpty())
                format_forced = fmts; // reset when media changed
        }
    } else if (opt.type() == QVariant::Hash) {
        QVariantHash avformat_dict(opt.toHash());
        if (avformat_dict.contains(QStringLiteral("format_whitelist"))) {
            const QString fmts(avformat_dict[QStringLiteral("format_whitelist")].toString());
            if (!fmts.contains(QLatin1Char(',')) && !fmts.isEmpty())
                format_forced = fmts; // reset when media changed
        }
    }
}

void AVMuxer::Private::applyOptionsForContext()
{
    if (!format_ctx)
        return;
    if (options.isEmpty()) {
        //av_opt_set_defaults(format_ctx);  //can't set default values! result maybe unexpected
        return;
    }
    QVariant opt(options);
    if (options.contains(QStringLiteral("avformat")))
        opt = options.value(QStringLiteral("avformat"));
    Internal::setOptionsToFFmpegObj(opt, format_ctx);
}
} //namespace QtAV
