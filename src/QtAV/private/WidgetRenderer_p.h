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


#ifndef QTAV_WIDGETRENDERER_P_H
#define QTAV_WIDGETRENDERER_P_H

#include <private/QPainterRenderer_p.h>
#include <QtAV/WidgetRenderer.h>

namespace QtAV {

class Q_EXPORT WidgetRendererPrivate : public QPainterRendererPrivate
{
public:
    virtual ~WidgetRendererPrivate(){}
};

} //namespace QtAV
#endif // QTAV_WIDGETRENDERER_P_H
