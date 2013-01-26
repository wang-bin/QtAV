/******************************************************************************
    VideoCapture.h: description
    Copyright (C) 2012-2013 Wang Bin <wbsecg1@gmail.com>
    
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/


#ifndef VIDEOCAPTURE_H
#define VIDEOCAPTURE_H

#include <QtCore/QObject>
#include <QtCore/QReadWriteLock>
#include <QtAV/QtAV_Global.h>

class QSize;
namespace QtAV {

//on capture per thread or all in one thread?
class Q_EXPORT VideoCapture : public QObject
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
    void request();
    ErrorCode errorCode() const;
    void setFormat(const QString& format);
    QString format() const;
    void setQuality(int quality);
    int quality() const;
    void setCaptureName(const QString& name);
    QString captureName() const;
    void setCaptureDir(const QString& dir);
    QString captureDir() const;
    //get/set: ensure thread safe because they are not in the same thread
    void setRawImage(const QByteArray& raw, const QSize& size);
    void setRawImage(const QByteArray& raw, int w, int h);
    //used to get current playing statistics.
    void getRawImage(QByteArray* raw, int *w, int *h);
signals:
    /*use it to popup a dialog for selecting dir, name etc. TODO: block avthread if not async*/
    void ready();
    void failed();
    void finished();
private:
    friend class CaptureTask;
    bool async;
    ErrorCode error;
    //TODO: use blocking queue? If not, the parameters will change when thre previous is not finished
    //or use a capture event that wrapper all these parameters
    int width, height;
    int qual;
    QString fmt;
    QString name, dir;
    QByteArray data;
    //QImage image;
    mutable QReadWriteLock lock;
};

} //namespace QtAV
#endif // VIDEOCAPTURE_H
