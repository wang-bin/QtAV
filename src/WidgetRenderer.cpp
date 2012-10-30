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
    ImageRenderer::write(data);

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
    return data.size();
}

void WidgetRenderer::resizeEvent(QResizeEvent *e)
{
    resizeVideo(e->size());
    emit sizeChanged(e->size());
}

void WidgetRenderer::mousePressEvent(QMouseEvent *e)
{
    gPos = e->globalPos();
    pos = e->pos();
}

void WidgetRenderer::mouseMoveEvent(QMouseEvent *e)
{
    int dx = e->globalPos().x() - gPos.x();
    int dy = e->globalPos().y() - gPos.y();
    gPos = e->globalPos();

    static int x = mapToGlobal(QPoint()).x();
    static int y = mapToGlobal(QPoint()).y();
    static int w = width();
    static int h = height();
    switch (action) {
    case GestureMove:
        x += dx;
        y += dy;
        move(x, y);
        break;
    case GestureResize:
        if(pos.x()<w/2) {
            x += dx;
            w -= dx;
        }
        if(pos.x()>w/2) {
            w += dx;
        }
        if(pos.y()<h/2) {
            y += dy;
            h -= dy;
        }
        if(pos.y()>h/2) {
            h += dy;
        }
        setGeometry(x, y, w, h);
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
#endif

}
