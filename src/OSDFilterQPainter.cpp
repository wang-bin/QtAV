/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013 Wang Bin <wbsecg1@gmail.com>

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

#include "QtAV/OSDFilterQPainter.h"
#include "private/OSDFilter_p.h"
#include <QtGui/QImage>
#include <QtGui/QPainter>

/* TODO:
 * Render on VideoRenderer directly to fix the position
 */

namespace QtAV {

class OSDFilterQPainterPrivate : public OSDFilterPrivate
{
public:
    OSDFilterQPainterPrivate()
    {
        font.setBold(true);
        font.setPixelSize(26);
    }

    QFont font;
};

OSDFilterQPainter::OSDFilterQPainter():
    OSDFilter(*new OSDFilterQPainterPrivate())
{
}

void OSDFilterQPainter::process(QByteArray &data)
{
    DPTR_D(OSDFilterQPainter);
    if (d.show_type == ShowNone || d.sec_current < 0 || d.sec_total < 0)
        return;
    QString text;
    //TODO: calculation move to a function
    if (hasShowType(ShowCurrentTime) || hasShowType(ShowCurrentAndTotalTime)) {
        int h, m, s;
        d.computeTime(d.sec_current, &h, &m, &s);
        text += QString("%1:%2:%3").arg(h, 2, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
        if (hasShowType(ShowCurrentAndTotalTime))
            text += " / ";
    }
    if (hasShowType(ShowCurrentAndTotalTime)) {
        text += QString("%1:%2:%3").arg(d.total_hour, 2, 10, QChar('0')).arg(d.total_min, 2, 10, QChar('0')).arg(d.total_sec, 2, 10, QChar('0'));
    }
    if (hasShowType(ShowRemainTime)) {
        int h, m, s;
        d.computeTime(d.sec_total-d.sec_current, &h, &m, &s);
        text += QString(" -%1:%2:%3").arg(h, 2, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
    }
    if (hasShowType(ShowPercent) && d.sec_total > 0)
        text += QString::number(qreal(d.sec_current)/qreal(d.sec_total)*100, 'f', 1) + "%";
    QImage image((uchar*)data.data(), d.width, d.height, QImage::Format_RGB32);
    QPainter painter(&image);
    //painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setFont(d.font);
    painter.setPen(Qt::white);
    painter.drawText(32, 32, text);
}

} //namespace QtAV
