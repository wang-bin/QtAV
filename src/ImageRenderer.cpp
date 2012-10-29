#include <QtAV/ImageRenderer.h>
#include <private/VideoRenderer_p.h>

namespace QtAV {
ImageRenderer::~ImageRenderer()
{

}

int ImageRenderer::write(const QByteArray &data)
{
    //picture.data[0]
#if QT_VERSION >= QT_VERSION_CHECK(4, 0, 0)
    image = QImage((uchar*)data.data(), d_func()->width, d_func()->height, QImage::Format_ARGB32_Premultiplied);
#else
    image = QImage((uchar*)data.data(), d_func()->width, d_func()->height, 16, NULL, 0, QImage::IgnoreEndian);
#endif
    return data.size();
}


void ImageRenderer::setPreview(const QImage &preivew)
{
    this->preview = preivew;
    image = preivew;
}

QImage ImageRenderer::previewImage() const
{
    return preview;
}

QImage ImageRenderer::currentImage() const
{
    return image;
}

/*
void WidgetRenderer::render()
{
#if CONFIG_EZX
    QPixmap pix;
    pix.convertFromImage(m_displayBuffer);
    QPainter v_p(&pix);
#else
    QPainter v_p(&m_displayBuffer);
#endif //CONFIG_EZX
    //v_p.drawImage(QPoint(0, 0), m_displayBuffer);
    QTime v_elapsed(0, 0, 0, 0);
    v_elapsed = v_elapsed.addMSecs(m_time_line.elapsed());
#if QT_VERSION >= 0x030000
    QString v_msg = QString("E=%1 F=%2 D=%3").arg(v_elapsed.toString("hh:mm:ss[zzz]")).arg(frameNo).arg(m_drop_count);
#else
    QString v_msg = QString("E=%1 F=%2 D=%3").arg(v_elapsed.toString()).arg(frameNo).arg(m_drop_count);
#endif
    QFont v_f("MS UI Gothic", 16, QFont::Bold);
    v_f.setFixedPitch(true);
    v_p.setFont(v_f);
    //[Time etc]
    int v_diff = 1;
    int v_x_off = 2;
    int v_y_off = height() - 4;
    v_p.setPen(Qt::white);
    v_p.drawText(QPoint(v_x_off+v_diff, v_y_off+v_diff), v_msg);
    v_p.setPen(Qt::black);
    v_p.drawText(QPoint(v_x_off, v_y_off), v_msg);
    //[FPS]
    int v_font_size = v_p.font().pointSize();
    v_x_off = 2;
    v_y_off = v_font_size + 4;
    //v_msg = QString("%1 fps").arg(m_fps1.value(), 6, 'f', 2, ' ');
    v_msg = QString::number(m_fps1.value(), 'f', 2) + "fps";
    v_p.setPen(Qt::white);
    v_p.drawText(QPoint(v_x_off+v_diff, v_y_off+v_diff), v_msg);
    v_p.setPen(Qt::black);
    v_p.drawText(QPoint(v_x_off, v_y_off), v_msg);

#if CONFIG_EZX
    bitBlt(this, QPoint(), &pix);
#else
    update();
#endif
}
*/

}
