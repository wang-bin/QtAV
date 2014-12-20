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

// TODO: cancel if qapp is quit
class CaptureTask : public QRunnable
{
public:
    CaptureTask(VideoCapture* c):cap(c){
        original_fmt = false;
        format = "PNG";
        qfmt = QImage::Format_ARGB32;
        setAutoDelete(true);
    }
    virtual void run() {
        bool main_thread = QThread::currentThread() == qApp->thread();
        qDebug("capture task running in thread %p [main thread=%d]", QThread::currentThreadId(), main_thread);
        if (!QDir(dir).exists()) {
            if (!QDir().mkpath(dir)) {
                qWarning("Failed to create capture dir [%s]", qPrintable(dir));
                QMetaObject::invokeMethod(cap, "failed");
                return;
            }
        }
        if (cap->autoSave()) {
            name += QString::number(frame.timestamp(), 'f', 3);
        }
        QString path(dir + "/" + name + ".");
        if (original_fmt) {
            path.append(frame.format().name());
            qDebug("Saving capture to %s", qPrintable(path));
            QFile file(path);
            if (!file.open(QIODevice::WriteOnly)) {
                qWarning("VideoCapture is failed to open file %s", qPrintable(path));
                QMetaObject::invokeMethod(cap, "failed");
                return;
            }
            if (file.write(frame.frameData()) <= 0) {
                qWarning("VideoCapture is failed to write captured frame with original format");
                QMetaObject::invokeMethod(cap, "failed");
                file.close();
                return;
            }
            file.close();
            QMetaObject::invokeMethod(cap, "saved", Q_ARG(QString, path));
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
            qWarning("Failed to save capture");
            QMetaObject::invokeMethod(cap, "failed");
        }
        QMetaObject::invokeMethod(cap, "saved", Q_ARG(QString, path));
    }

    VideoCapture *cap;
    bool original_fmt;
    int quality;
    QString format, dir, name;
    QImage::Format qfmt;
    VideoFrame frame;
};

VideoCapture::VideoCapture(QObject *parent) :
    QObject(parent)
  , async(true)
  , auto_save(true)
  , original_fmt(false)
  , qfmt(QImage::Format_ARGB32)
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

void VideoCapture::setAsync(bool value)
{
    if (async == value)
        return;
    async = value;
    emit asyncChanged();
}

bool VideoCapture::isAsync() const
{
    return async;
}

void VideoCapture::setAutoSave(bool value)
{
    if (auto_save == value)
        return;
    auto_save = value;
    emit autoSaveChanged();
}

bool VideoCapture::autoSave() const
{
    return auto_save;
}

void VideoCapture::setOriginalFormat(bool value)
{
    if (original_fmt == value)
        return;
    original_fmt = value;
    emit originalFormatChanged();
}

bool VideoCapture::isOriginalFormat() const
{
    return original_fmt;
}

void VideoCapture::request()
{
    emit requested();
}

void VideoCapture::start()
{
    VideoFrame vf(frame);
    emit ready(vf.clone()); //TODO: no copy
    if (!auto_save) {
        return;
    }
    CaptureTask *task = new CaptureTask(this);
    task->original_fmt = original_fmt;
    task->quality = qual;
    task->dir = dir;
    task->name = name;
    task->format = fmt;
    task->qfmt = qfmt;
    task->frame = frame; //copy here and it's safe in capture thread because start() is called immediatly after setVideoFrame
    if (isAsync()) {
        QThreadPool::globalInstance()->start(task);
    } else {
        task->run();
    }
}

void VideoCapture::setSaveFormat(const QString &format)
{
    if (format.toLower() == fmt.toLower())
        return;
    fmt = format;
    emit saveFormatChanged();
}

QString VideoCapture::saveFormat() const
{
    return fmt;
}

void VideoCapture::setQuality(int value)
{
    if (qual == value)
        return;
    qual = value;
    emit qualityChanged();
}

int VideoCapture::quality() const
{
    return qual;
}

void VideoCapture::setCaptureName(const QString &value)
{
    if (name == value)
        return;
    name = value;
    emit captureNameChanged();
}

QString VideoCapture::captureName() const
{
    return name;
}

void VideoCapture::setCaptureDir(const QString &value)
{
    if (dir == value)
        return;
    dir = value;
    emit captureDirChanged();
}

QString VideoCapture::captureDir() const
{
    return dir;
}

/*
 * If the frame is not created for direct rendering, then the frame data is already deep copied, so detach is enough.
 * TODO: map frame from texture etc.
 */
void VideoCapture::setVideoFrame(const VideoFrame &frame)
{
    // parameter in ready(QtAV::VideoFrame) ensure we can access the frame without lock
    /*
     * clone here may block VideoThread. But if not clone here, the frame may be
     * modified outside and is not safe.
     */
    this->frame = frame.clone(); // TODO: no clone, use detach()
    this->frame.setImageConverter(conv);
}

} //namespace QtAV

