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

#ifndef QTAV_QML_QUICKSUBTITLEITEM_H
#define QTAV_QML_QUICKSUBTITLEITEM_H

#include <QtCore/QMutex>
#include <QtQuick/QQuickItem>
#include <QtQuick/QSGTexture>
#include <QmlAV/QuickSubtitle.h>

class QuickSubtitleItem : public QQuickItem, public QuickSubtitleObserver
{
    Q_OBJECT
    Q_DISABLE_COPY(QuickSubtitleItem)
    Q_PROPERTY(QuickSubtitle* source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(int fillMode READ fillMode WRITE setFillMode NOTIFY fillModeChanged)
public:
    explicit QuickSubtitleItem(QQuickItem *parent = 0);
    ~QuickSubtitleItem();
    //QQuickItemRenderer* target() const;
    void setSource(QuickSubtitle* s);
    QuickSubtitle* source() const;
    /*!
     * \brief setFillMode
     * keep the value the same as VideoOutput:
     *  fillMode: vo.fillMode
     */
    void setFillMode(int value);
    int fillMode() const;
    virtual void update(const QImage& image, const QRect& r, int width, int height);
Q_SIGNALS:
    void sourceChanged();
    void fillModeChanged();
protected:
    QRectF mapSubRect(const QRect& r, qreal w, qreal h);
    virtual QSGNode *updatePaintNode(QSGNode *node, UpdatePaintNodeData *data);
    virtual bool event(QEvent *e);
    virtual void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry);
private:
    QuickSubtitle *m_sub;
    QSGTexture* m_texture;
    bool m_remap;
    int m_fillMode;
    int m_w_sub, m_h_sub; //subtitle frame width height
    QImage m_image;
    QRect m_rect; // subtitle rect in subtitle frame coorinate
    QRectF m_rect_mapped;
    QMutex m_mutex;
};

#endif // QTAV_QML_QUICKSUBTITLEITEM_H
