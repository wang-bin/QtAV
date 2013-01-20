/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2013 Wang Bin <wbsecg1@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include <QtAV/WidgetRenderer.h>
#include <private/WidgetRenderer_p.h>
#include <qfont.h>
#include <qevent.h>
#include <qpainter.h>
#if CONFIG_EZX
#include <qwallpaper.h>
#endif //CONFIG_EZX
#include <QtAV/EventFilter.h>

namespace QtAV {
WidgetRenderer::WidgetRenderer(QWidget *parent, Qt::WindowFlags f) :
    QWidget(parent, f),ImageRenderer(*new WidgetRendererPrivate())
{
    setFocusPolicy(Qt::StrongFocus);
#if CONFIG_EZX
    QWallpaper::setAppWallpaperMode(QWallpaper::Off);
#endif
    setAutoFillBackground(false);
}

WidgetRenderer::WidgetRenderer(WidgetRendererPrivate &d, QWidget *parent, Qt::WindowFlags f)
    :QWidget(parent, f),ImageRenderer(d)
{
}

WidgetRenderer::~WidgetRenderer()
{
}

void WidgetRenderer::registerEventFilter(EventFilter *filter)
{
    d_func().event_filter = filter;
    installEventFilter(filter);
}

bool WidgetRenderer::write()
{
#if CONFIG_EZX
    QPixmap pix;
    pix.convertFromImage(d_func().image);
    //QPainter v_p(&pix);
#else
    //QPainter v_p(&image);
#endif //CONFIG_EZX

#if CONFIG_EZX
    bitBlt(this, QPoint(), &pix);
#else
    update();
#endif
	return true;
}

void WidgetRenderer::resizeEvent(QResizeEvent *e)
{
    resizeVideo(e->size());
    update();
}

void WidgetRenderer::mousePressEvent(QMouseEvent *e)
{
    DPTR_D(WidgetRenderer);
    d.gMousePos = e->globalPos();
    d.iMousePos = e->pos();
}

void WidgetRenderer::mouseMoveEvent(QMouseEvent *e)
{
    if (parentWidget())
        return;
    DPTR_D(WidgetRenderer);
    int x = pos().x();
    int y = pos().y();
    int dx = e->globalPos().x() - d.gMousePos.x();
    int dy = e->globalPos().y() - d.gMousePos.y();
    d.gMousePos = e->globalPos();
    int w = width();
    int h = height();
    switch (d.action) {
    case GestureMove:
        x += dx;
        y += dy;
        move(x, y);
        break;
    case GestureResize:
        if(d.iMousePos.x() < w/2) {
            x += dx;
            w -= dx;
        }
        if(d.iMousePos.x() > w/2) {
            w += dx;
        }
        if(d.iMousePos.y() < h/2) {
            y += dy;
            h -= dy;
        }
        if(d.iMousePos.y() > h/2) {
            h += dy;
        }
        //setGeometry(x, y, w, h);
        move(x, y);
        resize(w, h);
        break;
    }
#if CONFIG_EZX
    repaint(false);
#else
    repaint();
#endif
}

void WidgetRenderer::mouseDoubleClickEvent(QMouseEvent *)
{
    DPTR_D(WidgetRenderer);
    if (d.action == GestureMove)
        d.action = GestureResize;
    else
        d.action = GestureMove;
}

#if !CONFIG_EZX
void WidgetRenderer::paintEvent(QPaintEvent *)
{
    DPTR_D(WidgetRenderer);
    QPainter p(this);
    if (!d.image.isNull()) {
        if (d.image.size() == QSize(d.width, d.height))
            p.drawImage(QPoint(), d.image);
        else
            p.drawImage(rect(), d.image);
    } else if (!d.preview.isNull()){
        if (d.preview.size() == QSize(d.width, d.height))
            p.drawImage(QPoint(), d.preview);
        else
            p.drawImage(rect(), d.preview);
    } else {
        d.preview = QImage(videoSize(), QImage::Format_RGB32);
        d.preview.fill(QColor(Qt::black));
        p.drawImage(QPoint(), d.preview);
    }
}

void WidgetRenderer::dragEnterEvent(QDragEnterEvent *)
{
}

void WidgetRenderer::dropEvent(QDropEvent *)
{
}
#endif //CONFIG_EZX
} //namespace QtAV
