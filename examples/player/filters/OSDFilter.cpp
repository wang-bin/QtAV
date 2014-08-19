/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>

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

#include "OSDFilter.h"
#include <QtAV/Statistics.h>
#include <QtAV/private/Filter_p.h>
#include <QtGui/QPainter>

class OSDFilterPrivate : public VideoFilterPrivate
{
public:
};

OSDFilter::OSDFilter(OSDFilterPrivate &d, QObject *parent):
    VideoFilter(d, parent)
{
}

OSDFilterQPainter::OSDFilterQPainter(QObject *parent):
    OSDFilter(*new OSDFilterPrivate(), parent)
{
}

void OSDFilterQPainter::process(Statistics *statistics, VideoFrame *frame)
{
    Q_UNUSED(frame);
    if (mShowType == ShowNone)
        return;
    DPTR_D(VideoFilter);
    QPainterFilterContext* ctx = static_cast<QPainterFilterContext*>(d.context);
    if (!ctx)
        return;
    //qDebug("ctx=%p tid=%p main tid=%p", ctx, QThread::currentThread(), qApp->thread());
    ctx->drawPlainText(ctx->rect, Qt::AlignCenter, text(statistics));
}
