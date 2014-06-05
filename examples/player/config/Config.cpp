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

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include <QtGui/QDesktopServices>
#else
#include <QtCore/QStandardPaths>
#endif
#include <QtDebug>

using namespace QtAV;


QStringList idsToNames(QVector<QtAV::VideoDecoderId> ids) {
    QStringList decs;
    foreach (VideoDecoderId id, ids) {
        decs.append(VideoDecoderFactory::name(id).c_str());
    }
    return decs;
}

QVector<VideoDecoderId> idsFromNames(const QStringList& names) {
    QVector<VideoDecoderId> decs;
    foreach (QString name, names) {
        if (name.isEmpty())
            continue;
        VideoDecoderId id = VideoDecoderFactory::id(name.toStdString(), false);
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
        QString decs_default("FFmpeg");
        QVector<QtAV::VideoDecoderId> all_decs_id = GetRegistedVideoDecoderIds();
        if (all_decs_id.contains(VideoDecoderId_CUDA))
            decs_default.append(" CUDA ");
        if (all_decs_id.contains(VideoDecoderId_DXVA))
            decs_default.append(" DXVA ");
        if (all_decs_id.contains(VideoDecoderId_VAAPI))
            decs_default.append(" VAAPI ");
        QStringList all_names = idsToNames(all_decs_id);
#if 0
        QString all_names_string = settings.value("all", QString()).toString();
        if (!all_names_string.isEmpty()) {
            all_names = all_names_string.split(" ", QString::SkipEmptyParts);
        }
#endif
        QStringList decs = settings.value("priority", decs_default).toString().split(" ", QString::SkipEmptyParts);
        if (decs.isEmpty())
            decs = decs_default.split(" ", QString::SkipEmptyParts);
        video_decoder_priority = idsFromNames(decs);
        video_decoder_all = idsFromNames(all_names);

        settings.endGroup();
        settings.endGroup();

        settings.beginGroup("capture");
        capture_dir = settings.value("dir", QString()).toString();
        if (capture_dir.isEmpty()) {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
            capture_dir = QDesktopServices::storageLocation(QDesktopServices::PicturesLocation);
#else
            capture_dir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
#endif
        }
        capture_fmt = settings.value("format", "png").toByteArray();
        capture_quality = settings.value("quality", 100).toInt();
        settings.endGroup();
        settings.beginGroup("avformat");
        direct = settings.value("avioflags", 0).toString() == "direct";
        probe_size = settings.value("probesize", 5000000).toUInt();
        analyze_duration = settings.value("analyzeduration", 5000000).toInt();
        avformat_extra = settings.value("extra", "").toString();
        settings.endGroup();
        settings.beginGroup("avfilter");
        avfilter_on = settings.value("enable", true).toBool();
        avfilter = settings.value("options", "").toString();
        settings.endGroup();
    }
    void save() {
        qDebug("************save config %s************", qPrintable(dir));
        QSettings settings(file, QSettings::IniFormat);
        settings.beginGroup("decoder");
        settings.beginGroup("video");
        settings.setValue("priority", idsToNames(video_decoder_priority).join(" "));
        settings.setValue("all", idsToNames(video_decoder_all).join(" "));
        settings.endGroup();
        settings.endGroup();
        settings.beginGroup("capture");
        settings.setValue("dir", capture_dir);
        settings.setValue("format", capture_fmt);
        settings.setValue("quality", capture_quality);
        settings.endGroup();
        settings.beginGroup("avformat");
        settings.setValue("avioflags", direct ? "direct" : 0);
        settings.setValue("probesize", probe_size);
        settings.setValue("analyzeduration", analyze_duration);
        settings.setValue("extra", avformat_extra);
        settings.endGroup();
        settings.beginGroup("avfilter");
        settings.setValue("enable", avfilter_on);
        settings.setValue("options", avfilter);
        settings.endGroup();
    }

    QString dir;
    QString file;

    QVector<QtAV::VideoDecoderId> video_decoder_priority;
    QVector<QtAV::VideoDecoderId> video_decoder_all;

    QString capture_dir;
    QByteArray capture_fmt;
    int capture_quality;

    bool direct;
    unsigned int probe_size;
    int analyze_duration;
    QString avformat_extra;
    bool avfilter_on;
    QString avfilter;
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

void Config::reload()
{
    mpData->load();
    emit decoderPriorityChanged(mpData->video_decoder_priority);
    emit registeredDecodersChanged(mpData->video_decoder_all);
    emit captureDirChanged(mpData->capture_dir);
    emit captureFormatChanged(mpData->capture_fmt);
    emit captureQualityChanged(mpData->capture_quality);
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
        qDebug("decoderPriority not changed");
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
    return mpData->video_decoder_all;
}

Config& Config::registeredDecoders(const QVector<QtAV::VideoDecoderId> &all)
{
    if (mpData->video_decoder_all == all) {
        qDebug("registeredDecoders not changed");
        return *this;
    }
    mpData->video_decoder_all = all;
    mpData->save();
    emit registeredDecodersChanged(all);
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


QString Config::captureDir() const
{
    return mpData->capture_dir;
}

Config& Config::captureDir(const QString& dir)
{
    if (mpData->capture_dir == dir)
        return *this;
    mpData->capture_dir = dir;
    emit captureDirChanged(dir);
    return *this;
}

QByteArray Config::captureFormat() const
{
    return mpData->capture_fmt;
}

Config& Config::captureFormat(const QByteArray& format)
{
    if (mpData->capture_fmt == format)
        return *this;
    mpData->capture_fmt = format;
    emit captureFormatChanged(format);
    return *this;
}

// only works for non-yuv capture
int Config::captureQuality() const
{
    return mpData->capture_quality;
}

Config& Config::captureQuality(int quality)
{
    if (mpData->capture_quality == quality)
        return *this;
    mpData->capture_quality = quality;
    emit captureQualityChanged(quality);
    return *this;
}

QVariantHash Config::avformatOptions() const
{
    QVariantHash vh;
    if (!mpData->avformat_extra.isEmpty()) {
        QStringList s(mpData->avformat_extra.split(" "));
        for (int i = 0; i < s.size(); ++i) {
            int eq = s[i].indexOf("=");
            if (eq < 0) {
                continue;
            } else {
                vh[s[i].mid(0, eq)] = s[i].mid(eq+1);
            }
        }
    }
    if (mpData->probe_size > 0) {
        vh["probesize"] = mpData->probe_size;
    }
    if (mpData->analyze_duration) {
        vh["analyzeduration"] = mpData->analyze_duration;
    }
    if (mpData->direct) {
        vh["avioflags"] = "direct";
    };
    return vh;
}

unsigned int Config::probeSize() const
{
    return mpData->probe_size;
}

Config& Config::probeSize(unsigned int ps)
{
    mpData->probe_size = ps;
    return *this;
}

int Config::analyzeDuration() const
{
    return mpData->analyze_duration;
}

Config& Config::analyzeDuration(int ad)
{
    mpData->analyze_duration = ad;
    return *this;
}

bool Config::reduceBuffering() const
{
    return mpData->direct;
}

Config& Config::reduceBuffering(bool y)
{
    mpData->direct = y;
    return *this;
}

QString Config::avformatExtra() const
{
    return mpData->avformat_extra;
}

Config& Config::avformatExtra(const QString &text)
{
    mpData->avformat_extra = text;
    return *this;
}

QString Config::avfilterOptions() const
{
    return mpData->avfilter;
}

Config& Config::avfilterOptions(const QString& options)
{
    if (mpData->avfilter == options)
        return *this;
    mpData->avfilter = options;
    emit avfilterChanged();
    return *this;
}

bool Config::avfilterEnable() const
{
    return mpData->avfilter_on;
}

Config& Config::avfilterEnable(bool e)
{
    if (mpData->avfilter_on == e)
        return *this;
    mpData->avfilter_on = e;
    emit avfilterChanged();
    return *this;
}
