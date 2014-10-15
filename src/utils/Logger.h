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

/*
 * Qt Logging Hack
 */

#ifndef QTAV_LOGGER_H
#define QTAV_LOGGER_H

#include <QtDebug>

#ifndef Q_DECL_CONSTEXPR
#define Q_DECL_CONSTEXPR
#endif //Q_DECL_CONSTEXPR
#ifndef Q_DECL_NOTHROW
#define Q_DECL_NOTHROW
#endif //Q_DECL_NOTHROW
#ifndef Q_ATTRIBUTE_FORMAT_PRINTF
#define Q_ATTRIBUTE_FORMAT_PRINTF(...)
#endif //Q_ATTRIBUTE_FORMAT_PRINTF

namespace QtAV {
namespace Internal {

// internal use when building QtAV library
class QtAVDebug {
public:
    // copy from QMessageLogger or others
    QtAVDebug(QtMsgType t = QtDebugMsg, QDebug *d = 0);
    ~QtAVDebug();
    template<typename T> QtAVDebug &operator<<(T t) {
        if (!dbg)
            return *this;
        if ((int)logLevel() < LogDebug)
            return *this;
        if ((int)logLevel() >= LogAll) {
            *dbg << t;
            return *this;
        }
        if ((int)logLevel() <= LogDebug) {
            *dbg << t;
            return *this;
        }
        if (logLevel() == LogWarning) {
            if ((int)type <= (int)QtWarningMsg)
                *dbg << t;
            return *this;
        }
        if (logLevel() == LogCritical) {
            if ((int)type <= (int)QtCriticalMsg)
                *dbg << t;
            return *this;
        }
        if (logLevel() == LogFatal) {
            if ((int)type <= (int)QtFatalMsg)
                *dbg << t;
            return *this;
        }
    }
private:
    QtMsgType type;
    // use ptr. ~QDebug() will print message
    QDebug *dbg;
};
class Logger {
    Q_DISABLE_COPY(Logger)
public:Q_DECL_CONSTEXPR Logger(const char *file = "unknown", int line = 0, const char *function = "unknown", const char *category = "default")
        : ctx(file, line, function, category) {}
    void debug(const char *msg, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(2, 3);
#if 0
    void noDebug(const char *, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(2, 3)
    {}
    void warning(const char *msg, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(2, 3);
    void critical(const char *msg, ...) const Q_ATTRIBUTE_FORMAT_PRINTF(2, 3);

    //TODO: QLoggingCategory
#ifndef Q_CC_MSVC
    Q_NORETURN
#endif
    void fatal(const char *msg, ...) const Q_DECL_NOTHROW Q_ATTRIBUTE_FORMAT_PRINTF(2, 3);
#endif //0
#ifndef QT_NO_DEBUG_STREAM
    //TODO: QLoggingCategory
    QtAVDebug debug() const ;/*
    QtAVDebug warning() const;
    QtAVDebug critical() const;*/
    //QNoDebug noDebug() const Q_DECL_NOTHROW;
#endif // QT_NO_DEBUG_STREAM
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
public: //public can typedef outside
    class Context {
        Q_DISABLE_COPY(Context)
    public:
        Q_DECL_CONSTEXPR Context(const char *fileName, int lineNumber, const char *functionName, const char *categoryName)
            : version(1), line(lineNumber), file(fileName), function(functionName), category(categoryName) {}
        inline QDebug debug() const { return QDebug(QtDebugMsg);}
        int version;
        int line;
        const char *file;
        const char *function;
        const char *category;
    };
private:
    Context ctx;
#else
private:
    QMessageLogger ctx;
#endif
};
//simple way
#if 0
#undef qDebug
#define qDebug(fmt, ...) QtAV::Logger::debug(#fmt, ##__VA_ARGS__)
#undef qDebug
#if !defined(QT_NO_DEBUG_STREAM)
QtAVDebug debug(const char *msg, ...); /* print debug message */
#else // QT_NO_DEBUG_STREAM
#undef qDebug
QNoDebug debug(const char *msg, ...); /* print debug message */
#define qDebug QT_NO_QDEBUG_MACRO
#endif //QT_NO_DEBUG_STREAM
#endif //0

// complex way like Qt5
// including qDebug() << ...
//qDebug() Log(__FILE__, __LINE__, Q_FUNC_INFO).debug
#undef qDebug //was defined as QMessageLogger...
#define qDebug QtAV::Internal::Logger(__FILE__, __LINE__, Q_FUNC_INFO).debug

} // namespace Internal
} // namespace QtAV

#endif // QTAV_LOGGER_H
