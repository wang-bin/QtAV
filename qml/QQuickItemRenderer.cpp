/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013 Wang Bin <wbsecg1@gmail.com>
    theoribeiro <theo@fictix.com.br>

*   This file is part of QtAV

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
******************************************************************************/

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
