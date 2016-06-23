/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>
    Initial QAVIOContext.cpp code is from Stefan Ladage <sladage@gmail.com>

*   This file is part of QtAV (from 2014)

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

extern bool RegisterMediaIOQIODevice_Man();
extern bool RegisterMediaIOQFile_Man();
extern bool RegisterMediaIOWinRT_Man();
void MediaIO::registerAll()
{
    static bool done = false;
    if (done)
        return;
    done = true;
    RegisterMediaIOQIODevice_Man();
    RegisterMediaIOQFile_Man();
#ifdef Q_OS_WINRT
    RegisterMediaIOWinRT_Man();
#endif
}

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
    if (!io->seek(offset, whence))
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
    // TODO: check protocol
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

#define IODATA_BUFFER_SIZE 32768 // TODO: user configurable
static const int kBufferSizeDefault = 32768;

void MediaIO::setBufferSize(int value)
{
    DPTR_D(MediaIO);
    if (d.buffer_size == value)
        return;
    d.buffer_size = value;
}

int MediaIO::bufferSize() const
{
    return d_func().buffer_size;
}

void* MediaIO::avioContext()
{
    DPTR_D(MediaIO);
    if (d.ctx)
        return d.ctx;
    // buffer will be released in av_probe_input_buffer2=>ffio_rewind_with_probe_data. always is? may be another context
    unsigned char* buf = (unsigned char*)av_malloc(IODATA_BUFFER_SIZE);
    // open for write if 1. SET 0 if open for read otherwise data ptr in av_read(data, ...) does not change
    const int write_flag = (accessMode() == Write) && isWritable();
    d.ctx = avio_alloc_context(buf, bufferSize() > 0 ? bufferSize() : kBufferSizeDefault, write_flag, this, &av_read, write_flag ? &av_write : NULL, &av_seek);
    // if seekable==false, containers that estimate duration from pts(or bit rate) will not seek to the last frame when computing duration
    // but it's still seekable if call seek outside(e.g. from demuxer)
    // TODO: isVariableSize: size = -real_size
    d.ctx->seekable = isSeekable() && !isVariableSize() ? AVIO_SEEKABLE_NORMAL : 0;
    return d.ctx;
}

void MediaIO::release()
{
    DPTR_D(MediaIO);
    if (!d.ctx)
        return;
    // avio_close is called by avformat_close_input. here we only allocate but no open
    av_freep(&d.ctx->buffer);
    av_freep(&d.ctx);
}
} //namespace QtAV
