/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2014-2015 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_MediaIO_P_H
#define QTAV_MediaIO_P_H

#include "QtAV/QtAV_Global.h"
#include "QtAV/private/AVCompat.h"
#include <QtCore/QString>
#include "QtAV/MediaIO.h"

namespace QtAV {

class MediaIO;
class Q_AV_PRIVATE_EXPORT MediaIOPrivate : public DPtrPrivate<MediaIO>
{
public:
    MediaIOPrivate()
        : ctx(0)
        , mode(MediaIO::Read)
    {}
    // TODO: how to manage ctx?
    AVIOContext *ctx;
    MediaIO::AccessMode mode;
    QString url;
};

} //namespace QtAV
#endif // QTAV_MediaIO_P_H
