/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2014 Wang Bin <wbsecg1@gmail.com>
    Copyright (C) 2014 Stefan Ladage <sladage@gmail.com>

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

#ifndef QTAV_AVIOCONTEXT_H
#define QTAV_AVIOCONTEXT_H

#include "QtAV/QtAV_Compat.h"
//struct AVIOContext; //anonymous struct in FFmpeg1.0.x
class QIODevice;

#define IODATA_BUFFER_SIZE 32768

namespace QtAV {

class QAVIOContext
{
public:
    QAVIOContext(QIODevice* io);
    ~QAVIOContext();

    AVIOContext* context();

    QIODevice* device() const;
    void setDevice(QIODevice* device);

private:
    unsigned char* m_ucDataBuffer;
    QIODevice* m_pIO;
};

}
#endif // QTAV_AVIOCONTEXT_H
