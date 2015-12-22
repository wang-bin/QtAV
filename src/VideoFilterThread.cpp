/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2015 Wang Bin <wbsecg1@gmail.com>

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

#include "VideoFilterThread.h"
#include "QtAV/VideoRenderer.h"
#include "QtAV/Filter.h"
#include "QtAV/FilterContext.h"

namespace QtAV {

VideoFilterThread::VideoFilterThread(QObject *parent)
    : QThread(parent)
    , m_seeking(false)
    , m_filter_context(VideoFilterContext::create(VideoFilterContext::QtPainter))
{
    m_queue.setCapacity(3);
    m_queue.setThreshold(1);
}

VideoFilterThread::~VideoFilterThread()
{
    //not neccesary context is managed by filters.
    if (m_filter_context) {
        delete m_filter_context;
        m_filter_context = 0;
    }
}

void VideoFilterThread::waitAndCheck(ulong value, qreal pts)
{
    if (value <= 0)
        return;
    //qDebug("wating for %lu msecs", value);
    ulong us = value * 1000UL;
    static const ulong kWaitSlice = 10 * 1000UL; //20ms
    while (us > kWaitSlice) {
        usleep(kWaitSlice);
        if (m_stop)
            us = 0;
        else
            us -= kWaitSlice;
        if (pts > 0)
            us = qMin(us, ulong((double)(qMax<qreal>(0, pts - 0.02 - m_clock->value()))*1000000.0));
        //qDebug("us: %ul, pts: %f, clock: %f", us, pts, d.clock->value());
    }
    if (us > 0) {
        usleep(us);
    }
}

// filters on vo will not change video frame, so it's safe to protect frame only in every individual vo
bool VideoFilterThread::deliverVideoFrame(VideoFrame &frame)
{
    /*
     * TODO: video renderers sorted by preferredPixelFormat() and convert in AVOutputSet.
     * Convert only once for the renderers has the same preferredPixelFormat().
     */
    m_outset->lock();
    QList<AVOutput *> outputs = m_outset->outputs();
    if (outputs.size() > 1) { //FIXME!
        VideoFrame outFrame(m_conv.convert(frame, VideoFormat::Format_RGB32));
        if (!outFrame.isValid()) {
            /*
             * use VideoFormat::Format_User to deliver user defined frame
             * renderer may update background but no frame to graw, so flickers
             * may crash for some renderer(e.g. d2d) without validate and render an invalid frame
             */
            m_outset->unlock();
            return false;
        }
        frame = outFrame;
    } else {
        VideoRenderer *vo = 0;
        if (!outputs.isEmpty())
            vo = static_cast<VideoRenderer*>(outputs.first());
        if (vo && (!vo->isSupported(frame.pixelFormat())
                || (vo->isPreferredPixelFormatForced() && vo->preferredPixelFormat() != frame.pixelFormat())
                )) {
            VideoFrame outFrame(m_conv.convert(frame, vo->preferredPixelFormat()));
            if (!outFrame.isValid()) {
                /*
                 * use VideoFormat::Format_User to deliver user defined frame
                 * renderer may update background but no frame to graw, so flickers
                 * may crash for some renderer(e.g. d2d) without validate and render an invalid frame
                 */
                m_outset->unlock();
                return false;
            }
            frame = outFrame;
        }
    }
    m_outset->sendVideoFrame(frame); //TODO: group by format, convert group by group
    m_outset->unlock();
    //m_clock->updateVideoTime(frame.timestamp());
    Q_EMIT frameDelivered();
    return true;
}

void VideoFilterThread::applyFilters(VideoFrame &frame)
{
    QMutexLocker locker(&m_mutex); //for capture, and filters update
    Q_UNUSED(locker);
    if (!m_filters.isEmpty()) {
        //sort filters by format. vo->defaultFormat() is the last
        foreach (Filter *filter, m_filters) {
            VideoFilter *vf = static_cast<VideoFilter*>(filter);
            if (!vf->isEnabled())
                continue;
            if (vf->prepareContext(m_filter_context, m_statistics, &frame))
                vf->apply(m_statistics, &frame);
            else
                qWarning("prepare context error");
        }
    }
}

void VideoFilterThread::run()
{
    qreal v_a = 0;
    m_stop = false;
    m_seeking = false;
    while (!m_stop) {
        VideoFrame frame = m_queue.take();
        if (!frame) //eof frame
            break;
        applyFilters(frame);
        const qreal pts = frame.timestamp();
        qreal diff = pts > 0? pts - m_clock->value() + v_a : v_a;
        //qDebug("dt=%.3f", diff - v_a);
        if (diff > 1) {
            //check seeking and next frame
            ulong ms = diff*1000.0;
            while (ms > 0) {
                if (m_seeking) {
                    qDebug("seeking flag is set");
                    break;
                }
                // check m_seeking is not enough because m_seeking can set to false very quickly
                // peak the last frame in the queue
                if (!m_queue.isEmpty()) {
                    if (qAbs(m_queue.back().timestamp() - pts) > qAbs(diff)) //queue_size*diff
                        break;
                }
                const qreal t = m_clock->value();
                //qDebug("pts: %.3f, clock: %.3f", pts, t);
                if (pts <= t || t <= 0) { //t = 0: player stopped and clock is reset.
                    //qDebug("time to show");
                    ms = 0;
                    break;
                }
                waitAndCheck(10, pts);
                ms -= 10;
            }
            if (ms > 0) {
                qDebug("seeking detected");
                continue;
            }
        } else if (diff > 0) {
            waitAndCheck(diff*1000.0, pts);
        }
        deliverVideoFrame(frame);


        const qreal v_a_ = pts - m_clock->value();
        if (!qFuzzyIsNull(v_a_)) {
            if (v_a_ < -0.1) {
                if (v_a <= v_a_)
                    v_a += -0.01;
                else
                    v_a = (v_a_ +v_a)*0.5;
            } else if (v_a_ < -0.002) {
                v_a += -0.001;
            } else if (v_a_ < 0.002) {
            } else if (v_a_ < 0.1) {
                v_a += 0.001;
            } else {
                if (v_a >= v_a_)
                    v_a += 0.01;
                else
                    v_a = (v_a_ +v_a)*0.5;
            }

            if (v_a < -2 || v_a > 2)
               v_a /= 2.0;
        }
        //qDebug("diff: %.3f, v_a: %.3f, v_a_:%.3f @%.3f", diff, v_a, v_a_, pts);
    }
    qDebug("filter thread finished");
    m_outset->sendVideoFrame(VideoFrame()); // TODO: let user decide what to display
}

} //namespace QtAV
