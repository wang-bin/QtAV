/******************************************************************************
    VideoCapture.cpp: description
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


#include <QtAV/VideoCapture.h>
#include <QtAV/ImageConverterTypes.h>
#include <QtCore/QThread>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QRunnable>
#include <QtCore/QThreadPool>
#include <QtGui/QImage>
#include "utils/Logger.h"

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include <QtGui/QDesktopServices>
#else
#include <QtCore/QStandardPaths>
#endif
namespace QtAV {

class CaptureTask : public QRunnable
{
public:
    CaptureTask(VideoCapture* c):cap(c){
        raw = false;
        format = "PNG";
        qfmt = QImage::Format_ARGB32;
        setAutoDelete(true);
    }
    virtual void run() {
        bool main_thread = QThread::currentThread() == qApp->thread();
        qDebug("capture task running in thread %p [main thread=%d]", QThread::currentThreadId(), main_thread);
        if (!QDir(dir).exists()) {
            if (!QDir().mkpath(dir)) {
                cap->error = VideoCapture::DirCreateError;
                qWarning("Failed to create capture dir [%s]", qPrintable(dir));
                QMetaObject::invokeMethod(cap, "failed");
                return;
            }
        }
        QString path(dir + "/" + name + ".");
#define FRAME_CLONE_OK 0
#if FRAME_CLONE_OK
        // if no clone, frameData() is empty
        frame = frame.clone();
        //frame.setImageConverter();
#endif //FRAME_CLONE_OK
        if (raw) {
            path.append(frame.format().name());
            QFile file(path);
            if (!file.open(QIODevice::WriteOnly)) {
                qWarning("Failed to open file %s", qPrintable(path));
                QMetaObject::invokeMethod(cap, "failed");
                return;
            }
#if FRAME_CLONE_OK
            if (file.write(frame.frameData()) <= 0) {
                qWarning("Failed to write captured raw frame");
                QMetaObject::invokeMethod(cap, "failed");
                file.close();
                return;
            }

#else
            int len = 0;
            for (int i = 0; i < frame.planeCount(); ++i) {
                qDebug("writing %d %d %d", i, frame.bytesPerLine(i), frame.planeWidth(i));
                len = file.write((const char*)frame.bits(i), frame.bytesPerLine(i)*frame.planeHeight(i));
                if (len < 0) {
                    qWarning("Failed to write caputred frame at plane %d. %d bytes written.", i, len);
                    QMetaObject::invokeMethod(cap, "failed");
                    file.close();
                    return;
                }
            }
#endif //FRAME_CLONE_OK
            file.close();
            qDebug("Saving capture to %s", qPrintable(path));
            QMetaObject::invokeMethod(cap, "finished");
            return;
        }
        path.append(format.toLower());
        if (!frame.convertTo(qfmt)) {
            qWarning("Failed to convert captured frame");
            return;
        }

        QImage image((const uchar*)frame.frameData().constData(), frame.width(), frame.height(), frame.bytesPerLine(), qfmt);
        qDebug("Saving capture to %s", qPrintable(path));
        bool ok = image.save(path, format.toLatin1().constData(), quality);
        if (!ok) {
            cap->error = VideoCapture::SaveError;
            qWarning("Failed to save capture");
            QMetaObject::invokeMethod(cap, "failed");
        }
        QMetaObject::invokeMethod(cap, "finished");
    }

    VideoCapture *cap;
    bool raw;
    int quality;
    QString format, dir, name;
    QImage::Format qfmt;
    VideoFrame frame;
};

VideoCapture::VideoCapture(QObject *parent) :
    QObject(parent)
  , async(true)
  , is_requested(false)
  , auto_save(true)
  , raw(false)
  , error(NoError)
  , qfmt(QImage::Format_ARGB32)
  , pts(0)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    dir = QDesktopServices::storageLocation(QDesktopServices::PicturesLocation);
#else
    dir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
#endif
    if (dir.isEmpty())
        dir = qApp->applicationDirPath() + "/capture";
    fmt = "PNG";
    qual = -1;
    conv = ImageConverterFactory::create(ImageConverterId_FF);
}

VideoCapture::~VideoCapture()
{
    qDebug("%p %s %s", QThread::currentThreadId(), __FILE__, __FUNCTION__);
}

void VideoCapture::setAsync(bool async)
{
    this->async = async;
}

bool VideoCapture::isAsync() const
{
    return async;
}

void VideoCapture::setAutoSave(bool a)
{
    auto_save = a;
}

bool VideoCapture::autoSave() const
{
    return auto_save;
}

void VideoCapture::setRaw(bool raw)
{
    this->raw = raw;
}

bool VideoCapture::isRaw() const
{
    return raw;
}

void VideoCapture::request()
{
    is_requested = true;
}

void VideoCapture::cancel()
{
    is_requested = false;
}

bool VideoCapture::isRequested() const
{
    return is_requested;
}

void VideoCapture::start()
{
    //QReadLocker locker(&lock);
    //Q_UNUSED(locker);
    is_requested = false;
    error = NoError;
    emit ready();
    if (!auto_save) {
        emit finished();
        return;
    }
    CaptureTask *task = new CaptureTask(this);
    task->raw = raw;
    task->quality = qual;
    task->dir = dir;
    task->name = name;
    task->format = fmt;
    task->qfmt = qfmt;
    task->frame = frame;
    if (isAsync()) {
        QThreadPool::globalInstance()->start(task);
    } else {
        task->run();
    }
}

qreal VideoCapture::position() const
{
    return pts;
}

void VideoCapture::setPosition(qreal pts)
{
    this->pts = pts;
}

void VideoCapture::setFormat(const QString &format)
{
    fmt = format;
}

QString VideoCapture::format() const
{
    return fmt;
}

void VideoCapture::setQuality(int quality)
{
    qual = quality;
}

int VideoCapture::quality() const
{
    return qual;
}

void VideoCapture::setCaptureName(const QString &name)
{
    this->name = name;
}

QString VideoCapture::captureName() const
{
    return name;
}

void VideoCapture::setCaptureDir(const QString &dir)
{
    this->dir = dir;
}

QString VideoCapture::captureDir() const
{
    return dir;
}

void VideoCapture::getRawImage(QByteArray *raw, int *w, int *h, QImage::Format *fmt)
{
    QReadLocker locker(&lock);
    Q_UNUSED(locker);
    if (!frame.isValid()) {
        *raw = 0;
        return;
    }
    /*
     * no clone required because the lock ensure the \a frame will not change now.
     * And the result QByteArray is implicitly shared so it's safe if frame changes later
     */
    frame.setImageConverter(conv);
    if (!frame.convertTo(qfmt)) {
        *raw = 0;
        return;
    }
    // frame is a cloned frame in setVideoFrame(), so frameData() is availabe
    *raw = frame.frameData();
    *w = frame.width();
    *h = frame.height();
    if (fmt)
        *fmt = qfmt;
}

QImage VideoCapture::getImage(QImage::Format format)
{
    QReadLocker locker(&lock);
    Q_UNUSED(locker);
    if (!frame.isValid()) {
        qWarning("getImage Invalid frame");
        return QImage();
    }
    /*
     * no clone required because the lock ensure the \a frame will not change now.
     * And the result QByteArray is implicitly shared so it's safe if frame changes later
     */
    frame.setImageConverter(conv);
    if (!frame.convertTo(format)) {
        qWarning("Failed to convert to QImage");
        return QImage();
    }
    // frame is a cloned frame in setVideoFrame(), so frameData() is availabe
    // return a copy of image. because QImage from memory does not own the memory
    return QImage((const uchar*)frame.frameData().constData(), frame.width(), frame.height(), frame.bytesPerLine(), format).copy();
}

void VideoCapture::getVideoFrame(VideoFrame &frame)
{
    QReadLocker locker(&lock);
    Q_UNUSED(locker);
    frame = this->frame.clone();
}

void VideoCapture::setVideoFrame(const VideoFrame &frame)
{
    QReadLocker locker(&lock);
    Q_UNUSED(locker);
    /*
     * clone here may block VideoThread. But if not clone here, the frame may be
     * modified outside and is not safe.
     */
    this->frame = frame.clone();
    this->frame.setImageConverter(conv);
}

} //namespace QtAV

