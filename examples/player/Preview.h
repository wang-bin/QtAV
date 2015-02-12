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

#ifndef PREVIEW_H
#define PREVIEW_H

#include <QtAV/VideoFrameExtractor.h>
#include <QtAV/VideoOutput.h>

class Preview : public QtAV::VideoOutput
{
    Q_OBJECT
public:
    Preview(QObject *parent = 0);
    void setTimestamp(int value);
    int timestamp() const;
    void setFile(const QString& value);
    QString file() const;

private slots:
    void displayFrame(const QtAV::VideoFrame& frame); //parameter VideoFrame
    void displayNoFrame();

private:
    QString m_file;
    QtAV::VideoFrameExtractor m_extractor;
};

#endif // PREVIEW_H
