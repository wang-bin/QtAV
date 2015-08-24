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
#include "Config.h"
#include <QtCore/QSettings>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QMetaEnum>
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include <QtGui/QDesktopServices>
#else
#include <QtCore/QStandardPaths>
#endif
#include <QtDebug>
#include "common.h"

class Config::Data
{
public:
    Data() {
        file = appDataDir() + QString::fromLatin1("/") + qApp->applicationName() + QString::fromLatin1(".ini");
        if (!QDir(appDataDir()).exists()) {
            if (!QDir().mkpath(appDataDir())) {
                qWarning() << "Failed to create appDataDir: " << appDataDir();
            }
        }
        load();
    }

    void load() {
        // for old config data migration
        QString dir_old = qApp->applicationDirPath() + QString::fromLatin1("/data");
        if (!QDir(dir_old).exists()) {
            dir_old = QDir::homePath() + QString::fromLatin1("/.QtAV");
        }
        if (QDir(dir_old).exists()) {
            if (!QFile(file).exists()) {
                QString old = dir_old + QString::fromLatin1("/") + qApp->applicationName() + QString::fromLatin1(".ini");
                if (QFile(old).exists()) {
                    QFile::copy(old, file);
                    QFile::remove(old);
                }
                old = dir_old + QString::fromLatin1("/playlist.qds");
                if (QFile(old).exists()) {
                    if (!QFile::copy(old, appDataDir() + QString::fromLatin1("/playlist.qds")))
                        qWarning("error to move old playlist data");
                    QFile::remove(old);
                }
                old = dir_old + QString::fromLatin1("/history.qds");
                if (QFile(old).exists()) {
                    if (!QFile::copy(old, appDataDir() + QString::fromLatin1("/history.qds")))
                        qWarning("error to move old history data");
                    QFile::remove(old);
                }
            }
        }

        QSettings settings(file, QSettings::IniFormat);
        last_file = settings.value(QString::fromLatin1("last_file"), QString()).toString();
        timeout = settings.value(QString::fromLatin1("timeout"), 30.0).toReal();
        abort_timeout = settings.value(QString::fromLatin1("abort_timeout"), true).toBool();
        force_fps = settings.value(QString::fromLatin1("force_fps"), 0.0).toReal();
        settings.beginGroup(QString::fromLatin1("decoder"));
        settings.beginGroup(QString::fromLatin1("video"));
        QString decs_default(QString::fromLatin1("FFmpeg"));
        //decs_default.append(QString::fromLatin1(" CUDA ")).append(QString::fromLatin1(" DXVA ")).append(QString::fromLatin1(" VAAPI ")).append(QString::fromLatin1(" VDA "));
#if 0
        QString all_names_string = settings.value("all", QString()).toString();
        if (!all_names_string.isEmpty()) {
            all_names = all_names_string.split(" ", QString::SkipEmptyParts);
        }
#endif
        video_decoders = settings.value(QString::fromLatin1("priority"), decs_default).toString().split(QString::fromLatin1(" "), QString::SkipEmptyParts);
        settings.endGroup(); //video
        settings.endGroup(); //decoder

        settings.beginGroup(QString::fromLatin1("capture"));
        capture_dir = settings.value(QString::fromLatin1("dir"), QString()).toString();
        if (capture_dir.isEmpty()) {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
            capture_dir = QDesktopServices::storageLocation(QDesktopServices::PicturesLocation);
#else
            capture_dir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
#endif
        }
        capture_fmt = settings.value(QString::fromLatin1("format"), QString::fromLatin1("png")).toString();
        capture_quality = settings.value(QString::fromLatin1("quality"), 100).toInt();
        settings.endGroup();
        settings.beginGroup(QString::fromLatin1("subtitle"));
        subtitle_autoload = settings.value(QString::fromLatin1("autoLoad"), true).toBool();
        subtitle_enabled = settings.value(QString::fromLatin1("enabled"), true).toBool();
        subtitle_engines = settings.value(QString::fromLatin1("engines"), QStringList() << QString::fromLatin1("FFmpeg") << QString::fromLatin1("LibASS")).toStringList();
        subtitle_delay = settings.value(QString::fromLatin1("delay"), 0.0).toInt();
        QFont f;
        f.setPointSize(20);
        f.setBold(true);
        subtitle_font = settings.value(QString::fromLatin1("font"), f).value<QFont>();
        subtitle_color = settings.value(QString::fromLatin1("color"), QColor("white")).value<QColor>();
        subtitle_outline_color = settings.value(QString::fromLatin1("outline_color"), QColor("blue")).value<QColor>();
        subtitle_outline = settings.value(QString::fromLatin1("outline"), true).toBool();
        subtilte_bottom_margin = settings.value(QString::fromLatin1("bottom margin"), 8).toInt();
        settings.beginGroup(QString::fromLatin1("ass"));
        ass_font_file = settings.value(QString::fromLatin1("font_file"), QString()).toString();
        ass_force_font_file = settings.value(QString::fromLatin1("force_font_file"), false).toBool();
        ass_fonts_dir = settings.value(QString::fromLatin1("fonts_dir"), QString()).toString();
        settings.endGroup();
        settings.endGroup();
        settings.beginGroup(QString::fromLatin1("preview"));
        preview_enabled = settings.value(QString::fromLatin1("enabled"), true).toBool();
        preview_w = settings.value(QString::fromLatin1("width"), 160).toInt();
        preview_h = settings.value(QString::fromLatin1("height"), 90).toInt();
        settings.endGroup();
        settings.beginGroup(QString::fromLatin1("avformat"));
        avformat_on = settings.value(QString::fromLatin1("enable"), false).toBool();
        direct = settings.value(QString::fromLatin1("avioflags"), 0).toString() == QLatin1String("direct");
        probe_size = settings.value(QString::fromLatin1("probesize"), 5000000).toUInt();
        analyze_duration = settings.value(QString::fromLatin1("analyzeduration"), 5000000).toInt();
        avformat_extra = settings.value(QString::fromLatin1("extra"), QString()).toString();
        settings.endGroup();
        settings.beginGroup(QString::fromLatin1("avfilterVideo"));
        avfilterVideo_on = settings.value(QString::fromLatin1("enable"), true).toBool();
        avfilterVideo = settings.value(QString::fromLatin1("options"), QString()).toString();
        settings.endGroup();
        settings.beginGroup(QString::fromLatin1("avfilterAudio"));
        avfilterAudio_on = settings.value(QString::fromLatin1("enable"), true).toBool();
        avfilterAudio = settings.value(QString::fromLatin1("options"), QString()).toString();
        settings.endGroup();
        settings.beginGroup(QString::fromLatin1("opengl"));
        const QString glname = settings.value(QString::fromLatin1("type"), QString::fromLatin1("OpenGLES")).toString();
        opengl = (Config::OpenGLType)Config::staticMetaObject.enumerator(Config::staticMetaObject.indexOfEnumerator("OpenGLType")).keysToValue(glname.toLatin1().constData());
        // d3d11 bad performance (gltexsubimage2d)
        angle_dx = settings.value(QString::fromLatin1("angle_platform"), QString::fromLatin1("d3d9")).toString();
        settings.endGroup();

        settings.beginGroup(QString::fromLatin1("buffer"));
        buffer_value = settings.value(QString::fromLatin1("value"), -1).toInt();
        settings.endGroup();
    }

    void save() {
        qDebug() << "sync config to " << file;
        QSettings settings(file, QSettings::IniFormat);
        // TODO: why crash on mac qt5.4 if call on aboutToQuit()
        settings.setValue(QString::fromLatin1("last_file"), last_file);
        settings.setValue(QString::fromLatin1("timeout"), timeout);
        settings.setValue(QString::fromLatin1("abort_timeout"), abort_timeout);
        settings.setValue(QString::fromLatin1("force_fps"), force_fps);
        settings.beginGroup(QString::fromLatin1("decoder"));
        settings.beginGroup(QString::fromLatin1("video"));
        settings.setValue(QString::fromLatin1("priority"), video_decoders.join(QString::fromLatin1(" ")));
        settings.endGroup();
        settings.endGroup();
        settings.beginGroup(QString::fromLatin1("capture"));
        settings.setValue(QString::fromLatin1("dir"), capture_dir);
        settings.setValue(QString::fromLatin1("format"), capture_fmt);
        settings.setValue(QString::fromLatin1("quality"), capture_quality);
        settings.endGroup();
        settings.beginGroup(QString::fromLatin1("subtitle"));
        settings.setValue(QString::fromLatin1("enabled"), subtitle_enabled);
        settings.setValue(QString::fromLatin1("autoLoad"), subtitle_autoload);
        settings.setValue(QString::fromLatin1("engines"), subtitle_engines);
        settings.setValue(QString::fromLatin1("delay"), subtitle_delay);
        settings.setValue(QString::fromLatin1("font"), subtitle_font);
        settings.setValue(QString::fromLatin1("color"), subtitle_color);
        settings.setValue(QString::fromLatin1("outline_color"), subtitle_outline_color);
        settings.setValue(QString::fromLatin1("outline"), subtitle_outline);
        settings.setValue(QString::fromLatin1("bottom margin"), subtilte_bottom_margin);
        settings.beginGroup(QString::fromLatin1("ass"));
        settings.setValue(QString::fromLatin1("font_file"), ass_font_file);
        settings.setValue(QString::fromLatin1("force_font_file"), ass_force_font_file);
        settings.setValue(QString::fromLatin1("fonts_dir"), ass_fonts_dir);
        settings.endGroup();
        settings.endGroup();
        settings.beginGroup(QString::fromLatin1("preview"));
        settings.setValue(QString::fromLatin1("enabled"), preview_enabled);
        settings.setValue(QString::fromLatin1("width"), preview_w);
        settings.setValue(QString::fromLatin1("height"), preview_h);
        settings.endGroup();
        settings.beginGroup(QString::fromLatin1("avformat"));
        settings.setValue(QString::fromLatin1("enable"), avformat_on);
        settings.setValue(QString::fromLatin1("avioflags"), direct ? QString::fromLatin1("direct") : QString::fromLatin1("0"));
        settings.setValue(QString::fromLatin1("probesize"), probe_size);
        settings.setValue(QString::fromLatin1("analyzeduration"), analyze_duration);
        settings.setValue(QString::fromLatin1("extra"), avformat_extra);
        settings.endGroup();
        settings.beginGroup(QString::fromLatin1("avfilterVideo"));
        settings.setValue(QString::fromLatin1("enable"), avfilterVideo_on);
        settings.setValue(QString::fromLatin1("options"), avfilterVideo);
        settings.endGroup();
        settings.beginGroup(QString::fromLatin1("avfilterAudio"));
        settings.setValue(QString::fromLatin1("enable"), avfilterAudio_on);
        settings.setValue(QString::fromLatin1("options"), avfilterAudio);
        settings.endGroup();
        settings.beginGroup(QString::fromLatin1("opengl"));
        const char* glname = Config::staticMetaObject.enumerator(Config::staticMetaObject.indexOfEnumerator("OpenGLType")).valueToKey(opengl);
        settings.setValue(QString::fromLatin1("type"), QString::fromLatin1(glname));
        settings.setValue(QString::fromLatin1("angle_platform"), angle_dx);
        settings.endGroup();
        settings.beginGroup(QString::fromLatin1("buffer"));
        settings.setValue(QString::fromLatin1("value"), buffer_value);
        settings.endGroup();
        qDebug() << "sync end";
    }

    QString file;

    qreal force_fps;
    QStringList video_decoders;

    QString last_file;

    QString capture_dir;
    QString capture_fmt;
    int capture_quality;

    bool avformat_on;
    bool direct;
    unsigned int probe_size;
    int analyze_duration;
    QString avformat_extra;
    bool avfilterVideo_on;
    QString avfilterVideo;
    bool avfilterAudio_on;
    QString avfilterAudio;

    QStringList subtitle_engines;
    bool subtitle_autoload;
    bool subtitle_enabled;
    QFont subtitle_font;
    QColor subtitle_color, subtitle_outline_color;
    bool subtitle_outline;
    int subtilte_bottom_margin;
    qreal subtitle_delay;

    bool ass_force_font_file;
    QString ass_font_file;
    QString ass_fonts_dir;

    bool preview_enabled;
    int preview_w, preview_h;

    Config::OpenGLType opengl;
    QString angle_dx;
    bool abort_timeout;
    qreal timeout;
    int buffer_value;
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
    // DO NOT call save() in dtor because it's a singleton and may be deleted later than qApp, QFont is not valid
    connect(qApp, SIGNAL(aboutToQuit()), SLOT(save()));
}

Config::~Config()
{
    delete mpData;
}

QString Config::defaultDir() const
{
    return appDataDir();
}

bool Config::reset()
{
    QFile cf(mpData->file);
    if (!cf.remove()) {
        qWarning() << "Failed to remove config file: " << cf.errorString();
        return false;
    }
    reload();
    Q_EMIT changed();
    save();
    return true;
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

qreal Config::forceFrameRate() const
{
    return mpData->force_fps;
}

Config& Config::setForceFrameRate(qreal value)
{
    if (mpData->force_fps == value)
        return *this;
    mpData->force_fps = value;
    emit forceFrameRateChanged();
    Q_EMIT changed();
    return *this;
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
    Q_EMIT changed();
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
    Q_EMIT changed();
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
    Q_EMIT changed();
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
    Q_EMIT changed();
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
    Q_EMIT changed();
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
    Q_EMIT changed();
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
    Q_EMIT changed();
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
    Q_EMIT changed();
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
    Q_EMIT changed();
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
    Q_EMIT changed();
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
    Q_EMIT changed();
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
    Q_EMIT changed();
    return *this;
}

qreal Config::subtitleDelay() const
{
    return mpData->subtitle_delay;
}

Config& Config::setSubtitleDelay(qreal value)
{
    if (mpData->subtitle_delay == value)
        return *this;
    mpData->subtitle_delay = value;
    Q_EMIT subtitleDelayChanged();
    return *this;
}

QString Config::assFontFile() const
{
    return mpData->ass_font_file;
}

Config& Config::setAssFontFile(const QString &value)
{
    if (mpData->ass_font_file == value)
        return *this;
    mpData->ass_font_file = value;
    Q_EMIT assFontFileChanged();
    return *this;
}


QString Config::assFontsDir() const
{
    return mpData->ass_fonts_dir;
}

Config& Config::setAssFontsDir(const QString &value)
{
    if (mpData->ass_fonts_dir == value)
        return *this;
    mpData->ass_fonts_dir = value;
    Q_EMIT assFontsDirChanged();
    return *this;
}

bool Config::isAssFontFileForced() const
{
    return mpData->ass_force_font_file;
}

Config& Config::setAssFontFileForced(bool value)
{
    if (mpData->ass_force_font_file == value)
        return *this;
    mpData->ass_force_font_file = value;
    Q_EMIT assFontFileForcedChanged();
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
    Q_EMIT changed();
    return *this;
}

int Config::previewWidth() const
{
    return mpData->preview_w;
}

Config& Config::setPreviewWidth(int value)
{
    if (mpData->preview_w == value)
        return *this;
    mpData->preview_w = value;
    emit previewWidthChanged();
    Q_EMIT changed();
    return *this;
}

int Config::previewHeight() const
{
    return mpData->preview_h;
}

Config& Config::setPreviewHeight(int value)
{
    if (mpData->preview_h == value)
        return *this;
    mpData->preview_h = value;
    emit previewHeightChanged();
    Q_EMIT changed();
    return *this;
}
QVariantHash Config::avformatOptions() const
{
    QVariantHash vh;
    if (!mpData->avformat_extra.isEmpty()) {
        QStringList s(mpData->avformat_extra.split(QString::fromLatin1(" ")));
        for (int i = 0; i < s.size(); ++i) {
            int eq = s[i].indexOf(QLatin1String("="));
            if (eq < 0) {
                continue;
            } else {
                vh[s[i].mid(0, eq)] = s[i].mid(eq+1);
            }
        }
    }
    if (mpData->probe_size > 0) {
        vh[QString::fromLatin1("probesize")] = mpData->probe_size;
    }
    if (mpData->analyze_duration) {
        vh[QString::fromLatin1("analyzeduration")] = mpData->analyze_duration;
    }
    if (mpData->direct) {
        vh[QString::fromLatin1("avioflags")] = QString::fromLatin1("direct");
    };
    return vh;
}

bool Config::avformatOptionsEnabled() const
{
    return mpData->avformat_on;
}

Config& Config::setAvformatOptionsEnabled(bool value)
{
    if (mpData->avformat_on == value)
        return *this;
    mpData->avformat_on = value;
    emit avformatOptionsEnabledChanged();
    Q_EMIT changed();
    return *this;
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

QString Config::avfilterVideoOptions() const
{
    return mpData->avfilterVideo;
}

Config& Config::avfilterVideoOptions(const QString& options)
{
    if (mpData->avfilterVideo == options)
        return *this;
    mpData->avfilterVideo = options;
    emit avfilterVideoChanged();
    Q_EMIT changed();
    return *this;
}

bool Config::avfilterVideoEnable() const
{
    return mpData->avfilterVideo_on;
}

Config& Config::avfilterVideoEnable(bool e)
{
    if (mpData->avfilterVideo_on == e)
        return *this;
    mpData->avfilterVideo_on = e;
    emit avfilterVideoChanged();
    Q_EMIT changed();
    return *this;
}

QString Config::avfilterAudioOptions() const
{
    return mpData->avfilterAudio;
}

Config& Config::avfilterAudioOptions(const QString& options)
{
    if (mpData->avfilterAudio == options)
        return *this;
    mpData->avfilterAudio = options;
    emit avfilterAudioChanged();
    Q_EMIT changed();
    return *this;
}

bool Config::avfilterAudioEnable() const
{
    return mpData->avfilterAudio_on;
}

Config& Config::avfilterAudioEnable(bool e)
{
    if (mpData->avfilterAudio_on == e)
        return *this;
    mpData->avfilterAudio_on = e;
    emit avfilterAudioChanged();
    Q_EMIT changed();
    return *this;
}

Config::OpenGLType Config::openGLType() const
{
    return mpData->opengl;
}

Config& Config::setOpenGLType(OpenGLType value)
{
    if (mpData->opengl == value)
        return *this;
    mpData->opengl = value;
    emit openGLTypeChanged();
    Q_EMIT changed();
    return *this;
}

QString Config::getANGLEPlatform() const
{
    return mpData->angle_dx;
}

Config& Config::setANGLEPlatform(const QString& value)
{
    if (mpData->angle_dx == value)
        return *this;
    mpData->angle_dx = value;
    emit ANGLEPlatformChanged();
    Q_EMIT changed();
    return *this;
}

int Config::bufferValue() const
{
    return mpData->buffer_value;
}

Config& Config::setBufferValue(int value)
{
    if (mpData->buffer_value == value)
        return *this;
    mpData->buffer_value = value;
    emit bufferValueChanged();
    Q_EMIT changed();
    return *this;
}

qreal Config::timeout() const
{
    return mpData->timeout;
}

Config& Config::setTimeout(qreal value)
{
    if (mpData->timeout == value)
        return *this;
    mpData->timeout = value;
    emit timeoutChanged();
    Q_EMIT changed();
    return *this;
}

bool Config::abortOnTimeout() const
{
    return mpData->abort_timeout;
}

Config& Config::setAbortOnTimeout(bool value)
{
    if (mpData->abort_timeout == value)
        return *this;
    mpData->abort_timeout = value;
    emit abortOnTimeoutChanged();
    Q_EMIT changed();
    return *this;
}

QString Config::lastFile() const
{
    return mpData->last_file;
}

Config& Config::setLastFile(const QString &value)
{
    if (mpData->last_file == value)
        return *this;
    mpData->last_file = value;
    Q_EMIT lastFileChanged();
    Q_EMIT changed();
    return *this;
}

void Config::save()
{
    mpData->save();
}
