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

#include "QtAV/SubtitleFilter.h"
#include "QtAV/private/Filter_p.h"
#include "QtAV/AVPlayer.h"
#include "QtAV/Subtitle.h"
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtDebug>

namespace QtAV {

class SubtitleFilterPrivate : public VideoFilterPrivate
{
public:
    SubtitleFilterPrivate()
        : autoLoad(true)
        , player(0)
        , rect(0.0, 0.0, 1.0, 0.9)
        , color(Qt::white)
    {
        font.setPointSize(22);
    }
    QRect realRect(int width, int height) {
        if (!rect.isValid()) {
            return QRect(0, 0, width, height);
        }
        QRect r = rect.toRect();
        // nomalized x, y < 1
        bool normalized = false;
        if (qAbs(rect.x()) < 1) {
            normalized = true;
            r.setX(rect.x()*qreal(width)); //TODO: why not video_frame.size()? roi not correct
        }
        if (qAbs(rect.y()) < 1) {
            normalized = true;
            r.setY(rect.y()*qreal(height));
        }
        // whole size use width or height = 0, i.e. null size
        // nomalized width, height <= 1. If 1 is normalized value iff |x|<1 || |y| < 1
        if (qAbs(rect.width()) < 1)
            r.setWidth(rect.width()*qreal(width));
        if (qAbs(rect.height() < 1))
            r.setHeight(rect.height()*qreal(height));
        if (rect.width() == 1.0 && normalized) {
            r.setWidth(width);
        }
        if (rect.height() == 1.0 && normalized) {
            r.setHeight(height);
        }
        return r;
    }

    bool autoLoad;
    AVPlayer *player;
    QString file;
    Subtitle sub;
    QRectF rect;
    QFont font;
    QColor color;
};

SubtitleFilter::SubtitleFilter(QObject *parent) :
    VideoFilter(*new SubtitleFilterPrivate(), parent)
{
    AVPlayer *player = qobject_cast<AVPlayer*>(parent);
    if (player)
        setPlayer(player);
    connect(this, SIGNAL(enableChanged(bool)), SLOT(onEnableChanged(bool)));
    connect(this, SIGNAL(codecChanged()), &d_func().sub, SLOT(load()));
}

void SubtitleFilter::setPlayer(AVPlayer *player)
{
    DPTR_D(SubtitleFilter);
    if (d.player == player)
        return;
    if (d.player) {
        disconnect(d.player, SIGNAL(sourceChanged()), this, SLOT(onPlayerSourceChanged()));
        disconnect(d.player, SIGNAL(positionChanged(qint64)), this, SLOT(onPlayerPositionChanged()));
        disconnect(d.player, SIGNAL(started()), this, SLOT(onPlayerStart()));
    }
    d.player = player;
    connect(d.player, SIGNAL(sourceChanged()), SLOT(onPlayerSourceChanged()));
    connect(d.player, SIGNAL(positionChanged(qint64)), SLOT(onPlayerPositionChanged()));
    connect(d.player, SIGNAL(started()), SLOT(onPlayerStart()));
}

void SubtitleFilter::setFile(const QString &file)
{
    DPTR_D(SubtitleFilter);
    // always load
    d.file = file;
    d.sub.setFileName(file);
    d.sub.setFuzzyMatch(false);
    d.sub.load();
}

QString SubtitleFilter::file() const
{
    return d_func().file;
}

void SubtitleFilter::setCodec(const QByteArray &value)
{
    DPTR_D(SubtitleFilter);
    if (d.sub.codec() == value)
        return;
    d.sub.setCodec(value);
    emit codecChanged();
}

QByteArray SubtitleFilter::codec() const
{
    return d_func().sub.codec();
}

void SubtitleFilter::setAutoLoad(bool value)
{
    DPTR_D(SubtitleFilter);
    if (d.autoLoad == value)
        return;
    d.autoLoad = value;
    emit autoLoadChanged(value);
}

bool SubtitleFilter::autoLoad() const
{
    return d_func().autoLoad;
}

void SubtitleFilter::setRect(const QRectF &r)
{
    DPTR_D(SubtitleFilter);
    if (d.rect == r)
        return;
    d.rect = r;
    emit rectChanged();
}

QRectF SubtitleFilter::rect() const
{
    return d_func().rect;
}

void SubtitleFilter::setFont(const QFont &f)
{
    DPTR_D(SubtitleFilter);
    if (d.font == f)
        return;
    d.font = f;
    emit fontChanged();
}

QFont SubtitleFilter::font() const
{
    return d_func().font;
}

void SubtitleFilter::setColor(const QColor &c)
{
    DPTR_D(SubtitleFilter);
    if (d.color == c)
        return;
    d.color = c;
    emit colorChanged();
}

QColor SubtitleFilter::color() const
{
    return d_func().color;
}

void SubtitleFilter::process(Statistics *statistics, VideoFrame *frame)
{
    Q_UNUSED(statistics);
    Q_UNUSED(frame);
    DPTR_D(SubtitleFilter);
    QPainterFilterContext* ctx = static_cast<QPainterFilterContext*>(d.context);
    if (!ctx)
        return;
    if (!ctx->paint_device) {
        qWarning("no paint device!");
        return;
    }
    if (d.sub.canRender()) {
        if (frame && frame->timestamp() > 0.0)
            d.sub.setTimestamp(frame->timestamp()); //TODO: set to current display video frame's timestamp
        QRect rect;
        /*
         * image quality maybe to low if use video frame resolution for large display.
         * The difference is small if use paint_device size and video frame aspect ratio ~ renderer aspect ratio
         * if use renderer's resolution, we have to map bounding rect from video frame coordinate to renderer's
         */
        //QImage img = d.sub.getImage(statistics->video_only.width, statistics->video_only.height, &rect);
        QImage img = d.sub.getImage(ctx->paint_device->width(), ctx->paint_device->height(), &rect);
        if (img.isNull())
            return;
        ctx->drawImage(rect, img);
        return;
    }
    ctx->font = d.font;
    ctx->pen.setColor(d.color);
    ctx->rect = d.realRect(ctx->paint_device->width(), ctx->paint_device->height());
    ctx->drawPlainText(ctx->rect, Qt::AlignHCenter | Qt::AlignBottom, d.sub.getText());
}

void SubtitleFilter::onPlayerSourceChanged()
{
    DPTR_D(SubtitleFilter);
    d.file = QString();
    if (!d.autoLoad) {
        return;
    }
    AVPlayer *p = qobject_cast<AVPlayer*>(sender());
    if (!p)
        return;
    QString path = p->file();
    //path.remove(p->source().scheme() + "://");
    QString name = QFileInfo(path).completeBaseName();
    path = QFileInfo(path).dir().absoluteFilePath(name);
    d.sub.setFileName(path);
    d.sub.setFuzzyMatch(true);
    d.sub.load();
}

void SubtitleFilter::onPlayerPositionChanged()
{
    AVPlayer *p = qobject_cast<AVPlayer*>(sender());
    if (!p)
        return;
    DPTR_D(SubtitleFilter);
    d.sub.setTimestamp(qreal(p->position())/1000.0);
}

void SubtitleFilter::onPlayerStart()
{
    DPTR_D(SubtitleFilter);
    if (!autoLoad()) {
        if (d.file == d.sub.fileName())
            return;
        d.sub.setFileName(d.file);
        d.sub.setFuzzyMatch(false);
        d.sub.load();
        return;
    }
    if (d.file != d.sub.fileName())
        return;
    // autoLoad was false then reload then true then reload
    // previous loaded is user selected subtitle
    QString path = d.player->file();
    //path.remove(p->source().scheme() + "://");
    QString name = QFileInfo(path).completeBaseName();
    path = QFileInfo(path).dir().absoluteFilePath(name);
    d.sub.setFileName(path);
    d.sub.setFuzzyMatch(true);
    d.sub.load();
    return;
}

void SubtitleFilter::onEnableChanged(bool value)
{
    DPTR_D(SubtitleFilter);
    if (value) {
        if (d.player) {
            connect(d.player, SIGNAL(sourceChanged()), this, SLOT(onPlayerSourceChanged()));
            connect(d.player, SIGNAL(positionChanged(qint64)), this, SLOT(onPlayerPositionChanged()));
            connect(d.player, SIGNAL(started()), this, SLOT(onPlayerStart()));
        }
        if (autoLoad()) {
            if (!d.player)
                return;
            QString path = d.player->file();
            //path.remove(p->source().scheme() + "://");
            QString name = QFileInfo(path).completeBaseName();
            path = QFileInfo(path).dir().absoluteFilePath(name);
            d.sub.setFileName(path);
            d.sub.setFuzzyMatch(true);
            d.sub.load();
        } else {
            d.sub.setFileName(d.file);
            d.sub.setFuzzyMatch(false);
            d.sub.load();
        }
    } else {
        if (d.player) {
            disconnect(d.player, SIGNAL(sourceChanged()), this, SLOT(onPlayerSourceChanged()));
            disconnect(d.player, SIGNAL(positionChanged(qint64)), this, SLOT(onPlayerPositionChanged()));
            disconnect(d.player, SIGNAL(started()), this, SLOT(onPlayerStart()));
        }
    }
}

} //namespace QtAV
