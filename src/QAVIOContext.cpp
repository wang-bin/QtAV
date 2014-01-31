#include <QtAV/QAVIOContext.h>
#include <QIODevice>
#include <QtAV/QtAV_Compat.h>
#include <QDebug>

namespace QtAV {

QAVIOContext::QAVIOContext(QIODevice *io) : m_pIO(io)
{
    m_ucDataBuffer = new unsigned char[IODATA_BUFFER_SIZE];
}

QAVIOContext::~QAVIOContext()
{
    delete m_ucDataBuffer;
}

int QAVIOContext::read(void *opaque, unsigned char *buf, int buf_size)
{
    QAVIOContext* avio = static_cast<QAVIOContext*>(opaque);
    //qDebug() << "read" << buf_size << avio->m_pIO->pos() << IODATA_BUFFER_SIZE;
    return avio->m_pIO->read((char*)buf,buf_size);
}

int QAVIOContext::write(void *opaque, unsigned char *buf, int buf_size)
{
    QAVIOContext* avio = static_cast<QAVIOContext*>(opaque);
    //qDebug() << "write";
    return avio->m_pIO->write((char*)buf,buf_size);
}

int64_t QAVIOContext::seek(void *opaque, int64_t offset, int whence)
{
    QAVIOContext* avio = static_cast<QAVIOContext*>(opaque);
    //qDebug() << "seek";
    int i = 0;

    if (whence == SEEK_END)
    {
        offset = avio->m_pIO->size() - offset;
    }
    else if (whence == SEEK_CUR)
    {
        offset = avio->m_pIO->pos() + offset;
    }
    else if (whence == AVSEEK_SIZE)
    {
        return avio->m_pIO->size();
    }

    if (!avio->m_pIO->seek(offset))
        i=-1;
    else
        i=offset;
    return i;
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
