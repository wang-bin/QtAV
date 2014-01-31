/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2014 Stefan Ladage <sladage@gmail.com>

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

#include <QtAV/QAVIOContext.h>
#include <QIODevice>
#include <QtAV/QtAV_Compat.h>
#include <QDebug>

int read(void *opaque, unsigned char *buf, int buf_size)
{
    QtAV::QAVIOContext* avio = static_cast<QtAV::QAVIOContext*>(opaque);
    //qDebug() << "read" << buf_size << avio->m_pIO->pos() << IODATA_BUFFER_SIZE;
    return avio->device()->read((char*)buf,buf_size);
}
/*
int write(void *opaque, unsigned char *buf, int buf_size)
{
    QtAV::QAVIOContext* avio = static_cast<QtAV::QAVIOContext*>(opaque);
    //qDebug() << "write";
    return avio->device()->write((char*)buf,buf_size);
}
*/
int64_t seek(void *opaque, int64_t offset, int whence)
{
    QtAV::QAVIOContext* avio = static_cast<QtAV::QAVIOContext*>(opaque);
    //qDebug() << "seek";
    int i = 0;

    if (whence == SEEK_END)
    {
        offset = avio->device()->size() - offset;
    }
    else if (whence == SEEK_CUR)
    {
        offset = avio->device()->pos() + offset;
    }
    else if (whence == AVSEEK_SIZE)
    {
        return avio->device()->size();
    }

    if (!avio->device()->seek(offset))
        i=-1;
    else
        i=offset;
    return i;
}

namespace QtAV {

QAVIOContext::QAVIOContext(QIODevice *io) : m_pIO(io)
{
    m_ucDataBuffer = new unsigned char[IODATA_BUFFER_SIZE];
}

QAVIOContext::~QAVIOContext()
{
    delete [] m_ucDataBuffer;
}

AVIOContext* QAVIOContext::context()
{
    return avio_alloc_context(m_ucDataBuffer,IODATA_BUFFER_SIZE,0,this,&read,0,&seek);
}

QIODevice* QAVIOContext::device() const
{
    return m_pIO;
}

void QAVIOContext::setDevice(QIODevice *device)
{
    m_pIO = device;
}

}
