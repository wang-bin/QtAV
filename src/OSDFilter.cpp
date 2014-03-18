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

#include "QtAV/OSDFilter.h"
#include "QtAV/Statistics.h"
#include <QtAV/private/Filter_p.h>
#include <QtGui/QPainter>
#if QTAV_HAVE(GL)
#include <QtOpenGL/QGLWidget>
#endif //QTAV_HAVE(GL)
namespace QtAV {

class OSDFilterPrivate : public FilterPrivate
{
public:
};

OSDFilter::OSDFilter(OSDFilterPrivate &d):
    Filter(d)
{
}

OSDFilterQPainter::OSDFilterQPainter():
    OSDFilter(*new OSDFilterPrivate())
{
}

void OSDFilterQPainter::process()
{
    if (mShowType == ShowNone)
        return;
    DPTR_D(Filter);
    QPainterFilterContext* ctx = static_cast<QPainterFilterContext*>(d.context);
    //qDebug("ctx=%p tid=%p main tid=%p", ctx, QThread::currentThread(), qApp->thread());
    ctx->drawPlainText(ctx->rect, Qt::AlignCenter, text(d.statistics));
}

OSDFilterGL::OSDFilterGL():
    OSDFilter(*new OSDFilterPrivate())
{
}

void OSDFilterGL::process()
{
    if (mShowType == ShowNone)
        return;
    DPTR_D(Filter);
    GLFilterContext *ctx = static_cast<GLFilterContext*>(d.context);
    //TODO: render off screen
#if QTAV_HAVE(GL)
    QGLWidget *glw = static_cast<QGLWidget*>(ctx->paint_device);
    if (!glw)
        return;
    glw->renderText(ctx->rect.x(), ctx->rect.y(), text(d.statistics), ctx->font);
#endif //QTAV_HAVE(GL)
}
} //namespace QtAV
