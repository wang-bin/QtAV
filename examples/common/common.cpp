/******************************************************************************
    QtAV Player Demo:  this file is part of QtAV examples
    Copyright (C) 2014-2015 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV

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

#include <QFileOpenEvent>
#include <QtCore/QLocale>
#include <QtCore/QTranslator>
#include <QtCore/QCoreApplication>
#include <QtDebug>

QOptions get_common_options()
{
    static QOptions ops = QOptions().addDescription("Options for QtAV players")
            .add("common options")
            ("help,h", "print this")
            ("ao", "", "audio output. Can be ordered combination of available backends (-ao help). Leave empty to use the default setting. Set 'null' to disable audio.")
            ("-gl", "OpenGL backend for Qt>=5.4(windows). can be 'desktop', 'opengles' and 'software'")
            ("x", 0, "")
            ("y", 0, "y")
            ("-width", 800, "width of player")
            ("height", 450, "height of player")
            ("fullscreen", "fullscreen")
            ("decoder", "FFmpeg", "use a given decoder")
            ("decoders,-vd", "cuda;vaapi;vda;dxva;cedarv;ffmpeg", "decoder name list in priority order seperated by ';'")
            ("file,f", "", "file or url to play")
            ("language", "system", "language on UI. can be 'system', 'none' and locale name e.g. zh_CN")
            ;
    return ops;
}

void load_qm(const QStringList &names, const QString& lang)
{
    if (lang.isEmpty() || lang.toLower() == "none")
        return;
    QString l(lang);
    if (l.toLower() == "system")
        l = QLocale::system().name();
    QStringList qms(names);
    qms << "QtAV" << "qt";
    foreach(QString qm, qms) {
        QTranslator *ts = new QTranslator(qApp);
        QString path = qApp->applicationDirPath() + "/i18n/" + qm + "_" + l;
        //qDebug() << "loading qm: " << path;
        if (ts->load(path)) {
            qApp->installTranslator(ts);
        } else {
            path = ":/i18n/" + qm + "_" + l;
            //qDebug() << "loading qm: " << path;
            if (ts->load(path))
                qApp->installTranslator(ts);
            else
                delete ts;
        }
    }
    QTranslator qtts;
    if (qtts.load("qt_" + QLocale::system().name()))
        qApp->installTranslator(&qtts);
}

void set_opengl_backend(const QString& glopt, const QString &appname)
{
    QString gl = appname.toLower().replace("\\", "/");
    int idx = gl.lastIndexOf("/");
    if (idx >= 0)
        gl = gl.mid(idx + 1);
    idx = gl.lastIndexOf(".");
    if (idx > 0)
        gl = gl.left(idx);
    if (gl.indexOf("-desktop") > 0)
        gl = "desktop";
    else if (gl.indexOf("-es") > 0 || gl.indexOf("-angle") > 0)
        gl = gl.mid(gl.indexOf("-es") + 1);
    else if (gl.indexOf("-sw") > 0 || gl.indexOf("-software") > 0)
        gl = "software";
    else
        gl = glopt.toLower();
    if (gl.isEmpty()) {
        switch (Config::instance().openGLType()) {
        case Config::Desktop:
            gl = "desktop";
            break;
        case Config::OpenGLES:
            gl = "es";
            break;
        case Config::Software:
            gl = "software";
            break;
        default:
            break;
        }
    }
    if (gl == "es" || gl == "angle" || gl == "opengles") {
        gl = "es_";
        gl.append(Config::instance().getANGLEPlatform().toLower());
    }
#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
    if (gl.startsWith("es")) {
        qApp->setAttribute(Qt::AA_UseOpenGLES);
#ifdef QT_OPENGL_DYNAMIC
        qputenv("QT_OPENGL", "angle");
#endif
#ifdef Q_OS_WIN
        if (gl.endsWith("d3d11"))
            qputenv("QT_ANGLE_PLATFORM", "d3d11");
        else if (gl.endsWith("d3d9"))
            qputenv("QT_ANGLE_PLATFORM", "d3d9");
        else if (gl.endsWith("warp"))
            qputenv("QT_ANGLE_PLATFORM", "warp");
#endif
    } else if (gl == "desktop") {
        qApp->setAttribute(Qt::AA_UseDesktopOpenGL);
    } else if (gl == "software") {
        qApp->setAttribute(Qt::AA_UseSoftwareOpenGL);
    }
#endif
}

AppEventFilter::AppEventFilter(QObject *player, QObject *parent)
    : QObject(parent)
    , m_player(player)
{}

bool AppEventFilter::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj != qApp)
        return false;
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


