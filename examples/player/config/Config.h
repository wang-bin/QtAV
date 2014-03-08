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

class Config : public QObject
{
    Q_OBJECT
public:
    static Config& instance();

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

signals:
    void decodingThreadsChanged(int n);
    void decoderPriorityChanged(const QVector<QtAV::VideoDecoderId>& p);

protected:
    explicit Config(QObject *parent = 0);
    ~Config();

private:
    class Data;
    Data *mpData;
};

#endif // PLAYER_CONFIG_H
