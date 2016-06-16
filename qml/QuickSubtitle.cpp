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

#include "QmlAV/QuickSubtitle.h"
#include "QmlAV/QmlAVPlayer.h"
#include <QtAV/Filter.h>
#include <QtAV/VideoFrame.h>
#include <QtAV/private/PlayerSubtitle.h>

using namespace QtAV;

class QuickSubtitle::Filter : public QtAV::VideoFilter
{
public:
    Filter(Subtitle *sub, QuickSubtitle *parent) :
        VideoFilter(parent)
      , m_empty_image(false)
      , m_sub(sub)
      , m_subject(parent)
    {}
protected:
    virtual void process(Statistics* statistics, VideoFrame* frame) {
        Q_UNUSED(statistics);
        if (!m_sub)
            return;
        if (frame && frame->timestamp() > 0.0) {
            m_sub->setTimestamp(frame->timestamp()); //TODO: set to current display video frame's timestamp
            QRect r;
            QImage image(m_sub->getImage(frame->width(), frame->height(), &r));
            if (image.isNull()) {
                if (m_empty_image)
                    return;
                m_empty_image = true;
            } else {
                m_empty_image = false;
            }
            m_subject->notifyObservers(image, r, frame->width(), frame->height());
        }
    }
private:
    bool m_empty_image;
    Subtitle *m_sub;
    QuickSubtitle *m_subject;
};

QuickSubtitle::QuickSubtitle(QObject *parent) :
    QObject(parent)
  , SubtitleAPIProxy(this)
  , m_enable(true)
  , m_player(0)
  , m_player_sub(new PlayerSubtitle(this))
  , m_filter(0)
{
    QmlAVPlayer *p = qobject_cast<QmlAVPlayer*>(parent);
    if (p)
        setPlayer(p);

    m_filter = new Filter(m_player_sub->subtitle(), this);
    setSubtitle(m_player_sub->subtitle()); //for proxy
    connect(this, SIGNAL(enabledChanged(bool)), m_player_sub, SLOT(onEnabledChanged(bool))); //////
    connect(m_player_sub, SIGNAL(autoLoadChanged(bool)), this, SIGNAL(autoLoadChanged()));
    connect(m_player_sub, SIGNAL(fileChanged()), this, SIGNAL(fileChanged()));
}

QString QuickSubtitle::getText() const
{
    return m_player_sub->subtitle()->getText();
}

void QuickSubtitle::addObserver(QuickSubtitleObserver *ob)
{
    if (!m_observers.contains(ob)) {
        QMutexLocker lock(&m_mutex);
        Q_UNUSED(lock);
        m_observers.append(ob);
    }
}

void QuickSubtitle::removeObserver(QuickSubtitleObserver *ob)
{
    QMutexLocker lock(&m_mutex);
    Q_UNUSED(lock);
    m_observers.removeAll(ob);
}

void QuickSubtitle::notifyObservers(const QImage &image, const QRect &r, int width, int height, QuickSubtitleObserver *ob)
{
    if (ob) {
        ob->update(image, r, width, height);
        return;
    }
    QMutexLocker lock(&m_mutex);
    Q_UNUSED(lock);
    if (m_observers.isEmpty())
        return;
    foreach (QuickSubtitleObserver* o, m_observers) {
        o->update(image, r, width, height);
    }
}

void QuickSubtitle::setPlayer(QObject *player)
{
    QmlAVPlayer *p = qobject_cast<QmlAVPlayer*>(player);
    if (m_player == p)
        return;
    if (m_player)
        m_filter->uninstall();
    m_player = p;
    if (!p)
        return;
    m_filter->installTo(p->player());
    // ~Filter() can not call uninstall() unless player is still exists
    // TODO: check AVPlayer null?
    m_player_sub->setPlayer(p->player());
}

QObject* QuickSubtitle::player()
{
    return m_player;
}

void QuickSubtitle::setEnabled(bool value)
{
    if (m_enable == value)
        return;
    m_enable = value;
    Q_EMIT enabledChanged(value);
    m_filter->setEnabled(m_enable);
    if (!m_enable) { //display nothing
        notifyObservers(QImage(), QRect(), 0, 0);
    }
}

bool QuickSubtitle::isEnabled() const
{
    return m_enable;
}

void QuickSubtitle::setFile(const QString &file)
{
    m_player_sub->setFile(file);
}

QString QuickSubtitle::file() const
{
    return m_player_sub->file();
}

void QuickSubtitle::setAutoLoad(bool value)
{
    m_player_sub->setAutoLoad(value);
}

bool QuickSubtitle::autoLoad() const
{
    return m_player_sub->autoLoad();
}

