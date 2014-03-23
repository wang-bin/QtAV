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

#include <QtAV/VideoDecoderTypes.h>
#include <QtCore/QObject>

//TODO: use hash to simplify api
/*
 * MVC model. signals from Config notify ui update. signals from ui does not change Config unless ui changes applyed by XXXPage.apply()
 * signals from ui will emit Config::xxxChanged() with the value in ui. ui cancel the change also emit it with the value stores in Config.
 * apply() will change the value in Config
 */


QStringList idsToNames(QVector<QtAV::VideoDecoderId> ids);
QVector<QtAV::VideoDecoderId> idsFromNames(const QStringList& names);

class Config : public QObject
{
    Q_OBJECT
public:
    static Config& instance();

    void reload();
    QString defaultDir() const;
    //void loadFromFile(const QString& file);

    int decodingThreads() const;
    Config& decodingThreads(int n);

    QVector<QtAV::VideoDecoderId> decoderPriority() const;
    Config& decoderPriority(const QVector<QtAV::VideoDecoderId>& p);
    QStringList decoderPriorityNames() const;
    Config& decoderPriorityNames(const QStringList& names);

    // in priority order. the same order as displayed in ui
    QVector<QtAV::VideoDecoderId> registeredDecoders() const;
    Config& registeredDecoders(const QVector<QtAV::VideoDecoderId>& all);
    QStringList registeredDecoderNames() const;
    Config& registeredDecoderNames(const QStringList& names);

    QString captureDir() const;
    Config& captureDir(const QString& dir);

    /*!
     * \brief captureFormat
     *  can be "yuv" to capture yuv image without convertion. the suffix is the yuv format, e.g. "yuv420p", "nv12"
     *  or can be "jpg", "png"
     * \return
     */
    QByteArray captureFormat() const;
    Config& captureFormat(const QByteArray& format);
    // only works for non-yuv capture. value: -1~100, -1: default
    int captureQuality() const;
    Config& captureQuality(int quality);

    QVariant operator ()(const QString& key) const;
    Config& operator ()(const QString& key, const QVariant& value);
public:
    //keyword 'signals' maybe protected. we need call the signals in other classes. Q_SIGNAL is empty
    Q_SIGNAL void decodingThreadsChanged(int n);
    Q_SIGNAL void decoderPriorityChanged(const QVector<QtAV::VideoDecoderId>& p);
    Q_SIGNAL void registeredDecodersChanged(const QVector<QtAV::VideoDecoderId>& r);
    Q_SIGNAL void captureDirChanged(const QString& dir);
    Q_SIGNAL void captureFormatChanged(const QByteArray& fmt);
    Q_SIGNAL void captureQualityChanged(int quality);

protected:
    explicit Config(QObject *parent = 0);
    ~Config();

private:
    class Data;
    Data *mpData;
};

#endif // PLAYER_CONFIG_H
