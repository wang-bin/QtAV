/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2014)

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

#include "QuickSubtitleItem.h"
#include <QtCore/QCoreApplication>
#include <QtQuick/QSGSimpleTextureNode>
#include <QtQuick/QQuickWindow>

QuickSubtitleItem::QuickSubtitleItem(QQuickItem *parent) :
    QQuickItem(parent)
  , m_sub(0)
  , m_texture(0)
  , m_remap(false)
  , m_fillMode(Qt::KeepAspectRatio)
  , m_w_sub(0)
  , m_h_sub(0)
{
    setFlag(QQuickItem::ItemHasContents, true);
}

QuickSubtitleItem::~QuickSubtitleItem()
{
    if (m_texture) {
        delete m_texture;
        m_texture = 0;
    }
}

void QuickSubtitleItem::setSource(QuickSubtitle *s)
{
    if (m_sub == s)
        return;
    if (m_sub)
        m_sub->removeObserver(this);
    m_sub = s;
    emit sourceChanged();
    if (m_sub)
        m_sub->addObserver(this);
}

QuickSubtitle* QuickSubtitleItem::source() const
{
    return m_sub;
}

void QuickSubtitleItem::setFillMode(int value)
{
    if (m_fillMode == value)
        return;
    m_fillMode = value;
    m_remap = true;
    emit fillModeChanged();
}

int QuickSubtitleItem::fillMode() const
{
    return m_fillMode;
}

void QuickSubtitleItem::update(const QImage &image, const QRect &r, int width, int height)
{
    {
        QMutexLocker lock(&m_mutex);
        Q_UNUSED(lock);
        m_image = image; //lock
        if (m_rect != r || m_w_sub != width || m_h_sub != height) {
            m_remap = true;
            m_rect = r;
            m_w_sub = width;
            m_h_sub = height;
        }
    }
    QCoreApplication::postEvent(this, new QEvent(QEvent::User));
}

QRectF QuickSubtitleItem::mapSubRect(const QRect &rect, qreal w, qreal h)
{
    if (w == 0 || h == 0)
        return QRectF();
    if (!m_remap)
        return m_rect_mapped;
    m_remap = false;
    qreal ww = width();
    qreal hh = height();
    qreal dx = 0;
    qreal dy = 0;
    if (m_fillMode == Qt::KeepAspectRatio) {
        if (ww*h > w*hh) { //item is too wide
            ww = hh*w/h;
            dx = (width() - ww)/2.0;
        } else {
            hh = ww*h/w;
            dy = (height() - hh)/2.0;
        }
    }
    m_rect_mapped.setX(qreal(rect.x())*ww/w);
    m_rect_mapped.setY(qreal(rect.y())*hh/h);
    m_rect_mapped.setWidth(qreal(rect.width())*ww/w);
    m_rect_mapped.setHeight(qreal(rect.height())*hh/h);
    m_rect_mapped.moveTo(m_rect_mapped.topLeft() + QPointF(dx, dy));
    //qDebug() << boundingRect() << "  " << width <<"x"<<height << "  " << r;
    return m_rect_mapped;
}

QSGNode* QuickSubtitleItem::updatePaintNode(QSGNode *node, UpdatePaintNodeData *data)
{
    Q_UNUSED(data);
    if (m_w_sub == 0 || m_h_sub == 0) {
        return node;
    }
    QSGSimpleTextureNode *stn = static_cast<QSGSimpleTextureNode*>(node);
    if (!node) {
        node = new QSGSimpleTextureNode();
        stn = static_cast<QSGSimpleTextureNode*>(node);
        stn->setFiltering(QSGTexture::Linear);
    }
    stn->setRect(mapSubRect(m_rect, m_w_sub, m_h_sub));
    if (m_texture)
        delete m_texture;
    {
        QMutexLocker lock(&m_mutex);
        Q_UNUSED(lock);
        m_texture = window()->createTextureFromImage(m_image);
    }
    stn->setTexture(m_texture);
    node->markDirty(QSGNode::DirtyGeometry);
    return node;
}

bool QuickSubtitleItem::event(QEvent *e)
{
    if (e->type() != QEvent::User)
        return QQuickItem::event(e);
    QQuickItem::update();
    return true;
}

void QuickSubtitleItem::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChanged(newGeometry, oldGeometry); //geometry will be updated
    m_remap = true;
    QCoreApplication::postEvent(this, new QEvent(QEvent::User));
}
