/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2013)

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

static AVError::ErrorCode errorFromFFmpeg(int fe)
{
    typedef struct {
        int ff;
        AVError::ErrorCode e;
    } err_entry;
    static const err_entry err_map[] = {
        { AVERROR_BSF_NOT_FOUND, AVError::FormatError },
#ifdef AVERROR_BUFFER_TOO_SMALL
        { AVERROR_BUFFER_TOO_SMALL, AVError::ResourceError },
#endif //AVERROR_BUFFER_TOO_SMALL
        { AVERROR_DECODER_NOT_FOUND, AVError::CodecError },
        { AVERROR_ENCODER_NOT_FOUND, AVError::CodecError },
        { AVERROR_DEMUXER_NOT_FOUND, AVError::FormatError },
        { AVERROR_MUXER_NOT_FOUND, AVError::FormatError },
        { AVERROR_PROTOCOL_NOT_FOUND, AVError::ResourceError },
        { AVERROR_STREAM_NOT_FOUND, AVError::ResourceError },
        { 0, AVError::UnknowError }
    };
    for (int i = 0; err_map[i].ff; ++i) {
        if (err_map[i].ff == fe)
            return err_map[i].e;
    }
    return AVError::UnknowError;
}

/*
 *  correct wrong AVError to a right category by ffmpeg error code
 *  does nothing if ffmpeg error code is 0
 */
static void correct_error_by_ffmpeg(AVError::ErrorCode *e, int fe)
{
    if (!fe || !e)
        return;
    const AVError::ErrorCode ec = errorFromFFmpeg(fe);
    if (*e > ec)
        *e = ec;
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
    correct_error_by_ffmpeg(&mError, mFFmpegError);
}

AVError::AVError(ErrorCode code, const QString &detail, int ffmpegError)
    : mError(code)
    , mFFmpegError(ffmpegError)
    , mDetail(detail)
{
    correct_error_by_ffmpeg(&mError, mFFmpegError);
}

AVError::AVError(const AVError& other)
    : mError(other.mError)
    , mFFmpegError(other.mFFmpegError)
    , mDetail(other.mDetail)
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
    QString errStr(mDetail);
    if (errStr.isEmpty()) {
        switch (mError) {
        case NoError:
            errStr = QObject::tr("No error");
            break;
        case OpenError:
            errStr = QObject::tr("Open error");
            break;
        case OpenTimedout:
            errStr = QObject::tr("Open timed out");
            break;
        case ParseStreamTimedOut:
            errStr = QObject::tr("Parse stream timed out");
            break;
        case ParseStreamError:
            errStr = QObject::tr("Parse stream error");
            break;
        case StreamNotFound:
            errStr = QObject::tr("Stream not found");
            break;
        case ReadTimedout:
            errStr = QObject::tr("Read packet timed out");
            break;
        case ReadError:
            errStr = QObject::tr("Read error");
            break;
        case SeekError:
            errStr = QObject::tr("Seek error");
            break;
        case ResourceError:
            errStr = QObject::tr("Resource error");
            break;

        case OpenCodecError:
            errStr = QObject::tr("Open codec error");
            break;
        case CloseCodecError:
            errStr = QObject::tr("Close codec error");
            break;
        case VideoCodecNotFound:
            errStr = QObject::tr("Video codec not found");
            break;
        case AudioCodecNotFound:
            errStr = QObject::tr("Audio codec not found");
            break;
        case SubtitleCodecNotFound:
            errStr = QObject::tr("Subtitle codec not found");
            break;
        case CodecError:
            errStr = QObject::tr("Codec error");
            break;

        case FormatError:
            errStr = QObject::tr("Format error");
            break;

        case NetworkError:
            errStr = QObject::tr("Network error");
            break;

        case AccessDenied:
            errStr = QObject::tr("Access denied");
            break;

        default:
            errStr = QObject::tr("Unknow error");
            break;
        }
    }
    if (mFFmpegError != 0) {
        errStr += QStringLiteral("\n(FFmpeg %1: %2)").arg(mFFmpegError, 0, 16).arg(ffmpegErrorString());
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
    return QString::fromUtf8(av_err2str(mFFmpegError));
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
