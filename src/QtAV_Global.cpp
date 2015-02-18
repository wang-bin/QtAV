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

#include "QtAV/QtAV_Global.h"
#include <QtCore/QObject>
#include <QtCore/QRegExp>
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
QString aboutFFmpeg_PlainText()
{
    return aboutFFmpeg_HTML().remove(QRegExp("<[^>]*>"));
}

namespace Internal {
typedef struct ff_component {
    const char* lib;
    unsigned build_version;
    unsigned rt_version;
    const char *config;
    const char *license;
} ffmpeg_component_info;

static const ffmpeg_component_info* get_ffmpeg_component_info(const ffmpeg_component_info* info = 0)
{
    static const ffmpeg_component_info components[] = {
        //TODO: auto check loaded libraries
#define FF_COMPONENT(name, NAME) #name, LIB##NAME##_VERSION_INT, name##_version(), name##_configuration(), name##_license()
        { FF_COMPONENT(avutil, AVUTIL) },
        { FF_COMPONENT(avcodec, AVCODEC) },
        { FF_COMPONENT(avformat, AVFORMAT) },
#if QTAV_HAVE(AVFILTER)
        { FF_COMPONENT(avfilter, AVFILTER) },
#endif //QTAV_HAVE(AVFILTER)
#if QTAV_HAVE(AVDEVICE)
        { FF_COMPONENT(avdevice, AVDEVICE) },
#endif //QTAV_HAVE(AVDEVICE)
#if QTAV_HAVE(AVRESAMPLE)
        { FF_COMPONENT(avresample, AVRESAMPLE) },
#endif //QTAV_HAVE(AVRESAMPLE)
#if QTAV_HAVE(SWRESAMPLE)
        { FF_COMPONENT(swresample, SWRESAMPLE) },
#endif //QTAV_HAVE(SWRESAMPLE)
        { FF_COMPONENT(swscale, SWSCALE) },
#undef FF_COMPONENT
        { 0, 0, 0, 0, 0 }
    };
    if (!info)
        return &components[0];
    // invalid input ptr
    if (((ptrdiff_t)info - (ptrdiff_t)(&components[0]))%sizeof(ffmpeg_component_info))
        return 0;
    const ffmpeg_component_info *next = info;
    next++;
    if (!next->lib)
        return 0;
    return next;
}

void print_library_info()
{
    qDebug() << aboutQtAV_PlainText();
    const ffmpeg_component_info* info = Internal::get_ffmpeg_component_info(0);
    while (info) {
        qDebug("Build with lib%s-%u.%u.%u"
               , info->lib
               , QTAV_VERSION_MAJOR(info->build_version)
               , QTAV_VERSION_MINOR(info->build_version)
               , QTAV_VERSION_PATCH(info->build_version)
               );
        unsigned rt_version = info->rt_version;
        if (info->build_version != rt_version) {
            qWarning("Warning: %s runtime version %u.%u.%u mismatch!"
                    , info->lib
                    , QTAV_VERSION_MAJOR(rt_version)
                    , QTAV_VERSION_MINOR(rt_version)
                    , QTAV_VERSION_PATCH(rt_version)
                    );
        }
        info = Internal::get_ffmpeg_component_info(info);
    }
}
} //namespace Internal

QString aboutFFmpeg_HTML()
{
    QString text = "<h3>FFmpeg/Libav</h3>\n";
    const Internal::ffmpeg_component_info* info = Internal::get_ffmpeg_component_info(0);
    while (info) {
        text += "<h4>" + QObject::tr("Build version")
                + QString(": lib%1-%2.%3.%4</h4>\n")
                .arg(info->lib)
                .arg(QTAV_VERSION_MAJOR(info->build_version))
                .arg(QTAV_VERSION_MINOR(info->build_version))
                .arg(QTAV_VERSION_PATCH(info->build_version))
                ;
        unsigned rt_version = info->rt_version;
        if (info->build_version != rt_version) {
            text += "<h4 style='color:#ff0000;'>" + QString(QObject::tr("Runtime version"))
                    + QString(": %1.%2.%3</h4>\n")
                    .arg(QTAV_VERSION_MAJOR(rt_version))
                    .arg(QTAV_VERSION_MINOR(rt_version))
                    .arg(QTAV_VERSION_PATCH(rt_version))
                    ;
        }
        text += "<p>" + QString(info->config) + "</p>\n"
                "<p>" + QString(info->license) + "</p>\n";
        info = Internal::get_ffmpeg_component_info(info);
    }
    return text;
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

static void qtav_ffmpeg_log_callback(void* ctx, int level,const char* fmt, va_list vl)
{
    AVClass *c = ctx ? *(AVClass**)ctx : 0;
    QString qmsg = QString().sprintf("[FFmpeg:%s] ", c ? c->item_name(ctx) : "?") + QString().vsprintf(fmt, vl);
    qmsg = qmsg.trimmed();
    if (level > AV_LOG_WARNING)
        qDebug() << qmsg;
    else if (level > AV_LOG_PANIC)
        qWarning() << qmsg;
}

#if 0
const QStringList& supportedInputMimeTypes()
{
    static QStringList mimes;
    if (!mimes.isEmpty())
        return mimes;
    av_register_all(); // MUST register all input/output formats
    AVOutputFormat *i = av_oformat_next(NULL);
    QStringList list;
    while (i) {
        list << QString(i->mime_type).split(QChar(','), QString::SkipEmptyParts);
        i = av_oformat_next(i);
    }
    foreach (const QString& v, list) {
        mimes.append(v.trimmed());
    }
    mimes.removeDuplicates();
    return mimes;
}

static QStringList s_audio_mimes, s_video_mimes, s_subtitle_mimes;
static void init_supported_codec_info() {
    const AVCodecDescriptor* cd = avcodec_descriptor_next(NULL);
    while (cd) {
        QStringList list;
        if (cd->mime_types) {
            for (int i = 0; cd->mime_types[i]; ++i) {
                list.append(QString(cd->mime_types[i]).trimmed());
            }
        }
        switch (cd->type) {
        case AVMEDIA_TYPE_AUDIO:
            s_audio_mimes << list;
            break;
        case AVMEDIA_TYPE_VIDEO:
            s_video_mimes << list;
        case AVMEDIA_TYPE_SUBTITLE:
            s_subtitle_mimes << list;
        default:
            break;
        }
        cd = avcodec_descriptor_next(cd);
    }
    s_audio_mimes.removeDuplicates();
    s_video_mimes.removeDuplicates();
    s_subtitle_mimes.removeDuplicates();
}
const QStringList& supportedAudioMimeTypes()
{
    if (s_audio_mimes.isEmpty())
        init_supported_codec_info();
    return s_audio_mimes;
}

const QStringList& supportedVideoMimeTypes()
{
    if (s_video_mimes.isEmpty())
        init_supported_codec_info();
    return s_video_mimes;
}
// TODO: subtitleprocessor support
const QStringList& supportedSubtitleMimeTypes()
{
    if (s_subtitle_mimes.isEmpty())
        init_supported_codec_info();
    return s_subtitle_mimes;
}
const QStringList& supportedInputExtensions()
{
    static QStringList exts;
    if (!exts.isEmpty())
        return exts;
    av_register_all(); // MUST register all input/output formats
    AVInputFormat *i = av_iformat_next(NULL);
    QStringList list;
    while (i) {
        list << QString(i->extensions).split(QChar(','), QString::SkipEmptyParts);
        i = av_iformat_next(i);
    }
    foreach (const QString& v, list) {
        exts.append(v.trimmed());
    }
    exts.removeDuplicates();
    return exts;
}
#endif
// TODO: static link. move all into 1
namespace {
class InitFFmpegLog {
public:
    InitFFmpegLog() {
        setFFmpegLogHandler(qtav_ffmpeg_log_callback);
        const QByteArray env = qgetenv("QTAV_FFMPEG_LOG").toLower();
        if (env.isEmpty())
            return;
        bool ok = false;
        const int level = env.toInt(&ok);
        if ((ok && level == 0) || env == "off" || env == "quiet") {
            av_log_set_level(AV_LOG_QUIET);
            setFFmpegLogHandler(0);
        }
        else if (env == "panic")
            av_log_set_level(AV_LOG_PANIC);
        else if (env == "fatal")
            av_log_set_level(AV_LOG_FATAL);
        else if (env == "error")
            av_log_set_level(AV_LOG_ERROR);
        else if (env.startsWith("warn"))
            av_log_set_level(AV_LOG_WARNING);
        else if (env == "info")
            av_log_set_level(AV_LOG_INFO);
        else if (env == "verbose")
            av_log_set_level(AV_LOG_VERBOSE);
        else if (env == "debug")
            av_log_set_level(AV_LOG_DEBUG);
    }
};
InitFFmpegLog fflog;
}
    
// Initialize Qt Resource System when the library is built
// statically
namespace {
    static void initResources() {
        Q_INIT_RESOURCE(shaders);
        Q_INIT_RESOURCE(QtAV);
    }
    class ResourceLoader {
        ResourceLoader() { initResources(); }
    };
    
    ResourceLoader QtAV_QRCLoader;
}
}
