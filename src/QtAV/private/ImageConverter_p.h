/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
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


#ifndef QTAV_IMAGECONVERTER_P_H
#define QTAV_IMAGECONVERTER_P_H

#include <QtAV/QtAV_Compat.h>
#include <QtCore/QByteArray>

namespace QtAV {

class ImageConverter;
class Q_EXPORT ImageConverterPrivate : public DPtrPrivate<ImageConverter>
{
public:
    ImageConverterPrivate():interlaced(false),w_in(0),h_in(0),w_out(0),h_out(0)
      ,fmt_in(PIX_FMT_YUV420P),fmt_out(PIX_FMT_RGB32){}
    bool interlaced;
    int w_in, h_in, w_out, h_out;
    int fmt_in, fmt_out;
    QByteArray data_out;
};

} //namespace QtAV
#endif // QTAV_IMAGECONVERTER_P_H
