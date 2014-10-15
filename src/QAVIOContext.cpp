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

#include "QAVIOContext.h"
#include "QtAV/private/AVCompat.h"
#include <QtCore/QIODevice>
#include "utils/Logger.h"

namespace QtAV {

static int read(void *opaque, unsigned char *buf, int buf_size)
{
    QtAV::QAVIOContext* avio = static_cast<QtAV::QAVIOContext*>(opaque);
    //qDebug() << "read" << buf_size << avio->m_pIO->pos() << IODATA_BUFFER_SIZE;
    return avio->device()->read((char*)buf, buf_size);
}
/*
static int write(void *opaque, unsigned char *buf, int buf_size)
{
    QtAV::QAVIOContext* avio = static_cast<QtAV::QAVIOContext*>(opaque);
    //qDebug() << "write";
    return avio->device()->write((char*)buf,buf_size);
}
*/
static int64_t seek(void *opaque, int64_t offset, int whence)
{
    QtAV::QAVIOContext* avio = static_cast<QtAV::QAVIOContext*>(opaque);
    //qDebug() << "seek";
    int i = 0;

    if (whence == SEEK_END) {
        offset = avio->device()->size() - offset;
    } else if (whence == SEEK_CUR) {
        offset = avio->device()->pos() + offset;
    } else if (whence == AVSEEK_SIZE) {
        return avio->device()->size();
    }

    if (!avio->device()->seek(offset))
        i = -1;
    else
        i = offset;
    return i;
}

QAVIOContext::QAVIOContext(QIODevice *io) : m_pIO(io)
  , m_avio(0)
{
}

QAVIOContext::~QAVIOContext()
{
}

AVIOContext* QAVIOContext::context()
{
    // buffer will be released in av_probe_input_buffer2=>ffio_rewind_with_probe_data. always is? may be another context
    unsigned char* m_ucDataBuffer = (unsigned char*)av_malloc(IODATA_BUFFER_SIZE);
    m_avio = avio_alloc_context(m_ucDataBuffer, IODATA_BUFFER_SIZE, 0, this, &read, 0, &seek);
    return m_avio;
}

void QAVIOContext::release()
{
    if (!m_avio)
        return;
    m_avio->opaque = 0; //in avio_close() opaque is URLContext* and will call ffurl_close()
    //m_avio->buffer = 0; //already released by ffio_rewind_with_probe_data; may be another context was freed
    avio_close(m_avio); //avio_closep defined since ffmpeg1.1
    m_avio = 0;
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
