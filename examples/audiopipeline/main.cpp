/******************************************************************************
    audiopipeline:  this file is part of QtAV examples
    Copyright (C) 2015-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV

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
#include <QtAV>
#include <QtCore/QCoreApplication>
#include <QtCore/QScopedPointer>
#include <QtDebug>
using namespace QtAV;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    qDebug() << QLatin1String("usage: ") << a.applicationFilePath().split(QLatin1String("/")).last().append(QLatin1String(" url"));
    if (a.arguments().size() < 2)
        return 0;
    QScopedPointer<AudioOutput> ao(new AudioOutput());
    AVDemuxer demuxer;
    demuxer.setMedia(a.arguments().last());
    if (!demuxer.load()) {
        qWarning() << "Failed to load file " << demuxer.fileName();
        return 1;
    }
    QScopedPointer<AudioDecoder> dec(AudioDecoder::create()); // delete by user
    dec->setCodecContext(demuxer.audioCodecContext());
    //dec->prepare();
    if (!dec->open())
        qFatal("open decoder error");
    int astream = demuxer.audioStream();
    Packet pkt;
    while (!demuxer.atEnd()) {
        if (!pkt.isValid()) { // continue to decode previous undecoded data
            if (!demuxer.readFrame() || demuxer.stream() != astream)
                continue;
            pkt = demuxer.packet();
        }
        if (!dec->decode(pkt)) {
            pkt = Packet(); // set invalid to read from demuxer
            continue;
        }
        // decode the rest data in the next loop. read from demuxer if no data remains
        pkt.data = QByteArray::fromRawData(pkt.data.constData() + pkt.data.size() - dec->undecodedSize(), dec->undecodedSize());
        AudioFrame frame(dec->frame()); // why is faster to call frame() for hwdec? no frame() is very slow for VDA
        if (!frame)
            continue;
        //frame.setAudioResampler(dec->resampler()); // if not set, always create a resampler in AudioFrame.to()
        AudioFormat af(frame.format());
        if (ao->isOpen()) {
            af = ao->audioFormat();
        } else {
            ao->setAudioFormat(af);
            dec->resampler()->setOutAudioFormat(ao->audioFormat());
            // if decoded format is not supported by audio renderer, change decoder output format
            if (af != ao->audioFormat())
                dec->resampler()->prepare();
            // now af is supported by audio renderer. it's safe to open
            if (!ao->open())
                qFatal("Open audio output error");
#if 0 // always resample ONCE due to QtAV bug
            // the first format unsupported frame still need to be converted to a supported format
            if (!ao->isSupported(frame.format()))
                frame = frame.to(af);
#endif
            qDebug() << "Input: " << frame.format();
            qDebug() << "Output: " << af;
        }
        printf("playing: %.3f...\r", frame.timestamp());
        fflush(0);
        // always resample ONCE. otherwise data are all 0x0. QtAV bug
        frame = frame.to(af);
        QByteArray data(frame.data()); // plane data. currently only packet sample formats are supported.
        while (!data.isEmpty()) {
            ao->play(QByteArray::fromRawData(data.constData(), qMin(data.size(), ao->bufferSize())));
            data.remove(0, qMin(data.size(), ao->bufferSize()));
        }
    }
    // dec, ao will be closed in dtor. demuxer will call unload in dtor
    return 0;
}
