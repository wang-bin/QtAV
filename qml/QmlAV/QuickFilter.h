/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2016)

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
#ifndef QTAV_QUICKFILTER_H
#define QTAV_QUICKFILTER_H
#include <QtAV/Filter.h>

//namespace QtAV { //FIXME: why has error 'Invalid property assignment: "videoFilters" is a read-only property' if use namespace?
using namespace QtAV;
class QuickVideoFilterPrivate;
class QuickVideoFilter : public VideoFilter
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(QuickVideoFilter)
    Q_PROPERTY(QString avfilter READ avfilter WRITE setAVFilter NOTIFY avfilterChanged)
    Q_PROPERTY(QStringList supportedAVFilters READ supportedAVFilters)
    Q_PROPERTY(VideoFilter* userFilter READ userFilter WRITE setUserFilter NOTIFY userFilterChanged)
    Q_PROPERTY(QString fragHeader READ fragHeader WRITE setFragHeader NOTIFY fragHeaderChanged)
    Q_PROPERTY(QString fragSample READ fragSample WRITE setFragSample NOTIFY fragSampleChanged)
    Q_PROPERTY(QString fragPostProcess READ fragPostProcess WRITE setFragPostProcess NOTIFY fragPostProcessChanged)
    Q_ENUMS(FilterType)
    Q_PROPERTY(FilterType type READ type WRITE setType NOTIFY typeChanged)
public:
    enum FilterType {
        AVFilter,
        GLSLFilter,
        UserFilter
    };

    QuickVideoFilter(QObject* parent = 0);

    bool isSupported(VideoFilterContext::Type ct) const Q_DECL_OVERRIDE;

    FilterType type() const;
    void setType(FilterType value);

    QStringList supportedAVFilters() const;
    QString avfilter() const;
    void setAVFilter(const QString& options);

    VideoFilter *userFilter() const;
    void setUserFilter(VideoFilter* f);

    QString fragHeader() const;
    void setFragHeader(const QString& c);
    QString fragSample() const;
    void setFragSample(const QString& c);
    QString fragPostProcess() const;
    void setFragPostProcess(const QString& c);
Q_SIGNALS:
    void avfilterChanged();
    void userFilterChanged();
    void fragHeaderChanged();
    void fragSampleChanged();
    void fragPostProcessChanged();
    void typeChanged();
protected:
    void process(Statistics* statistics, VideoFrame* frame = 0) Q_DECL_OVERRIDE;
};

class QuickAudioFilterPrivate;
class QuickAudioFilter : public AudioFilter
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(QuickAudioFilter)
    Q_PROPERTY(QString avfilter READ avfilter WRITE setAVFilter NOTIFY avfilterChanged)
    Q_PROPERTY(QStringList supportedAVFilters READ supportedAVFilters)
    Q_PROPERTY(AudioFilter* userFilter READ userFilter WRITE setUserFilter NOTIFY userFilterChanged)
public:
    QuickAudioFilter(QObject* parent = 0);

    QStringList supportedAVFilters() const;
    QString avfilter() const;
    void setAVFilter(const QString& options);

    AudioFilter* userFilter() const;
    void setUserFilter(AudioFilter *f);
Q_SIGNALS:
    void avfilterChanged();
    void userFilterChanged();
protected:
    void process(Statistics* statistics, AudioFrame* frame = 0) Q_DECL_OVERRIDE;
};
//} //namespace QtAV
#endif //QTAV_QUICKFILTER_H
