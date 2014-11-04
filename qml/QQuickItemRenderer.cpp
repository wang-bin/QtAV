/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013-2014 Wang Bin <wbsecg1@gmail.com>
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

#include "QmlAV/QQuickItemRenderer.h"
#include "QmlAV/private/QQuickItemRenderer_p.h"
#include "QtCore/QCoreApplication"
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGFlatColorMaterial>
#include <QtAV/FactoryDefine.h>
#include <QtAV/AVPlayer.h>
#include <QtAV/VideoRendererTypes.h> //it declares a factory we need
#include "QtAV/private/prepost.h"
#include "QmlAV/QmlAVPlayer.h"
#include "QmlAV/SGVideoNode.h"

namespace QtAV
{
VideoRendererId VideoRendererId_QQuickItem = 65; //leave some for QtAV

FACTORY_REGISTER_ID_AUTO(VideoRenderer, QQuickItem, "QQuickItem")

QQuickItemRenderer::QQuickItemRenderer(QQuickItem *parent) :
    VideoRenderer(*new QQuickItemRendererPrivate)
{
    Q_UNUSED(parent);
    setFlag(QQuickItem::ItemHasContents, true);
}

VideoRendererId QQuickItemRenderer::id() const
{
    return VideoRendererId_QQuickItem;
}

bool QQuickItemRenderer::isSupported(VideoFormat::PixelFormat pixfmt) const
{
    if (!isOpenGL())
        return VideoFormat::isRGB(pixfmt);
    return pixfmt != VideoFormat::Format_YUYV && pixfmt != VideoFormat::Format_UYVY;
}

bool QQuickItemRenderer::event(QEvent *e)
{
    if (e->type() != QEvent::User)
        return QQuickItem::event(e);
    update();
    return true;
}

void QQuickItemRenderer::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChanged(newGeometry, oldGeometry); //geometry will be updated
    DPTR_D(QQuickItemRenderer);
    resizeRenderer(newGeometry.size().toSize());
    if (d.fill_mode == PreserveAspectCrop) {
        QSizeF scaled = d.out_rect.size();
        scaled.scale(boundingRect().size(), Qt::KeepAspectRatioByExpanding);
        d.out_rect = QRect(QPoint(), scaled.toSize());
        d.out_rect.moveCenter(boundingRect().center().toPoint());
    }
}

bool QQuickItemRenderer::receiveFrame(const VideoFrame &frame)
{
    DPTR_D(QQuickItemRenderer);
    QMutexLocker locker(&d.img_mutex);
    Q_UNUSED(locker);
    d.video_frame = frame;
    if (!isOpenGL()) {
        d.image = QImage((uchar*)frame.bits(), frame.width(), frame.height(), frame.bytesPerLine(), frame.imageFormat());
        QRect r = realROI();
        if (r != QRect(0, 0, frame.width(), frame.height()))
            d.image = d.image.copy(r);
    }
    d.frame_changed = true;
//    update();  // why update slow? because of calling in a different thread?
    //QMetaObject::invokeMethod(this, "update"); // slower than directly postEvent
    QCoreApplication::postEvent(this, new QEvent(QEvent::User));
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
    if (d.fill_mode == Stretch) {
        setOutAspectRatioMode(RendererAspectRatio);
    } else {//compute out_rect fits video aspect ratio then compute again if crop
        setOutAspectRatioMode(VideoAspectRatio);
    }
    //m_geometryDirty = true;
    //update();
    emit fillModeChanged(mode);
}

bool QQuickItemRenderer::isOpenGL() const
{
    return d_func().opengl;
}

void QQuickItemRenderer::setOpenGL(bool o)
{
    DPTR_D(QQuickItemRenderer);
    if (d.opengl == o)
        return;
    d.opengl = o;
    emit openGLChanged();
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
    if (isOpenGL()) {
        SGVideoNode *sgvn = static_cast<SGVideoNode*>(d.node);
        Q_ASSERT(sgvn);
        sgvn->setTexturedRectGeometry(d.out_rect, normalizedROI(), d.orientation);
        if (d.frame_changed)
            sgvn->setCurrentFrame(d.video_frame);
        d.frame_changed = false;
        return;
    }
    if (d.image.isNull()) {
        d.image = QImage(rendererSize(), QImage::Format_RGB32);
        d.image.fill(Qt::black);
    }
    static_cast<QSGSimpleTextureNode*>(d.node)->setRect(d.out_rect);

    if (d.texture)
        delete d.texture;

    if (d.orientation == 0) {
        d.texture = window()->createTextureFromImage(d.image);
    } else if (d.orientation == 180) {
        d.texture = window()->createTextureFromImage(d.image.mirrored(true, true));
    }
    static_cast<QSGSimpleTextureNode*>(d.node)->setTexture(d.texture);
    d.node->markDirty(QSGNode::DirtyGeometry);
}

QSGNode *QQuickItemRenderer::updatePaintNode(QSGNode *node, QQuickItem::UpdatePaintNodeData *data)
{
    Q_UNUSED(data);
    DPTR_D(QQuickItemRenderer);
    if (d.frame_changed) {
        if (!node) {
            if (isOpenGL()) {
                node = new SGVideoNode();
            } else {
                node = new QSGSimpleTextureNode();
            }
        }
    }
    if (!node) {
        d.frame_changed = false;
        return 0;
    }
    d.node = node;
    handlePaintEvent();
    d.node = 0;
    return node;
}

bool QQuickItemRenderer::onSetRegionOfInterest(const QRectF &roi)
{
    Q_UNUSED(roi);
    emit regionOfInterestChanged();
    return true;
}

bool QQuickItemRenderer::onSetOrientation(int value)
{
    Q_UNUSED(value);
    if (!isOpenGL()) {
        if (value == 90 || value == 270)
            return false;
    }
    emit orientationChanged();
    return true;
}

} // namespace QtAV
