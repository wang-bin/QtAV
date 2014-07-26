/******************************************************************************
    AVError.cpp: description
    Copyright (C) 2013 Wang Bin <wbsecg1@gmail.com>

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

    Alternatively, this file may be used under the terms of the GNU
    General Public License version 3.0 as published by the Free Software
    Foundation and appearing in the file LICENSE.GPL included in the
    packaging of this file.  Please review the following information to
    ensure the GNU General Public License version 3.0 requirements will be
    met: http://www.gnu.org/copyleft/gpl.html.
******************************************************************************/


#include "QtAV/AVError.h"
#include "QtAV/private/AVCompat.h"
#ifndef QT_NO_DEBUG_STREAM
#include <QtCore/qdebug.h>
#endif

namespace QtAV {

namespace
{
class RegisterMetaTypes
{
public:
    RegisterMetaTypes()
    {
        qRegisterMetaType<QtAV::AVError>("QtAV::AVError");
    }
} _registerMetaTypes;
}

AVError::AVError()
    : mError(NoError)
    , mFFmpegError(0)
{
}

AVError::AVError(ErrorCode code, int ffmpegError)
    : mError(code)
    , mFFmpegError(ffmpegError)
{
}

AVError::AVError(const AVError& other)
    : mError(other.mError)
    , mFFmpegError(other.mFFmpegError)
{
}

AVError& AVError::operator=(const AVError& other)
{
    mError = other.mError;
    mFFmpegError = other.mFFmpegError;
    return *this;
}

bool AVError::operator==(const AVError& other) const
{
    return (mError == other.mError && mFFmpegError == other.mFFmpegError);
}

void AVError::setError(ErrorCode ec)
{
    mError = ec;
}

AVError::ErrorCode AVError::error() const
{
    return mError;
}

QString AVError::string() const
{
    QString errStr;
    switch (mError) {
    case NoError:
        errStr = "No error";
        break;
    case OpenError:
        errStr = "Open error";
        break;
    case OpenTimedout:
        errStr = "Open timed out";
        break;
    case FindStreamInfoError:
        errStr = "Could not find stream info";
        break;
    case StreamNotFound:
        errStr = "Stream not found";
        break;
    case ReadTimedout:
        errStr = "Read packet timed out";
        break;
    case ReadError:
        errStr = "Read error";
        break;
    case SeekError:
        errStr = "Seek error";
        break;
    case OpenCodecError:
        errStr = "Open codec error";
        break;
    case CloseCodecError:
        errStr = "Close codec error";
        break;
    case DecodeError:
        errStr = "Decode error";
        break;
    case ResampleError:
        errStr = "Resample error";
        break;
    default:
        errStr = "Unknow error";
        break;
    }

    if (mFFmpegError != 0) {
        errStr += QString(" (FFmpeg %1: %2)").arg(mFFmpegError, 0, 16).arg(ffmpegErrorString());
    }
    return errStr;
}

int AVError::ffmpegErrorCode() const
{
    return mFFmpegError;
}

QString AVError::ffmpegErrorString() const
{
    if (mFFmpegError == 0)
        return QString();
    return av_err2str(mFFmpegError);
}

} //namespace QtAV


#ifndef QT_NO_DEBUG_STREAM
//class QDebug;
QDebug operator<<(QDebug debug, const QtAV::AVError &error)
{
    debug << error.string();
    return debug;
}
#endif
