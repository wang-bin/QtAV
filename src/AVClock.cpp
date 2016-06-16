/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

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
#include <QtAV/AVClock.h>
#include <QtCore/QTimer>
#include <QtCore/QTimerEvent>
#include <QtCore/QDateTime>
#include "utils/Logger.h"

namespace QtAV {
enum {
    kRunning,
    kPaused,
    kStopped
};
AVClock::AVClock(AVClock::ClockType c, QObject *parent):
    QObject(parent)
  , auto_clock(true)
  , m_state(kStopped)
  , clock_type(c)
  , mSpeed(1.0)
  , value0(0)
  , avg_err(0)
  , nb_restarted(0)
  , nb_sync(0)
  , sync_id(0)
{
    last_pts = pts_ = pts_v = delay_ = 0;
}

AVClock::AVClock(QObject *parent):
    QObject(parent)
  , auto_clock(true)
  , m_state(kStopped)
  , clock_type(AudioClock)
  , mSpeed(1.0)
  , value0(0)
  , avg_err(0)
  , nb_restarted(0)
  , nb_sync(0)
  , sync_id(0)
{
    last_pts = pts_ = pts_v = delay_ = 0;
}

void AVClock::setClockType(ClockType ct)
{
    if (clock_type == ct)
        return;
    clock_type = ct;
    QTimer::singleShot(0, this, SLOT(restartCorrectionTimer()));
}

AVClock::ClockType AVClock::clockType() const
{
    return clock_type;
}

bool AVClock::isActive() const
{
    return clock_type == AudioClock || timer.isValid();
}

void AVClock::setInitialValue(double v)
{
    value0 = v;
    qDebug("Clock initial value: %f", v);
}

double AVClock::initialValue() const
{
    return value0;
}

void AVClock::setClockAuto(bool a)
{
    auto_clock = a;
}

bool AVClock::isClockAuto() const
{
    return auto_clock;
}

void AVClock::updateExternalClock(qint64 msecs)
{
    if (clock_type == AudioClock)
        return;
    qDebug("External clock change: %f ==> %f", value(), double(msecs) * kThousandth);
    pts_ = double(msecs) * kThousandth; //can not use msec/1000.
    if (!isPaused())
        timer.restart();

    last_pts = pts_;
    t = QDateTime::currentMSecsSinceEpoch();
    if (clockType() == VideoClock)
        pts_v = pts_;
}

void AVClock::updateExternalClock(const AVClock &clock)
{
    if (clock_type != ExternalClock)
        return;
    qDebug("External clock change: %f ==> %f", value(), clock.value());
    pts_ = clock.value();
    if (!isPaused())
        timer.restart();

    last_pts = pts_;
    t = QDateTime::currentMSecsSinceEpoch();
}

void AVClock::setSpeed(qreal speed)
{
    mSpeed = speed;
}

bool AVClock::isPaused() const
{
    return m_state == kPaused;
}

int AVClock::syncStart(int count)
{
    static int sId = 0;
    nb_sync = count;
    if (sId == -1)
        sId = 0;
    sync_id = ++sId;
    return sId;
}

bool AVClock::syncEndOnce(int id)
{
    if (id != sync_id) {
        qWarning("bad sync id: %d, current: %d", id, sync_id);
        return true;
    }
    if (!nb_sync.deref())
        sync_id = 0;
    return sync_id;
}

void AVClock::start()
{
    m_state = kRunning;
    qDebug("AVClock started!!!!!!!!");
    timer.start();
    QTimer::singleShot(0, this, SLOT(restartCorrectionTimer()));
    Q_EMIT started();
}
//remember last value because we don't reset  pts_, pts_v, delay_
void AVClock::pause(bool p)
{
    if (isPaused() == p)
        return;
    if (clock_type == AudioClock)
        return;
    m_state = p ? kPaused : kRunning;
    if (p) {
        QTimer::singleShot(0, this, SLOT(stopCorrectionTimer()));
#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
        timer.invalidate();
#else
        timer.stop();
#endif //QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
        Q_EMIT paused();
    } else {
        timer.start();
        QTimer::singleShot(0, this, SLOT(restartCorrectionTimer()));
        Q_EMIT resumed();
    }
    t = QDateTime::currentMSecsSinceEpoch();
    Q_EMIT paused(p);
}

void AVClock::reset()
{
    nb_sync = 0;
    sync_id = 0;
    // keep mSpeed
    m_state = kStopped;
    value0 = 0;
    pts_ = pts_v = delay_ = 0;
    QTimer::singleShot(0, this, SLOT(stopCorrectionTimer()));
#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
    timer.invalidate();
#else
    timer.stop();
#endif //QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
    t = QDateTime::currentMSecsSinceEpoch();
    Q_EMIT resetted();
}

void AVClock::timerEvent(QTimerEvent *event)
{
    Q_ASSERT_X(clockType() != AudioClock, "AVClock::timerEvent", "Internal error. AudioClock can not call this");
    if (event->timerId() != correction_schedule_timer.timerId())
        return;
    if (isPaused())
        return;
    const double delta_pts = (value() - last_pts)/speed();
    //const double err = double(correction_timer.restart()) * kThousandth - delta_pts;
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const double err = double(now - t) * kThousandth - delta_pts;
    t = now;
    // FIXME: avfoundation camera error is large (about -0.6s)
    if (qAbs(err*10.0) < kCorrectionInterval || clock_type == VideoClock) {
        avg_err += err/(nb_restarted+1);
    }
    //qDebug("correction timer event. error = %f, avg_err=%f, nb_restarted=%d", err, avg_err, nb_restarted);
    last_pts = value();
    nb_restarted = 0;
}

void AVClock::restartCorrectionTimer()
{
    nb_restarted = 0;
    avg_err = 0;
    correction_schedule_timer.stop();
    if (clockType() == AudioClock) // TODO: for all clock type
        return;
    // parameters are reset. do not start correction timer if not running
    if (m_state != kRunning)
        return;
    // timer is always started in AVClock::start()
    if (!timer.isValid())
        return;
    t = QDateTime::currentMSecsSinceEpoch();
    correction_schedule_timer.start(kCorrectionInterval*1000, this);
}

void AVClock::stopCorrectionTimer()
{
    nb_restarted = 0;
    avg_err = 0;
    correction_schedule_timer.stop();
}
} //namespace QtAV
