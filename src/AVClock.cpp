/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2013 Wang Bin <wbsecg1@gmail.com>

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
#include <QtCore/QTimerEvent>
#include <QtCore/QDateTime>
#include "utils/Logger.h"

namespace QtAV {

AVClock::AVClock(AVClock::ClockType c, QObject *parent):
    QObject(parent)
  , auto_clock(true)
  , m_paused(false)
  , clock_type(c)
  , mSpeed(1.0)
  , value0(0)
  , avg_err(0)
  , nb_restarted(0)
{
    last_pts = pts_ = pts_v = delay_ = 0;
}

AVClock::AVClock(QObject *parent):
    QObject(parent)
  , auto_clock(true)
  , m_paused(false)
  , clock_type(AudioClock)
  , mSpeed(1.0)
  , value0(0)
  , avg_err(0)
  , nb_restarted(0)
{
    last_pts = pts_ = pts_v = delay_ = 0;
}

void AVClock::setClockType(ClockType ct)
{
    if (clock_type == ct)
        return;
    clock_type = ct;
    if (clock_type != AudioClock) {// TODO: for all clock
        nb_restarted = 0;
        avg_err = 0;
        // timer is always started in AVClock::start()
        if (timer.isValid()) {
            t = QDateTime::currentMSecsSinceEpoch();
            correction_schedule_timer.start(kCorrectionInterval*1000, this);
        }
    }
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
    return m_paused;
}

void AVClock::start()
{
    qDebug("AVClock started!!!!!!!!");
    timer.start();
    correction_schedule_timer.stop();
    // TODO: for all clock type
    if (clockType() != AudioClock) {
        t = QDateTime::currentMSecsSinceEpoch();
        correction_schedule_timer.start(kCorrectionInterval*1000, this);
    }
    emit started();
}
//remember last value because we don't reset  pts_, pts_v, delay_
void AVClock::pause(bool p)
{
    if (isPaused() == p)
        return;
    if (clock_type == AudioClock)
        return;
    m_paused = p;
    if (p) {
        correction_schedule_timer.stop();
#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
        timer.invalidate();
#else
        timer.stop();
#endif //QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
        emit paused();
    } else {
        timer.start();
        correction_schedule_timer.start(kCorrectionInterval*1000, this);
        emit resumed();
    }
    t = QDateTime::currentMSecsSinceEpoch();
    emit paused(p);
}

void AVClock::reset()
{
    // keep mSpeed
    m_paused = false;
    value0 = 0;
    pts_ = pts_v = delay_ = 0;
    correction_schedule_timer.stop();
#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
    timer.invalidate();
#else
    timer.stop();
#endif //QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
    t = QDateTime::currentMSecsSinceEpoch();
    emit resetted();
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

} //namespace QtAV
