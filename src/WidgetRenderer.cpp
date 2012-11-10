/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012 Wang Bin <wbsecg1@gmail.com>

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
#include <qfont.h>
#include <qevent.h>
#include <qpainter.h>
#if CONFIG_EZX
#include <qwallpaper.h>
#endif //CONFIG_EZX

namespace QtAV {
WidgetRenderer::WidgetRenderer(QWidget *parent) :
    QWidget(parent),ImageRenderer()
{
#if CONFIG_EZX
    QWallpaper::setAppWallpaperMode(QWallpaper::Off);
#endif
    action = GestureMove;
}

WidgetRenderer::~WidgetRenderer()
{
}

int WidgetRenderer::write(const QByteArray &data)
{
    int s = ImageRenderer::write(data);

#if CONFIG_EZX
    QPixmap pix;
    pix.convertFromImage(image);
    //QPainter v_p(&pix);
#else
    //QPainter v_p(&image);
#endif //CONFIG_EZX

#if CONFIG_EZX
    bitBlt(this, QPoint(), &pix);
#else
    update();
#endif
    return s;
}

void WidgetRenderer::resizeEvent(QResizeEvent *e)
{
    resizeVideo(e->size());
    update();
}

void WidgetRenderer::mousePressEvent(QMouseEvent *e)
{
    gMousePos = e->globalPos();
    iMousePos = e->pos();
}

void WidgetRenderer::mouseMoveEvent(QMouseEvent *e)
{
    int x = pos().x();
    int y = pos().y();
    int dx = e->globalPos().x() - gMousePos.x();
    int dy = e->globalPos().y() - gMousePos.y();
    gMousePos = e->globalPos();
    int w = width();
    int h = height();
    switch (action) {
    case GestureMove:
        x += dx;
        y += dy;
        move(x, y);
        break;
    case GestureResize:
        if(iMousePos.x() < w/2) {
            x += dx;
            w -= dx;
        }
        if(iMousePos.x() > w/2) {
            w += dx;
        }
        if(iMousePos.y() < h/2) {
            y += dy;
            h -= dy;
        }
        if(iMousePos.y() > h/2) {
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
    if (action == GestureMove)
        action = GestureResize;
    else
        action = GestureMove;
}

#if !CONFIG_EZX
void WidgetRenderer::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.drawImage(QPoint(), image);
}

void WidgetRenderer::dragEnterEvent(QDragEnterEvent *)
{

}

void WidgetRenderer::dropEvent(QDropEvent *)
{

}
#endif //CONFIG_EZX
} //namespace QtAV
