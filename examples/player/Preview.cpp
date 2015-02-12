/******************************************************************************
    QtAV Player Demo:  this file is part of QtAV examples
    Copyright (C) 2015 Wang Bin <wbsecg1@gmail.com>

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

#include "Preview.h"
#include <QtAVWidgets>
using namespace QtAV;

Preview::Preview(QObject *parent)
    : VideoOutput(VideoRendererId_Widget, parent)
    // FIXME: opengl may crash, so use software renderer here
{
    //connect(&m_extractor, SIGNAL(positionChanged()), this, SIGNAL(timestampChanged()));
    connect(&m_extractor, SIGNAL(frameExtracted(QtAV::VideoFrame)), SLOT(displayFrame(QtAV::VideoFrame)));
    connect(&m_extractor, SIGNAL(error()), SLOT(displayNoFrame()));
    connect(this, SIGNAL(fileChanged()), SLOT(displayNoFrame()));
    widget()->resize(160, 90);
}

void Preview::setTimestamp(int value)
{
    m_extractor.setPosition((qint64)value);
}

int Preview::timestamp() const
{
    return (int)m_extractor.position();
}

void Preview::setFile(const QString &value)
{
    if (m_file == value)
        return;
    m_file = value;
    m_extractor.setSource(m_file);
}

QString Preview::file() const
{
    return m_file;
}

void Preview::displayFrame(const QtAV::VideoFrame &frame)
{
    int diff = qAbs(qint64(frame.timestamp()*1000.0) - m_extractor.position());
    if (diff > m_extractor.precision()) {
        //qWarning("timestamp difference (%d/%lld) is too large! ignore", diff);
    }
    if (isSupported(frame.format().pixelFormat())) {
        receive(frame);
        return;
    }
    VideoFrame f(frame.to(VideoFormat::Format_RGB32, widget()->rect().size()));
    if (!f.isValid())
        return;
    receive(f);
}

void Preview::displayNoFrame()
{
    receive(VideoFrame());
}

