/******************************************************************************
    ImageConverter: Base class for image resizing & color model convertion
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


#ifndef QTAV_IMAGECONVERTER_H
#define QTAV_IMAGECONVERTER_H

#include <QtAV/QtAV_Global.h>

namespace QtAV {

class ImageConverterPrivate;
class Q_EXPORT ImageConverter
{
    DPTR_DECLARE_PRIVATE(ImageConverter)
public:
    ImageConverter();

    QByteArray outData() const;
    void setInSize(int width, int height);
    void setOutSize(int width, int height);
    //TODO: new enum. Now using FFmpeg's enum
    void setInFormat(int format);
    void setOutFormat(int format);
    void setInterlaced(bool interlaced);
    bool isInterlaced() const;
    virtual bool convert(const quint8 *const srcSlice[], const int srcStride[]) = 0;
    //virtual bool convertColor(const quint8 *const srcSlice[], const int srcStride[]) = 0;
    //virtual bool resize(const quint8 *const srcSlice[], const int srcStride[]) = 0;
protected:
    ImageConverter(ImageConverterPrivate& d);
    //Allocate memory for out data. Called in setOutFormat()
    virtual bool prepareData();
    DPTR_DECLARE(ImageConverter)
};

} //namespace QtAV
#endif // QTAV_IMAGECONVERTER_H
