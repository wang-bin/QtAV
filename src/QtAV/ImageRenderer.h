#ifndef QAVIMAGERENDERER_H
#define QAVIMAGERENDERER_H

#include <QtAV/VideoRenderer.h>
#include <qimage.h>

namespace QtAV {

class Q_EXPORT ImageRenderer : public VideoRenderer
{
public:
    virtual ~ImageRenderer();
    virtual int write(const QByteArray &data);
    void setPreview(const QImage& preview);
    QImage previewImage() const;
    QImage currentImage() const;
protected:
    QImage image, preview;
};

} //namespace QtAV
#endif // QAVIMAGERENDERER_H
