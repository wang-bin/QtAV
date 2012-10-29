#include <QtAV/VideoRenderer.h>
#include <private/VideoRenderer_p.h>
/*!
    EZX:
    PIX_FMT_BGR565, 16bpp,
    PIX_FMT_RGB18,  18bpp,
*/

namespace QtAV {

VideoRendererPrivate::~VideoRendererPrivate()
{
    sws_freeContext(sws_ctx); //NULL: does nothing
    sws_ctx = NULL;
}

void VideoRendererPrivate::resizePicture(int width, int height)
{
    int v_numBytes = avpicture_get_size(pix_fmt, width, height);
    qDebug("[v_numBytes][m_numBytes] %d %d", v_numBytes, numBytes);
    if(numBytes < v_numBytes) {
        numBytes = v_numBytes;
        data.resize(numBytes);
    }
    //picture的数据按pix_fmt格式自动"关联"到 data
    avpicture_fill(
            &picture,
            reinterpret_cast<uint8_t*>(data.data()),
            pix_fmt,
            width,
            height
            );
    this->width = width;
    this->height = height;
}



VideoRenderer::VideoRenderer()
    :d_ptr(new VideoRendererPrivate)
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
    if (width >0 && height >0)
        d->resizePicture(width, height);
}

//TODO: if size not change, reuse.
QByteArray VideoRenderer::scale(AVCodecContext *codecCtx, AVFrame *frame)
{
    Q_D(VideoRenderer);
    /*
    d->sws_ctx = sws_getContext(
            codecCtx->width, //int srcW,
            codecCtx->height, //int srcH,
            codecCtx->pix_fmt, //enum PixelFormat srcFormat,
            d->width, //int dstW,
            d->height, //int dstH,
            d->pix_fmt, //enum PixelFormat dstFormat,
            SWS_BICUBIC, //int flags,
            NULL, //SwsFilter *srcFilter,
            NULL, //SwsFilter *dstFilter,
            NULL  //const double *param
            );
    */
    d->sws_ctx = sws_getCachedContext(d->sws_ctx
            , codecCtx->width, codecCtx->height, codecCtx->pix_fmt
            , d->width, d->height, d->pix_fmt
            , (d->width == codecCtx->width && d->height == codecCtx->height) ? SWS_POINT : SWS_BICUBIC
            , NULL, NULL, NULL
            );
    int v_scale_result = sws_scale(
            d->sws_ctx,
            frame->data,
            frame->linesize,
            0,
            codecCtx->height,
            d->picture.data,
            d->picture.linesize
            );
    Q_UNUSED(v_scale_result);
    //sws_freeContext(v_sws_ctx);
    if (frame->interlaced_frame) //?
        avpicture_deinterlace(&d->picture, &d->picture, d->pix_fmt, d->width, d->height);
    return d->data;
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
