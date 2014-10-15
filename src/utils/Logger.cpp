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


//#include "Logger.h"
// we need LogLevel so must include QtAV_Global.h
#include "QtAV/QtAV_Global.h"

namespace QtAV {
namespace Internal {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
typedef Logger::Context QMessageLogger;
#endif
void log_helper(QtMsgType msgType, const QMessageLogger *qlog, const char *msg, ...) {
    QString qmsg;
    va_list ap;
    va_start(ap, msg); // use variable arg list
    if (msg)
        qmsg = QString().vsprintf(msg, ap);
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
    va_end(ap);
}

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

void Logger::debug(const char *msg, ...) const
{
    if ((int)logLevel() < LogDebug) //LogOff
        return;
    if ((int)logLevel() > (int)LogDebug && logLevel() != LogAll) {
        return;
    }
    va_list ap;
    va_start(ap, msg); // use variable arg list
    log_helper(QtDebugMsg, &ctx, msg, ap);
    va_end(ap);
}

QtAVDebug Logger::debug() const
{
    if ((int)logLevel() < (int)LogDebug)
        return QtAVDebug(QtDebugMsg);
    // will print message in ~QDebug()
    // can not use QDebug on stack. It must lives in QtAVDebug
    return QtAVDebug(QtDebugMsg, new QDebug(ctx.debug()));
}

QtAVDebug::QtAVDebug(QtMsgType t, QDebug *d)
    : type(t)
    , dbg(d)
{}

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

