/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

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

#include "QtAV/SubtitleFilter.h"
#include "QtAV/private/Filter_p.h"
#include "QtAV/private/PlayerSubtitle.h"
#include "QtAV/Subtitle.h"
#include "QtAV/VideoFrame.h"
#include <QtCore/QScopedPointer>
#include "utils/Logger.h"

namespace QtAV {

class SubtitleFilterPrivate : public VideoFilterPrivate
{
public:
    SubtitleFilterPrivate()
        : player_sub(new PlayerSubtitle(0))
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
        if (qAbs(rect.height()) < 1)
            r.setHeight(rect.height()*qreal(height));
        if (rect.width() == 1.0 && normalized) {
            r.setWidth(width);
        }
        if (rect.height() == 1.0 && normalized) {
            r.setHeight(height);
        }
        return r;
    }

    QScopedPointer<PlayerSubtitle> player_sub;
    QRectF rect;
    QFont font;
    QColor color;
};

SubtitleFilter::SubtitleFilter(QObject *parent) :
    VideoFilter(*new SubtitleFilterPrivate(), parent)
  , SubtitleAPIProxy(this)
{
    DPTR_D(SubtitleFilter);
    setSubtitle(d.player_sub->subtitle());
    connect(this, SIGNAL(enabledChanged(bool)), d.player_sub.data(), SLOT(onEnabledChanged(bool)));
    connect(d.player_sub.data(), SIGNAL(autoLoadChanged(bool)), this, SIGNAL(autoLoadChanged(bool)));
    connect(d.player_sub.data(), SIGNAL(fileChanged()), this, SIGNAL(fileChanged()));

    if (parent && !qstrcmp(parent->metaObject()->className(), "AVPlayer")) {
        AVPlayer* p = reinterpret_cast<AVPlayer*>(parent);
        setPlayer(p);
    }
}

void SubtitleFilter::setPlayer(AVPlayer *player)
{
    d_func().player_sub->setPlayer(player);
}

void SubtitleFilter::setFile(const QString &file)
{
    d_func().player_sub->setFile(file);
}

QString SubtitleFilter::file() const
{
    return d_func().player_sub->file();
}

void SubtitleFilter::setAutoLoad(bool value)
{
    d_func().player_sub->setAutoLoad(value);
}

bool SubtitleFilter::autoLoad() const
{
    return d_func().player_sub->autoLoad();
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

QString SubtitleFilter::subtitleText(qreal t) const
{
    DPTR_D(const SubtitleFilter);
    d.player_sub->subtitle()->setTimestamp(t);
    return d.player_sub->subtitle()->getText();
}

void SubtitleFilter::process(Statistics *statistics, VideoFrame *frame)
{
    Q_UNUSED(statistics);
    Q_UNUSED(frame);
    DPTR_D(SubtitleFilter);
    if (!context()->paint_device) {
        qWarning("no paint device!");
        return;
    }
    if (frame && frame->timestamp() > 0.0)
        d.player_sub->subtitle()->setTimestamp(frame->timestamp());
    if (d.player_sub->subtitle()->canRender()) {
        QRect rect;
        /*
         * image quality maybe to low if use video frame resolution for large display.
         * The difference is small if use paint_device size and video frame aspect ratio ~ renderer aspect ratio
         * if use renderer's resolution, we have to map bounding rect from video frame coordinate to renderer's
         */
        //QImage img = d.player_sub->subtitle()->getImage(statistics->video_only.width, statistics->video_only.height, &rect);
        QImage img = d.player_sub->subtitle()->getImage(context()->paint_device->width(), context()->paint_device->height(), &rect);
        if (img.isNull())
            return;
        context()->drawImage(rect, img);
        return;
    }
    context()->font = d.font;
    context()->pen.setColor(d.color);
    context()->rect = d.realRect(context()->paint_device->width(), context()->paint_device->height());
    context()->drawPlainText(context()->rect, Qt::AlignHCenter | Qt::AlignBottom, d.player_sub->subtitle()->getText());
}

} //namespace QtAV
