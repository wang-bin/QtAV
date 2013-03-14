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

namespace QtAV {

class OSDFilterQPainterPrivate : public OSDFilterPrivate
{

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
        int h = d.sec_current/3600;
        int m = (d.sec_current%3600)/60;
        int s = d.sec_current%60;
        text += QString("%1:%2:%3").arg(h, 2, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
        if (hasShowType(ShowCurrentAndTotalTime))
            text += " / ";
    }
    if (hasShowType(ShowCurrentAndTotalTime)) {
        int h = d.sec_total/3600;
        int m = (d.sec_total%3600)/60;
        int s = d.sec_total%60;
        text += QString("%1:%2:%3").arg(h, 2, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
    }
    if (hasShowType(ShowRemainTime)) {
        int h = (d.sec_total-d.sec_current)/3600;
        int m = ((d.sec_total-d.sec_current)%3600)/60;
        int s = (d.sec_total-d.sec_current)%60;
        text += QString(" -%1:%2:%3").arg(h, 2, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
    }
    if (hasShowType(ShowPercent) && d.sec_total > 0)
        text += QString::number(qreal(d.sec_current)/qreal(d.sec_total)*100, 'f', 1) + "%";

    QImage image((uchar*)data.data(), d.width, d.height, QImage::Format_RGB32);
    QPainter painter(&image);
    QFont f;
    f.setBold(true);
    f.setPixelSize(26);
    painter.setFont(f);
    painter.setPen(Qt::white);
    painter.drawText(60, 60, text);
}

} //namespace QtAV
