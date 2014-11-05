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
        file = dir + "/" + qApp->applicationName() + ".ini";
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
        //decs_default.append(" CUDA ").append(" DXVA ").append(" VAAPI ").append(" VDA ");
#if 0
        QString all_names_string = settings.value("all", QString()).toString();
        if (!all_names_string.isEmpty()) {
            all_names = all_names_string.split(" ", QString::SkipEmptyParts);
        }
#endif
        video_decoders = settings.value("priority", decs_default).toString().split(" ", QString::SkipEmptyParts);
        settings.endGroup(); //video
        settings.endGroup(); //decoder

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
        settings.beginGroup("subtitle");
        subtitle_autoload = settings.value("autoLoad", true).toBool();
        subtitle_enabled = settings.value("enabled", true).toBool();
        subtitle_engines = settings.value("engines", QStringList() << "FFmpeg" << "LibASS").toStringList();
        QFont f;
        f.setPointSize(20);
        f.setBold(true);
        subtitle_font = settings.value("font", f).value<QFont>();
        subtitle_color = settings.value("color", QColor("white")).value<QColor>();
        subtitle_outline_color = settings.value("outline_color", QColor("blue")).value<QColor>();
        subtitle_outline = settings.value("outline", true).toBool();
        subtilte_bottom_margin = settings.value("bottom margin", 8).toInt();
        settings.endGroup();
        settings.beginGroup("preview");
        preview_enabled = settings.value("enabled", true).toBool();
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
        qDebug() << "sync config to " << file;
        QSettings settings(file, QSettings::IniFormat);
        settings.beginGroup("decoder");
        settings.beginGroup("video");
        settings.setValue("priority", video_decoders.join(" "));
        settings.endGroup();
        settings.endGroup();
        settings.beginGroup("capture");
        settings.setValue("dir", capture_dir);
        settings.setValue("format", capture_fmt);
        settings.setValue("quality", capture_quality);
        settings.endGroup();
        settings.beginGroup("subtitle");
        settings.setValue("enabled", subtitle_enabled);
        settings.setValue("autoLoad", subtitle_autoload);
        settings.setValue("engines", subtitle_engines);
        settings.setValue("font", subtitle_font);
        settings.setValue("color", subtitle_color);
        settings.setValue("outline_color", subtitle_outline_color);
        settings.setValue("outline", subtitle_outline);
        settings.setValue("bottom margin", subtilte_bottom_margin);
        settings.endGroup();
        settings.beginGroup("preview");
        settings.setValue("enabled", preview_enabled);
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

    QStringList video_decoders;

    QString capture_dir;
    QString capture_fmt;
    int capture_quality;

    bool direct;
    unsigned int probe_size;
    int analyze_duration;
    QString avformat_extra;
    bool avfilter_on;
    QString avfilter;

    QStringList subtitle_engines;
    bool subtitle_autoload;
    bool subtitle_enabled;
    QFont subtitle_font;
    QColor subtitle_color, subtitle_outline_color;
    bool subtitle_outline;
    int subtilte_bottom_margin;

    bool preview_enabled;
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
    qDebug() << decoderPriorityNames();
    emit decoderPriorityNamesChanged();
    emit captureDirChanged(mpData->capture_dir);
    emit captureFormatChanged(mpData->capture_fmt);
    emit captureQualityChanged(mpData->capture_quality);
}

QStringList Config::decoderPriorityNames() const
{
    return mpData->video_decoders;
}

Config& Config::setDecoderPriorityNames(const QStringList &value)
{
    if (mpData->video_decoders == value) {
        qDebug("decoderPriority not changed");
        return *this;
    }
    mpData->video_decoders = value;
    emit decoderPriorityNamesChanged();
    mpData->save();
    return *this;
}

QString Config::captureDir() const
{
    return mpData->capture_dir;
}

Config& Config::setCaptureDir(const QString& dir)
{
    if (mpData->capture_dir == dir)
        return *this;
    mpData->capture_dir = dir;
    emit captureDirChanged(dir);
    return *this;
}

QString Config::captureFormat() const
{
    return mpData->capture_fmt;
}

Config& Config::setCaptureFormat(const QString& format)
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

Config& Config::setCaptureQuality(int quality)
{
    if (mpData->capture_quality == quality)
        return *this;
    mpData->capture_quality = quality;
    emit captureQualityChanged(quality);
    return *this;
}

QStringList Config::subtitleEngines() const
{
    return mpData->subtitle_engines;
}

Config& Config::setSubtitleEngines(const QStringList &value)
{
    if (mpData->subtitle_engines == value)
        return *this;
    mpData->subtitle_engines = value;
    emit subtitleEnginesChanged();
    return *this;
}

bool Config::subtitleAutoLoad() const
{
    return mpData->subtitle_autoload;
}

Config& Config::setSubtitleAutoLoad(bool value)
{
    if (mpData->subtitle_autoload == value)
        return *this;
    mpData->subtitle_autoload = value;
    emit subtitleAutoLoadChanged();
    return *this;
}

bool Config::subtitleEnabled() const
{
    return mpData->subtitle_enabled;
}

Config& Config::setSubtitleEnabled(bool value)
{
    if (mpData->subtitle_enabled == value)
        return *this;
    mpData->subtitle_enabled = value;
    emit subtitleEnabledChanged();
    return *this;
}

QFont Config::subtitleFont() const
{
    return mpData->subtitle_font;
}

Config& Config::setSubtitleFont(const QFont& value)
{
    if (mpData->subtitle_font == value)
        return *this;
    mpData->subtitle_font = value;
    emit subtitleFontChanged();
    return *this;
}

bool Config::subtitleOutline() const
{
    return mpData->subtitle_outline;
}
Config& Config::setSubtitleOutline(bool value)
{
    if (mpData->subtitle_outline == value)
        return *this;
    mpData->subtitle_outline = value;
    emit subtitleOutlineChanged();
    return *this;
}

QColor Config::subtitleColor() const
{
    return mpData->subtitle_color;
}
Config& Config::setSubtitleColor(const QColor& value)
{
    if (mpData->subtitle_color == value)
        return *this;
    mpData->subtitle_color = value;
    emit subtitleColorChanged();
    return *this;
}

QColor Config::subtitleOutlineColor() const
{
    return mpData->subtitle_outline_color;
}
Config& Config::setSubtitleOutlineColor(const QColor& value)
{
    if (mpData->subtitle_outline_color == value)
        return *this;
    mpData->subtitle_outline_color = value;
    emit subtitleOutlineColorChanged();
    return *this;
}

int Config::subtitleBottomMargin() const
{
    return mpData->subtilte_bottom_margin;
}

Config& Config::setSubtitleBottomMargin(int value)
{
    if (mpData->subtilte_bottom_margin == value)
        return *this;
    mpData->subtilte_bottom_margin = value;
    emit subtitleBottomMarginChanged();
    return *this;
}

bool Config::previewEnabled() const
{
    return mpData->preview_enabled;
}

Config& Config::setPreviewEnabled(bool value)
{
    if (mpData->preview_enabled == value)
        return *this;
    mpData->preview_enabled = value;
    emit previewEnabledChanged();
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
