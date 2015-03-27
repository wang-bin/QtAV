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

class Q_AV_EXPORT LibAVFilter
{
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

    LibAVFilter();
    virtual ~LibAVFilter();
    /*!
     * \brief setOptions
     * Set new option. Filter graph will be setup if receives a frame if options changed.
     * \param options option string for libavfilter. libav and ffmpeg are different
     */
    void setOptions(const QString& options);
    QString options() const;

    Status status() const;
protected:
    virtual QString sourceArguments() const = 0;
    bool pushVideoFrame(Frame* frame, bool changed);
    bool pushAudioFrame(Frame* frame, bool changed);
    void* pullFrameHolder();
private:
    virtual void emitOptionsChanged() {}
    class Private;
    Private *priv;
};

class Q_AV_EXPORT LibAVFilterVideo : public VideoFilter, public LibAVFilter
{
    Q_OBJECT
    Q_PROPERTY(QString options READ options WRITE setOptions NOTIFY optionsChanged)
public:
    LibAVFilterVideo(QObject *parent = 0);
Q_SIGNALS:
    void optionsChanged();
protected:
    void process(Statistics *statistics, VideoFrame *frame) Q_DECL_FINAL;
    QString sourceArguments() const Q_DECL_FINAL;
private:
    void emitOptionsChanged() Q_DECL_FINAL;
};

class Q_AV_EXPORT LibAVFilterAudio : public AudioFilter, public LibAVFilter
{
    Q_OBJECT
    Q_PROPERTY(QString options READ options WRITE setOptions NOTIFY optionsChanged)
public:
    LibAVFilterAudio(QObject *parent = 0);
Q_SIGNALS:
    void optionsChanged();
protected:
    void process(Statistics *statistics, AudioFrame *frame) Q_DECL_FINAL;
    QString sourceArguments() const Q_DECL_FINAL;
private:
    void emitOptionsChanged() Q_DECL_FINAL;
};

} //namespace QtAV

#endif // QTAV_LIBAVFILTER_H
