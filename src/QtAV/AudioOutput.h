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

#ifndef QAV_AUDIOOUTPUT_H
#define QAV_AUDIOOUTPUT_H

#include <QtAV/AVOutput.h>

namespace QtAV {

class AudioOutputPrivate;
class Q_EXPORT AudioOutput : public AVOutput
{
    DPTR_DECLARE_PRIVATE(AudioOutput)
public:
    AudioOutput();
    virtual ~AudioOutput() = 0;

    void setSampleRate(int rate);
    int sampleRate() const;

    void setChannels(int channels);
    int channels() const;

    void setVolume(qreal volume);
    qreal volume() const;
    void setMute(bool yes);
    bool isMute() const;

protected:
    AudioOutput(AudioOutputPrivate& d);
	virtual void convertData(const QByteArray &data);
};

} //namespace QtAV
#endif // QAV_AUDIOOUTPUT_H
