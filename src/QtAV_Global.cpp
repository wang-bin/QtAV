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

#include <QtAV/QtAV_Global.h>
#include <QtCore/QObject>
#include <QtCore/QRegExp>

unsigned QtAV_Version()
{
    return QTAV_VERSION;
}

namespace QtAV {

QString aboutQtAV()
{
    return aboutQtAV_HTML().remove(QRegExp("<[^>]*>"));
}

QString aboutQtAV_HTML()
{
    static QString about = "<h3>QtAV " QTAV_VERSION_STR_LONG "</h3>\n"
                "<p>" + QObject::tr("A media playing library base on Qt and FFmpeg.\n") + "</p>"
                "<p>" + QObject::tr("Distributed under the terms of LGPLv2.1 or later.\n") + "</p>"
                "<p>Copyright (C) 2012-2013 Wang Bin (aka. Lucas Wang) <a href='mailto:wbsecg1@gmail.com'>wbsecg1@gmail.com</a></p>\n"
                "<p>" + QObject::tr("Shanghai University, Shanghai, China\n") + "</p>"
                "<br>"
                "<p><a href='https://github.com/wang-bin/QtAV'>https://github.com/wang-bin/QtAV</a></p>\n"
                "<p><a href='https://sourceforge.net/projects/qtav'>https://sourceforge.net/projects/qtav</a></p>";

    return about;
}

}
