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

/*!
 * DO NOT appear qDebug, qWanring etc in Logger.cpp! They are undefined and redefined to QtAV:Internal::Logger.xxx
 */
// we need LogLevel so must include QtAV_Global.h
#include <QtCore/QString>
#include <QtCore/QMutex>
#include "QtAV/QtAV_Global.h"
#include "Logger.h"

#ifndef QTAV_NO_LOG_LEVEL

void ffmpeg_version_print();
namespace QtAV {
namespace Internal {
static QString gQtAVLogTag = QString();

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
typedef Logger::Context QMessageLogger;
#endif

static void log_helper(QtMsgType msgType, const QMessageLogger *qlog, const char* msg, va_list ap) {
    static QMutex m;
    QMutexLocker lock(&m);
    Q_UNUSED(lock);
    static int repeat = 0;
    static QString last_msg;
    static QtMsgType last_type = QtDebugMsg;
    QString qmsg(gQtAVLogTag);
    QString formated;
    if (msg) {
        formated = QString().vsprintf(msg, ap);
    }
    // repeate check
    if (last_type == msgType && last_msg == formated) {
        repeat++;
        return;
    }
    if (repeat > 0) {
        // print repeat message and current message
        qmsg = QStringLiteral("%1(repeat %2)%3\n%4%5")
                .arg(qmsg).arg(repeat).arg(last_msg).arg(qmsg).arg(formated);
    } else {
        qmsg += formated;
    }
    repeat = 0;
    last_type = msgType;
    last_msg = formated;

    // qt_message_output is a public api
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    qt_message_output(msgType, qmsg.toUtf8().constData());
    return;
#else
    if (!qlog) {
        QMessageLogContext ctx;
        qt_message_output(msgType, ctx, qmsg);
        return;
    }
#endif
#ifndef QT_NO_DEBUG_STREAM
    if (msgType == QtWarningMsg)
        qlog->warning() << qmsg;
    else if (msgType == QtCriticalMsg)
        qlog->critical() << qmsg;
    else if (msgType == QtFatalMsg)
        qlog->fatal("%s", qmsg.toUtf8().constData());
    else
        qlog->debug() << qmsg;
#else
    if (msgType == QtFatalMsg)
        qlog->fatal("%s", qmsg.toUtf8().constData());
#endif //
}

// macro does not support A::##X

void Logger::debug(const char *msg, ...) const
{
    QtAVDebug d; // initialize something. e.g. environment check
    Q_UNUSED(d);
    const int v = (int)logLevel();
    if (v <= (int)LogOff)
        return;
    if (v > (int)LogDebug && v < (int)LogAll)
        return;
    va_list ap;
    va_start(ap, msg);
    // can not use ctx.debug() <<... because QT_NO_DEBUG_STREAM maybe defined
    log_helper(QtDebugMsg, &ctx, msg, ap);
    va_end(ap);
}

void Logger::warning(const char *msg, ...) const
{
    QtAVDebug d; // initialize something. e.g. environment check
    Q_UNUSED(d);
    const int v = (int)logLevel();
    if (v <= (int)LogOff)
        return;
    if (v > (int)LogWarning && v < (int)LogAll)
        return;
    va_list ap;
    va_start(ap, msg);
    log_helper(QtWarningMsg, &ctx, msg, ap);
    va_end(ap);
}

void Logger::critical(const char *msg, ...) const
{
    QtAVDebug d; // initialize something. e.g. environment check
    Q_UNUSED(d);
    const int v = (int)logLevel();
    if (v <= (int)LogOff)
        return;
    if (v > (int)LogCritical && v < (int)LogAll)
        return;
    va_list ap;
    va_start(ap, msg);
    log_helper(QtCriticalMsg, &ctx, msg, ap);
    va_end(ap);
}

void Logger::fatal(const char *msg, ...) const Q_DECL_NOTHROW
{
    QtAVDebug d; // initialize something. e.g. environment check
    Q_UNUSED(d);
    const int v = (int)logLevel();
    /*
    if (v <= (int)LogOff)
        abort();
    if (v > (int)LogFatal && v < (int)LogAll)
        abort();
    */
    if (v > (int)LogOff) {
        va_list ap;
        va_start(ap, msg);
        log_helper(QtFatalMsg, &ctx, msg, ap);
        va_end(ap);
    }
    abort();
}

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#ifndef QT_NO_DEBUG_STREAM
// internal used by Logger::fatal(const char*,...) with log level checked, so always do things here
void Logger::Context::fatal(const char *msg, ...) const Q_DECL_NOTHROW
{
    va_list ap;
    va_start(ap, msg);
    QString qmsg;
    if (msg)
        qmsg += QString().vsprintf(msg, ap);
    qt_message_output(QtFatalMsg, qmsg.toUtf8().constData());
    va_end(ap);
    abort();
}
#endif //QT_NO_DEBUG_STREAM
#endif //QT_VERSION

#ifndef QT_NO_DEBUG_STREAM
// will print message in ~QDebug()
// can not use QDebug on stack. It must lives in QtAVDebug
QtAVDebug Logger::debug() const
{
    QtAVDebug d(QtDebugMsg); //// initialize something. e.g. environment check
    const int v = (int)logLevel();
    if (v <= (int)LogOff)
        return d;
    if (v <= (int)LogDebug || v >= (int)LogAll)
        d.setQDebug(new QDebug(ctx.debug()));
    return d; //ref > 0
}

QtAVDebug Logger::warning() const
{
    QtAVDebug d(QtWarningMsg);
    const int v = (int)logLevel();
    if (v <= (int)LogOff)
        return d;
    if (v <= (int)LogWarning || v >= (int)LogAll)
        d.setQDebug(new QDebug(ctx.warning()));
    return d;
}

QtAVDebug Logger::critical() const
{
    QtAVDebug d(QtCriticalMsg);
    const int v = (int)logLevel();
    if (v <= (int)LogOff)
        return d;
    if (v <= (int)LogCritical || v >= (int)LogAll)
        d.setQDebug(new QDebug(ctx.critical()));
    return d;
}
// no QMessageLogger::fatal()
#endif //QT_NO_DEBUG_STREAM

bool isLogLevelSet();
void print_library_info();

QtAVDebug::QtAVDebug(QtMsgType t, QDebug *d)
    : type(t)
    , dbg(0)
{
    if (d)
        setQDebug(d); // call *dbg << gQtAVLogTag
    static bool sFirstRun = true;
    if (!sFirstRun)
        return;
    sFirstRun = false;
    printf("%s\n", aboutQtAV_PlainText().toUtf8().constData());
    // check environment var and call other functions at first Qt logging call
    // always override setLogLevel()
    QByteArray env = qgetenv("QTAV_LOG_LEVEL");
    if (env.isEmpty())
        env = qgetenv("QTAV_LOG");
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
    env = qgetenv("QTAV_LOG_TAG");
    if (!env.isEmpty()) {
        gQtAVLogTag = QString::fromUtf8(env);
    }

    if ((int)logLevel() > (int)LogOff) {
        print_library_info();
    }
}

QtAVDebug::~QtAVDebug()
{
}

void QtAVDebug::setQDebug(QDebug *d)
{
    dbg = QSharedPointer<QDebug>(d);
    if (dbg && !gQtAVLogTag.isEmpty()) {
        *dbg << gQtAVLogTag;
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

#endif //QTAV_NO_LOG_LEVEL
