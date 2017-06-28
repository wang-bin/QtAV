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
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtDebug>
#include "common.h"

class Config::Data
{
public:
    Data() {
        is_loading = false;
        if (!Data::name.isEmpty())
            file = appDataDir() + QString::fromLatin1("/") + Data::name + QString::fromLatin1(".ini");
        else
            file = appDataDir() + QString::fromLatin1("/") + qApp->applicationName() + QString::fromLatin1(".ini");
        if (!QDir(appDataDir()).exists()) {
            if (!QDir().mkpath(appDataDir())) {
                qWarning() << "Failed to create appDataDir: " << appDataDir();
            }
        }
        moveOldCfg();
    }

    void moveOldCfg() {
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
    }

    void save() {
        if (is_loading)
            return;
        qDebug() << "sync config to " << file;
        QSettings settings(file, QSettings::IniFormat);
        // TODO: why crash on mac qt5.4 if call on aboutToQuit()
        settings.setValue(QString::fromLatin1("log"), log);
        settings.setValue(QString::fromLatin1("language"), lang);
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
        settings.setValue(QString::fromLatin1("zeroCopy"), zero_copy);
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
        settings.setValue(QString::fromLatin1("egl"), egl);
        const char* glname = Config::staticMetaObject.enumerator(Config::staticMetaObject.indexOfEnumerator("OpenGLType")).valueToKey(opengl);
        settings.setValue(QString::fromLatin1("type"), QString::fromLatin1(glname));
        settings.setValue(QString::fromLatin1("angle_platform"), angle_dx);
        settings.endGroup();
        settings.beginGroup(QString::fromLatin1("shader"));
        settings.setValue(QString::fromLatin1("enable"), user_shader);
        settings.setValue(QString::fromLatin1("fbo"), fbo);
        settings.setValue(QString::fromLatin1("fragHeader"), frag_header);
        settings.setValue(QString::fromLatin1("fragSample"), frag_sample);
        settings.setValue(QString::fromLatin1("fragPostProcess"), frag_pp);
        settings.endGroup();
        settings.beginGroup(QString::fromLatin1("buffer"));
        settings.setValue(QString::fromLatin1("value"), buffer_value);
        settings.endGroup();
        qDebug() << "sync end";
    }

    QString file;
    bool is_loading;

    qreal force_fps;
    QStringList video_decoders;
    bool zero_copy;

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

    bool egl;
    Config::OpenGLType opengl;
    QString angle_dx;
    bool abort_timeout;
    qreal timeout;
    int buffer_value;
    QString log;
    QString lang;

    QVariantList history;

    bool user_shader;
    bool fbo;
    QString frag_header;
    QString frag_sample;
    QString frag_pp;

    static QString name;
};

QString Config::Data::name;

Config& Config::instance()
{
    static Config cfg;
    return cfg;
}

void Config::setName(const QString &name)
{
    Config::Data::name = name;
}

QString Config::getName()
{
    return Config::Data::name;
}

QString Config::defaultConfigFile()
{
    return appDataDir() + QString::fromLatin1("/") + Data::name + QString::fromLatin1(".ini");;
}

Config::Config(QObject *parent)
    : QObject(parent)
    , mpData(new Data())
{
    // DO NOT call save() in dtor because it's a singleton and may be deleted later than qApp, QFont is not valid
    connect(qApp, SIGNAL(aboutToQuit()), SLOT(save())); //FIXME: what if qapp not ready
    reload();
}

Config::~Config()
{
    delete mpData;
}

QString Config::defaultDir()
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
    save();
    return true;
}

void Config::reload()
{
    QSqlDatabase db(QSqlDatabase::database());
    if (!db.isOpen()) {
        db = QSqlDatabase::addDatabase(QString::fromUtf8("QSQLITE"));
        db.setDatabaseName(appDataDir().append(QString("/%1.db").arg(mpData->name)));
        if (!db.open())
            qWarning("error open db");
        db.exec("CREATE TABLE IF NOT EXISTS history (url TEXT primary key, start BIGINT, duration BIGINT)");
    }
    QSqlQuery query(db.exec(QString::fromUtf8("SELECT * FROM history")));
    while (query.next()) {
        QVariantMap var;
        var[QString::fromUtf8("url")] = query.value(0).toString();
        var[QString::fromUtf8("start")] = query.value(1).toLongLong();
        var[QString::fromUtf8("duration")] = query.value(2).toLongLong();
        mpData->history.append(var);
    }
    mpData->is_loading = true;
    QSettings settings(mpData->file, QSettings::IniFormat);
    setLogLevel(settings.value(QString::fromLatin1("log"), QString()).toString());
    setLanguage(settings.value(QString::fromLatin1("language"),
#if QT_VERSION == QT_VERSION_CHECK(5, 6, 0) && defined(Q_OS_WINPHONE) //qt bug
                               QString::fromLatin1("en_US")
#else
                               QString::fromLatin1("system")
#endif
                               ).toString());
    setLastFile(settings.value(QString::fromLatin1("last_file"), QString()).toString());
    setTimeout(settings.value(QString::fromLatin1("timeout"), 30.0).toReal());
    setAbortOnTimeout(settings.value(QString::fromLatin1("abort_timeout"), true).toBool());
    setForceFrameRate(settings.value(QString::fromLatin1("force_fps"), 0.0).toReal());
    settings.beginGroup(QString::fromLatin1("decoder"));
    settings.beginGroup(QString::fromLatin1("video"));
    QString decs_default(QString::fromLatin1("HW FFmpeg")); // HW is ignored
    setDecoderPriorityNames(settings.value(QString::fromLatin1("priority"), decs_default).toString().split(QString::fromLatin1(" "), QString::SkipEmptyParts));
    setZeroCopy(settings.value(QString::fromLatin1("zeroCopy"), true).toBool());
    settings.endGroup(); //video
    settings.endGroup(); //decoder

    settings.beginGroup(QString::fromLatin1("capture"));
    setCaptureDir(settings.value(QString::fromLatin1("dir"), QString()).toString());
    if (captureDir().isEmpty()) {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
        setCaptureDir(QDesktopServices::storageLocation(QDesktopServices::PicturesLocation));
#else
        setCaptureDir(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation));
#endif
    }
    setCaptureFormat(settings.value(QString::fromLatin1("format"), QString::fromLatin1("png")).toString());
    setCaptureQuality(settings.value(QString::fromLatin1("quality"), 100).toInt());
    settings.endGroup();
    settings.beginGroup(QString::fromLatin1("subtitle"));
    setSubtitleAutoLoad(settings.value(QString::fromLatin1("autoLoad"), true).toBool());
    setSubtitleEnabled(settings.value(QString::fromLatin1("enabled"), true).toBool());
    setSubtitleEngines(settings.value(QString::fromLatin1("engines"), QStringList() << QString::fromLatin1("FFmpeg") << QString::fromLatin1("LibASS")).toStringList());
    setSubtitleDelay(settings.value(QString::fromLatin1("delay"), 0.0).toInt());
    QFont f;
    f.setPointSize(20);
    f.setBold(true);
    setSubtitleFont(settings.value(QString::fromLatin1("font"), f).value<QFont>());
    setSubtitleColor(settings.value(QString::fromLatin1("color"), QColor("white")).value<QColor>());
    setSubtitleOutlineColor(settings.value(QString::fromLatin1("outline_color"), QColor("blue")).value<QColor>());
    setSubtitleOutline(settings.value(QString::fromLatin1("outline"), true).toBool());
    setSubtitleBottomMargin(settings.value(QString::fromLatin1("bottom margin"), 8).toInt());
    settings.beginGroup(QString::fromLatin1("ass"));
    setAssFontFile(settings.value(QString::fromLatin1("font_file"), QString()).toString());
    setAssFontFileForced(settings.value(QString::fromLatin1("force_font_file"), false).toBool());
    setAssFontsDir(settings.value(QString::fromLatin1("fonts_dir"), QString()).toString());
    settings.endGroup();
    settings.endGroup();
    settings.beginGroup(QString::fromLatin1("preview"));
    setPreviewEnabled(settings.value(QString::fromLatin1("enabled"), true).toBool());
    setPreviewWidth(settings.value(QString::fromLatin1("width"), 160).toInt());
    setPreviewHeight(settings.value(QString::fromLatin1("height"), 90).toInt());
    settings.endGroup();
    settings.beginGroup(QString::fromLatin1("avformat"));
    setAvformatOptionsEnabled(settings.value(QString::fromLatin1("enable"), false).toBool());
    reduceBuffering(settings.value(QString::fromLatin1("avioflags"), 0).toString() == QLatin1String("direct"));
    probeSize(settings.value(QString::fromLatin1("probesize"), 5000000).toUInt());
    analyzeDuration(settings.value(QString::fromLatin1("analyzeduration"), 5000000).toInt());
    avformatExtra(settings.value(QString::fromLatin1("extra"), QString()).toString());
    settings.endGroup();
    settings.beginGroup(QString::fromLatin1("avfilterVideo"));
    avfilterVideoEnable(settings.value(QString::fromLatin1("enable"), true).toBool());
    avfilterVideoOptions(settings.value(QString::fromLatin1("options"), QString()).toString());
    settings.endGroup();
    settings.beginGroup(QString::fromLatin1("avfilterAudio"));
    avfilterAudioEnable(settings.value(QString::fromLatin1("enable"), true).toBool());
    avfilterAudioOptions(settings.value(QString::fromLatin1("options"), QString()).toString());
    settings.endGroup();

    settings.beginGroup(QString::fromLatin1("opengl"));
    setEGL(settings.value(QString::fromLatin1("egl"), false).toBool());
    const QString glname = settings.value(QString::fromLatin1("type"), QString::fromLatin1("OpenGLES")).toString();
    setOpenGLType((Config::OpenGLType)Config::staticMetaObject.enumerator(Config::staticMetaObject.indexOfEnumerator("OpenGLType")).keysToValue(glname.toLatin1().constData()));
    // d3d11 bad performance (gltexsubimage2d)
    setANGLEPlatform(settings.value(QString::fromLatin1("angle_platform"), QString::fromLatin1("d3d9")).toString());
    settings.endGroup();

    settings.beginGroup(QString::fromLatin1("shader"));
    setUserShaderEnabled(settings.value(QString::fromLatin1("enable"), false).toBool());
    setIntermediateFBO(settings.value(QString::fromLatin1("fbo"), false).toBool());
    setFragHeader(settings.value(QString::fromLatin1("fragHeader"), QString()).toString());
    setFragSample(settings.value(QString::fromLatin1("fragSample"), QString::fromLatin1("// horizontal mirror effect\n"
                                                                                        "vec4 sample2d(sampler2D tex, vec2 pos, int p) {\n"
                                                                                        "    return texture(tex, vec2(1.0-pos.x, pos.y));\n"
                                                                                        "}")).toString());
    setFragPostProcess(settings.value(QString::fromLatin1("fragPostProcess"), QString::fromLatin1("//negate color effect\n"
                                                                                                  "gl_FragColor.rgb = vec3(1.0-gl_FragColor.r, 1.0-gl_FragColor.g, 1.0-gl_FragColor.b);")).toString());
    settings.endGroup();

    settings.beginGroup(QString::fromLatin1("buffer"));
    setBufferValue(settings.value(QString::fromLatin1("value"), -1).toInt());
    settings.endGroup();
    mpData->is_loading = false;
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
    Q_EMIT forceFrameRateChanged();
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
    Q_EMIT decoderPriorityNamesChanged();
    Q_EMIT changed();
    mpData->save();
    return *this;
}

bool Config::zeroCopy() const
{
    return mpData->zero_copy;
}

Config& Config::setZeroCopy(bool value)
{
    if (mpData->zero_copy == value)
        return *this;
    mpData->zero_copy = value;
    Q_EMIT zeroCopyChanged();
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
    Q_EMIT captureDirChanged(dir);
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
    Q_EMIT captureFormatChanged(format);
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
    Q_EMIT captureQualityChanged(quality);
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
    Q_EMIT subtitleEnginesChanged();
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
    Q_EMIT subtitleAutoLoadChanged();
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
    Q_EMIT subtitleEnabledChanged();
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
    Q_EMIT subtitleFontChanged();
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
    Q_EMIT subtitleOutlineChanged();
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
    Q_EMIT subtitleColorChanged();
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
    Q_EMIT subtitleOutlineColorChanged();
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
    Q_EMIT subtitleBottomMarginChanged();
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
    Q_EMIT changed();
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
    Q_EMIT changed();
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
    Q_EMIT changed();
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
    Q_EMIT changed();
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
    Q_EMIT previewEnabledChanged();
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
    Q_EMIT previewWidthChanged();
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
    Q_EMIT previewHeightChanged();
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
    Q_EMIT avformatOptionsEnabledChanged();
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
    Q_EMIT avfilterVideoChanged();
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
    Q_EMIT avfilterVideoChanged();
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
    Q_EMIT avfilterAudioChanged();
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
    Q_EMIT avfilterAudioChanged();
    Q_EMIT changed();
    return *this;
}

bool Config::isEGL() const
{
    return mpData->egl;
}

Config& Config::setEGL(bool value)
{
    if (mpData->egl == value)
        return *this;
    mpData->egl = value;
    Q_EMIT EGLChanged();
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
    Q_EMIT openGLTypeChanged();
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
    Q_EMIT ANGLEPlatformChanged();
    Q_EMIT changed();
    return *this;
}

bool Config::userShaderEnabled() const
{
    return mpData->user_shader;
}

Config& Config::setUserShaderEnabled(bool value)
{
    if (mpData->user_shader == value)
        return *this;
    mpData->user_shader = value;
    Q_EMIT userShaderEnabledChanged();
    Q_EMIT changed();
    return *this;
}

bool Config::intermediateFBO() const
{
    return mpData->fbo;
}

Config& Config::setIntermediateFBO(bool value)
{
    if (mpData->fbo == value)
        return *this;
    mpData->fbo = value;
    Q_EMIT intermediateFBOChanged();
    Q_EMIT changed();
    return *this;
}

QString Config::fragHeader() const
{
    return mpData->frag_header;
}

Config& Config::setFragHeader(const QString &text)
{
    if (mpData->frag_header == text)
        return *this;
    mpData->frag_header = text;
    Q_EMIT fragHeaderChanged();
    Q_EMIT changed();
    return *this;
}

QString Config::fragSample() const
{
    return mpData->frag_sample;
}

Config& Config::setFragSample(const QString &text)
{
    if (mpData->frag_sample == text)
        return *this;
    mpData->frag_sample = text;
    Q_EMIT fragSampleChanged();
    Q_EMIT changed();
    return *this;
}

QString Config::fragPostProcess() const
{
    return mpData->frag_pp;
}

Config& Config::setFragPostProcess(const QString &text)
{
    if (mpData->frag_pp == text)
        return *this;
    mpData->frag_pp = text;
    Q_EMIT fragPostProcessChanged();
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
    Q_EMIT bufferValueChanged();
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
    Q_EMIT timeoutChanged();
    Q_EMIT changed();
    return *this;
}

QString Config::logLevel() const
{
    return mpData->log;
}

Config& Config::setLogLevel(const QString& value)
{
    if (mpData->log == value.toLower())
        return *this;
    mpData->log = value.toLower();
    Q_EMIT logLevelChanged();
    Q_EMIT changed();
    return *this;
}

QString Config::language() const
{
    return mpData->lang;
}

Config& Config::setLanguage(const QString& value)
{
    if (mpData->lang == value)
        return *this;
    mpData->lang = value;
    Q_EMIT languageChanged();
    Q_EMIT changed();
    return *this;
}

QVariantList Config::history() const
{
    return mpData->history;
}

void Config::addHistory(const QVariantMap &value)
{
    mpData->history.prepend(value);
    Q_EMIT historyChanged();
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query(db);
    if (!query.prepare(QString::fromUtf8("INSERT INTO history (url, start, duration) "
                              "VALUES (:url, :start, :duration)"))) {
            qWarning("error prepare sql query");
    }
    query.bindValue(QString::fromUtf8(":url"), value.value("url").toString());
    query.bindValue(QString::fromUtf8(":start"), value.value("start").toLongLong());
    query.bindValue(QString::fromUtf8(":duration"), value.value("duration").toLongLong());
    if (!query.exec())
        qWarning("failed to add history: %d", db.isOpen());
}

void Config::removeHistory(const QString &url)
{
    QVariantList::Iterator it = mpData->history.begin();
    bool change = false;
    while (it != mpData->history.end()) {
        if (it->toMap().value("url") != url) {
            ++it;
            continue;
        }
        it = mpData->history.erase(it);
        change = true;
    }
    if (!change)
        return;
    Q_EMIT historyChanged();
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query(db);
    query.prepare(QString::fromUtf8("DELETE FROM history WHERE url = :url"));
    query.bindValue(QString::fromUtf8(":url"), url);
    if (!query.exec())
        qWarning("failed to remove history");
}

void Config::clearHistory()
{
    if (mpData->history.isEmpty())
        return;
    mpData->history.clear();
    Q_EMIT historyChanged();
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query(db);
    query.prepare(QString::fromUtf8("DELETE FROM history"));
    // 'TRUNCATE table history' is faster
    if (!query.exec())
        qWarning("failed to clear history");
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
    Q_EMIT abortOnTimeoutChanged();
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
