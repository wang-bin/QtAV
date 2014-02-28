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

// TODO: yuv
bool QQuickItemRenderer::isSupported(VideoFormat::PixelFormat pixfmt) const
{
    return VideoFormat::isRGB(pixfmt);
}

bool QQuickItemRenderer::receiveFrame(const VideoFrame &frame)
{
    DPTR_D(QQuickItemRenderer);
    QMutexLocker locker(&d.img_mutex);
    Q_UNUSED(locker);
    d.video_frame = frame;
    d.image = QImage((uchar*)frame.bits(), frame.width(), frame.height(), frame.imageFormat());

    //update(); //why not this?
    QMetaObject::invokeMethod(this, "update");
    return true;
}

QObject* QQuickItemRenderer::source() const
{
    return d_func().source;
}

void QQuickItemRenderer::setSource(QObject *source)
{
    DPTR_D(QQuickItemRenderer);
    if (d.source == source)
        return;
    d.source = source;
    ((QmlAVPlayer*)source)->player()->addVideoRenderer(this);
    emit sourceChanged();
}

QQuickItemRenderer::FillMode QQuickItemRenderer::fillMode() const
{
    return d_func().fill_mode;
}

void QQuickItemRenderer::setFillMode(FillMode mode)
{
    DPTR_D(QQuickItemRenderer);
    if (d.fill_mode == mode)
        return;
    d_func().fill_mode = mode;
    //m_geometryDirty = true;
    //update();
    emit fillModeChanged(mode);
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
    if (d.image.isNull()) {
        d.image = QImage(rendererSize(), QImage::Format_RGB32);
        d.image.fill(Qt::black);
    }
    if (d.fill_mode == Stretch) {
        static_cast<QSGSimpleTextureNode*>(d.node)->setRect(boundingRect());
    } else if (d.fill_mode == PreserveAspectFit || d.fill_mode == PreserveAspectCrop) {
        //TODO: optimize by updating content rect only when fill mode changed
        QSizeF scaled = d.out_rect.size();
        scaled.scale(boundingRect().size(), d.fill_mode == PreserveAspectFit ?
                         Qt::KeepAspectRatio : Qt::KeepAspectRatioByExpanding);
        QRectF rect(QPointF(), scaled);
        rect.moveCenter(boundingRect().center());
        //d.out_rect = rect;
        //rect &= boundingRect(); //FIXME: why QtMultiMedia does not have it?
        static_cast<QSGSimpleTextureNode*>(d.node)->setRect(rect);
    }

    if (d.texture)
        delete d.texture;
    d.texture = this->window()->createTextureFromImage(d.image);
    static_cast<QSGSimpleTextureNode*>(d.node)->setTexture(d.texture);
    d.node->markDirty(QSGNode::DirtyGeometry);
}

QSGNode *QQuickItemRenderer::updatePaintNode(QSGNode *node, QQuickItem::UpdatePaintNodeData *data)
{
    Q_UNUSED(data);
    DPTR_D(QQuickItemRenderer);
    if (!node) {
        node = new QSGSimpleTextureNode();
    }
    d.node = node;
    handlePaintEvent();
    d.node = 0;
    return node;
}

} // namespace QtAV
