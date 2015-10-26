/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2015 Wang Bin <wbsecg1@gmail.com>

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

#ifndef QTAV_FRAME_H
#define QTAV_FRAME_H

#include <QtAV/QtAV_Global.h>
#include <QtCore/QVariant>
#include <QtCore/QSharedData>

// TODO: fromAVFrame() asAVFrame()?
namespace QtAV {

class FramePrivate;
class Q_AV_EXPORT Frame
{
    Q_DECLARE_PRIVATE(Frame)
public:
    Frame(const Frame& other);
    virtual ~Frame() = 0;
    Frame& operator =(const Frame &other);
    /*!
     * \brief planeCount
     *  a decoded frame can be packed and planar. packed format has only 1 plane, while planar
     *  format has more than 1 plane. For audio, the number plane equals channel count. For
     *  video, rgb is 1 plane, yuv420p is 3 plane, p means planar
     * \param plane default is the first plane
     * \return
     */
    int planeCount() const;
    /*!
     * \brief channelCount
     * for audio, channel count equals plane count
     * for video, channels >= planes
     * \return
     */
    virtual int channelCount() const;
    /*!
     * \brief bytesPerLine
     *   For video, it's size of each picture line. For audio, it's the whole size of plane
     * \param plane
     * \return line size of plane
     */
    int bytesPerLine(int plane = 0) const;
    // the whole frame data. may be empty unless clone() or allocate is called
    QByteArray frameData() const;
    // deep copy 1 plane data
    QByteArray data(int plane = 0) const;
    uchar* bits(int plane = 0);
    const uchar *bits(int plane = 0) const { return constBits(plane);}
    const uchar* constBits(int plane = 0) const;
    /*!
     * \brief setBits
     * does nothing if plane is invalid. if given array size is greater than planeCount(), only planeCount() elements is used
     * \param b slice
     * \param plane color/audio channel
     */
    // TODO: const?
    void setBits(uchar *b, int plane = 0);
    void setBits(const QVector<uchar*>& b);
    void setBits(quint8 *slice[]);
    /*!
     * \brief setBytesPerLine
     * does nothing if plane is invalid. if given array size is greater than planeCount(), only planeCount() elements is used
     */
    void setBytesPerLine(int lineSize, int plane = 0);
    void setBytesPerLine(const QVector<int>& lineSize);
    void setBytesPerLine(int stride[]);

    QVariantMap availableMetaData() const;
    QVariant metaData(const QString& key) const;
    void setMetaData(const QString &key, const QVariant &value);
    void setTimestamp(qreal ts);
    qreal timestamp() const;
    inline void swap(Frame &other) { qSwap(d_ptr, other.d_ptr); }

protected:
    Frame(FramePrivate *d);
    QExplicitlySharedDataPointer<FramePrivate> d_ptr;
};

} //namespace QtAV

#endif // QTAV_FRAME_H
