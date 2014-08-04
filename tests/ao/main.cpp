/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2014 Wang Bin <wbsecg1@gmail.com>

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

#include <cmath>
#include <limits>
#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QStringList>
#include <QtAV/AudioOutput.h>
#include <QtAV/AudioOutputTypes.h>
#include <QtDebug>

using namespace QtAV;
const int kTableSize = 200;
const int kFrames = 1024;
qint16 sin_table[kTableSize];

void help() {
    QStringList backends;
    std::vector<std::string> names = AudioOutputFactory::registeredNames();
    for (int i = 0; i < (int)names.size(); ++i) {
        backends.append(names[i].c_str());
    }
    qDebug() << "parameters: [-ao " << backends.join("|") << "]";
}

int main(int argc, char** argv)
{
    help();

    /* initialise sinusoidal wavetable */
    for(int i=0; i<kTableSize; i++) {
        sin_table[i] = (qint16)((double)std::numeric_limits<qint16>::max() * sin(((double)i/(double)kTableSize)*3.1415926*2.0));
    }

    QCoreApplication app(argc, argv); //only used qapp to get parameter easily
    AudioOutputId aid = AudioOutputId_OpenAL;
    int idx = app.arguments().indexOf("-ao");
    if (idx > 0)
        aid = AudioOutputFactory::id(app.arguments().at(idx+1).toUtf8().constData(), false);
    if (!aid) {
        qWarning("unknow backend");
        return -1;
    }
    AudioOutput *ao = AudioOutputFactory::create(aid);
    AudioFormat af;
    af.setChannels(2);
    af.setSampleFormat(AudioFormat::SampleFormat_Signed16);
    af.setSampleRate(44100);
    if (!ao->isSupported(af)) {
        qDebug() << "does not support format: " << af;
        return -1;
    }
    ao->setAudioFormat(af);
    QByteArray data(af.bytesPerFrame()*kFrames, 0); //bytesPerSample*channels*1024
    ao->setBufferSize(data.size());
    if (!ao->open()) {
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
        ao->waitForNextBuffer();
        ao->receiveData(data);
        ao->play();
    }
    ao->close();
    return 0;
}
