/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
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


#ifndef QTAV_COMMONTYPES_H
#define QTAV_COMMONTYPES_H

#include <QtCore/QMetaType>

namespace QtAV {

enum MediaStatus
{
    UnknownMediaStatus,
    NoMedia,
    LoadingMedia, // when source is set
    LoadedMedia, // if auto load and source is set. player is stopped state
    StalledMedia, // insufficient buffering or other interruptions (timeout, user interrupt)
    BufferingMedia, // NOT IMPLEMENTED
    BufferedMedia, // when playing //NOT IMPLEMENTED
    EndOfMedia, // Playback has reached the end of the current media. The player is in the StoppedState.
    InvalidMedia // what if loop > 0 or stopPosition() is not mediaStopPosition()?
};

enum BufferMode {
    BufferTime,
    BufferBytes,
    BufferPackets
};

enum MediaEndActionFlag {
    MediaEndAction_Default, /// stop playback (if loop end) and clear video renderer
    MediaEndAction_KeepDisplay = 1, /// stop playback but video renderer keeps the last frame
    MediaEndAction_Pause = 1 << 1 /// pause playback. Currently AVPlayer repeat mode will not work if this flag is set
};
Q_DECLARE_FLAGS(MediaEndAction, MediaEndActionFlag)

enum SeekUnit {
    SeekByTime, // only this is supported now
    SeekByByte,
    SeekByFrame
};
enum SeekType {
    AccurateSeek, // slow
    KeyFrameSeek, // fast
    AnyFrameSeek
};

//http://www.itu.int/dms_pubrec/itu-r/rec/bt/R-REC-BT.709-5-200204-I!!PDF-E.pdf
// TODO: other color spaces (yuv itu.xxxx, XYZ, ...)
enum ColorSpace {
    ColorSpace_Unknown,
    ColorSpace_RGB,
    ColorSpace_GBR, // for planar gbr format(e.g. video from x264) used in glsl
    ColorSpace_BT601,
    ColorSpace_BT709
};

/*!
 * \brief The SurfaceType enum
 * HostMemorySurface:
 * Map the decoded frame to host memory
 * GLTextureSurface:
 * Map the decoded frame as an OpenGL texture
 * SourceSurface:
 * get the original surface from decoder, for example VASurfaceID for va-api, CUdeviceptr for CUDA and IDirect3DSurface9* for DXVA.
 * Zero copy mode is required.
 * UserSurface:
 * Do your own magic mapping with it
 */
enum SurfaceType {
    HostMemorySurface,
    GLTextureSurface,
    SourceSurface,
    UserSurface = 0xffff
};

} //namespace QtAV
Q_DECLARE_METATYPE(QtAV::MediaStatus)
Q_DECLARE_METATYPE(QtAV::MediaEndAction)
#endif // QTAV_COMMONTYPES_H
