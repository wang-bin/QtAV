/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>

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

#include "QtAV/QtAV_Global.h"
// TODO: move to an internal header
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0) || defined(QT_WIDGETS_LIB)
#ifndef QTAV_HAVE_WIDGETS
#define QTAV_HAVE_WIDGETS 1
#endif //QTAV_HAVE_WIDGETS
#endif

#include <QtCore/QObject>
#include <QtCore/QRegExp>
#if QTAV_HAVE(WIDGETS)
#include <QBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTextBrowser>
#endif //QTAV_HAVE(WIDGETS)
#include "QtAV/version.h"
#include "QtAV/private/AVCompat.h"
#include "utils/Logger.h"

unsigned QtAV_Version()
{
    return QTAV_VERSION;
}

QString QtAV_Version_String()
{
    return QTAV_VERSION_STR;
}

#define QTAV_VERSION_STR_LONG   QTAV_VERSION_STR "(" __DATE__ ", " __TIME__ ")"

QString QtAV_Version_String_Long()
{
    return QTAV_VERSION_STR_LONG;
}

namespace QtAV {

namespace Internal {
// disable logging for release. you can manually enable it.
#ifdef QT_NO_DEBUG
static QtAV::LogLevel gLogLevel = QtAV::LogOff;
#else
static QtAV::LogLevel gLogLevel = QtAV::LogAll;
#endif
static bool gLogLevelSet = false;
bool isLogLevelSet() { return gLogLevelSet;}
} //namespace Internal

//TODO: auto add new depend libraries information
void about()
{
#if QTAV_HAVE(WIDGETS)
    //we should use new because a qobject will delete it's children
    QTextBrowser *viewQtAV = new QTextBrowser;
    QTextBrowser *viewFFmpeg = new QTextBrowser;
    viewQtAV->setOpenExternalLinks(true);
    viewFFmpeg->setOpenExternalLinks(true);
    viewQtAV->setHtml(aboutQtAV_HTML());
    viewFFmpeg->setHtml(aboutFFmpeg_HTML());
    QTabWidget *tab = new QTabWidget;
    tab->addTab(viewQtAV, "QtAV");
    tab->addTab(viewFFmpeg, "FFmpeg");
    QPushButton *btn = new QPushButton(QObject::tr("Ok"));
    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    btnLayout->addWidget(btn);
    QDialog dialog;
    dialog.setWindowTitle(QObject::tr("About") + "  QtAV");
    QVBoxLayout *layout = new QVBoxLayout;
    dialog.setLayout(layout);
    layout->addWidget(tab);
    layout->addLayout(btnLayout);
    QObject::connect(btn, SIGNAL(clicked()), &dialog, SLOT(accept()));
    dialog.exec();
#else
    aboutQtAV();
    aboutFFmpeg();
#endif //QTAV_HAVE(WIDGETS)
}

void aboutFFmpeg()
{
#if QTAV_HAVE(WIDGETS)
    QMessageBox::about(0, QObject::tr("About FFmpeg"), aboutFFmpeg_HTML());
#else
    qDebug() << aboutFFmpeg_PlainText();
#endif
}

QString aboutFFmpeg_PlainText()
{
    return aboutFFmpeg_HTML().remove(QRegExp("<[^>]*>"));
}

QString aboutFFmpeg_HTML()
{
    QString text = "<h3>FFmpeg/Libav</h3>\n";
    struct ff_component {
        const char* lib;
        unsigned build_version;
        unsigned rt_version;
        const char *config;
        const char *license;
    } components[] = {
//TODO: auto check loaded libraries
#define FF_COMPONENT(name, NAME) #name, LIB##NAME##_VERSION_INT, name##_version(), name##_configuration(), name##_license()
        { FF_COMPONENT(avcodec, AVCODEC) },
        { FF_COMPONENT(avformat, AVFORMAT) },
        { FF_COMPONENT(avutil, AVUTIL) },
        { FF_COMPONENT(swscale, SWSCALE) },
    #if QTAV_HAVE(SWRESAMPLE)
        { FF_COMPONENT(swresample, SWRESAMPLE) },
    #endif //QTAV_HAVE(SWRESAMPLE)
    #if QTAV_HAVE(AVRESAMPLE)
        { FF_COMPONENT(avresample, AVRESAMPLE) },
    #endif //QTAV_HAVE(AVRESAMPLE)
    #if QTAV_HAVE(AVDEVICE)
        { FF_COMPONENT(avdevice, AVDEVICE) },
    #endif //QTAV_HAVE(AVDEVICE)
#undef FF_COMPONENT
        { 0, 0, 0, 0, 0 }
    };
    for (int i = 0; components[i].lib != 0; ++i) {
        text += "<h4>" + QObject::tr("Build version")
                + QString(": lib%1-%2.%3.%4</h4>\n")
                .arg(components[i].lib)
                .arg(QTAV_VERSION_MAJOR(components[i].build_version))
                .arg(QTAV_VERSION_MINOR(components[i].build_version))
                .arg(QTAV_VERSION_PATCH(components[i].build_version))
                ;
        unsigned rt_version = components[i].rt_version;
        if (components[i].build_version != rt_version) {
            text += "<h4 style='color:#ff0000;'>" + QString(QObject::tr("Runtime version"))
                    + QString(": %1.%2.%3</h4>\n")
                    .arg(QTAV_VERSION_MAJOR(rt_version))
                    .arg(QTAV_VERSION_MINOR(rt_version))
                    .arg(QTAV_VERSION_PATCH(rt_version))
                    ;
        }
        text += "<p>" + QString(components[i].config) + "</p>\n"
                "<p>" + QString(components[i].license) + "</p>\n";
    }
    return text;
}

void aboutQtAV()
{
#if QTAV_HAVE(WIDGETS)
    QMessageBox::about(0, QObject::tr("About QtAV"), aboutQtAV_HTML());
#else
    qDebug() << aboutQtAV_PlainText();
#endif //QTAV_HAVE(WIDGETS)
}

QString aboutQtAV_PlainText()
{
    return aboutQtAV_HTML().remove(QRegExp("<[^>]*>"));
}

QString aboutQtAV_HTML()
{
    static QString about = "<h3>QtAV " QTAV_VERSION_STR_LONG "</h3>\n"
            "<p>" + QObject::tr("A media playing library base on Qt and FFmpeg.\n") + "</p>"
            "<p>" + QObject::tr("Distributed under the terms of LGPLv2.1 or later.\n") + "</p>"
            "<p>Copyright (C) 2012-2014 Wang Bin (aka. Lucas Wang) <a href='mailto:wbsecg1@gmail.com'>wbsecg1@gmail.com</a></p>\n"
            "<p>" + QObject::tr("Shanghai University->S3 Graphics->Deepin, Shanghai, China") + "</p>\n"
            "<p>" + QObject::tr("Donate") + ": <a href='http://www.qtav.org#donate'>http://www.qtav.org#donate</a></p>\n"
            "<p>" + QObject::tr("Source") + ": <a href='https://github.com/wang-bin/QtAV'>https://github.com/wang-bin/QtAV</a></p>\n"
            "<p>" + QObject::tr("Home page") + ": <a href='http://www.qtav.org'>http://www.qtav.org</a></p>";
    return about;
}

void setLogLevel(LogLevel value)
{
    Internal::gLogLevelSet = true;
    Internal::gLogLevel = value;
}

LogLevel logLevel()
{
    return (LogLevel)Internal::gLogLevel;
}

void setFFmpegLogHandler(void (*callback)(void *, int, const char *, va_list))
{
    // libav does not check null callback
    if (!callback)
        callback = av_log_default_callback;
    av_log_set_callback(callback);
}

static void qtav_ffmpeg_log_callback(void* , int level,const char* fmt, va_list vl)
{
    QString qmsg = "{FFmpeg} " + QString().vsprintf(fmt, vl);
    qmsg = qmsg.trimmed();
    if (level == AV_LOG_WARNING || level == AV_LOG_ERROR)
        qWarning() << qmsg;
    else if (level == AV_LOG_FATAL)
        qFatal("%s", qmsg.toUtf8().constData());
    else if (level == AV_LOG_PANIC)
        qFatal("%s", qmsg.toUtf8().constData());
    else
        qDebug() << qmsg;
}

// TODO: static link. move all into 1
namespace {
class InitFFmpegLog {
public:
    InitFFmpegLog() {
        setFFmpegLogHandler(qtav_ffmpeg_log_callback);
        const QByteArray env = qgetenv("QTAV_FFMPEG_LOG");
        if (env.isEmpty())
            return;
        bool ok = false;
        const int level = env.toInt(&ok);
        if ((ok && level == 0) || env.toLower().endsWith("off"))
            setFFmpegLogHandler(0);
    }
};
InitFFmpegLog fflog;
}
}
