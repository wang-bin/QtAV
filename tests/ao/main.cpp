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

#include <limits>
#include <QtCore/qmath.h>
#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QStringList>
#include <QtAV/AudioOutput.h>
#include <QtDebug>

using namespace QtAV;
const int kTableSize = 200;
const int kFrames = 512;
qint16 sin_table[kTableSize];

void help() {
    qDebug() << QLatin1String("parameters: [-ao ") << AudioOutput::backendsAvailable().join(QLatin1String("|")) << QLatin1String("]");
}

int main(int argc, char** argv)
{
    help();

    /* initialise sinusoidal wavetable */
    for(int i=0; i<kTableSize; i++) {
        sin_table[i] = (qint16)((double)std::numeric_limits<qint16>::max() * sin(((double)i/(double)kTableSize)*3.1415926*2.0));
    }

    QCoreApplication app(argc, argv); //only used qapp to get parameter easily
    AudioOutput ao;
    int idx = app.arguments().indexOf(QLatin1String("-ao"));
    if (idx > 0)
        ao.setBackends(QStringList() << app.arguments().at(idx+1));
    if (ao.backend().isEmpty()) {
        qWarning("unknow backend");
        return -1;
    }
    AudioFormat af;
    af.setChannels(2);
    af.setSampleFormat(AudioFormat::SampleFormat_Signed16);
    af.setSampleRate(44100);
    if (!ao.isSupported(af)) {
        qDebug() << "does not support format: " << af;
        return -1;
    }
    ao.setAudioFormat(af);
    QByteArray data(af.bytesPerFrame()*kFrames, 0); //bytesPerSample*channels*1024
    ao.setBufferSamples(kFrames);
    if (!ao.open()) {
        qWarning("open audio error");
        return -1;
    }
    int left =0, right = 0;
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 3000) {
        qint16 *d = (qint16*)data.data();
        for (int k = 0; k < kFrames; k++) {
            *d++ = sin_table[left];
            *d++ = sin_table[right];
            left = (left+1) % kTableSize;
            right = (right+3)% kTableSize;
        }
        ao.setVolume(2*sin(2.0*M_PI/1000.0*timer.elapsed()));
        ao.play(data);
    }
    ao.close();
    return 0;
}
