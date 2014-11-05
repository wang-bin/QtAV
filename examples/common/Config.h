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


#ifndef PLAYER_CONFIG_H
#define PLAYER_CONFIG_H

#include "common_export.h"
#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtGui/QColor>
#include <QtGui/QFont>
//TODO: use hash to simplify api
/*
 * MVC model. signals from Config notify ui update. signals from ui does not change Config unless ui changes applyed by XXXPage.apply()
 * signals from ui will emit Config::xxxChanged() with the value in ui. ui cancel the change also emit it with the value stores in Config.
 * apply() will change the value in Config
 */

class COMMON_EXPORT Config : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList decoderPriorityNames READ decoderPriorityNames WRITE setDecoderPriorityNames NOTIFY decoderPriorityNamesChanged)
    Q_PROPERTY(QString captureDir READ captureDir WRITE setCaptureDir NOTIFY captureDirChanged)
    Q_PROPERTY(QString captureFormat READ captureFormat WRITE setCaptureFormat NOTIFY captureFormatChanged)
    Q_PROPERTY(int captureQuality READ captureQuality WRITE setCaptureQuality NOTIFY captureQualityChanged)
    Q_PROPERTY(QStringList subtitleEngines READ subtitleEngines WRITE setSubtitleEngines NOTIFY subtitleEnginesChanged)
    Q_PROPERTY(bool subtitleAutoLoad READ subtitleAutoLoad WRITE setSubtitleAutoLoad NOTIFY subtitleAutoLoadChanged)
    Q_PROPERTY(bool subtitleEnabled READ subtitleEnabled WRITE setSubtitleEnabled NOTIFY subtitleEnabledChanged)
    Q_PROPERTY(QFont subtitleFont READ subtitleFont WRITE setSubtitleFont NOTIFY subtitleFontChanged)
    Q_PROPERTY(QColor subtitleColor READ subtitleColor WRITE setSubtitleColor NOTIFY subtitleColorChanged)
    Q_PROPERTY(QColor subtitleOutlineColor READ subtitleOutlineColor WRITE setSubtitleOutlineColor NOTIFY subtitleOutlineColorChanged)
    Q_PROPERTY(bool subtitleOutline READ subtitleOutline WRITE setSubtitleOutline NOTIFY subtitleOutlineChanged)
    Q_PROPERTY(int subtitleBottomMargin READ subtitleBottomMargin WRITE setSubtitleBottomMargin NOTIFY subtitleBottomMarginChanged)
    Q_PROPERTY(bool previewEnabled READ previewEnabled WRITE setPreviewEnabled NOTIFY previewEnabledChanged)
public:
    static Config& instance();

    void reload();
    QString defaultDir() const;
    //void loadFromFile(const QString& file);

    // in priority order. the same order as displayed in ui
    QStringList decoderPriorityNames() const;
    Config& setDecoderPriorityNames(const QStringList& names);

    QString captureDir() const;
    Config& setCaptureDir(const QString& dir);

    /*!
     * \brief captureFormat
     *  can be "yuv" to capture yuv image without convertion. the suffix is the yuv format, e.g. "yuv420p", "nv12"
     *  or can be "jpg", "png"
     * \return
     */
    QString captureFormat() const;
    Config& setCaptureFormat(const QString& format);
    // only works for non-yuv capture. value: -1~100, -1: default
    int captureQuality() const;
    Config& setCaptureQuality(int quality);

    QStringList subtitleEngines() const;
    Config& setSubtitleEngines(const QStringList& value);
    bool subtitleAutoLoad() const;
    Config& setSubtitleAutoLoad(bool value);
    bool subtitleEnabled() const;
    Config& setSubtitleEnabled(bool value);

    QFont subtitleFont() const;
    Config& setSubtitleFont(const QFont& value);
    bool subtitleOutline() const;
    Config& setSubtitleOutline(bool value);
    QColor subtitleColor() const;
    Config& setSubtitleColor(const QColor& value);
    QColor subtitleOutlineColor() const;
    Config& setSubtitleOutlineColor(const QColor& value);
    int subtitleBottomMargin() const;
    Config& setSubtitleBottomMargin(int value);

    bool previewEnabled() const;
    Config& setPreviewEnabled(bool value);

    QVariantHash avformatOptions() const;
    int analyzeDuration() const;
    Config& analyzeDuration(int ad);
    unsigned int probeSize() const;
    Config& probeSize(unsigned int ps);
    bool reduceBuffering() const;
    Config& reduceBuffering(bool y);
    QString avformatExtra() const;
    Config& avformatExtra(const QString& text);

    QString avfilterOptions() const;
    Config& avfilterOptions(const QString& options);
    bool avfilterEnable() const;
    Config& avfilterEnable(bool e);

    Q_INVOKABLE QVariant operator ()(const QString& key) const;
    Q_INVOKABLE Config& operator ()(const QString& key, const QVariant& value);
public:
    //keyword 'signals' maybe protected. we need call the signals in other classes. Q_SIGNAL is empty
    Q_SIGNAL void decodingThreadsChanged(int n);
    Q_SIGNAL void decoderPriorityNamesChanged();
    Q_SIGNAL void registeredDecodersChanged(const QVector<int>& r);
    Q_SIGNAL void captureDirChanged(const QString& dir);
    Q_SIGNAL void captureFormatChanged(const QString& fmt);
    Q_SIGNAL void captureQualityChanged(int quality);
    Q_SIGNAL void avfilterChanged();
    Q_SIGNAL void subtitleEnabledChanged();
    Q_SIGNAL void subtitleAutoLoadChanged();
    Q_SIGNAL void subtitleEnginesChanged();
    Q_SIGNAL void subtitleFontChanged();
    Q_SIGNAL void subtitleColorChanged();
    Q_SIGNAL void subtitleOutlineChanged();
    Q_SIGNAL void subtitleOutlineColorChanged();
    Q_SIGNAL void subtitleBottomMarginChanged();
    Q_SIGNAL void previewEnabledChanged();
protected:
    explicit Config(QObject *parent = 0);
    ~Config();

private:
    class Data;
    Data *mpData;
};

#endif // PLAYER_CONFIG_H
