/******************************************************************************
    VideoCapture.h: description
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


#ifndef VIDEOCAPTURE_H
#define VIDEOCAPTURE_H

#include <QtCore/QObject>
#include <QtCore/QReadWriteLock>
#include <QtGui/QImage>
#include <QtAV/QtAV_Global.h>
#include <QtAV/VideoFrame.h>

class QSize;
namespace QtAV {

class ImageConverter;
//on capture per thread or all in one thread?
class Q_AV_EXPORT VideoCapture : public QObject
{
    Q_OBJECT
public:
    enum ErrorCode {
        NoError, DirCreateError, SaveError
    };

    explicit VideoCapture(QObject *parent = 0);
    ~VideoCapture();
    void setAsync(bool async);
    bool isAsync() const;
    /*!
     * \brief setAutoSave
     *  If auto save is true, then the captured video frame will be saved as a file when frame is available.
     *  Otherwise, signal ready() and finished() will be emitted without doing other things.
     * \param a
     */
    void setAutoSave(bool a);
    bool autoSave() const;
    /*!
     * \brief setRaw
     *  Save the original frame, can be YUV, NV12 etc. No format converting. default is false
     *  The file name suffix is video frame's format name in lower case, e.g. yuv420p, nv12.
     */
    void setRaw(bool raw);
    bool isRaw() const;
    void request();
    void cancel();
    bool isRequested() const;   
    // you may not call this. It's called by VideoThread and AVPlayer. Will emit ready()
    void start();
    qreal position() const;
    ErrorCode errorCode() const;
    /*!
     * \brief setFormat
     *  Set saved format. can be "PNG", "jpg" etc. Not be used if save raw frame data.
     * \param format image format string like "png", "jpg"
     */
    void setFormat(const QString& format);
    QString format() const;
    /*!
     * \brief setQuality
     *  Set saved image quality. Not be used if save raw frame data.
     * \param quality
     */
    void setQuality(int quality);
    int quality() const;
    /*!
     * \brief name
     * suffix is auto add
     * empty name: filename_timestamp.format(suffix is videoframe.format.name() if save as raw data)
     */
    void setCaptureName(const QString& name);
    QString captureName() const;
    void setCaptureDir(const QString& dir);
    QString captureDir() const;
    Q_DECL_DEPRECATED void getRawImage(QByteArray* raw, int *w, int *h, QImage::Format *fmt = 0);
    /*!
     * \brief getImage, getVideoFrame
     * The result image is not the frame playing when you call getImage(). It's the last requested frame.
     * You may call request() first, and connect ready() signal to your slot. call getImage/VideoFrame()
     * You may also want to disable save as file by calling setAutoSave(false).
     * in the slot
     */
    QImage getImage(QImage::Format format = QImage::Format_RGB32);
    /*!
     * a cloned frame
     */
    void getVideoFrame(VideoFrame& frame);
signals:
    /*use it to popup a dialog for selecting dir, name etc. TODO: block avthread if not async*/
    /*!
     * \brief ready
     *  emitted when requested frame is available.
     */
    void ready();
    void failed();
    void finished();
private:
    void setPosition(qreal pts);
    //get/set: ensure thread safe because they are not in the same thread
    // clone the frame
    void setVideoFrame(const VideoFrame& frame);

    friend class CaptureTask;
    friend class VideoThread;
    bool async;
    bool is_requested;
    bool auto_save;
    bool raw;
    ErrorCode error;
    //TODO: use blocking queue? If not, the parameters will change when thre previous is not finished
    //or use a capture event that wrapper all these parameters
    int qual;
    QImage::Format qfmt;
    QString fmt;
    QString name, dir;
    //QImage image;
    mutable QReadWriteLock lock;
    qreal pts;
    VideoFrame frame;
    ImageConverter *conv;
};

} //namespace QtAV
#endif // VIDEOCAPTURE_H
