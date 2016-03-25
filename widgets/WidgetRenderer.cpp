/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

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

#include "QtAVWidgets/WidgetRenderer.h"
#include "QtAV/private/QPainterRenderer_p.h"
#include <QtGui/QFont>
#include <QtGui/QPainter>
#include <QApplication>
#include <QResizeEvent>
#include "QtAV/Filter.h"

namespace QtAV {
class WidgetRendererPrivate : public QPainterRendererPrivate
{
public:
    virtual ~WidgetRendererPrivate(){}
};

VideoRendererId WidgetRenderer::id() const
{
    return VideoRendererId_Widget;
}

WidgetRenderer::WidgetRenderer(QWidget *parent, Qt::WindowFlags f) :
    QWidget(parent, f),QPainterRenderer(*new WidgetRendererPrivate())
{
    DPTR_D(WidgetRenderer);
    d.painter = new QPainter();
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    /* To rapidly update custom widgets that constantly paint over their entire areas with
     * opaque content, e.g., video streaming widgets, it is better to set the widget's
     * Qt::WA_OpaquePaintEvent, avoiding any unnecessary overhead associated with repainting the
     * widget's background
     */
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAutoFillBackground(false);
    QPainterFilterContext *ctx = static_cast<QPainterFilterContext*>(d.filter_context);
    if (ctx) {
        ctx->painter = d.painter;
    } else {
        qWarning("FilterContext not available!");
    }
}

WidgetRenderer::WidgetRenderer(WidgetRendererPrivate &d, QWidget *parent, Qt::WindowFlags f)
    :QWidget(parent, f),QPainterRenderer(d)
{
    d.painter = new QPainter();
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    setAutoFillBackground(false);
    QPainterFilterContext *ctx = static_cast<QPainterFilterContext*>(d.filter_context);
    if (ctx) {
        ctx->painter = d.painter;
    } else {
        qWarning("FilterContext not available!");
    }
}

bool WidgetRenderer::receiveFrame(const VideoFrame &frame)
{
    preparePixmap(frame);
    updateUi();
    /*
     * workaround for the widget not updated if has parent. don't know why it works and why update() can't
     * Thanks to Vito Covito and Carlo Scarpato
     * Now it's fixed by posting a QUpdateLaterEvent
     */
    Q_EMIT imageReady();
    return true;
}

void WidgetRenderer::resizeEvent(QResizeEvent *e)
{
    DPTR_D(WidgetRenderer);
    d.update_background = true;
    resizeRenderer(e->size());
    update();
}

void WidgetRenderer::paintEvent(QPaintEvent *)
{
    DPTR_D(WidgetRenderer);
    d.painter->begin(this); //Widget painting can only begin as a result of a paintEvent
    handlePaintEvent();
    if (d.painter->isActive())
        d.painter->end();
}

bool WidgetRenderer::onSetOrientation(int value)
{
    Q_UNUSED(value);
    update();
    return true;
}

} //namespace QtAV
