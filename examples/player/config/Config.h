/******************************************************************************
    Config.h: description
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


#ifndef PLAYER_CONFIG_H
#define PLAYER_CONFIG_H

#include <QtAV/VideoDecoderTypes.h>
#include <QtCore/QObject>

class Config : public QObject
{
    Q_OBJECT
public:
    explicit Config(QObject *parent = 0);
    ~Config();

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

private:
    class Data;
    Data *mpData;
};

#endif // PLAYER_CONFIG_H
