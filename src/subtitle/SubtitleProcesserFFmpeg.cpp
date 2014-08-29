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
#include "QtAV/private/SubtitleProcesser.h"
#include "QtAV/prepost.h"
#include "QtAV/AVDemuxer.h"
#include "QtAV/Packet.h"
#include "QtAV/private/AVCompat.h"

namespace QtAV {

class SubtitleProcesserFFmpeg : public SubtitleProcesser
{
public:
    SubtitleProcesserFFmpeg();
    //virtual ~SubtitleProcesserFFmpeg() {}
    virtual SubtitleProcesserId id() const;
    virtual QString name() const;
    bool isSupported(SourceType value) const;
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

static const SubtitleProcesserId SubtitleProcesserId_FFmpeg = "qtav.subtitle.processor.ffmpeg";
namespace {
static const std::string kName("FFmpeg");
}
FACTORY_REGISTER_ID_AUTO(SubtitleProcesser, FFmpeg, kName)

void RegisterSubtitleProcesserFFmpeg_Man()
{
    FACTORY_REGISTER_ID_MAN(SubtitleProcesser, FFmpeg, kName)
}

SubtitleProcesserFFmpeg::SubtitleProcesserFFmpeg()
{
    m_font.setBold(true);
    m_font.setPixelSize(24);
}

SubtitleProcesserId SubtitleProcesserFFmpeg::id() const
{
    return SubtitleProcesserId_FFmpeg;
}

QString SubtitleProcesserFFmpeg::name() const
{
    return QString(kName.c_str());//SubtitleProcesserFactory::name(id());
}

bool SubtitleProcesserFFmpeg::isSupported(SourceType value) const
{
    Q_UNUSED(value);
    return true;
}

bool SubtitleProcesserFFmpeg::process(QIODevice *dev)
{
    if (!dev->isOpen()) {
        if (!dev->open(QIODevice::ReadOnly)) {
            qWarning() << "open qiodevice error: " << dev->errorString();
            return false;
        }
    }
    if (!m_reader.load(dev))
        return false;
    if (m_reader.subtitleStreams().isEmpty())
        return false;
    if (!processSubtitle())
        return false;
    return true;
}

bool SubtitleProcesserFFmpeg::process(const QString &path)
{
    if (!m_reader.loadFile(path))
        return false;
    if (m_reader.subtitleStreams().isEmpty())
        return false;
    if (!processSubtitle())
        return false;
    return true;
}

QList<SubtitleFrame> SubtitleProcesserFFmpeg::frames() const
{
    return m_frames;
}

QString SubtitleProcesserFFmpeg::getText(qreal pts) const
{
    for (int i = 0; i < m_frames.size(); ++i) {
        if (m_frames[i].begin <= pts && m_frames[i].end >= pts)
            return m_frames[i].text;
    }
    return QString();
}

QImage SubtitleProcesserFFmpeg::getImage(qreal pts, int width, int height)
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

bool SubtitleProcesserFFmpeg::processSubtitle()
{
    int ss = m_reader.subtitleStream();
    if (ss < 0) {
        qWarning("no subtitle stream found");
        return false;
    }
    AVCodecContext *ctx = m_reader.subtitleCodecContext();
    AVCodec *dec = avcodec_find_decoder(ctx->codec_id);
    if (!dec) {
        qWarning("Failed to find subtitle codec %s", avcodec_get_name(ctx->codec_id));
        return false;
    }
    const AVCodecDescriptor *dec_desc = avcodec_descriptor_get(ctx->codec_id);
    if (dec_desc && !(dec_desc->props & AV_CODEC_PROP_TEXT_SUB)) {
        qWarning("Only text based subtitles are currently supported");
        return false;
    }
    AVDictionary *codec_opts = NULL;
    int ret = avcodec_open2(ctx, dec, &codec_opts);
    if (ret < 0) {
        qWarning("open subtitle codec error: %s", av_err2str(ret));
        av_dict_free(&codec_opts);
        return false;
    }
    while (!m_reader.atEnd()) {
        if (!m_reader.readFrame())
            continue;
        if (!m_reader.packet()->isValid())
            continue;
        if (m_reader.stream() != ss)
            continue;
        AVPacket packet;
        av_init_packet(&packet);
        packet.size = m_reader.packet()->data.size();
        packet.data = (uint8_t*)m_reader.packet()->data.constData();
        int got_subtitle = 0;
        AVSubtitle sub;
        memset(&sub, 0, sizeof(sub));
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
                frame.text.append(sub.rects[i]->ass).append('\n');
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
