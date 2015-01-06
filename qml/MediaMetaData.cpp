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
#include <QtCore/QFile>
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
        setValue(VideoFrameRate, st.video_only.frame_rate);
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
        const QString keyName(it.key().toLower());
        if (keyName == QStringLiteral("track")) {
            int slash = it.value().indexOf(QChar('/'));
            if (slash < 0) {
                setValue(TrackNumber, it.value().toInt());
                continue;
            }
            setValue(TrackNumber, it.value().leftRef(slash).toInt());
            setValue(TrackCount, it.value().midRef(slash+1).toInt());
            continue;
        }
        if (keyName == QStringLiteral("date")
                || it.key().toLower() == QStringLiteral("creation_time")
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
        if (keyName.contains(QStringLiteral("genre"))) {
            setValue(Genre, it.value().split(QChar(','))); // ',' ? not sure
            continue;
        }
    }
    // qtmm read Size from metadata on win
    QFile f(st.url);
    if (f.exists())
        setValue(Size, f.size());
}

#if defined(Q_COMPILER_INITIALIZER_LISTS)
MediaMetaData::Key MediaMetaData::fromFFmpegName(const QString &name) const
{
    typedef struct {
        Key key;
        QString name;
    } key_t;
    key_t key_map[] = {
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

    // below are keys not listed in ffmpeg generic tag names and value is a QString
    { Description, QStringLiteral("description") }, //dx
    { (Key)-1, QString() },
    };
    for (int i = 0; (int)key_map[i].key >= 0; ++i) {
        if (name.toLower() == key_map[i].name)
            return key_map[i].key;
    }
    // below are keys not listed in ffmpeg generic tag names and value is a QString
    key_t wm_key[] = {
        { UserRating, QStringLiteral("rating") }, //dx, WM/
        { ParentalRating, QStringLiteral("parentalrating") }, //dx, WM/
        //{ RatingOrganization, QStringLiteral("rating_organization") },
        { Conductor, QStringLiteral("conductor") }, //dx, WM/
        { Lyrics, QStringLiteral("lyrics") }, //dx, WM/
        { Mood, QStringLiteral("mood") }, //dx, WM/
        { (Key)-1, QString() },
    };
    for (int i = 0; (int)wm_key[i].key >= 0; ++i) {
        if (name.toLower().contains(wm_key[i].name))
            return wm_key[i].key;
    }
    return (Key)-1;
}
#else
MediaMetaData::Key MediaMetaData::fromFFmpegName(const QString &name) const
{
    typedef QMap<QString, Key> key_t;

    static key_t key_map;
    if ( key_map.isEmpty() ) {
        key_map.insert( QStringLiteral("album"), AlbumTitle ); //?
        key_map.insert( QStringLiteral("album_artist"), AlbumArtist );
        key_map.insert( QStringLiteral("artist"), Author ); //?
        key_map.insert( QStringLiteral("comment"), Comment );
        key_map.insert( QStringLiteral("composer"), Composer );
        key_map.insert( QStringLiteral("copyright"), Copyright );
        // key_map.insert( QStringLiteral("date"), Date ); //QDate
        key_map.insert( QStringLiteral("language"), Language ); //maybe a list in ffmpeg
        key_map.insert( QStringLiteral("publisher"), Publisher );
        key_map.insert( QStringLiteral("title"), Title );
        //key_map.insert( QStringLiteral("track"), TrackNumber ); // can be "current/total"
        //key_map.insert( QStringLiteral("track"),  TrackCount ); // can be "current/total"

        // below are keys not listed in ffmpeg generic tag names and value is a QString
        key_map.insert( QStringLiteral("description"), Description ); //dx
    }

    key_t::const_iterator it = key_map.find( name );
    if ( it != key_map.constEnd() )
        return it.value();

    // below are keys not listed in ffmpeg generic tag names and value is a QString
    static key_t wm_key;
    if ( wm_key.isEmpty() ) {
        wm_key.insert( QStringLiteral("rating"), UserRating ); //dx, WM/
        wm_key.insert( QStringLiteral("parentalrating"), ParentalRating ); //dx, WM/
        //wm_key.insert( RatingOrganization, QStringLiteral("rating_organization") );
        wm_key.insert( QStringLiteral("conductor"), Conductor ); //dx, WM/
        wm_key.insert( QStringLiteral("lyrics"), Lyrics ); //dx, WM/
        wm_key.insert( QStringLiteral("mood"), Mood ); //dx, WM/
    }

    it = wm_key.find( name );
    if ( it != wm_key.constEnd() )
        return it.value();

    return (Key)-1;
}
#endif

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
