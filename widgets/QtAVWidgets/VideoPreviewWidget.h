/******************************************************************************
    VideoRendererTypes: type id and manually id register function
    Copyright (C) 2015 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_VIDEOPREVIEWWIDGET_H
#define QTAV_VIDEOPREVIEWWIDGET_H

#include <QtAVWidgets/global.h>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QtWidgets/QWidget>
#else
#include <QtGui/QWidget>
#endif

namespace QtAV {

class VideoFrame;
class VideoOutput;
class VideoFrameExtractor;
class Q_AVWIDGETS_EXPORT VideoPreviewWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VideoPreviewWidget(QWidget *parent = 0);
    void setTimestamp(qint64 msec);
    qint64 timestamp() const;
    void preview();
    void setFile(const QString& file);
    QString file() const;
    // default is false
    void setKeepAspectRatio(bool value = true);
    bool isKeepAspectRatio() const;
Q_SIGNALS:
    void timestampChanged();
    void fileChanged();
protected:
    virtual void resizeEvent(QResizeEvent *);
private Q_SLOTS:
    void displayFrame(const QtAV::VideoFrame& frame); //parameter VideoFrame
    void displayNoFrame();

private:
    bool m_keep_ar;
    QString m_file;
    VideoFrameExtractor *m_extractor;
    VideoOutput *m_out;
};

} //namespace QtAV
#endif // QTAV_VIDEOPREVIEWWIDGET_H
