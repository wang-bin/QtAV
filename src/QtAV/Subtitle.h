/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2014 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_SUBTITLE_H
#define QTAV_SUBTITLE_H

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtGui/QImage>
#include <QtAV/QtAV_Global.h>

/*
 * to avoid read error, subtitle size > 10*1024*1024 will be ignored.
 */
namespace QtAV {

class Q_AV_EXPORT SubtitleFrame
{
public:
    SubtitleFrame() :
        begin(0)
      , end(0)
    {}
    // valied: begin < end
    bool isValid() const { return begin < end;}
    operator bool() const { return isValid();}
    bool operator !() const { return !isValid();}
    inline bool operator <(const SubtitleFrame& f) const { return begin < f.begin;}
    inline bool operator <(qreal t) const { return end < t;}
    qreal begin;
    qreal end;
    QString text; //plain text. always valid
    QImage image; //the whole image. it's invalid unless user set it
};

class Q_AV_EXPORT Subtitle : public QObject
{
    Q_OBJECT
    // QList<SubtitleProcessorId>
    Q_PROPERTY(QStringList engines READ engines WRITE setEngines NOTIFY enginesChanged)
    Q_PROPERTY(bool fuzzyMatch READ fuzzyMatch WRITE setFuzzyMatch NOTIFY fuzzyMatchChanged)
    Q_PROPERTY(QByteArray rawData READ rawData WRITE setRawData NOTIFY rawDataChanged)
    Q_PROPERTY(QString fileName READ fileName WRITE setFileName NOTIFY fileNameChanged)
    Q_PROPERTY(QStringList suffixes READ suffixes WRITE setSuffixes NOTIFY suffixesChanged)
    Q_PROPERTY(qreal timestamp READ timestamp WRITE setTimestamp)
    Q_PROPERTY(QString text READ getText)
    Q_PROPERTY(bool loaded READ isLoaded)
public:
    explicit Subtitle(QObject *parent = 0);
    virtual ~Subtitle();
    /*!
     * \brief isValid
     * indicate whether the subtitle can be found and processed
     * \return
     */
    bool isLoaded() const;
    /*!
     * \brief setEngines
     * set subtitle processor engine names.
     * \param value
     */
    void setEngines(const QStringList& value);
    QStringList engines() const;
    void setFuzzyMatch(bool value);
    bool fuzzyMatch() const;
    void setRawData(const QByteArray& data);
    QByteArray rawData() const;
    /*!
     * \brief setFileName
     * the given name will be in the 1st place to try to open(if using fuzzy match). then files in suffixes() order
     * or in processor's supported suffixes order
     * \param name
     */
    void setFileName(const QString& name);
    QString fileName() const;
    void setDir(const QString& dir);
    QString dir() const;
    /*!
     * \brief setSuffixes
     * default is using SubtitleProcessor. empty equals default value.
     */
    void setSuffixes(const QStringList& value);
    QStringList suffixes() const;
    /*!
     * \brief start
     * start to process the whole subtitle content in a thread
     */
    Q_INVOKABLE bool load(); //TODO: slot?

    qreal timestamp() const;
    // call setTimestamp before getText/Image
    Q_INVOKABLE QString getText() const; //plain text
    Q_INVOKABLE QImage getImage(int width, int height);

    // if
    //bool setHeader(const QByteArray& data);
    //bool process(const QByteArray& data, qreal pts = -1);

public slots:
    void setTimestamp(qreal t);
signals:
    void enginesChanged();
    void fuzzyMatchChanged();
    void contentChanged();
    void rawDataChanged();
    void fileNameChanged();
    void suffixesChanged();
private:
    class Private;
    Private *priv;
};

} //namespace QtAV
#endif // QTAV_SUBTITLE_H
