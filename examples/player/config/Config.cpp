/******************************************************************************
    QtAV Player Demo:  this file is part of QtAV examples
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>

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
            decs_default.append(" DXVA ");
        if (all_decs_id.contains(VideoDecoderId_VAAPI))
            decs_default.append(" VAAPI ");
        QStringList all_names = idsToNames(all_decs_id);

        QStringList decs = settings.value("priority", decs_default).toString().split(" ", QString::SkipEmptyParts);
        if (decs.isEmpty())
            decs = decs_default.split(" ", QString::SkipEmptyParts);
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
