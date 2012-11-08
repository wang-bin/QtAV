/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QAV_AUDIOWRITER_H
#define QAV_AUDIOWRITER_H

#include <QtAV/AVOutput.h>

struct PaStreamParameters;
typedef void PaStream;

namespace QtAV {

class Q_EXPORT AudioOutput : public AVOutput
{
public:
    AudioOutput();
    virtual ~AudioOutput();

    void setSampleRate(int rate);
    void setChannels(int channels);
    int write(const QByteArray& data);

    virtual bool open();
    virtual bool close();

    void setVolume(qreal volume); //double?
    qreal volume() const;
    void setMute(bool yes);
    bool isMute() const;

protected:
    bool mute;
    qreal vol;
    PaStreamParameters *outputParameters;
    PaStream *stream;
    int sample_rate;
    double outputLatency;
#ifdef Q_OS_LINUX
    bool autoFindMultichannelDevice;
#endif
};

} //namespace QtAV
#endif // QAV_AUDIOWRITER_H
