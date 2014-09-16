/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2014 Wang Bin <wbsecg1@gmail.com>
    theoribeiro <theo@fictix.com.br>

*   This file is part of QtAV

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
******************************************************************************/

#include "QmlAV/MediaMetaData.h"
#include <QtCore/QMetaEnum>
#include <QtAV/Statistics.h>

using namespace QtAV;

MediaMetaData::MediaMetaData(QObject *parent) :
    QObject(parent)
{
}

void MediaMetaData::setValuesFromStatistics(const QtAV::Statistics &st)
{
    m_metadata.clear();
    //setValue(Size, st.);
    setValue(Duration, (qint64)QTime(0, 0, 0).msecsTo(st.duration));
    if (st.video.available) {
        setValue(MediaType, "video");
        setValue(VideoFrameRate, st.video_only.fps);
        setValue(VideoBitRate, st.video.bit_rate);
        setValue(VideoCodec, st.video.codec);
        setValue(Resolution, QSize(st.video_only.width, st.video_only.height));
    }
    if (st.audio.available) {
        // TODO: what if thumbnail?
        if (!st.video.available)
            setValue(MediaType, "audio");
        setValue(AudioBitRate, st.audio.bit_rate);
        setValue(AudioCodec, st.audio.codec);
        setValue(ChannelCount, st.audio_only.channels);
        setValue(SampleRate, st.audio_only.sample_rate);
    }

    QHash<QString, QString> md(st.metadata);
    if (md.isEmpty())
        return;
    QHash<QString, QString>::const_iterator it = md.constBegin();
    for (; it != md.constEnd(); ++it) {
        Key key = fromFFmpegName(it.key());
        if ((int)key >= 0) {
            setValue(key, it.value());
            continue;
        }
        if (it.key().toLower() == QStringLiteral("track")) {
            int slash = it.value().indexOf(QChar('/'));
            if (slash < 0) {
                setValue(TrackNumber, it.value().toInt());
                continue;
            }
            setValue(TrackNumber, it.value().leftRef(slash).toInt());
            setValue(TrackCount, it.value().midRef(slash+1).toInt());
            continue;
        }
        if (it.key().toLower() == QStringLiteral("date")) {
            // ISO 8601 is preferred in ffmpeg
            setValue(Date, QDate::fromString(it.value(), Qt::ISODate));
            continue;
        }
    }
}

MediaMetaData::Key MediaMetaData::fromFFmpegName(const QString &name) const
{
    struct {
        Key key;
        QString name;
    } key_map[] = {
    { AlbumTitle, QStringLiteral("album") }, //?
    { AlbumArtist, QStringLiteral("album_artist") },
    { Author, QStringLiteral("artist") }, //?
    { Comment, QStringLiteral("comment") },
    { Composer, QStringLiteral("composer") },
    { Copyright, QStringLiteral("copyright") },
    // { Date, QStringLiteral("date") }, //QDate
    { Language, QStringLiteral("language") }, //maybe a list in ffmpeg
    { Publisher, QStringLiteral("publisher") },
    { Title, QStringLiteral("title") },
    //{ TrackNumber, QStringLiteral("track") }, // can be "current/total"
    //{ TrackCount, QStringLiteral("track") }, // can be "current/total"
    { (Key)-1, QString() },
    };
    for (int i = 0; (int)key_map[i].key >= 0; ++i) {
        if (name.toLower() == key_map[i].name)
            return key_map[i].key;
    }
    return (Key)-1;
}

void MediaMetaData::setValue(Key k, const QVariant &v)
{
    if (value(k) == v)
        return;
    m_metadata[k] = v;
    emit metaDataChanged();
}

QVariant MediaMetaData::value(Key k, const QVariant &defaultValue) const
{
    return m_metadata.value(k, defaultValue);
}

QString MediaMetaData::name(Key k) const
{
    int idx = staticMetaObject.indexOfEnumerator("Key");
    const QMetaEnum me = staticMetaObject.enumerator(idx);
    return me.valueToKey(k);
}
