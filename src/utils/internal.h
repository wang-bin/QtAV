/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2015 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_INTERNAL_H
#define QTAV_INTERNAL_H

#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtCore/QObject>
#include "QtAV/private/AVCompat.h"

namespace QtAV {
namespace Internal {

QString optionsToString(void* obj);
void setOptionsToFFmpegObj(const QVariant& opt, void* obj);
void setOptionsToDict(const QVariant& opt, AVDictionary** dict);
// set qobject meta properties
void setOptionsForQObject(const QVariant& opt, QObject* obj);
} //namespace Internal
} //namespace QtAV
#endif //QTAV_INTERNAL_H
