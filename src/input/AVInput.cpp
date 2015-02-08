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

#include "QtAV/AVInput.h"
#include "QtAV/private/AVInput_p.h"
#include "QtAV/private/factory.h"
#include <QtCore/QStringList>

namespace QtAV {

FACTORY_DEFINE(AVInput)

QStringList AVInput::builtInNames()
{
    static QStringList names;
    if (!names.isEmpty())
        return names;
    std::vector<std::string> stdnames(AVInputFactory::registeredNames());
    foreach (const std::string stdname, stdnames) {
        names.append(stdname.c_str());
    }
    return names;
}

// TODO: plugin
AVInput* AVInput::create(const QString &name)
{
    return AVInputFactory::create(AVInputFactory::id(name.toStdString()));
}
// TODO: plugin use metadata(Qt plugin system) to avoid loading
AVInput* AVInput::createForProtocol(const QString &protocol)
{
    std::vector<AVInputId> ids(AVInputFactory::registeredIds());
    foreach (AVInputId id, ids) {
        AVInput *in = AVInputFactory::create(id);
        if (in->protocols().contains(protocol))
            return in;
        delete in;
    }
    return 0;
}

static int av_read(void *opaque, unsigned char *buf, int buf_size)
{
    AVInput* input = static_cast<AVInput*>(opaque);
    return input->read((char*)buf, buf_size);
}

static int64_t av_seek(void *opaque, int64_t offset, int whence)
{
    if (whence == SEEK_SET && offset < 0)
        return -1;
    AVInput* input = static_cast<AVInput*>(opaque);
    if (!input->isSeekable())
        return -1;
    if (whence == AVSEEK_SIZE) {
        // return the filesize without seeking anywhere. Supporting this is optional.
        return input->size() > 0 ? input->size() : 0;
    }
    int from = whence;
    if (whence == SEEK_SET)
        from = 0;
    else if (whence == SEEK_CUR)
        from = 1;
    else if (whence == SEEK_END)
        from = 2;
    if (!input->seek(offset, from))
        return -1;
    return input->position();
}

AVInput::AVInput()
    : QObject(0)
{}

AVInput::AVInput(QObject *parent)
    : QObject(parent)
{}

AVInput::AVInput(AVInputPrivate &d, QObject *parent)
    : QObject(parent)
    , DPTR_INIT(&d)
{}

AVInput::~AVInput()
{
    release();
}

void AVInput::setUrl(const QString &url)
{
    DPTR_D(AVInput);
    if (d.url == url)
        return;
    d.url = url;
    onUrlChanged();
}

QString AVInput::url() const
{
    return d_func().url;
}

void AVInput::onUrlChanged()
{}

const QStringList& AVInput::protocols() const
{
    static QStringList no_protocols;
    return no_protocols;
}

#define IODATA_BUFFER_SIZE 32768 //

void* AVInput::avioContext()
{
    DPTR_D(AVInput);
    // buffer will be released in av_probe_input_buffer2=>ffio_rewind_with_probe_data. always is? may be another context
    unsigned char* buf = (unsigned char*)av_malloc(IODATA_BUFFER_SIZE);
    d.ctx = avio_alloc_context(buf, IODATA_BUFFER_SIZE, 0, this, &av_read, 0, &av_seek);
    d.ctx->seekable = isSeekable() ? 0 : AVIO_SEEKABLE_NORMAL;
    return d.ctx;
}

void AVInput::release()
{
    DPTR_D(AVInput);
    if (!d.ctx)
        return;
    d.ctx->opaque = 0; //in avio_close() opaque is URLContext* and will call ffurl_close()
    //d.ctx->buffer = 0; //already released by ffio_rewind_with_probe_data; may be another context was freed
    avio_close(d.ctx); //avio_closep defined since ffmpeg1.1
    d.ctx = 0;
}

} //namespace QtAV
