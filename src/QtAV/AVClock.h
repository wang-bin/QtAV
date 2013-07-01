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

#ifndef QTAV_AVCLOCK_H
#define QTAV_AVCLOCK_H

#include <QtAV/QtAV_Global.h>
#include <QtCore/QObject>
#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
#include <QtCore/QElapsedTimer>
#else
#include <QtCore/QTimer>
typedef QTimer QElapsedTimer
#endif

/*
 * AVClock is created by AVPlayer. The only way to access AVClock is through AVPlayer::masterClock()
 * The default clock type is Audio's clock, i.e. vedio synchronizes to audio. If audio stream is not
 * detected, then the clock will set to External clock automatically.
 * I name it ExternalClock because the clock can be corrected outside, though it is a clock inside AVClock
 */
namespace QtAV {

static const double kThousandth = 0.001;

class Q_EXPORT AVClock : public QObject
{
    Q_OBJECT
public:
    typedef enum {
        AudioClock, ExternalClock
    } ClockType;

    AVClock(ClockType c, QObject* parent = 0);
    AVClock(QObject* parent = 0);
    void setClockType(ClockType ct);
    ClockType clockType() const;
    bool isActive() const;
    /*
     * auto clock: use audio clock if audio stream found, otherwise use external clock
     */
    void setClockAuto(bool a);
    bool isClockAuto() const;
    /*in seconds*/
    inline double pts() const;
    inline double value() const; //the real timestamp: pts + delay
    inline void updateValue(double pts); //update the pts
    /*used when seeking and correcting from external*/
    void updateExternalClock(qint64 msecs);
    /*external clock outside still running, so it's more accurate for syncing multiple clocks serially*/
    void updateExternalClock(const AVClock& clock);

    inline void updateVideoPts(double pts);
    inline double videoPts() const;
    inline double delay() const; //playing audio spends some time
    inline void updateDelay(double delay);

    void setSpeed(qreal speed);
    inline qreal speed() const;

signals:
    void paused(bool);
    void paused(); //equals to paused(true)
    void resumed();//equals to paused(false)
    void started();
    void resetted();

public slots:
    //these slots are not frequently used. so not inline
    /*start the external clock*/
    void start();
    /*pause external clock*/
    void pause(bool p);
    /*reset(stop) external clock*/
    void reset();

private:
    bool auto_clock;
    ClockType clock_type;
    mutable double pts_;
    double pts_v;
    double delay_;
    mutable QElapsedTimer timer;
    qreal mSpeed;
};

double AVClock::value() const
{
    if (clock_type == AudioClock) {
        return pts_ + delay_;
    } else {
        if (timer.isValid()) {
            pts_ += double(timer.restart()) * kThousandth;
        } else {//timer is paused
            qDebug("clock is paused. return the last value %f", pts_);
        }
        return pts_ * speed();
    }
}

void AVClock::updateValue(double pts)
{
    if (clock_type == AudioClock)
        pts_ = pts;
}

void AVClock::updateVideoPts(double pts)
{
    pts_v = pts;
}

double AVClock::videoPts() const
{
    return pts_v;
}

double AVClock::delay() const
{
    return delay_;
}

void AVClock::updateDelay(double delay)
{
    delay_ = delay;
}

qreal AVClock::speed() const
{
    return mSpeed;
}

} //namespace QtAV
#endif // QTAV_AVCLOCK_H
