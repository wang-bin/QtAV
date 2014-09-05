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
#include <QtDebug>
#include <QtCore/QTime>
#include <QtGui/QPainter>
#include "QtAV/private/SubtitleProcessor.h"
#include "QtAV/prepost.h"
#include "QtAV/AVDemuxer.h"
#include "QtAV/Packet.h"
#include "QtAV/private/AVCompat.h"
#include "PlainText.h"

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
    virtual QString getText(qreal pts) const;
    virtual QImage getImage(qreal pts, int width, int height);
private:
    bool processSubtitle();
    AVDemuxer m_reader;
    QList<SubtitleFrame> m_frames;
    QFont m_font;
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
{
    m_font.setBold(true);
    m_font.setPixelSize(24);
}

SubtitleProcessorId SubtitleProcessorFFmpeg::id() const
{
    return SubtitleProcessorId_FFmpeg;
}

QString SubtitleProcessorFFmpeg::name() const
{
    return QString(kName.c_str());//SubtitleProcessorFactory::name(id());
}

QStringList SubtitleProcessorFFmpeg::supportedTypes() const
{
    // from ffmpeg/tests/fate/subtitles.mak
    // TODO: mp4
    static QStringList sSuffixes = QStringList() << "ass" << "ssa" << "sub" << "srt" << "txt" << "vtt" << "smi" << "pjs" << "jss" << "aqt";
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
    for (int i = 0; i < m_frames.size(); ++i) {
        if (m_frames[i].begin <= pts && m_frames[i].end >= pts)
            return m_frames[i].text;
    }
    return QString();
}

QImage SubtitleProcessorFFmpeg::getImage(qreal pts, int width, int height)
{
    QString text = getText(pts);
    if (text.isEmpty())
        return QImage();
    QImage img(width, height, QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.setPen(QColor(Qt::white));
    p.setFont(m_font);
    const int flags = Qt::AlignHCenter | Qt::AlignBottom | Qt::TextWordWrap;
    //QRect box = fm.boundingRect(0, 0, width, height, flags, text);
    p.drawText(0, 0, width, height, flags, text);
    return img;
}

bool SubtitleProcessorFFmpeg::processSubtitle()
{
    m_frames.clear();
    int ss = m_reader.subtitleStream();
    if (ss < 0) {
        qWarning("no subtitle stream found");
        return false;
    }
    AVCodecContext *ctx = m_reader.subtitleCodecContext();
    AVCodec *dec = avcodec_find_decoder(ctx->codec_id);
    const AVCodecDescriptor *dec_desc = avcodec_descriptor_get(ctx->codec_id);
    if (!dec) {
        if (dec_desc)
            qWarning("Failed to find subtitle codec %s", dec_desc->name);
        else
            qWarning("Failed to find subtitle codec %d", ctx->codec_id);
        return false;
    }
    qDebug("found subtitle decoder '%s'", dec_desc->name);
    // AV_CODEC_PROP_TEXT_SUB: ffmpeg >= 2.0
#if FFMPEG_MODULE_CHECK(AVCODEC, 55, 18, 100)
    if (dec_desc && !(dec_desc->props & AV_CODEC_PROP_TEXT_SUB)) {
        qWarning("Only text based subtitles are currently supported");
        return false;
    }
#endif
    AVDictionary *codec_opts = NULL;
    int ret = avcodec_open2(ctx, dec, &codec_opts);
    if (ret < 0) {
        qWarning("open subtitle codec error: %s", av_err2str(ret));
        av_dict_free(&codec_opts);
        return false;
    }
    while (!m_reader.atEnd()) {
        if (!m_reader.readFrame()) {
            avcodec_close(ctx);
            return false;
        }
        if (!m_reader.packet()->isValid())
            continue;
        if (m_reader.stream() != ss)
            continue;
        AVPacket packet;
        av_init_packet(&packet);
        const Packet *pkt = m_reader.packet();
        packet.size = pkt->data.size();
        packet.data = (uint8_t*)pkt->data.constData();
        packet.pts = pkt->pts * 1000.0;
        packet.duration = pkt->duration * 1000.0;
        int got_subtitle = 0;
        AVSubtitle sub;
        memset(&sub, 0, sizeof(sub));
        // TODO: AVPacket.data for srt contains plain text. but decoded text is ass format. skip decoding? what about other format?
        ret = avcodec_decode_subtitle2(ctx, &sub, &got_subtitle, &packet);
        if (ret < 0 || !got_subtitle) {
            av_free_packet(&packet);
            avsubtitle_free(&sub);
            continue;
        }
        qreal pts = m_reader.packet()->pts;
        SubtitleFrame frame;
        frame.begin = pts + qreal(sub.start_display_time)/1000.0;
        frame.end = pts + qreal(sub.end_display_time)/1000.0;
       // qDebug() << QTime(0, 0, 0).addMSecs(frame.begin*1000.0) << "-" << QTime(0, 0, 0).addMSecs(frame.end*1000.0) << " fmt: " << sub.format << " pts: " << m_reader.packet()->pts //sub.pts
        //            << " rects: " << sub.num_rects;
        for (unsigned i = 0; i < sub.num_rects; i++) {
            switch (sub.rects[i]->type) {
            case SUBTITLE_ASS:
                frame.text.append(PlainText::fromAss(sub.rects[i]->ass)).append('\n');
                break;
            case SUBTITLE_TEXT:
                frame.text.append(sub.rects[i]->text).append('\n');
                break;
            case SUBTITLE_BITMAP:
                break;
            default:
                break;
            }
        }
        m_frames.append(frame);
        av_free_packet(&packet);
        avsubtitle_free(&sub);
    }
    avcodec_close(ctx);
    return true;
}

} //namespace QtAV
