#ifndef QAVVIDEORENDERER_H
#define QAVVIDEORENDERER_H

#include <qbytearray.h>
#include <qsize.h>
#include <QtAV/AVOutput.h>

struct AVCodecContext;
struct AVFrame;

namespace QtAV {
class VideoRendererPrivate;
class Q_EXPORT VideoRenderer : public AVOutput
{
public:
    VideoRenderer();
    virtual ~VideoRenderer() = 0;

    virtual bool open() {return true;}
    virtual bool close() {return true;}
    void resizeVideo(const QSize& size);
    void resizeVideo(int width, int height);
    QByteArray scale(AVCodecContext* codecCtx, AVFrame *frame);
    QSize videoSize() const;
    int videoWidth() const;
    int videoHeight() const;
    //return the bytes writed
    //virtual int write(const QByteArray& data) = 0;

protected:
    Q_DECLARE_PRIVATE(VideoRenderer)
    VideoRendererPrivate* d_ptr;
};
}

#endif // QAVVIDEORENDERER_H
