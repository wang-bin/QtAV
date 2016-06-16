/******************************************************************************
    framereader:  this file is part of QtAV examples
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2016)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/
#include <cstdio>
#include <cstdlib>
#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QQueue>
#include <QtCore/QStringList>
#include <QtAV/FrameReader.h>
using namespace QtAV;
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    FrameReader r;
    r.setMedia(a.arguments().last());
    QQueue<qint64> t;
    int count = 0;
    qint64 t0 = QDateTime::currentMSecsSinceEpoch();
    while (r.readMore()) {
        while (r.hasEnoughVideoFrames()) {
            const VideoFrame f = r.getVideoFrame(); //TODO: if eof
            if (!f)
                continue;
            count++;
            //r.readMore();
            const qint64 now = QDateTime::currentMSecsSinceEpoch();
            const qint64 dt = now - t0;
            t.enqueue(now);
            printf("decode @%.3f count: %d, elapsed: %lld, fps: %.1f/%.1f\r", f.timestamp(), count, dt, count*1000.0/dt, t.size()*1000.0/(now - t.first()));fflush(0);
            if (t.size() > 10)
                t.dequeue();
        }
    }
    while (r.hasVideoFrame()) {
        const VideoFrame f = r.getVideoFrame();
        qDebug("pts: %.3f", f.timestamp());
    }
    qDebug("read done");
    return 0;
}

