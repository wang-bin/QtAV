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
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGFlatColorMaterial>
#include <QtAV/FactoryDefine.h>
#include <QtAV/AVPlayer.h>
#include <QmlAV/QmlAVPlayer.h>
#include <QtAV/VideoRendererTypes.h> //it declares a factory we need
#include "prepost.h"

namespace QtAV
{
VideoRendererId VideoRendererId_QQuickItem = 65; //leave some for QtAV

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

QObject* QQuickItemRenderer::source() const
{
    return d_func().source;
}

void QQuickItemRenderer::setSource(QObject *source)
{
    DPTR_D(QQuickItemRenderer);
    d.source = source;
    ((QmlAVPlayer*)source)->player()->addVideoRenderer(this);
    emit sourceChanged();
}

void QQuickItemRenderer::convertData(const QByteArray &data)
{
    DPTR_D(QQuickItemRenderer);
    QMutexLocker locker(&d.img_mutex);
    Q_UNUSED(locker);
    d.data = data;
    d.image = QImage((uchar*)data.data(), d.src_width, d.src_height, QImage::Format_RGB32);
}

bool QQuickItemRenderer::needUpdateBackground() const
{
    DPTR_D(const QQuickItemRenderer);
    return d.out_rect != boundingRect().toRect();
}

bool QQuickItemRenderer::needDrawFrame() const
{
    return true; //always call updatePaintNode, node must be set
}

void QQuickItemRenderer::drawFrame()
{
    DPTR_D(QQuickItemRenderer);
    if (!d.node)
        return;
    static_cast<QSGSimpleTextureNode*>(d.node)->setRect(boundingRect());
    if (d.image.isNull()) { //TODO: move to QPainterRenderer.convertData?
        d.image = QImage(rendererSize(), QImage::Format_RGB32);
        d.image.fill(Qt::black);
    }
    if (d.texture)
        delete d.texture;
    d.texture = this->window()->createTextureFromImage(d.image);
    static_cast<QSGSimpleTextureNode*>(d.node)->setTexture(d.texture);
    d.node->markDirty(QSGNode::DirtyGeometry);
}

QSGNode *QQuickItemRenderer::updatePaintNode(QSGNode *node, QQuickItem::UpdatePaintNodeData *data)
{
    DPTR_D(QQuickItemRenderer);
    if (!node) {
        node = new QSGSimpleTextureNode();
    }
    d.node = node;
    handlePaintEvent();
    d.node = 0;
    return node;
}

bool QQuickItemRenderer::write()
{
    QMetaObject::invokeMethod(this, "update");
    return true;
}

} // namespace QtAV
