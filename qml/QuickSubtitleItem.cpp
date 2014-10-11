#include "QuickSubtitleItem.h"
#include <QtCore/QCoreApplication>
#include <QtQuick/QSGSimpleTextureNode>
#include <QtQuick/QQuickWindow>

QuickSubtitleItem::QuickSubtitleItem(QQuickItem *parent) :
    QQuickItem(parent)
  , m_sub(0)
  , m_texture(0)
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

void QuickSubtitleItem::update(const QImage &image, const QRect &r, int width, int height)
{
    {
        QMutexLocker lock(&m_mutex);
        Q_UNUSED(lock);
        m_image = image; //lock
        m_rect = r;
        m_w_sub = width;
        m_h_sub = height;
    }
    QCoreApplication::postEvent(this, new QEvent(QEvent::User));
}

QRectF QuickSubtitleItem::mapSubRect(const QRect &rect, qreal width, qreal height)
{
    if (width == 0 || height == 0)
        return rect;
    QRectF r;
    const qreal w = boundingRect().width();
    const qreal h = boundingRect().height();
    r.setX(qreal(rect.x())*w/width);
    r.setY(qreal(rect.y())*h/height);
    r.setWidth(qreal(rect.width())*w/width);
    r.setHeight(qreal(rect.height())*h/height);
    //qDebug() << boundingRect() << "  " << width <<"x"<<height << "  " << r;
    return r;
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
