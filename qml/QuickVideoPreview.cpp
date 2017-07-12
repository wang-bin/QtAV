/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

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

#include "QmlAV/QuickVideoPreview.h"
#include <QtCore/QRectF>

namespace QtAV {

QuickVideoPreview::QuickVideoPreview(QQuickItem *parent) : BaseQuickRenderer(parent)
{
    connect(&m_extractor, SIGNAL(positionChanged()), this, SIGNAL(timestampChanged()));
    connect(&m_extractor, SIGNAL(frameExtracted(QtAV::VideoFrame)), SLOT(displayFrame(QtAV::VideoFrame)));
    connect(&m_extractor, SIGNAL(error(const QString &)), SLOT(displayNoFrame()));
    connect(&m_extractor, SIGNAL(aborted(const QString &)), SLOT(displayNoFrame()));
    connect(this, SIGNAL(fileChanged()), SLOT(displayNoFrame()));
}

void QuickVideoPreview::setTimestamp(int value)
{
    m_extractor.setPosition((qint64)value);
}

int QuickVideoPreview::timestamp() const
{
    return (int)m_extractor.position();
}

void QuickVideoPreview::setFile(const QUrl &value)
{
    if (m_file == value)
        return;
    m_file = value;
    emit fileChanged();
    m_extractor.setSource(QUrl::fromPercentEncoding(m_file.toEncoded()));
}

QUrl QuickVideoPreview::file() const
{
    return m_file;
}

void QuickVideoPreview::displayFrame(const QtAV::VideoFrame &frame)
{
    int diff = qAbs(qint64(frame.timestamp()*1000.0) - m_extractor.position());
    if (diff > m_extractor.precision()) {
        //qWarning("timestamp difference (%d/%lld) is too large! ignore", diff);
    }
    if (isOpenGL() || frame.imageFormat() != QImage::Format_Invalid) {
        receive(frame);
        return;
    }
    VideoFrame f(frame.to(VideoFormat::Format_RGB32, boundingRect().toRect().size()));
    if (!f.isValid())
        return;
    receive(f);
}

void QuickVideoPreview::displayNoFrame()
{
    receive(VideoFrame());
}

} //namespace QtAV
