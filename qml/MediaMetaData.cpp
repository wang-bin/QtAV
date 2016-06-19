/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2014)

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
#include <QtCore/QFile>
#include <QtCore/QMetaEnum>
#include <QtCore/QStringList>
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
    setValue(StartTime, (qint64)QTime(0, 0, 0).msecsTo(st.start_time));
    if (st.video.available) {
        setValue(MediaType, QStringLiteral("video"));
        setValue(VideoFrameRate, st.video.frame_rate);
        setValue(VideoBitRate, st.video.bit_rate);
        setValue(VideoCodec, st.video.codec);
        setValue(Resolution, QSize(st.video_only.width, st.video_only.height));
        setValue(PixelFormat, st.video_only.pix_fmt);
        setValue(VideoFrames, st.video.frames);
    }
    if (st.audio.available) {
        // TODO: what if thumbnail?
        if (!st.video.available)
            setValue(MediaType, QStringLiteral("audio"));
        setValue(AudioBitRate, st.audio.bit_rate);
        setValue(AudioCodec, st.audio.codec);
        setValue(ChannelCount, st.audio_only.channels);
        setValue(SampleRate, st.audio_only.sample_rate);
        setValue(ChannelLayout, st.audio_only.channel_layout);
        setValue(SampleFormat, st.audio_only.sample_fmt);
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
        const QString keyName(it.key().toLower());
        if (keyName == QLatin1String("track")) {
            int slash = it.value().indexOf(QLatin1Char('/'));
            if (slash < 0) {
                setValue(TrackNumber, it.value().toInt());
                continue;
            }
            setValue(TrackNumber, it.value().leftRef(slash).string()->toInt()); //QStringRef.toInt(): Qt5.2
            setValue(TrackCount, it.value().midRef(slash+1).string()->toInt());
            continue;
        }
        if (keyName == QLatin1String("date")
                || it.key().toLower() == QLatin1String("creation_time")
                ) {
            // ISO 8601 is preferred in ffmpeg
            bool ok = false;
            int year = it.value().toInt(&ok);
            if (ok) {
                //if (year > 1000) //?
                    setValue(Year, year);
                    continue;
            }
            setValue(Date, QDate::fromString(it.value(), Qt::ISODate));
            continue;
        }
        if (keyName.contains(QLatin1String("genre"))) {
            setValue(Genre, it.value().split(QLatin1Char(','))); // ',' ? not sure
            continue;
        }
    }
    // qtmm read Size from metadata on win
    QFile f(st.url);
    if (f.exists())
        setValue(Size, f.size());
}

MediaMetaData::Key MediaMetaData::fromFFmpegName(const QString &name) const
{
    typedef struct {
        Key key;
        const char* name;
    } key_t;
    key_t key_map[] = {
    { AlbumTitle, "album" }, //?
    { AlbumArtist, "album_artist" },
    { Author, "artist" }, //?
    { Comment, "comment" },
    { Composer, "composer" },
    { Copyright, "copyright" },
    // { Date, "date" }, //QDate
    { Language, "language" }, //maybe a list in ffmpeg
    { Language, "lang" },
    { Publisher, "publisher" },
    { Title, "title" },
    //{ TrackNumber, "track" }, // can be "current/total"
    //{ TrackCount, "track" }, // can be "current/total"

    // below are keys not listed in ffmpeg generic tag names and value is a QString
    { Description, "description" }, //dx
    { (Key)-1, 0 },
    };
    for (int i = 0; (int)key_map[i].key >= 0; ++i) {
        if (name.toLower() == QLatin1String(key_map[i].name))
            return key_map[i].key;
    }
    // below are keys not listed in ffmpeg generic tag names and value is a QString
    key_t wm_key[] = {
        { UserRating, "rating" }, //dx, WM/
        { ParentalRating, "parentalrating" }, //dx, WM/
        //{ RatingOrganization, "rating_organization" },
        { Conductor, "conductor" }, //dx, WM/
        { Lyrics, "lyrics" }, //dx, WM/
        { Mood, "mood" }, //dx, WM/
        { (Key)-1, 0 },
    };
    for (int i = 0; (int)wm_key[i].key >= 0; ++i) {
        if (name.toLower().contains(QLatin1String(wm_key[i].name)))
            return wm_key[i].key;
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
    return QString::fromLatin1(me.valueToKey(k));
}
