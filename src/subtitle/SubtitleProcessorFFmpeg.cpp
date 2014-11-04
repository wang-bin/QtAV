/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2014 Wang Bin <wbsecg1@gmail.com>

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

#include <string>
#include "QtAV/private/SubtitleProcessor.h"
#include "QtAV/private/prepost.h"
#include "QtAV/AVDemuxer.h"
#include "QtAV/Packet.h"
#include "QtAV/private/AVCompat.h"
#include "PlainText.h"
#include "utils/Logger.h"

namespace QtAV {

class SubtitleProcessorFFmpeg : public SubtitleProcessor
{
public:
    SubtitleProcessorFFmpeg();
    //virtual ~SubtitleProcessorFFmpeg() {}
    virtual SubtitleProcessorId id() const;
    virtual QString name() const;
    virtual QStringList supportedTypes() const;
    virtual bool process(QIODevice* dev);
    // supportsFromFile must be true
    virtual bool process(const QString& path);
    virtual QList<SubtitleFrame> frames() const;
    virtual SubtitleFrame processLine(const QByteArray& data, qreal pts = -1, qreal duration = 0);
    virtual QString getText(qreal pts) const;
private:
    bool processSubtitle();
    AVCodecContext *codec_ctx;
    AVDemuxer m_reader;
    QList<SubtitleFrame> m_frames;
};

static const SubtitleProcessorId SubtitleProcessorId_FFmpeg = "qtav.subtitle.processor.ffmpeg";
namespace {
static const std::string kName("FFmpeg");
}
FACTORY_REGISTER_ID_AUTO(SubtitleProcessor, FFmpeg, kName)

void RegisterSubtitleProcessorFFmpeg_Man()
{
    FACTORY_REGISTER_ID_MAN(SubtitleProcessor, FFmpeg, kName)
}

SubtitleProcessorFFmpeg::SubtitleProcessorFFmpeg()
    : codec_ctx(0)
{
}

SubtitleProcessorId SubtitleProcessorFFmpeg::id() const
{
    return SubtitleProcessorId_FFmpeg;
}

QString SubtitleProcessorFFmpeg::name() const
{
    return QString(kName.c_str());//SubtitleProcessorFactory::name(id());
}

QStringList ffmpeg_supported_sub_extensions_by_codec()
{
    QStringList exts;
    AVCodec *c = av_codec_next(NULL);
    while (c) {
        if (c->type != AVMEDIA_TYPE_SUBTITLE) {
            c = av_codec_next(c);
            continue;
        }
        qDebug("sub codec: %s", c->name);
        AVInputFormat *i = av_iformat_next(NULL);
        while (i) {
            if (!strcmp(i->name, c->name)) {
                qDebug("found iformat");
                if (i->extensions) {
                    exts.append(QString(i->extensions).split(QChar(',')));
                } else {
                    qDebug("has no exts");
                    exts.append(i->name);
                }
                break;
            }
            i = av_iformat_next(i);
        }
        if (!i) {
            //qDebug("codec name '%s' is not found in AVInputFormat, just append codec name", c->name);
            //exts.append(c->name);
        }
        c = av_codec_next(c);
    }
    return exts;
}

QStringList ffmpeg_supported_sub_extensions()
{
    QStringList exts;
    AVInputFormat *i = av_iformat_next(NULL);
    while (i) {
        // strstr parameters can not be null
        if (i->long_name && strstr(i->long_name, "subtitle")) {
            if (i->extensions) {
                exts.append(QString(i->extensions).split(QChar(',')));
            } else {
                exts.append(i->name);
            }
        }
        i = av_iformat_next(i);
    }
    return exts;
}

QStringList SubtitleProcessorFFmpeg::supportedTypes() const
{
    // TODO: mp4. check avformat classes
#if 0
    typedef struct {
        const char* ext;
        const char* name;
    } sub_ext_t;
    static const sub_ext_t sub_ext[] = {
        { "ass", "ass" },
        { "ssa", "ass" },
        { "sub", "subviewer" },
        { ""
    }
    // from ffmpeg/tests/fate/subtitles.mak
    static const QStringList sSuffixes = QStringList() << "ass" << "ssa" << "sub" << "srt" << "txt" << "vtt" << "smi" << "pjs" << "jss" << "aqt";
#endif
    static const QStringList sSuffixes = ffmpeg_supported_sub_extensions();
    return sSuffixes;
}

bool SubtitleProcessorFFmpeg::process(QIODevice *dev)
{
    if (!dev->isOpen()) {
        if (!dev->open(QIODevice::ReadOnly)) {
            qWarning() << "open qiodevice error: " << dev->errorString();
            return false;
        }
    }
    if (!m_reader.load(dev))
            goto error;
    if (m_reader.subtitleStreams().isEmpty())
        goto error;
    qDebug("subtitle format: %s", m_reader.formatContext()->iformat->name);
    if (!processSubtitle())
        goto error;
    m_reader.close();
    return true;
error:
    m_reader.close();
    return false;
}

bool SubtitleProcessorFFmpeg::process(const QString &path)
{
    if (!m_reader.loadFile(path))
        goto error;
    if (m_reader.subtitleStreams().isEmpty())
        goto error;
    qDebug("subtitle format: %s", m_reader.formatContext()->iformat->name);
    if (!processSubtitle())
        goto error;
    m_reader.close();
    return true;
error:
    m_reader.close();
    return false;
}

QList<SubtitleFrame> SubtitleProcessorFFmpeg::frames() const
{
    return m_frames;
}

QString SubtitleProcessorFFmpeg::getText(qreal pts) const
{
    QString text;
    for (int i = 0; i < m_frames.size(); ++i) {
        if (m_frames[i].begin <= pts && m_frames[i].end >= pts) {
            text += m_frames[i].text + "\n";
            continue;
        }
        if (!text.isEmpty())
            break;
    }
    return text.trimmed();
}

SubtitleFrame SubtitleProcessorFFmpeg::processLine(const QByteArray &data, qreal pts, qreal duration)
{
    // AV_CODEC_ID_xxx and srt, subrip are available for ffmpeg >= 1.0. AV_CODEC_ID_xxx
    // TODO: what about other formats?
    // libav-9: packet data from demuxer contains time and but duration is 0, must decode
    if (duration > 0 && (!codec_ctx
#if QTAV_USE_FFMPEG(LIBAVCODEC)
            || codec_ctx->codec_id == AV_CODEC_ID_SUBRIP
#endif
            || codec_ctx->codec_id == AV_CODEC_ID_SRT
            )) {
        SubtitleFrame f;
        f.begin = pts;
        f.end = pts + duration;
        if (data.startsWith("Dialogue:")) // e.g. decoding embedded subtitles
            f.text = PlainText::fromAss(data.constData());
        else
            f.text = QString::fromUtf8(data.constData(), data.size()); //utf-8 is required
        return f;
    }
    AVPacket packet;
    av_init_packet(&packet);
    packet.size = data.size();
    packet.data = (uint8_t*)data.constData();
    packet.pts = pts * 1000.0;
    packet.duration = duration * 1000.0;
    AVSubtitle sub;
    memset(&sub, 0, sizeof(sub));
    int got_subtitle = 0;
    int ret = avcodec_decode_subtitle2(codec_ctx, &sub, &got_subtitle, &packet);
    if (ret < 0 || !got_subtitle) {
        av_free_packet(&packet);
        avsubtitle_free(&sub);
        return SubtitleFrame();
    }
    SubtitleFrame frame;
    frame.begin = pts + qreal(sub.start_display_time)/1000.0;
    frame.end = pts + qreal(sub.end_display_time)/1000.0;
   // qDebug() << QTime(0, 0, 0).addMSecs(frame.begin*1000.0) << "-" << QTime(0, 0, 0).addMSecs(frame.end*1000.0) << " fmt: " << sub.format << " pts: " << m_reader.packet()->pts //sub.pts
    //            << " rects: " << sub.num_rects;
    for (unsigned i = 0; i < sub.num_rects; i++) {
        switch (sub.rects[i]->type) {
        case SUBTITLE_ASS:
            //qDebug("frame: %s", sub.rects[i]->ass);
            frame.text.append(PlainText::fromAss(sub.rects[i]->ass)).append('\n');
            break;
        case SUBTITLE_TEXT:
            //qDebug("frame: %s", sub.rects[i]->text);
            frame.text.append(sub.rects[i]->text).append('\n');
            break;
        case SUBTITLE_BITMAP:
            break;
        default:
            break;
        }
    }
    av_free_packet(&packet);
    avsubtitle_free(&sub);
    return frame;
}

bool SubtitleProcessorFFmpeg::processSubtitle()
{
    m_frames.clear();
    int ss = m_reader.subtitleStream();
    if (ss < 0) {
        qWarning("no subtitle stream found");
        return false;
    }
    codec_ctx = m_reader.subtitleCodecContext();
    AVCodec *dec = avcodec_find_decoder(codec_ctx->codec_id);
    const AVCodecDescriptor *dec_desc = avcodec_descriptor_get(codec_ctx->codec_id);
    if (!dec) {
        if (dec_desc)
            qWarning("Failed to find subtitle codec %s", dec_desc->name);
        else
            qWarning("Failed to find subtitle codec %d", codec_ctx->codec_id);
        return false;
    }
    qDebug("found subtitle decoder '%s'", dec_desc->name);
    // AV_CODEC_PROP_TEXT_SUB: ffmpeg >= 2.0
#ifdef AV_CODEC_PROP_TEXT_SUB
    if (dec_desc && !(dec_desc->props & AV_CODEC_PROP_TEXT_SUB)) {
        qWarning("Only text based subtitles are currently supported");
        return false;
    }
#endif
    AVDictionary *codec_opts = NULL;
    int ret = avcodec_open2(codec_ctx, dec, &codec_opts);
    if (ret < 0) {
        qWarning("open subtitle codec error: %s", av_err2str(ret));
        av_dict_free(&codec_opts);
        return false;
    }
    while (!m_reader.atEnd()) {
        if (!m_reader.readFrame()) {
            avcodec_close(codec_ctx);
            codec_ctx = 0;
            return false;
        }
        if (!m_reader.packet()->isValid())
            continue;
        if (m_reader.stream() != ss)
            continue;
        const Packet *pkt = m_reader.packet();
        SubtitleFrame frame = processLine(pkt->data, pkt->pts, pkt->duration);
        if (frame.isValid())
            m_frames.append(frame);
    }
    avcodec_close(codec_ctx);
    codec_ctx = 0;
    return true;
}

} //namespace QtAV
