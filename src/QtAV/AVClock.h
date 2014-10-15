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
#include <QtCore/QBasicTimer>
#include <QtCore/QObject>
#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
#include <QtCore/QElapsedTimer>
#else
#include <QtCore/QTime>
typedef QTime QElapsedTimer;
#endif

/*
 * AVClock is created by AVPlayer. The only way to access AVClock is through AVPlayer::masterClock()
 * The default clock type is Audio's clock, i.e. vedio synchronizes to audio. If audio stream is not
 * detected, then the clock will set to External clock automatically.
 * I name it ExternalClock because the clock can be corrected outside, though it is a clock inside AVClock
 */
namespace QtAV {

static const double kThousandth = 0.001;

class Q_AV_EXPORT AVClock : public QObject
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
    /*!
     * \brief setInitialValue
     * Usually for ExternalClock. For example, media start time is not 0, clock have to set initial value as media start time
     */
    void setInitialValue(double v);
    double initialValue() const;
    /*
     * auto clock: use audio clock if audio stream found, otherwise use external clock
     */
    void setClockAuto(bool a);
    bool isClockAuto() const;
    /*in seconds*/
    inline double pts() const;
    /*!
     * \brief value
     * the real timestamp in seconds: pts + delay
     * \return
     */
    inline double value() const;
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
    /*reset clock intial value and external clock parameters (and stop timer). keep speed() and isClockAuto()*/
    void reset();

protected:
    virtual void timerEvent(QTimerEvent *event);
private:
    bool auto_clock;
    ClockType clock_type;
    mutable double pts_;
    double pts_v;
    double delay_;
    mutable QElapsedTimer timer;
    qreal mSpeed;
    double value0;
    /*!
     * \brief correction_schedule_timer
     * accumulative error is too large using QElapsedTimer.restart() frequently.
     * we periodically correct the pts_ value to keep the error always less
     * than the error of calling QElapsedTimer.restart() once
     * see github issue 46, 307 etc
     */
    QBasicTimer correction_schedule_timer;
    QElapsedTimer correction_timer;
    static const int kCorrectionInterval = 1; // 1000ms
    double last_pts;
};

double AVClock::value() const
{
    if (clock_type == AudioClock) {
        return pts_ + delay_ + value0;
    } else {
        if (timer.isValid()) {
            pts_ += double(timer.restart()) * kThousandth;
        } else {//timer is paused
            //qDebug("clock is paused. return the last value %f", pts_);
        }
        return pts_ * speed() + value0;
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
