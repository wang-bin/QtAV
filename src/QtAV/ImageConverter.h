/******************************************************************************
    ImageConverter: Base class for image resizing & color model convertion
    Copyright (C) 2012-2013 Wang Bin <wbsecg1@gmail.com>
    
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


#ifndef QTAV_IMAGECONVERTER_H
#define QTAV_IMAGECONVERTER_H

#include <QtAV/QtAV_Global.h>
#include <QtAV/FactoryDefine.h>
#include <QtAV/VideoFormat.h>
#include <QtCore/QVector>

namespace QtAV {

typedef int ImageConverterId;
class ImageConverter;
FACTORY_DECLARE(ImageConverter)

class ImageConverterPrivate;
class Q_AV_EXPORT ImageConverter //export is not needed
{
    DPTR_DECLARE_PRIVATE(ImageConverter)
public:
    ImageConverter();
    virtual ~ImageConverter();

    QByteArray outData() const;

    // return false if i/o format not supported, or size is not valid.
    virtual bool check() const;
    void setInSize(int width, int height);
    void setOutSize(int width, int height);
    void setInFormat(const VideoFormat& format);
    void setInFormat(VideoFormat::PixelFormat format);
    void setInFormat(int format);
    void setOutFormat(const VideoFormat& format);
    void setOutFormat(VideoFormat::PixelFormat format);
    void setOutFormat(int formate);
    void setInterlaced(bool interlaced);
    bool isInterlaced() const;
    /*!
     * brightness, contrast, saturation: -100~100
     * If value changes, setup sws
     */
    void setBrightness(int value);
    int brightness() const;
    void setContrast(int value);
    int contrast() const;
    void setSaturation(int value);
    int saturation() const;
    QVector<quint8*> outPlanes() const;
    QVector<int> outLineSizes() const;
    virtual bool convert(const quint8 *const srcSlice[], const int srcStride[]) = 0;
    //virtual bool convertColor(const quint8 *const srcSlice[], const int srcStride[]) = 0;
    //virtual bool resize(const quint8 *const srcSlice[], const int srcStride[]) = 0;
protected:
    ImageConverter(ImageConverterPrivate& d);
    //Allocate memory for out data. Called in setOutFormat()
    virtual bool setupColorspaceDetails();
    virtual bool prepareData(); //Allocate memory for out data
    DPTR_DECLARE(ImageConverter)
};

} //namespace QtAV
#endif // QTAV_IMAGECONVERTER_H
