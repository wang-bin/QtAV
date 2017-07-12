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
    void setKeepAspectRatio(bool value = true) { m_keep_ar = value; }
    bool isKeepAspectRatio() const { return m_keep_ar; }
    /// AutoDisplayFrame -- default is true. if true, new frames from underlying extractor will update display widget automatically.
    bool isAutoDisplayFrame() const { return m_auto_display; }
    /// If false, new frames (or frame errors) won't automatically update widget
    /// (caller must ensure to call displayFrame()/displayFrame(frame) for this if false).
    /// set to false only if you want to do your own frame caching magic with preview frames.
    void setAutoDisplayFrame(bool b=true);
public Q_SLOTS: // these were previously private but made public to allow calling code to cache some preview frames and directly display frames to this class
    void displayFrame(const QtAV::VideoFrame& frame); //parameter VideoFrame
    void displayNoFrame();
Q_SIGNALS:
    void timestampChanged();
    void fileChanged();
    /// emitted on real decode error -- in that case displayNoFrame() will be automatically called
    void gotError(const QString &);
    /// usually emitted when a new request for a frame came in and current request was aborted. displayNoFrame() will be automatically called
    void gotAbort(const QString &);
    /// useful if calling code is interested in keeping stats on good versus bad frame counts,
    /// or if it wants to cache preview frames. Keeping counts helps caller decide if
    /// preview is working reliably or not for the designated file.
    /// parameter frame will always have: frame.isValid() == true, and will be
    /// already-scaled and in the right format to fit in the preview widget
    void gotFrame(const QtAV::VideoFrame & frame);
protected:
    virtual void resizeEvent(QResizeEvent *);

private:
    bool m_keep_ar, m_auto_display;
    QString m_file;
    VideoFrameExtractor *m_extractor;
    VideoOutput *m_out;
};

} //namespace QtAV
#endif // QTAV_VIDEOPREVIEWWIDGET_H
