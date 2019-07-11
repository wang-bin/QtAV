/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2019 Wang Bin <wbsecg1@gmail.com>

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
#include <QtCore/QLibraryInfo>
#include <QtCore/QObject>
#include <QtCore/QRegExp>
#include "QtAV/version.h"
#include "QtAV/private/AVCompat.h"
#include "utils/internal.h"
#include "utils/Logger.h"

unsigned QtAV_Version()
{
    return QTAV_VERSION;
}

QString QtAV_Version_String()
{
    // vs<2015: C2308: concatenating mismatched strings for QStringLiteral("a" "b")
    return QString::fromLatin1(QTAV_VERSION_STR);
}

#define QTAV_VERSION_STR_LONG   QTAV_VERSION_STR "(" __DATE__ ", " __TIME__ ")"

QString QtAV_Version_String_Long()
{
    return QString::fromLatin1(QTAV_VERSION_STR_LONG);
}

namespace QtAV {

namespace Internal {
// disable logging for release. you can manually enable it.
#if defined(QT_NO_DEBUG)// && !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS) && !defined(Q_OS_WINRT)
static QtAV::LogLevel gLogLevel = QtAV::LogOff;
#else
static QtAV::LogLevel gLogLevel = QtAV::LogAll;
#endif
static bool gLogLevelSet = false;
bool isLogLevelSet() { return gLogLevelSet;}
static int gAVLogLevel = AV_LOG_INFO;
} //namespace Internal

//TODO: auto add new depend libraries information
QString aboutFFmpeg_PlainText()
{
    return aboutFFmpeg_HTML().remove(QRegExp(QStringLiteral("<[^>]*>")));
}

namespace Internal {
typedef struct depend_component {
    const char* lib;
    unsigned build_version;
    unsigned rt_version;
    const char *config;
    const char *license;
} depend_component;

static unsigned get_qt_version() {
    int major = 0, minor = 0, patch = 0;
    if (sscanf(qVersion(), "%d.%d.%d", &major, &minor, &patch) != 3)
        qWarning("Can not recognize Qt runtime version");
    return QT_VERSION_CHECK(major, minor, patch);
}

static const depend_component* get_depend_component(const depend_component* info = 0)
{
    // DO NOT use QStringLiteral here because the install script use strings to search "Qt-" in the library. QStringLiteral will place it in .ro and strings can not find it
    static const QByteArray qt_license(QLibraryInfo::licensee().prepend(QLatin1String("Qt-" QT_VERSION_STR " licensee: ")).toUtf8());
#if QT_VERSION >= QT_VERSION_CHECK(5, 3, 0)
    static const char* qt_build_info = get_qt_version() >= QT_VERSION_CHECK(5, 3, 0) ? QLibraryInfo::build() : "";
#else
    static const char* qt_build_info = "";
#endif
    static const depend_component components[] = {
        {  "Qt", QT_VERSION, get_qt_version(), qt_build_info, qt_license.constData() },
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
    if (((ptrdiff_t)info - (ptrdiff_t)(&components[0]))%sizeof(depend_component))
        return 0;
    const depend_component *next = info;
    next++;
    if (!next->lib)
        return 0;
    return next;
}

void print_library_info()
{
    qDebug() << aboutQtAV_PlainText().toUtf8().constData();
    const depend_component* info = Internal::get_depend_component(0);
    while (info) {
        if (!qstrcmp(info->lib, "avutil"))
            qDebug("FFmpeg/Libav configuration: %s", info->config);
        qDebug("Build with %s-%u.%u.%u"
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
        info = Internal::get_depend_component(info);
    }
}

} //namespace Internal

QString aboutFFmpeg_HTML()
{
    QString text = QStringLiteral("<h3>FFmpeg/Libav</h3>\n");
    const Internal::depend_component* info = Internal::get_depend_component(0);
    while (info) {
        text += QStringLiteral("<h4>%1: %2-%3.%4.%5</h4>\n")
                .arg(QObject::tr("Build version"))
                .arg(QLatin1String(info->lib))
                .arg(QTAV_VERSION_MAJOR(info->build_version))
                .arg(QTAV_VERSION_MINOR(info->build_version))
                .arg(QTAV_VERSION_PATCH(info->build_version))
                ;
        unsigned rt_version = info->rt_version;
        if (info->build_version != rt_version) {
            text += QStringLiteral("<h4 style='color:#ff0000;'>%1: %2.%3.%4</h4>\n")
                    .arg(QObject::tr("Runtime version"))
                    .arg(QTAV_VERSION_MAJOR(rt_version))
                    .arg(QTAV_VERSION_MINOR(rt_version))
                    .arg(QTAV_VERSION_PATCH(rt_version))
                    ;
        }
        text += QStringLiteral("<p>%1</p>\n<p>%2</p>\n").arg(QString::fromUtf8(info->config)).arg(QString::fromUtf8(info->license));
        info = Internal::get_depend_component(info);
    }
    return text;
}

QString aboutQtAV_PlainText()
{
    return aboutQtAV_HTML().remove(QRegExp(QStringLiteral("<[^>]*>")));
}

QString aboutQtAV_HTML()
{
    static QString about = QString::fromLatin1("<img src='qrc:/QtAV.svg'><h3>QtAV " QTAV_VERSION_STR_LONG "</h3>\n"
            "<p>%1</p><p>%2</p><p>%3 </p>"
            "<p>Copyright (C) 2012-2019 Wang Bin (aka. Lucas Wang) <a href='mailto:wbsecg1@gmail.com'>wbsecg1@gmail.com</a></p>\n"
            "<p>%4: <a href='http://qtav.org/donate.html'>http://qtav.org/donate.html</a></p>\n"
            "<p>%5: <a href='https://github.com/wang-bin/QtAV'>https://github.com/wang-bin/QtAV</a></p>\n"
            "<p>%6: <a href='http://qtav.org'>http://qtav.org</a></p>"
           ).arg(QObject::tr("Multimedia framework base on Qt and FFmpeg.\n"))
            .arg(QObject::tr("Distributed under the terms of LGPLv2.1 or later.\n"))
            .arg(QObject::tr("Shanghai, China"))
            .arg(QObject::tr("Donate"))
            .arg(QObject::tr("Source"))
            .arg(QObject::tr("Home page"));
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

void setFFmpegLogLevel(const QByteArray &level)
{
    if (level.isEmpty())
        return;
    bool ok = false;
    const int value = level.toInt(&ok);
    if ((ok && value == 0) || level == "off" || level == "quiet")
        Internal::gAVLogLevel = AV_LOG_QUIET;
    else if (level == "panic")
        Internal::gAVLogLevel = AV_LOG_PANIC;
    else if (level == "fatal")
        Internal::gAVLogLevel = AV_LOG_FATAL;
    else if (level == "error")
        Internal::gAVLogLevel = AV_LOG_ERROR;
    else if (level.startsWith("warn"))
        Internal::gAVLogLevel = AV_LOG_WARNING;
    else if (level == "info")
        Internal::gAVLogLevel = AV_LOG_INFO;
    else if (level == "verbose")
        Internal::gAVLogLevel = AV_LOG_VERBOSE;
    else if (level == "debug")
        Internal::gAVLogLevel = AV_LOG_DEBUG;
#ifdef AV_LOG_TRACE
    else if (level == "trace")
        Internal::gAVLogLevel = AV_LOG_TRACE;
#endif
    else
        Internal::gAVLogLevel = AV_LOG_INFO;
    av_log_set_level(Internal::gAVLogLevel);
}

static void qtav_ffmpeg_log_callback(void* ctx, int level,const char* fmt, va_list vl)
{
    // AV_LOG_DEBUG is used by ffmpeg developers
    if (level > Internal::gAVLogLevel)
        return;
    AVClass *c = ctx ? *(AVClass**)ctx : 0;
    QString qmsg = QString().sprintf("[FFmpeg:%s] ", c ? c->item_name(ctx) : "?") + QString().vsprintf(fmt, vl);
    qmsg = qmsg.trimmed();
    if (level > AV_LOG_WARNING)
        qDebug() << qPrintable(qmsg);
    else if (level > AV_LOG_PANIC)
        qWarning() << qPrintable(qmsg);
}

QString avformatOptions()
{
    static QString opts;
    if (!opts.isEmpty())
        return opts;
    void* obj =  const_cast<void*>(reinterpret_cast<const void*>(avformat_get_class()));
    opts = Internal::optionsToString((void*)&obj);
    opts.append(ushort('\n'));
#if AVFORMAT_STATIC_REGISTER
    const AVInputFormat *i = NULL;
    void* it = NULL;
    while ((i = av_demuxer_iterate(&it))) {
#else
    AVInputFormat *i = NULL;
    av_register_all(); // MUST register all input/output formats
    while ((i = av_iformat_next(i))) {
#endif
        QString opt(Internal::optionsToString((void*)&i->priv_class).trimmed());
        if (opt.isEmpty())
            continue;
        opts.append(QStringLiteral("options for input format %1:\n%2\n\n")
                    .arg(QLatin1String(i->name))
                    .arg(opt));
    }
#if AVFORMAT_STATIC_REGISTER
    const AVOutputFormat *o = NULL;
    it = NULL;
    while ((o = av_muxer_iterate(&it))) {
#else
    av_register_all(); // MUST register all input/output formats
    AVOutputFormat *o = NULL;
    while ((o = av_oformat_next(o))) {
#endif
        QString opt(Internal::optionsToString((void*)&o->priv_class).trimmed());
        if (opt.isEmpty())
            continue;
        opts.append(QStringLiteral("options for output format %1:\n%2\n\n")
                    .arg(QLatin1String(o->name))
                    .arg(opt));
    }
    return opts;
}

QString avcodecOptions()
{
    static QString opts;
    if (!opts.isEmpty())
        return opts;
    void* obj = const_cast<void*>(reinterpret_cast<const void*>(avcodec_get_class()));
    opts = Internal::optionsToString((void*)&obj);
    opts.append(ushort('\n'));
    const AVCodec* c = NULL;
#if AVCODEC_STATIC_REGISTER
    void* it = NULL;
    while ((c = av_codec_iterate(&it))) {
#else
    avcodec_register_all();
    while ((c = av_codec_next(c))) {
#endif
        QString opt(Internal::optionsToString((void*)&c->priv_class).trimmed());
        if (opt.isEmpty())
            continue;
        opts.append(QStringLiteral("Options for codec %1:\n%2\n\n").arg(QLatin1String(c->name)).arg(opt));
    }
    return opts;
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
        list << QString(i->mime_type).split(QLatin1Char(','), QString::SkipEmptyParts);
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
#endif


/*
 * AVColorSpace:
 * libav11 libavutil54.3.0 pixfmt.h, ffmpeg2.1*libavutil52.48.101 frame.h
 * ffmpeg2.5 pixfmt.h. AVFrame.colorspace
 * earlier versions: avcodec.h, avctx.colorspace
 */
ColorSpace colorSpaceFromFFmpeg(AVColorSpace cs)
{
    switch (cs) {
    // from ffmpeg: order of coefficients is actually GBR
    case AVCOL_SPC_RGB: return ColorSpace_GBR;
    case AVCOL_SPC_BT709: return ColorSpace_BT709;
    case AVCOL_SPC_BT470BG: return ColorSpace_BT601;
    case AVCOL_SPC_SMPTE170M: return ColorSpace_BT601;
    default: return ColorSpace_Unknown;
    }
}

ColorRange colorRangeFromFFmpeg(AVColorRange cr)
{
    switch (cr) {
    case AVCOL_RANGE_MPEG: return ColorRange_Limited;
    case AVCOL_RANGE_JPEG: return ColorRange_Full;
    default: return ColorRange_Unknown;
    }
}

namespace
{
static const struct RegisterMetaTypes {
    RegisterMetaTypes() {
        qRegisterMetaType<QtAV::MediaStatus>("QtAV::MediaStatus");
    }
} _registerMetaTypes;
}

// TODO: static link. move all into 1
namespace {
class InitFFmpegLog {
public:
    InitFFmpegLog() {
        setFFmpegLogHandler(qtav_ffmpeg_log_callback);
        QtAV::setFFmpegLogLevel(qgetenv("QTAV_FFMPEG_LOG").toLower());
    }
};
InitFFmpegLog fflog;
}
}

// Initialize Qt Resource System when the library is built
// statically
static void initResources() {
    Q_INIT_RESOURCE(shaders);
    Q_INIT_RESOURCE(QtAV);
}

namespace {
    class ResourceLoader {
    public:
        ResourceLoader() { initResources(); }
    };
    
    ResourceLoader QtAV_QRCLoader;
}
