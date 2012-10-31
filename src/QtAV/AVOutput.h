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

#ifndef QAV_WRITER_H
#define QAV_WRITER_H

#include <qbytearray.h>
#include <QtAV/QtAV_Global.h>

namespace QtAV {

class AVDecoder;
class AVOutputPrivate;
class Q_EXPORT AVOutput
{
public:
    AVOutput();
    virtual ~AVOutput() = 0;
    virtual int write(const QByteArray& data);// = 0; //TODO: why pure may case "pure virtual method called"
    virtual bool open() = 0;
    virtual bool close() = 0;
    void bindDecoder(AVDecoder *dec); //for resizing video (or resample audio?)

protected:
    AVOutput(AVOutputPrivate& d);

    Q_DECLARE_PRIVATE(AVOutput)
    AVOutputPrivate *d_ptr; //TODO: ambiguous with Qt class d_ptr when it is a base class together with Qt classes
};

} //namespace QtAV
#endif //QAV_WRITER_H
