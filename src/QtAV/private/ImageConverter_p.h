/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
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


#ifndef QTAV_IMAGECONVERTER_P_H
#define QTAV_IMAGECONVERTER_P_H

#include <QtAV/QtAV_Global.h>
#include <QtCore/QByteArray>

namespace QtAV {

class ImageConverter;
class Q_EXPORT ImageConverterPrivate : public DPtrPrivate<ImageConverter>
{
public:
    ImageConverterPrivate():interlaced(false){}
    bool interlaced;
    int w_in, h_in, w_out, h_out;
    int fmt_in, fmt_out;
    QByteArray data_out;
};

} //namespace QtAV
#endif // QTAV_IMAGECONVERTER_P_H
