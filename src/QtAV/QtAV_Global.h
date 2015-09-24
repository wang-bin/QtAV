/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2015 Wang Bin <wbsecg1@gmail.com>

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


#ifndef QTAV_GLOBAL_H
#define QTAV_GLOBAL_H

#include <stdarg.h>
#include <QtCore/QByteArray> //QByteArrayLiteral check
#include <QtCore/qglobal.h>
#include "dptr.h"

#ifdef BUILD_QTAV_STATIC
#define Q_AV_EXPORT
#else
#if defined(BUILD_QTAV_LIB)
#  undef Q_AV_EXPORT
#  define Q_AV_EXPORT Q_DECL_EXPORT
#else
#  undef Q_AV_EXPORT
#  define Q_AV_EXPORT Q_DECL_IMPORT //only for vc?
#endif
#endif //BUILD_QTAV_STATIC
#define Q_AV_PRIVATE_EXPORT Q_AV_EXPORT

/* runtime version. used to compare with compile time version */
Q_AV_EXPORT unsigned QtAV_Version();
Q_AV_EXPORT QString QtAV_Version_String();
Q_AV_EXPORT QString QtAV_Version_String_Long();
namespace QtAV {
enum LogLevel {
    LogOff,
    LogDebug, // log all
    LogWarning, // log warning, critical, fatal
    LogCritical, // log critical, fatal
    LogFatal, // log fatal
    LogAll
};
Q_AV_EXPORT QString aboutFFmpeg_PlainText();
Q_AV_EXPORT QString aboutFFmpeg_HTML();
Q_AV_EXPORT QString aboutQtAV_PlainText();
Q_AV_EXPORT QString aboutQtAV_HTML();
/*!
 * Default value: LogOff for release build. LogAll for debug build.
 * The level can also be changed at runtime by setting the QTAV_LOG_LEVEL or QTAV_LOG environment variable;
 * QTAV_LOG_LEVEL can be: off, debug, warning, critical, fatal, all. Or use their enum values
 * if both setLogLevel() is called and QTAV_LOG_LEVEL is set, the environment variable takes preceden.
*/
Q_AV_EXPORT void setLogLevel(LogLevel value);
Q_AV_EXPORT LogLevel logLevel();
/// Default handler is qt message logger. Set environment QTAV_FFMPEG_LOG=0 or setFFmpegLogHandler(0) to disable.
Q_AV_EXPORT void setFFmpegLogHandler(void(*)(void *, int, const char *, va_list));
/*!
 * \brief setFFmpegLogLevel
 * \param level can be: quiet, panic, fatal, error, warn, info, verbose, debug
 */
Q_AV_EXPORT void setFFmpegLogLevel(const QByteArray& level);
} //namespace QtAV

// TODO: internal use. move to a private header
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#define QStringLiteral(X) QString::fromUtf8(X)
#endif //QT_VERSION
#ifndef QByteArrayLiteral
#define QByteArrayLiteral(str) QByteArray(str, sizeof(str) - 1)
#endif
/*
 * msvc sucks! can not deal with (defined QTAV_HAVE_##FEATURE && QTAV_HAVE_##FEATURE)
 */
// TODO: internal use. move to a private header
#define QTAV_HAVE(FEATURE) (defined QTAV_HAVE_##FEATURE && QTAV_HAVE_##FEATURE)

#ifndef Q_DECL_OVERRIDE
#define Q_DECL_OVERRIDE
#endif
#ifndef Q_DECL_FINAL
#define Q_DECL_FINAL
#endif

#if defined(BUILD_QTAV_LIB)
#define QTAV_DEPRECATED
#else
#define QTAV_DEPRECATED Q_DECL_DEPRECATED
#endif
#endif // QTAV_GLOBAL_H

