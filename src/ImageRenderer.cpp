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

}
