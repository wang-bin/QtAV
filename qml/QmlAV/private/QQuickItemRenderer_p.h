#ifndef QQUICKRENDERER_P_H
#define QQUICKRENDERER_P_H

#include <QtAV/private/VideoRenderer_p.h>
#include <QtQuick/QSGTexture>


namespace QtAV
{

class QQuickItemRendererPrivate : public VideoRendererPrivate
{
public:
    QQuickItemRendererPrivate() : texture(NULL),
        image(NULL)
    {

    }

    QImage* image;
    QSGTexture* texture;

};

}

#endif // QQUICKRENDERER_P_H_
