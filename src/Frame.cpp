/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013 Wang Bin <wbsecg1@gmail.com>

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

#include "QtAV/Frame.h"
#include "private/Frame_p.h"

namespace QtAV {

Frame::Frame(FramePrivate &d):
    DPTR_INIT(&d)
{
}

Frame::~Frame()
{

}

int Frame::bytesPerLine(int plane) const
{
    DPTR_D(const Frame);
    if (plane < 0 || plane >= planeCount()) {
        qWarning("Invalid plane! Valid range is [0, %d)", planeCount());
        return 0;
    }
    return 0;
}

QByteArray Frame::data(int plane) const
{
    DPTR_D(const Frame);
    if (plane < 0 || plane >= planeCount()) {
        qWarning("Invalid plane! Valid range is [0, %d)", planeCount());
        return QByteArray();
    }
    return d.planes[plane];
}

uchar* Frame::bits(int plane)
{
    DPTR_D(Frame);
    if (plane < 0 || plane >= planeCount()) {
        qWarning("Invalid plane! Valid range is [0, %d)", planeCount());
        return 0;
    }
    return (uchar*)d.planes[plane].data();
}

const uchar* Frame::bits(int plane) const
{
    DPTR_D(const Frame);
    if (plane < 0 || plane >= planeCount()) {
        qWarning("Invalid plane! Valid range is [0, %d)", planeCount());
        return 0;
    }
    return (const uchar*)d.planes[plane].constData();
}

int Frame::planeCount() const
{
    DPTR_D(const Frame);
    return d.planes.size();
}


/*!
    Returns any extra metadata associated with this frame.
 */
QVariantMap Frame::availableMetaData() const
{
    return d_func().metadata;
}

/*!
    Returns any metadata for this frame for the given \a key.

    This might include frame specific information from
    a camera, or subtitles from a decoded video stream.

    See the documentation for the relevant video frame
    producer for further information about available metadata.
 */
QVariant Frame::metaData(const QString &key) const
{
    return d_func().metadata.value(key);
}

/*!
    Sets the metadata for the given \a key to \a value.

    If \a value is a null variant, any metadata for this key will be removed.

    The producer of the video frame might use this to associate
    certain data with this frame, or for an intermediate processor
    to add information for a consumer of this frame.
 */
void Frame::setMetaData(const QString &key, const QVariant &value)
{
    DPTR_D(Frame);
    if (!value.isNull())
        d.metadata.insert(key, value);
    else
        d.metadata.remove(key);
}

} //namespace QtAV
