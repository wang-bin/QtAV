/******************************************************************************
    QtAV Player Demo:  this file is part of QtAV examples
    Copyright (C) 2012-2017 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2014)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "common.h"
#include <cstdio>
#include <cstdlib>
#include <QtCore/QSettings>
#include <QFileOpenEvent>
#include <QtCore/QLocale>
#include <QtCore/QTranslator>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include <QtGui/QDesktopServices>
#else
#include <QtCore/QStandardPaths>
#endif
#include <QtDebug>
#include <QMutex>

#ifdef Q_OS_WINRT
#include <wrl.h>
#include <windows.foundation.h>
#include <windows.storage.pickers.h>
#include <Windows.ApplicationModel.activation.h>
#include <qfunctions_winrt.h>
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::ApplicationModel::Activation;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Foundation::Collections;
using namespace ABI::Windows::Storage;
using namespace ABI::Windows::Storage::Pickers;

#define COM_LOG_COMPONENT "WinRT"
#define COM_ENSURE(f, ...) COM_CHECK(f, return __VA_ARGS__;)
#define COM_WARN(f) COM_CHECK(f)
#define COM_CHECK(f, ...) \
    do { \
        HRESULT hr = f; \
        if (FAILED(hr)) { \
            qWarning() << QString::fromLatin1(COM_LOG_COMPONENT " error@%1. " #f ": (0x%2) %3").arg(__LINE__).arg(hr, 0, 16).arg(qt_error_string(hr)); \
            __VA_ARGS__ \
        } \
    } while (0)

QString UrlFromFileArgs(IInspectable *args)
{
    ComPtr<IFileActivatedEventArgs> fileArgs;
    COM_ENSURE(args->QueryInterface(fileArgs.GetAddressOf()), QString());
    ComPtr<IVectorView<IStorageItem*>> files;
    COM_ENSURE(fileArgs->get_Files(&files), QString());
    ComPtr<IStorageItem> item;
    COM_ENSURE(files->GetAt(0, &item), QString());
    HString path;
    COM_ENSURE(item->get_Path(path.GetAddressOf()), QString());

    quint32 pathLen;
    const wchar_t *pathStr = path.GetRawBuffer(&pathLen);
    const QString filePath = QString::fromWCharArray(pathStr, pathLen);
    qDebug() << "file path: " << filePath;
    item->AddRef(); //ensure we can access it later. TODO: how to release?
    return QString::fromLatin1("winrt:@%1:%2").arg((qint64)(qptrdiff)item.Get()).arg(filePath);
}
#endif

Q_GLOBAL_STATIC(QFile, fileLogger)
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
class QMessageLogContext {};
typedef void (*QtMessageHandler)(QtMsgType, const QMessageLogContext &, const QString &);
QtMsgHandler qInstallMessageHandler(QtMessageHandler h) {
    static QtMessageHandler hh;
    hh = h;
    struct MsgHandlerWrapper {
        static void handler(QtMsgType type, const char *msg) {
            static QMessageLogContext ctx;
            hh(type, ctx, QString::fromUtf8(msg));
        }
    };
    return qInstallMsgHandler(MsgHandlerWrapper::handler);
}
#endif

QMutex loggerMutex;
void Logger(QtMsgType type, const QMessageLogContext &, const QString& qmsg)
{
    // QFile is not thread-safe
    QMutexLocker locker(&loggerMutex);

    const QByteArray msgArray = qmsg.toUtf8();
    const char* msg = msgArray.constData();
     switch (type) {
     case QtDebugMsg:
         printf("Debug: %s\n", msg);
         fileLogger()->write(QByteArray("Debug: "));
         break;
     case QtWarningMsg:
         printf("Warning: %s\n", msg);
         fileLogger()->write(QByteArray("Warning: "));
         break;
     case QtCriticalMsg:
         fprintf(stderr, "Critical: %s\n", msg);
         fileLogger()->write(QByteArray("Critical: "));
         break;
     case QtFatalMsg:
         fprintf(stderr, "Fatal: %s\n", msg);
         fileLogger()->write(QByteArray("Fatal: "));
         abort();
     }
     fflush(0);
     fileLogger()->write(msgArray);
     fileLogger()->write(QByteArray("\n"));
     //fileLogger()->flush(); // crash in qt5.7
}

QOptions get_common_options()
{
    static QOptions ops = QOptions().addDescription(QString::fromLatin1("Options for QtAV players"))
            .add(QString::fromLatin1("common options"))
            ("help,h", QLatin1String("print this"))
            ("ao", QString(), QLatin1String("audio output. Can be ordered combination of available backends (-ao help). Leave empty to use the default setting. Set 'null' to disable audio."))
            ("-egl", QLatin1String("Use EGL. Only works for Qt>=5.5+XCB"))
            ("-gl", QLatin1String("OpenGL backend for Qt>=5.4(windows). can be 'desktop', 'opengles' and 'software'"))
            ("x", 0, QString())
            ("y", 0, QLatin1String("y"))
            ("-width", 800, QLatin1String("width of player"))
            ("height", 450, QLatin1String("height of player"))
            ("fullscreen", QLatin1String("fullscreen"))
            ("decoder", QLatin1String("FFmpeg"), QLatin1String("use a given decoder"))
            ("decoders,-vd", QLatin1String("cuda;vaapi;vda;dxva;cedarv;ffmpeg"), QLatin1String("decoder name list in priority order separated by ';'"))
            ("file,f", QString(), QLatin1String("file or url to play"))
            ("language", QString(), QLatin1String("language on UI. can be 'system' and locale name e.g. zh_CN"))
            ("log", QString(), QLatin1String("log level. can be 'off', 'fatal', 'critical', 'warning', 'debug', 'all'"))
            ("logfile"
#if defined(Q_OS_IOS)
             , appDataDir().append(QString::fromLatin1("/log-%1.txt"))
#elif defined(Q_OS_WINRT) || defined(Q_OS_ANDROID)
             , QString()
#else
             , QString::fromLatin1("log-%1.txt")
#endif
             , QString::fromLatin1("log to file. Set empty to disable log file (-logfile '')"))
            ;
    return ops;
}

void do_common_options_before_qapp(const QOptions& options)
{
#ifdef Q_OS_LINUX
    QSettings cfg(Config::defaultConfigFile(), QSettings::IniFormat);
    const bool set_egl = cfg.value("opengl/egl").toBool();
    //https://bugreports.qt.io/browse/QTBUG-49529
    // it's too late if qApp is created. but why ANGLE is not?
    if (options.value(QString::fromLatin1("egl")).toBool() || set_egl) { //FIXME: Config is constructed too early because it requires qApp
        // only apply to current run. no config change
        qputenv("QT_XCB_GL_INTEGRATION", "xcb_egl");
    } else {
        qputenv("QT_XCB_GL_INTEGRATION", "xcb_glx");
    }
    qDebug() << "QT_XCB_GL_INTEGRATION: " << qgetenv("QT_XCB_GL_INTEGRATION");
#endif //Q_OS_LINUX
}

void do_common_options(const QOptions &options, const QString& appName)
{
    if (options.value(QString::fromLatin1("help")).toBool()) {
        options.print();
        exit(0);
    }
    // has no effect if qInstallMessageHandler() called
    //qSetMessagePattern("%{function} @%{line}: %{message}");
#if !defined(Q_OS_WINRT) && !defined(Q_OS_ANDROID)
    QString app(appName);
    if (app.isEmpty() && qApp)
        app = qApp->applicationName();
    QString logfile(options.option(QString::fromLatin1("logfile")).value().toString().arg(app));
    if (!logfile.isEmpty()) {
        if (QDir(logfile).isRelative()) {
            QString log_path(QString::fromLatin1("%1/%2").arg(qApp->applicationDirPath()).arg(logfile));
            QFile f(log_path);
            if (!f.open(QIODevice::WriteOnly)) {
                log_path = QString::fromLatin1("%1/%2").arg(appDataDir()).arg(logfile);
                qDebug() << "executable dir is not writable. log to " << log_path;
            }
            logfile = log_path;
        }
        qDebug() << "set log file: " << logfile;
        fileLogger()->setFileName(logfile);
        if (fileLogger()->open(QIODevice::WriteOnly)) {
            qInstallMessageHandler(Logger);
        } else {
            qWarning() << "Failed to open log file '" << fileLogger()->fileName() << "': " << fileLogger()->errorString();
        }
    }
#endif
    QByteArray level(options.value(QString::fromLatin1("log")).toByteArray());
    if (level.isEmpty())
        level = Config::instance().logLevel().toLatin1();
    if (!level.isEmpty())
        qputenv("QTAV_LOG", level);
}

void load_qm(const QStringList &names, const QString& lang)
{
    QString l(Config::instance().language());
    if (!lang.isEmpty())
        l = lang;
    if (l.toLower() == QLatin1String("system"))
        l = QLocale::system().name();
    QStringList qms(names);
    qms << QLatin1String("QtAV") << QLatin1String("qt");
    foreach(QString qm, qms) {
        QTranslator *ts = new QTranslator(qApp);
        QString path = qApp->applicationDirPath() + QLatin1String("/i18n/") + qm + QLatin1String("_") + l;
        //qDebug() << "loading qm: " << path;
        if (ts->load(path)) {
            qApp->installTranslator(ts);
        } else {
            path = QString::fromUtf8(":/i18n/%1_%2").arg(qm).arg(l);
            //qDebug() << "loading qm: " << path;
            if (ts->load(path))
                qApp->installTranslator(ts);
            else
                delete ts;
        }
    }
    QTranslator qtts;
    if (qtts.load(QLatin1String("qt_") + QLocale::system().name()))
        qApp->installTranslator(&qtts);
}

void set_opengl_backend(const QString& glopt, const QString &appname)
{
    QString gl = appname.toLower().replace(QLatin1String("\\"), QLatin1String("/"));
    int idx = gl.lastIndexOf(QLatin1String("/"));
    if (idx >= 0)
        gl = gl.mid(idx + 1);
    idx = gl.lastIndexOf(QLatin1String("."));
    if (idx > 0)
        gl = gl.left(idx);
    if (gl.indexOf(QLatin1String("-desktop")) > 0)
        gl = QLatin1String("desktop");
    else if (gl.indexOf(QLatin1String("-es")) > 0 || gl.indexOf(QLatin1String("-angle")) > 0)
        gl = gl.mid(gl.indexOf(QLatin1String("-es")) + 1);
    else if (gl.indexOf(QLatin1String("-sw")) > 0 || gl.indexOf(QLatin1String("-software")) > 0)
        gl = QLatin1String("software");
    else
        gl = glopt.toLower();
    if (gl.isEmpty()) {
        switch (Config::instance().openGLType()) {
        case Config::Desktop:
            gl = QLatin1String("desktop");
            break;
        case Config::OpenGLES:
            gl = QLatin1String("es");
            break;
        case Config::Software:
            gl = QLatin1String("software");
            break;
        default:
            break;
        }
    }
    if (gl == QLatin1String("es") || gl == QLatin1String("angle") || gl == QLatin1String("opengles")) {
        gl = QLatin1String("es_");
        gl.append(Config::instance().getANGLEPlatform().toLower());
    }
#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
    if (gl.startsWith(QLatin1String("es"))) {
        qApp->setAttribute(Qt::AA_UseOpenGLES);
#ifdef QT_OPENGL_DYNAMIC
        qputenv("QT_OPENGL", "angle");
#endif
#ifdef Q_OS_WIN
        if (gl.endsWith(QLatin1String("d3d11")))
            qputenv("QT_ANGLE_PLATFORM", "d3d11");
        else if (gl.endsWith(QLatin1String("d3d9")))
            qputenv("QT_ANGLE_PLATFORM", "d3d9");
        else if (gl.endsWith(QLatin1String("warp")))
            qputenv("QT_ANGLE_PLATFORM", "warp");
#endif
    } else if (gl == QLatin1String("desktop")) {
        qApp->setAttribute(Qt::AA_UseDesktopOpenGL);
    } else if (gl == QLatin1String("software")) {
        qApp->setAttribute(Qt::AA_UseSoftwareOpenGL);
    }
#endif
}


QString appDataDir()
{
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    return QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#else
#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
    return QStandardPaths::writableLocation(QStandardPaths::DataLocation);
#else
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#endif //5.4.0
#endif // 5.0.0
}

AppEventFilter::AppEventFilter(QObject *player, QObject *parent)
    : QObject(parent)
    , m_player(player)
{}

bool AppEventFilter::eventFilter(QObject *obj, QEvent *ev)
{
    //qDebug() << __FUNCTION__ << " watcher: " << obj << ev;
    if (obj != qApp)
        return false;
    if (ev->type() == QEvent::WinEventAct) {
        // winrt file open/pick. since qt5.6.1
        qDebug("QEvent::WinEventAct");
#ifdef Q_OS_WINRT
        class QActivationEvent : public QEvent {
        public:
            void* args() const {return d;} //IInspectable*
        };
        QActivationEvent *ae = static_cast<QActivationEvent*>(ev);
        const QString url(UrlFromFileArgs((IInspectable*)ae->args()));
        if (!url.isEmpty()) {
            qDebug() << "winrt url: " << url;
            if (m_player)
                QMetaObject::invokeMethod(m_player, "play", Q_ARG(QUrl, QUrl(url)));
        }
        return true;
#endif
    }
    if (ev->type() != QEvent::FileOpen)
        return false;
    QFileOpenEvent *foe = static_cast<QFileOpenEvent*>(ev);
    if (m_player)
        QMetaObject::invokeMethod(m_player, "play", Q_ARG(QUrl, QUrl(foe->url())));
    return true;
}

static void initResources() {
    Q_INIT_RESOURCE(theme);
}

namespace {
    struct ResourceLoader {
    public:
        ResourceLoader() { initResources(); }
    } qrc;
}


