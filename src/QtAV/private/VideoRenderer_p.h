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

#ifndef QAV_VIDEORENDERER_P_H
#define QAV_VIDEORENDERER_P_H

#include <private/AVOutput_p.h>
//#include <QtAV/ImageConverter.h>
#include <QtAV/QtAV_Compat.h>

namespace QtAV {

class EventFilter;
class Q_EXPORT VideoRendererPrivate : public AVOutputPrivate
{
public:
    VideoRendererPrivate():width(480),height(320),src_width(0)
      ,src_height(0),event_filter(0) {
        //conv.setInFormat(PIX_FMT_YUV420P);
        //conv.setOutFormat(PIX_FMT_BGR32); //TODO: why not RGB32?
    }
    virtual ~VideoRendererPrivate(){}
    int width, height;
    int src_width, src_height;
    EventFilter *event_filter;
    //ImageConverter conv;
};

} //namespace QtAV
#endif // QAV_VIDEORENDERER_P_H
