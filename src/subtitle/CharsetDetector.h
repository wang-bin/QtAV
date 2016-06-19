/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2014)

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

#ifndef QTAV_UCHARDET_H
#define QTAV_UCHARDET_H

#include <QtCore/QByteArray>

class CharsetDetector
{
public:
    CharsetDetector();
    ~CharsetDetector();
    bool isAvailable() const;
    /*!
     * \brief detect
     * \param data text to parse
     * \return charset name
     */
    QByteArray detect(const QByteArray& data);
private:
    class Private;
    Private *priv;
};

#endif // QTAV_UCHARDET_H
