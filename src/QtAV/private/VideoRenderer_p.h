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

#ifndef QAV_VIDEORENDERER_P_H
#define QAV_VIDEORENDERER_P_H

#include <private/AVOutput_p.h>
//#include <QtAV/ImageConverter.h>
#include <QtAV/QtAV_Compat.h>
#include <QtCore/QMutex>

class QObject;
namespace QtAV {
class Q_EXPORT VideoRendererPrivate : public AVOutputPrivate
{
public:
    VideoRendererPrivate():scale_in_qt(true),width(480),height(320),src_width(0)
      ,src_height(0) {
        //conv.setInFormat(PIX_FMT_YUV420P);
        //conv.setOutFormat(PIX_FMT_BGR32); //TODO: why not RGB32?
    }
    virtual ~VideoRendererPrivate(){}
    bool scale_in_qt;
    int width, height;
    int src_width, src_height;
    //ImageConverter conv;
    QMutex img_mutex;
};

} //namespace QtAV
#endif // QAV_VIDEORENDERER_P_H
