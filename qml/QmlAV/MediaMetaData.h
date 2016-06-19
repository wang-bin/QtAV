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

#ifndef QTAV_QML_MEDIAMETADATA_H
#define QTAV_QML_MEDIAMETADATA_H

#include <QtCore/QDateTime>
#include <QtCore/QObject>
#include <QtCore/QHash>
#include <QtCore/QUrl>
#include <QtCore/QVariant>
#include <QtGui/QImage>

namespace QtAV {
class Statistics;
}
/*!
 * \brief The MediaMetaData class
 * TODO: Music, Movie
 */
class MediaMetaData : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariant title READ title NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant subTitle READ subTitle NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant author READ author NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant comment READ comment NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant description READ description NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant category READ category NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant genre READ genre NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant year READ year NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant date READ date NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant userRating READ userRating NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant keywords READ keywords NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant language READ language NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant publisher READ publisher NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant copyright READ copyright NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant parentalRating READ parentalRating NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant ratingOrganization READ ratingOrganization NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant size READ size NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant mediaType READ mediaType NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant duration READ duration NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant startTime READ startTime NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant audioBitRate READ audioBitRate NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant audioCodec READ audioCodec NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant averageLevel READ averageLevel NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant channelCount READ channelCount NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant channelLayout READ channelLayout NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant peakValue READ peakValue NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant sampleFormat READ sampleFormat NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant sampleRate READ sampleRate NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant albumTitle READ albumTitle NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant albumArtist READ albumArtist NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant contributingArtist READ contributingArtist NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant composer READ composer NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant conductor READ conductor NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant lyrics READ lyrics NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant mood READ mood NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant trackNumber READ trackNumber NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant trackCount READ trackCount NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant coverArtUrlSmall READ coverArtUrlSmall NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant coverArtUrlLarge READ coverArtUrlLarge NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant resolution READ resolution NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant pixelAspectRatio READ pixelAspectRatio NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant videoFrameRate READ videoFrameRate NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant videoBitRate READ videoBitRate NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant videoCodec READ videoCodec NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant pixelFormat READ pixelFormat NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant videoFrames READ videoFrames NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant posterUrl READ posterUrl NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant chapterNumber READ chapterNumber NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant director READ director NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant leadPerformer READ leadPerformer NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant writer READ writer NOTIFY metaDataChanged)
    Q_ENUMS(Key)
public:
    // The same as Qt.Multimedia. Some are not implemented, marked as // X
    enum Key {
        Title,
        SubTitle, // X
        Author,
        Comment,
        Description, // ?
        Category, // X. dx
        Genre, // X
        Year, // X
        Date,
        UserRating, // ?. dx
        Keywords, // x. dx
        Language,
        Publisher,
        Copyright,
        ParentalRating, // X
        RatingOrganization, // X

        // Media
        Size, // X. Qt only implemented in dshow "FileSize"
        MediaType, // ?
        Duration,
        StartTime,

        // Audio
        AudioBitRate,
        AudioCodec,
        AverageLevel, // X. dx
        ChannelCount,
        PeakValue, // X. dx
        SampleRate,
        SampleFormat,
        ChannelLayout,

        // Music
        AlbumTitle, // ?
        AlbumArtist,
        ContributingArtist, // X
        Composer,
        Conductor, // ?
        Lyrics, // ?
        Mood, // ?
        TrackNumber,
        TrackCount,

        CoverArtUrlSmall, // X
        CoverArtUrlLarge, // X

        // Image/Video
        Resolution,
        PixelAspectRatio, // X, AVStream.sample_aspect_ratio

        // Video
        VideoFrameRate,
        VideoBitRate,
        VideoCodec,
        PixelFormat,
        VideoFrames,

        PosterUrl, // X

        // Movie // X
        ChapterNumber,
        Director,
        LeadPerformer,
        Writer,

        PosterImage, // video
        CoverArtImage, // music
    };

    explicit MediaMetaData(QObject *parent = 0);
    void setValuesFromStatistics(const QtAV::Statistics& st);
    void setValue(Key k, const QVariant& v);
    QVariant value(Key k, const QVariant& defaultValue = QVariant()) const;
    QString name(Key k) const;

    QVariant title() const { return value(Title); }
    QVariant subTitle() const { return value(SubTitle); }
    QVariant author() const { return value(Author); }
    QVariant comment() const { return value(Comment); }
    QVariant description() const { return value(Description); }
    QVariant category() const { return value(Category); }
    QVariant genre() const { return value(Genre); }
    QVariant year() const { return value(Year); }
    QVariant date() const { return value(Date); }
    QVariant userRating() const { return value(UserRating); }
    QVariant keywords() const { return value(Keywords); }
    QVariant language() const { return value(Language); }
    QVariant publisher() const { return value(Publisher); }
    QVariant copyright() const { return value(Copyright); }
    QVariant parentalRating() const { return value(ParentalRating); }
    QVariant ratingOrganization() const { return value(RatingOrganization); }
    QVariant size() const { return value(Size); }
    QVariant mediaType() const { return value(MediaType); }
    QVariant duration() const { return value(Duration); }
    QVariant startTime() const { return value(StartTime); }
    QVariant audioBitRate() const { return value(AudioBitRate); }
    QVariant audioCodec() const { return value(AudioCodec); }
    QVariant averageLevel() const { return value(AverageLevel); }
    QVariant channelCount() const { return value(ChannelCount); }
    QVariant channelLayout() const { return value(ChannelLayout); }
    QVariant peakValue() const { return value(PeakValue); }
    QVariant sampleFormat() const { return value(SampleFormat); }
    QVariant sampleRate() const { return value(SampleRate); }
    QVariant albumTitle() const { return value(AlbumTitle); }
    QVariant albumArtist() const { return value(AlbumArtist); }
    QVariant contributingArtist() const { return value(ContributingArtist); }
    QVariant composer() const { return value(Composer); }
    QVariant conductor() const { return value(Conductor); }
    QVariant lyrics() const { return value(Lyrics); }
    QVariant mood() const { return value(Mood); }
    QVariant trackNumber() const { return value(TrackNumber); }
    QVariant trackCount() const { return value(TrackCount); }
    QVariant coverArtUrlSmall() const { return value(CoverArtUrlSmall); }
    QVariant coverArtUrlLarge() const { return value(CoverArtUrlLarge); }
    QVariant resolution() const { return value(Resolution); }
    QVariant pixelAspectRatio() const { return value(PixelAspectRatio); }
    QVariant videoFrameRate() const { return value(VideoFrameRate); }
    QVariant videoBitRate() const { return value(VideoBitRate); }
    QVariant videoCodec() const { return value(VideoCodec); }
    QVariant pixelFormat() const { return value(PixelFormat); }
    QVariant videoFrames() const { return value(VideoFrames); }
    QVariant posterUrl() const { return value(PosterUrl); }
    QVariant chapterNumber() const { return value(ChapterNumber); }
    QVariant director() const { return value(Director); }
    QVariant leadPerformer() const { return value(LeadPerformer); }
    QVariant writer() const { return value(Writer); }

Q_SIGNALS:
    void metaDataChanged();

private:
    Key fromFFmpegName(const QString& name) const;
    QHash<Key, QVariant> m_metadata;
};

#endif // QTAV_QML_MEDIAMETADATA_H
