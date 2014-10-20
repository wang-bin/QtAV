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

        //open/read/seek network stream error. value must be less then ResourceError because of correct_error_by_ffmpeg
        NetworkError, // all above and before NoError are NetworkError

        OpenTimedout,
        OpenError,
        FindStreamInfoTimedout,
        FindStreamInfoError,
        StreamNotFound, //a,v,s?
        ReadTimedout,
        ReadError,
        SeekError,
        ResourceError, // all above and before NetworkError are ResourceError

        OpenCodecError,
        CloseCodecError,
        AudioCodecNotFound,
        VideoCodecNotFound,
        SubtitleCodecNotFound,
        CodecError, // all above and before NoError are CodecError

        FormatError, // all above and before CodecError are FormatError

        // decrypt error. Not implemented
        AccessDenied, // all above and before NetworkError are AccessDenied

        UnknowError
    };

    AVError();
    AVError(ErrorCode code, int ffmpegError = 0);
    /*!
     * \brief AVError
     * string() will be detail. If ffmpeg error not 0, also contains ffmpegErrorString()
     * \param code ErrorCode value
     * \param detail ErrorCode string will be overrided by detail.
     * \param ffmpegError ffmpeg error code. If not 0, string() will contains ffmpeg error string.
     */
    AVError(ErrorCode code, const QString& detail, int ffmpegError = 0);
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
    QString mDetail;
};

} //namespace QtAV

Q_DECLARE_METATYPE(QtAV::AVError)

#ifndef QT_NO_DEBUG_STREAM
class QDebug;
Q_AV_EXPORT QDebug operator<<(QDebug debug, const QtAV::AVError &error);
#endif

#endif // QTAV_AVERROR_H
