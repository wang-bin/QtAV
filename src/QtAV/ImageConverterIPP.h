/******************************************************************************
    ImageConverterIPP: Image resizing & color model convertion using Intel IPP
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

#ifndef QTAV_IMAGECONVERTERIPP_H
#define QTAV_IMAGECONVERTERIPP_H

#include <QtAV/ImageConverter.h>

namespace QtAV {

class ImageConverterIPPPrivate;
class Q_EXPORT ImageConverterIPP : public ImageConverter
{
    DPTR_DECLARE_PRIVATE(ImageConverterIPP)
public:
    ImageConverterIPP();
    virtual bool convert(const quint8 *const srcSlice[], const int srcStride[]);
protected:
    virtual bool prepareData(); //Allocate memory for out data
};
} //namespace QtAV
#endif // QTAV_IMAGECONVERTERIPP_H
