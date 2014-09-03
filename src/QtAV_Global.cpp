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
#include <QtCore/QObject>
#include <QtCore/QRegExp>
#include <QtDebug>
#if QTAV_HAVE(WIDGETS)
#include <QBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTextBrowser>
#endif //QTAV_HAVE(WIDGETS)
#include "QtAV/version.h"
#include "QtAV/private/AVCompat.h"

unsigned QtAV_Version()
{
    return QTAV_VERSION;
}

QString QtAV_Version_String()
{
    return QTAV_VERSION_STR;
}

QString QtAV_Version_String_Long()
{
    return QTAV_VERSION_STR_LONG;
}

namespace QtAV {

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
            "<p>" + QObject::tr("Shanghai University->S3 Graphics, Shanghai, China") + "</p>\n"
            "<p>" + QObject::tr("Donate") + ": <a href='http://wang-bin.github.io/QtAV#donate'>http://wang-bin.github.io/QtAV#donate</a></p>\n"
            "<p>" + QObject::tr("Source") + ": <a href='https://github.com/wang-bin/QtAV'>https://github.com/wang-bin/QtAV</a></p>\n"
            "<p>" + QObject::tr("Web Site") + ": <a href='http://wang-bin.github.io/QtAV'>http://wang-bin.github.io/QtAV</a></p>";
    return about;
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
    QtMsgType mt = QtDebugMsg;
    if (level == AV_LOG_WARNING || level == AV_LOG_ERROR)
        mt = QtWarningMsg;
    else if (level == AV_LOG_FATAL)
        mt = QtCriticalMsg;
    else if (level == AV_LOG_PANIC)
        mt = QtFatalMsg;
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QMessageLogContext ctx;
    qt_message_output(mt, ctx, qmsg);
#else
    qt_message_output(mt, qPrintable(qmsg));
#endif //QT_VERSION
}

// TODO: static link. move all into 1
namespace {
class InitFFmpegLog {
public:
    InitFFmpegLog() {
        setFFmpegLogHandler(qtav_ffmpeg_log_callback);
    }
};
InitFFmpegLog fflog;
}
}
