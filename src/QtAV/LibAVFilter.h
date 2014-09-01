/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_LIBAVFILTER_H
#define QTAV_LIBAVFILTER_H

#include <QtAV/Filter.h>

// It works only if build against libavfilter. Currently not does not support libav's libavfilter.
namespace QtAV {

class LibAVFilterPrivate;
class Q_AV_EXPORT LibAVFilter : public VideoFilter
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(LibAVFilter)
    Q_PROPERTY(Status status READ status WRITE setStatus NOTIFY statusChanged)
    Q_PROPERTY(QString options READ options WRITE setOptions NOTIFY optionsChanged)
public:
    /*!
     * \brief The Status enum
     * Status of filter graph.
     * If setOptions() is called, the status is NotConfigured, and will to configure when processing
     * a new frame.
     * Filter graph will be reconfigured if options, incoming video frame format or size changed
     */
    enum Status {
        NotConfigured,
        ConfigureFailed,
        ConfigreOk
    };

    LibAVFilter(QObject *parent = 0);
    virtual ~LibAVFilter();
    /*!
     * */
    /*!
     * \brief setOptions
     * Set new option. Filter graph will be setup if receives a frame if options changed.
     * \param options option string for libavfilter. libav and ffmpeg are different
     */
    void setOptions(const QString& options);
    QString options() const;

    Status status() const;
    void setStatus(Status value);

signals:
    void statusChanged();
    void optionsChanged();
protected:
    virtual void process(Statistics *statistics, VideoFrame *frame);
};

} //namespace QtAV

#endif // QTAV_LIBAVFILTER_H
