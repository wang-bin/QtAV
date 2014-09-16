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

#ifndef QTAV_QML_MEDIAMETADATA_H
#define QTAV_QML_MEDIAMETADATA_H

#include <QtCore/QDateTime>
#include <QtCore/QObject>
#include <QtCore/QHash>
#include <QtCore/QUrl>
#include <QtCore/QVariant>
#include <QtGui/QImage>
#include <QmlAV/Export.h>

namespace QtAV {
class Statistics;
}
/*!
 * \brief The MediaMetaData class
 * TODO: Music, Movie
 */
class QMLAV_EXPORT MediaMetaData : public QObject
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
    Q_PROPERTY(QVariant audioBitRate READ audioBitRate NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant audioCodec READ audioCodec NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant averageLevel READ averageLevel NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant channelCount READ channelCount NOTIFY metaDataChanged)
    Q_PROPERTY(QVariant peakValue READ peakValue NOTIFY metaDataChanged)
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
        Description, // X
        Category, // X
        Genre, // X
        Year, // X
        Date,
        UserRating, // X
        Keywords, // X
        Language,
        Publisher,
        Copyright,
        ParentalRating, // X
        RatingOrganization, // X

        // Media
        Size, // X. Qt only implemented in dshow "FileSize"
        MediaType, // ?
        Duration,

        // Audio
        AudioBitRate,
        AudioCodec,
        AverageLevel, // X
        ChannelCount,
        PeakValue, // X
        SampleRate,

        // Music
        AlbumTitle, // ?
        AlbumArtist,
        ContributingArtist, // X
        Composer,
        Conductor, // X
        Lyrics, // X
        Mood, // X
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

    QVariant title() const { return value(Title, QString()); }
    QVariant subTitle() const { return value(SubTitle, QString()); }
    QVariant author() const { return value(Author, QStringList()); }
    QVariant comment() const { return value(Comment, QString()); }
    QVariant description() const { return value(Description, QString()); }
    QVariant category() const { return value(Category, QStringList()); }
    QVariant genre() const { return value(Genre, QStringList()); }
    QVariant year() const { return value(Year, int(0)); }
    QVariant date() const { return value(Date, QDate()); }
    QVariant userRating() const { return value(UserRating, int(0)); }
    QVariant keywords() const { return value(Keywords, QStringList()); }
    QVariant language() const { return value(Language, QString()); }
    QVariant publisher() const { return value(Publisher, QString()); }
    QVariant copyright() const { return value(Copyright, QString()); }
    QVariant parentalRating() const { return value(ParentalRating, QString()); }
    QVariant ratingOrganization() const { return value(RatingOrganization, QString()); }
    QVariant size() const { return value(Size, qint64(0)); }
    QVariant mediaType() const { return value(MediaType, QString()); }
    QVariant duration() const { return value(Duration, qint64(0)); }
    QVariant audioBitRate() const { return value(AudioBitRate, int(0)); }
    QVariant audioCodec() const { return value(AudioCodec, QString()); }
    QVariant averageLevel() const { return value(AverageLevel, int(0)); }
    QVariant channelCount() const { return value(ChannelCount, int(0)); }
    QVariant peakValue() const { return value(PeakValue, int(0)); }
    QVariant sampleRate() const { return value(SampleRate, int(0)); }
    QVariant albumTitle() const { return value(AlbumTitle, QString()); }
    QVariant albumArtist() const { return value(AlbumArtist, QString()); }
    QVariant contributingArtist() const { return value(ContributingArtist, QStringList()); }
    QVariant composer() const { return value(Composer, QStringList()); }
    QVariant conductor() const { return value(Conductor, QString()); }
    QVariant lyrics() const { return value(Lyrics, QString()); }
    QVariant mood() const { return value(Mood, QString()); }
    QVariant trackNumber() const { return value(TrackNumber, int(0)); }
    QVariant trackCount() const { return value(TrackCount, int(0)); }
    QVariant coverArtUrlSmall() const { return value(CoverArtUrlSmall, QUrl()); }
    QVariant coverArtUrlLarge() const { return value(CoverArtUrlLarge, QUrl()); }
    QVariant resolution() const { return value(Resolution, QSize()); }
    QVariant pixelAspectRatio() const { return value(PixelAspectRatio, QSize()); }
    QVariant videoFrameRate() const { return value(VideoFrameRate, qreal(0.0)); }
    QVariant videoBitRate() const { return value(VideoBitRate, int(0)); }
    QVariant videoCodec() const { return value(VideoCodec, QString()); }
    QVariant posterUrl() const { return value(PosterUrl, QUrl()); }
    QVariant chapterNumber() const { return value(ChapterNumber, int(0)); }
    QVariant director() const { return value(Director, QString()); }
    QVariant leadPerformer() const { return value(LeadPerformer, QStringList()); }
    QVariant writer() const { return value(Writer, QStringList()); }

Q_SIGNALS:
    void metaDataChanged();

private:
    Key fromFFmpegName(const QString& name) const;
    QHash<Key, QVariant> m_metadata;
};

#endif // QTAV_QML_MEDIAMETADATA_H
