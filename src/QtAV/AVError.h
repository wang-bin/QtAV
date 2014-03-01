/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_AVERROR_H
#define QTAV_AVERROR_H

#include <QtAV/QtAV_Global.h>
#include <QtCore/QString>
#include <QtCore/QMetaType>

namespace QtAV {


class Q_AV_EXPORT AVError
{
public:
    enum ErrorCode {
        NoError,
        OpenTimedout,
        OpenError,

        FindStreamInfoTimedout,
        FindStreamInfoError,
        StreamNotFound, //a,v,s?
        ReadTimedout,
        ReadError,
        SeekError,
        OpenCodecError,
        CloseCodecError,
        DecodeError,
        ResampleError,

        UnknowError
    };

    AVError();
    AVError(ErrorCode code, int ffmpegError = 0);
    AVError(const AVError& other);

    AVError &operator=(const AVError &other);
    bool operator==(const AVError &other) const;
    inline bool operator!=(const AVError &other) const
    { return !(*this == other); }

    void setError(ErrorCode ec);
    ErrorCode error() const;
    QString string() const;

    int ffmpegErrorCode() const;
    QString ffmpegErrorString() const;

private:
    ErrorCode mError;
    int mFFmpegError;
};

} //namespace QtAV

Q_DECLARE_METATYPE(QtAV::AVError)

#ifndef QT_NO_DEBUG_STREAM
class QDebug;
Q_AV_EXPORT QDebug operator<<(QDebug debug, const QtAV::AVError &error);
#endif

#endif // QTAV_AVERROR_H
