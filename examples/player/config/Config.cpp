/******************************************************************************
    Config.cpp: description
    Copyright (C) 2013 Wang Bin <wbsecg1@gmail.com>

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

    Alternatively, this file may be used under the terms of the GNU
    General Public License version 3.0 as published by the Free Software
    Foundation and appearing in the file LICENSE.GPL included in the
    packaging of this file.  Please review the following information to
    ensure the GNU General Public License version 3.0 requirements will be
    met: http://www.gnu.org/copyleft/gpl.html.
******************************************************************************/


#include "Config.h"

#include <QtCore/QSettings>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>

using namespace QtAV;

static QStringList idsToNames(QVector<QtAV::VideoDecoderId> ids) {
    QStringList decs;
    foreach (VideoDecoderId id, ids) {
        decs.append(VideoDecoderFactory::name(id).c_str());
    }
    return decs;
}

static QVector<VideoDecoderId> idsFromNames(const QStringList& names) {
    QVector<VideoDecoderId> decs;
    foreach (QString name, names) {
        if (name.isEmpty())
            continue;
        VideoDecoderId id = VideoDecoderFactory::id(name.toStdString());
        if (id == 0)
            continue;
        decs.append(id);
    }
    return decs;
}
class Config::Data
{
public:
    Data() {
        dir = qApp->applicationDirPath() + "/data";
        if (!QDir(dir).exists()) {
            dir = QDir::homePath() + "/.QtAV";
            if (!QDir(dir).exists())
                QDir().mkpath(dir);
        }
        file = dir + "/config.ini";
        load();
    }
    ~Data() {
        save();
    }

    void load() {
        QSettings settings(file, QSettings::IniFormat);
        settings.beginGroup("decoder");
        settings.beginGroup("video");
        decode_threads = settings.value("threads", 0).toInt();
        QString decs_default("FFmpeg");
        QVector<QtAV::VideoDecoderId> all_decs_id = GetRegistedVideoDecoderIds();
        if (all_decs_id.contains(VideoDecoderId_DXVA))
            decs_default.prepend("DXVA ");
        if (all_decs_id.contains(VideoDecoderId_VAAPI))
            decs_default.prepend("VAAPI");
        QString all_default = decs_default;
        QStringList all_names = idsToNames(all_decs_id);
        foreach (QString name, all_names) {
            if (!all_default.contains(name))
                all_default += " " + name;
        }

        QStringList decs = settings.value("priority", decs_default).toString().split(" ", QString::SkipEmptyParts);
        all_names = settings.value("all", all_default).toString().split(" ", QString::SkipEmptyParts);
        video_decoder_priority = idsFromNames(decs);
        video_decoder_all = idsFromNames(all_names);
        settings.endGroup();
        settings.endGroup();
    }
    void save() {
        qDebug("************save config %s************", qPrintable(dir));
        QSettings settings(file, QSettings::IniFormat);
        settings.beginGroup("decoder");
        settings.beginGroup("video");
        settings.setValue("threads", decode_threads);
        settings.setValue("priority", idsToNames(video_decoder_priority).join(" "));
        settings.setValue("all", idsToNames(video_decoder_all).join(" "));
        settings.endGroup();
        settings.endGroup();
    }

    QString dir;
    QString file;

    int decode_threads;
    QVector<QtAV::VideoDecoderId> video_decoder_priority;
    QVector<QtAV::VideoDecoderId> video_decoder_all;
};

Config& Config::instance()
{
    static Config cfg;
    return cfg;
}

Config::Config(QObject *parent)
    : QObject(parent)
    , mpData(new Data())
{
}

Config::~Config()
{
    delete mpData;
}

QString Config::defaultDir() const
{
    return mpData->dir;
}

int Config::decodingThreads() const
{
    return mpData->decode_threads;
}

Config& Config::decodingThreads(int n)
{
    if (mpData->decode_threads == n)
        return *this;
    mpData->decode_threads = n;
    emit decodingThreadsChanged(n);
    mpData->save();
    return *this;
}

QVector<QtAV::VideoDecoderId> Config::decoderPriority() const
{
    return mpData->video_decoder_priority;
}

Config& Config::decoderPriority(const QVector<QtAV::VideoDecoderId> &p)
{
    qDebug("=================new decoders: %d", p.size());
    qDebug("video_decoder_priority.size: %d", mpData->video_decoder_priority.size());
    if (mpData->video_decoder_priority == p) {
        qDebug("not changed");
        return *this;
    }
    mpData->video_decoder_priority = p;
    mpData->save();
    emit decoderPriorityChanged(p);
    return *this;
}

QStringList Config::decoderPriorityNames() const
{
    return idsToNames(mpData->video_decoder_priority);
}

Config& Config::decoderPriorityNames(const QStringList &names)
{
    return decoderPriority(idsFromNames(names));
}

QVector<QtAV::VideoDecoderId> Config::registeredDecoders() const
{
    return mpData->video_decoder_priority;
}

Config& Config::registeredDecoders(const QVector<QtAV::VideoDecoderId> &all)
{
    if (mpData->video_decoder_all == all) {
        qDebug("not changed");
        return *this;
    }
    mpData->video_decoder_all = all;
    mpData->save();
    return *this;
}

QStringList Config::registeredDecoderNames() const
{
    return idsToNames(mpData->video_decoder_all);
}

Config& Config::registeredDecoderNames(const QStringList &names)
{
    return registeredDecoders(idsFromNames(names));
}
