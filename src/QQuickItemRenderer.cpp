#include <QtAV/QQuickItemRenderer.h>

#include <QtAV/private/QQuickItemRenderer_p.h>
#include <QImage>

#include <QtQuick/QSGSimpleTextureNode>
#include <QtQuick/QQuickWindow>
#include <QTime>

#include <QtQuick/QSGFlatColorMaterial>

namespace QtAV
{

QQuickItemRenderer::QQuickItemRenderer(QQuickItem *parent) :
    VideoRenderer(*new QQuickItemRendererPrivate)
{

    DPTR_D(QQuickItemRenderer);

    this->setFlag(QQuickItem::ItemHasContents, true);

}


void QQuickItemRenderer::drawFrame()
{
//    DPTR_D(QQuickRenderer);
//    d.texture = this->window()->createTextureFromImage(*d.image);
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

//    handlePaintEvent();

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

    // Creating a copy of the image to avoid thread simultaneous access
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
