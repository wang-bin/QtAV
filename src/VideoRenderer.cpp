#include <QtAV/VideoRenderer.h>
#include <QtAV/VideoDecoder.h>
#include <private/VideoRenderer_p.h>

/*!
    EZX:
    PIX_FMT_BGR565, 16bpp,
    PIX_FMT_RGB18,  18bpp,
*/

namespace QtAV {

VideoRenderer::VideoRenderer()
    :AVOutput(*new VideoRendererPrivate)
{
}

VideoRenderer::~VideoRenderer()
{
    if (d_ptr) {
        delete d_ptr;
        d_ptr = 0;
    }
}

void VideoRenderer::resizeVideo(const QSize &size)
{
    resizeVideo(size.width(), size.height());
}

void VideoRenderer::resizeVideo(int width, int height)
{
    Q_D(VideoRenderer);
    if (width == 0 || height == 0)
        return;
    if (d->dec) {
        static_cast<VideoDecoder*>(d->dec)->resizeVideo(width, height);
//        d->resizePicture(width, height);
    }
    d->width = width;
    d->height = height;
}



QSize VideoRenderer::videoSize() const
{
    Q_D(const VideoRenderer);
    return QSize(d->width, d->width);
}

int VideoRenderer::videoWidth() const
{
    Q_D(const VideoRenderer);
    return d->width;
}

int VideoRenderer::videoHeight() const
{
    Q_D(const VideoRenderer);
    return d->width;
}

}
