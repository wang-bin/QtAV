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

/*!
 * DO NOT appear qDebug, qWanring etc in Logger.cpp! They are undefined and redefined to QtAV:Internal::Logger.xxx
 */
//#include "Logger.h"
// we need LogLevel so must include QtAV_Global.h
#include "QtAV/QtAV_Global.h"

namespace QtAV {
namespace Internal {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
typedef Logger::Context QMessageLogger;
#endif

static void log_helper(QtMsgType msgType, const QMessageLogger *qlog, const char* msg, va_list ap) {
    QString qmsg;
    if (msg)
        qmsg = "<QtAV> " + QString().vsprintf(msg, ap);
    // qt_message_output is a public api
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    if (qlog) {
        qlog->debug() << qmsg;
    } else {
        QMessageLogContext ctx;
        qt_message_output(msgType, ctx, qmsg);
    }
#else
    qt_message_output(msgType, qPrintable(qmsg));
#endif //QT_VERSION
}

// macro does not support A::##X

void Logger::debug(const char *msg, ...) const
{
    QtAVDebug d(QtDebugMsg); // initialize something. e.g. environment check
    Q_UNUSED(d);
    const int v = (int)logLevel();
    if (v < (int)LogDebug)
        return;
    if (v > (int)LogDebug && v != (int)LogAll)
        return;
    va_list ap;
    va_start(ap, msg);
    log_helper(QtDebugMsg, &ctx, msg, ap);
    va_end(ap);
}

void Logger::warning(const char *msg, ...) const
{
    QtAVDebug d(QtWarningMsg); // initialize something. e.g. environment check
    Q_UNUSED(d);
    const int v = (int)logLevel();
    if (v < (int)LogWarning)
        return;
    if (v > (int)LogWarning && v != (int)LogAll)
        return;
    va_list ap;
    va_start(ap, msg);
    log_helper(QtWarningMsg, &ctx, msg, ap);
    va_end(ap);
}

void Logger::critical(const char *msg, ...) const
{
    QtAVDebug d(QtCriticalMsg); // initialize something. e.g. environment check
    Q_UNUSED(d);
    const int v = (int)logLevel();
    if (v < (int)LogCritical)
        return;
    if (v > (int)LogCritical && v != (int)LogAll)
        return;
    va_list ap;
    va_start(ap, msg);
    log_helper(QtCriticalMsg, &ctx, msg, ap);
    va_end(ap);
}

void Logger::fatal(const char *msg, ...) const
{
    QtAVDebug d(QtFatalMsg); // initialize something. e.g. environment check
    Q_UNUSED(d);
    const int v = (int)logLevel();
    if (v < (int)LogFatal)
        return;
    if (v > (int)LogFatal && v != (int)LogAll)
        return;
    va_list ap;
    va_start(ap, msg);
    log_helper(QtFatalMsg, &ctx, msg, ap);
    va_end(ap);
}

#ifndef QT_NO_DEBUG_STREAM
QtAVDebug Logger::debug() const
{
    if ((int)logLevel() < (int)LogDebug)
        return QtAVDebug(QtDebugMsg);
    // will print message in ~QDebug()
    // can not use QDebug on stack. It must lives in QtAVDebug
    return QtAVDebug(QtDebugMsg, new QDebug(ctx.debug()));
}

QtAVDebug Logger::warning() const
{
    if ((int)logLevel() < (int)LogWarning)
        return QtAVDebug(QtWarningMsg);
    return QtAVDebug(QtWarningMsg, new QDebug(ctx.warning()));
}

QtAVDebug Logger::critical() const
{
    if ((int)logLevel() < (int)LogCritical)
        return QtAVDebug(QtCriticalMsg);
    return QtAVDebug(QtCriticalMsg, new QDebug(ctx.critical()));
}
// no QMessageLogger::fatal()
/*
QtAVDebug Logger::fatal() const
{
    if ((int)logLevel() < (int)LogFatal)
        return QtAVDebug(QtFatalMsg);
    return QtAVDebug(QtFatalMsg, new QDebug(ctx.fatal()));
}*/
#endif //QT_NO_DEBUG_STREAM

QtAVDebug::QtAVDebug(QtMsgType t, QDebug *d)
    : type(t)
    , dbg(d)
{
    static bool sFirstRun = true;
    if (!sFirstRun)
        return;
    sFirstRun = false;
    //printf("Qt Logging first run........\n");
    // check environment var and call other functions at first Qt logging call
    QByteArray env = qgetenv("QTAV_LOG_LEVEL");
    if (!env.isEmpty()) {
        bool ok = false;
        const int level = env.toInt(&ok);
        if (ok) {
            if (level < (int)LogOff)
                setLogLevel(LogOff);
            else if (level > (int)LogAll)
                setLogLevel(LogAll);
            else
                setLogLevel((LogLevel)level);
        } else {
            env = env.toLower();
            if (env.endsWith("off"))
                setLogLevel(LogOff);
            else if (env.endsWith("debug"))
                setLogLevel(LogDebug);
            else if (env.endsWith("warning"))
                setLogLevel(LogWarning);
            else if (env.endsWith("critical"))
                setLogLevel(LogCritical);
            else if (env.endsWith("fatal"))
                setLogLevel(LogFatal);
            else if (env.endsWith("all") || env.endsWith("default"))
                setLogLevel(LogAll);
        }
    }
}

QtAVDebug::~QtAVDebug()
{
    if (dbg) {
        delete dbg;
        dbg = 0;
    }
}

#if 0
QtAVDebug debug(const char *msg, ...)
{
    if ((int)logLevel() > (int)Debug && logLevel() != All) {
        return QtAVDebug();
    }
    va_list ap;
    va_start(ap, msg); // use variable arg list
    QMessageLogContext ctx;
    log_helper(QtDebugMsg, &ctx, msg, ap);
    va_end(ap);
    return QtAVDebug();
}
#endif

} //namespace Internal
} // namespace QtAV
