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

#ifndef QTAV_AVCLOCK_H
#define QTAV_AVCLOCK_H

#include <QtAV/QtAV_Global.h>
#include <QtCore/QAtomicInt>
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
        AudioClock,
        ExternalClock,
        VideoClock     //sync to video timestamp
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

    inline void updateVideoTime(double pts);
    inline double videoTime() const;
    inline double delay() const; //playing audio spends some time
    inline void updateDelay(double delay);
    inline qreal diff() const;

    void setSpeed(qreal speed);
    inline qreal speed() const;

    bool isPaused() const;

    /*!
     * \brief syncStart
     * For internal use now
     * Start to sync "count" objects. Call syncEndOnce(id) "count" times to end sync.
     * \param count Number of objects to sync. Each one should call syncEndOnce(int id)
     * \return an id
     */
    int syncStart(int count);
    int syncId() const {return sync_id;}
    /*!
     * \brief syncEndOnce
     * Decrease sync objects count if id is current sync id.
     * \return true if sync is end for id or id is not current sync id
     */
    bool syncEndOnce(int id);

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
private Q_SLOTS:
    /// make sure QBasic timer start/stop in a right thread
    void restartCorrectionTimer();
    void stopCorrectionTimer();
private:
    bool auto_clock;
    int m_state;
    ClockType clock_type;
    mutable double pts_;
    mutable double pts_v;
    double delay_;
    mutable QElapsedTimer timer;
    qreal mSpeed;
    double value0;
    /*!
     * \brief correction_schedule_timer
     * accumulative error is too large using QElapsedTimer.restart() frequently.
     * we periodically correct value() to keep the error always less
     * than the error of calling QElapsedTimer.restart() once
     * see github issue 46, 307 etc
     */
    QBasicTimer correction_schedule_timer;
    qint64 t; // absolute time for elapsed timer correction
    static const int kCorrectionInterval = 1; // 1000ms
    double last_pts;
    double avg_err; // average error of restart()
    mutable int nb_restarted;
    QAtomicInt nb_sync;
    int sync_id;
};

double AVClock::value() const
{
    if (clock_type == AudioClock) {
        // TODO: audio clock need a timer too
        // timestamp from media stream is >= value0
        return pts_ == 0 ? value0 : pts_ + delay_;
    } else if (clock_type == ExternalClock) {
        if (timer.isValid()) {
            ++nb_restarted;
            pts_ += (double(timer.restart()) * kThousandth + avg_err)* speed();
        } else {//timer is paused
            //qDebug("clock is paused. return the last value %f", pts_);
        }
        return pts_ + value0;
    } else {
        if (timer.isValid()) {
            ++nb_restarted;
            pts_v += (double(timer.restart()) * kThousandth + avg_err)* speed();
        }
        return pts_v; // value0 is 1st video pts_v already
    }
}

void AVClock::updateValue(double pts)
{
    if (clock_type == AudioClock)
        pts_ = pts;
}

void AVClock::updateVideoTime(double pts)
{
    pts_v = pts;
    if (clock_type == VideoClock)
        timer.restart();
}

double AVClock::videoTime() const
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

qreal AVClock::diff() const
{
    return value() - videoTime();
}

qreal AVClock::speed() const
{
    return mSpeed;
}

} //namespace QtAV
#endif // QTAV_AVCLOCK_H
