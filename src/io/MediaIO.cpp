/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2014-2015 Wang Bin <wbsecg1@gmail.com>
    Initial QAVIOContext.cpp code is from Stefan Ladage <sladage@gmail.com>

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

#include "QtAV/MediaIO.h"
#include "QtAV/private/MediaIO_p.h"
#include "QtAV/private/factory.h"
#include <QtCore/QStringList>
#include "utils/Logger.h"

namespace QtAV {

FACTORY_DEFINE(MediaIO)

QStringList MediaIO::builtInNames()
{
    static QStringList names;
    if (!names.isEmpty())
        return names;
    std::vector<const char*> ns(MediaIOFactory::Instance().registeredNames());
    foreach (const char* n, ns) {
        names.append(QLatin1String(n));
    }
    return names;
}

// TODO: plugin

// TODO: plugin use metadata(Qt plugin system) to avoid loading
MediaIO* MediaIO::createForProtocol(const QString &protocol)
{
    std::vector<MediaIOId> ids(MediaIOFactory::Instance().registeredIds());
    foreach (MediaIOId id, ids) {
        MediaIO *in = MediaIO::create(id);
        if (in->protocols().contains(protocol))
            return in;
        delete in;
    }
    return 0;
}

MediaIO* MediaIO::createForUrl(const QString &url)
{
    const int p = url.indexOf(QLatin1String(":"));
    if (p < 0)
        return 0;
    MediaIO *io = MediaIO::createForProtocol(url.left(p));
    if (!io)
        return 0;
    io->setUrl(url);
    return io;
}

static int av_read(void *opaque, unsigned char *buf, int buf_size)
{
    MediaIO* io = static_cast<MediaIO*>(opaque);
    return io->read((char*)buf, buf_size);
}

static int av_write(void *opaque, unsigned char *buf, int buf_size)
{
    MediaIO* io = static_cast<MediaIO*>(opaque);
    return io->write((const char*)buf, buf_size);
}

static int64_t av_seek(void *opaque, int64_t offset, int whence)
{
    if (whence == SEEK_SET && offset < 0)
        return -1;
    MediaIO* io = static_cast<MediaIO*>(opaque);
    if (!io->isSeekable()) {
        qWarning("Can not seek. MediaIO[%s] is not a seekable IO", MediaIO::staticMetaObject.className());
        return -1;
    }
    if (whence == AVSEEK_SIZE) {
        // return the filesize without seeking anywhere. Supporting this is optional.
        return io->size() > 0 ? io->size() : 0;
    }
    int from = whence;
    if (whence == SEEK_SET)
        from = 0;
    else if (whence == SEEK_CUR)
        from = 1;
    else if (whence == SEEK_END)
        from = 2;
    if (!io->seek(offset, from))
        return -1;
    return io->position();
}

MediaIO::MediaIO(QObject *parent)
    : QObject(parent)
{}

MediaIO::MediaIO(MediaIOPrivate &d, QObject *parent)
    : QObject(parent)
    , DPTR_INIT(&d)
{}

MediaIO::~MediaIO()
{
    release();
}

void MediaIO::setUrl(const QString &url)
{
    DPTR_D(MediaIO);
    if (d.url == url)
        return;
    d.url = url;
    onUrlChanged();
}

QString MediaIO::url() const
{
    return d_func().url;
}

void MediaIO::onUrlChanged()
{}

bool MediaIO::setAccessMode(AccessMode value)
{
    DPTR_D(MediaIO);
    if (d.mode == value)
        return true;
    if (value == Write && !isWritable()) {
        qWarning("Can not set Write access mode to this MediaIO");
        return false;
    }
    d.mode = value;
    return true;
}

MediaIO::AccessMode MediaIO::accessMode() const
{
    return d_func().mode;
}

const QStringList& MediaIO::protocols() const
{
    static QStringList no_protocols;
    return no_protocols;
}

#define IODATA_BUFFER_SIZE 32768 //

void* MediaIO::avioContext()
{
    DPTR_D(MediaIO);
    // buffer will be released in av_probe_input_buffer2=>ffio_rewind_with_probe_data. always is? may be another context
    unsigned char* buf = (unsigned char*)av_malloc(IODATA_BUFFER_SIZE);
    // open for write if 1. SET 0 if open for read otherwise data ptr in av_read(data, ...) does not change
    const int write_flag = (accessMode() == Write) && isWritable();
    d.ctx = avio_alloc_context(buf, IODATA_BUFFER_SIZE, write_flag, this, &av_read, write_flag ? &av_write : NULL, &av_seek);
    // if seekable==false, containers that estimate duration from pts(or bit rate) will not seek to the last frame when computing duration
    // but it's still seekable if call seek outside(e.g. from demuxer)
    d.ctx->seekable = isSeekable() && !isVariableSize() ? AVIO_SEEKABLE_NORMAL : 0;
    return d.ctx;
}

void MediaIO::release()
{
    DPTR_D(MediaIO);
    if (!d.ctx)
        return;
    d.ctx->opaque = 0; //in avio_close() opaque is URLContext* and will call ffurl_close()
    //d.ctx->buffer = 0; //already released by ffio_rewind_with_probe_data; may be another context was freed
    avio_close(d.ctx); //avio_closep defined since ffmpeg1.1
    d.ctx = 0;
}

} //namespace QtAV
