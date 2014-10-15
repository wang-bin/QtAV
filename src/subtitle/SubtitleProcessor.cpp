/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2014 Wang Bin <wbsecg1@gmail.com>

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

#include "QtAV/private/SubtitleProcessor.h"
#include "QtAV/FactoryDefine.h"
#include "QtAV/private/factory.h"
#include <QtCore/QFile>
#include "utils/Logger.h"

namespace QtAV {

FACTORY_DEFINE(SubtitleProcessor)

SubtitleProcessor::SubtitleProcessor()
    : m_width(0)
    , m_height(0)
{}

bool SubtitleProcessor::process(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "open subtitle file error: " << f.errorString();
        return false;
    }
    bool ok = process(&f);
    f.close();
    return ok;
}

QImage SubtitleProcessor::getImage(qreal pts, QRect *boundingRect)
{
    Q_UNUSED(pts)
    Q_UNUSED(boundingRect)
    return QImage();
}

void SubtitleProcessor::setFrameSize(int width, int height)
{
    if (width == m_width && height == m_height)
        return;
    m_width = width;
    m_height = height;
    onFrameSizeChanged(m_width, m_height);
}

QSize SubtitleProcessor::frameSize() const
{
    return QSize(m_width, m_height);
}

void SubtitleProcessor::onFrameSizeChanged(int width, int height)
{
    Q_UNUSED(width);
    Q_UNUSED(height);
}

} //namespace QtAV
