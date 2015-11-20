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

#ifndef COMMON_H
#define COMMON_H
#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include "qoptions.h"
#include "Config.h"
#include "ScreenSaver.h"

QOptions COMMON_EXPORT get_common_options();
void COMMON_EXPORT do_common_options_before_qapp(const QOptions& options);
/// help, log file, ffmpeg log level
void COMMON_EXPORT do_common_options(const QOptions& options, const QString& appName = QString());
void COMMON_EXPORT load_qm(const QStringList& names, const QString &lang = QLatin1String("system"));
// if appname ends with 'desktop', 'es', 'angle', software', 'sw', set by appname. otherwise set by command line option glopt, or Config file
// TODO: move to do_common_options_before_qapp
void COMMON_EXPORT set_opengl_backend(const QString& glopt = QString::fromLatin1("auto"), const QString& appname = QString());

QString COMMON_EXPORT appDataDir();

class COMMON_EXPORT AppEventFilter : public QObject
{
public:
    AppEventFilter(QObject *player = 0, QObject* parent = 0);
    QUrl url() const;
    virtual bool eventFilter(QObject *obj, QEvent *ev);
private:
    QObject *m_player;
};

#endif // COMMON_H
