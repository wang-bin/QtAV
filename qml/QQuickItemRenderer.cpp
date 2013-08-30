#include <QmlAV/QQuickItemRenderer.h>
#include <QmlAV/private/QQuickItemRenderer_p.h>
#include <QImage>
#include <QtQuick/QSGSimpleTextureNode>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGFlatColorMaterial>
#include <QtAV/FactoryDefine.h>
#include <QtAV/VideoRendererTypes.h> //it declares a factory we need
#include "prepost.h"

namespace QtAV
{
VideoRendererId VideoRendererId_QQuickItem = 7;

FACTORY_REGISTER_ID_AUTO(VideoRenderer, QQuickItem, "QQuickItem")

QQuickItemRenderer::QQuickItemRenderer(QQuickItem *parent) :
    VideoRenderer(*new QQuickItemRendererPrivate)
{
    Q_UNUSED(parent);
    DPTR_D(QQuickItemRenderer);
    this->setFlag(QQuickItem::ItemHasContents, true);
}

VideoRendererId QQuickItemRenderer::id() const
{
    return VideoRendererId_QQuickItem;
}

void QQuickItemRenderer::drawFrame()
{

}

QSGNode *QQuickItemRenderer::updatePaintNode(QSGNode *node, QQuickItem::UpdatePaintNodeData *data)
{
    Q_UNUSED(data);
    DPTR_D(QQuickItemRenderer);

    QMutexLocker locker(&d.img_mutex);
    Q_UNUSED(locker)

    if (!node)
    {
        node = new QSGSimpleTextureNode();
        static_cast<QSGSimpleTextureNode*>(node)
                ->setFiltering(QSGTexture::Nearest);
    }

    static_cast<QSGSimpleTextureNode*>(node)->setRect(boundingRect());

    if (d.image)
    {
        QSGTexture* cur_texture = this->window()->createTextureFromImage(*d.image);
        static_cast<QSGSimpleTextureNode*>(node)->setTexture(cur_texture);

        if (d.texture)
            delete d.texture;

        d.texture = cur_texture;

    }
    else
    {
        QSGTexture* texture = this->window()->createTextureFromImage(QImage(200,200, QImage::Format_ARGB32));
        static_cast<QSGSimpleTextureNode*>(node)->setTexture(texture);
        if (d.texture)
            delete d.texture;
        d.texture = texture;
    }

    node->markDirty(QSGNode::DirtyGeometry);

    return node;

}

void QQuickItemRenderer::convertData(const QByteArray &data)
{
    DPTR_D(QQuickItemRenderer);
    QMutexLocker locker(&d.img_mutex);
    Q_UNUSED(locker)

    d.data = data;
    if (d.image)
        delete d.image;
    d.image = new QImage((uchar*)data.data(), d.src_width, d.src_height, QImage::Format_RGB32);
}

bool QQuickItemRenderer::write()
{
    QMetaObject::invokeMethod(this, "update");
    return true;
}


} // namespace QtAV
