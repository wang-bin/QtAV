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

#ifndef QTAV_STATISTICS_H
#define QTAV_STATISTICS_H

#include <QtAV/QtAV_Global.h>
#include <QtCore/QTime>

/*
 * time unit is s
 */

namespace QtAV {

class StatisticsPrivate;
class Q_EXPORT Statistics
{
public:
    Statistics();
    ~Statistics();
    void reset();

    QString url;
    QTime start_time, duration;
    //TODO: filter, decoder, resampler info etc.
    class Common {
    public:
        Common();
        bool available;
        QString format;//?
        QString codec, codec_long;
        //common audio/video info that may be used(visualize) by filters
        QTime current_time, total_time, start_time; //TODO: in AVFormatContext and AVStream, what's the difference?
        //AVStream.avg_frame_rate may be 0, then use AVStream.r_frame_rate
        //http://libav-users.943685.n4.nabble.com/Libav-user-Reading-correct-frame-rate-fps-of-input-video-td4657666.html
        qreal fps_guess;
        qreal fps; //playing fps
        qreal bit_rate;
        qreal avg_frame_rate; //AVStream.avg_frame_rate Kps
        qint64 frames; //AVStream.nb_frames. AVCodecContext.frame_number?
        qint64 size; //audio/video stream size. AVCodecContext.frame_size?

        //union member with ctor, dtor, copy ctor only works in c++11
        /*union {
            audio_only audio;
            video_only video;
        } only;*/
    } audio, video; //init them

    //from AVCodecContext
    class AudioOnly {
    public:
        AudioOnly();
        int sample_rate; ///< samples per second
        int channels;    ///< number of audio channels
        //enum AVSampleFormat sample_fmt;  ///< sample format
        /**
         * Number of samples per channel in an audio frame.
         * - decoding: may be set by some decoders to indicate constant frame size
         */
        int frame_size;
        /**
         * Frame counter, set by libavcodec.
         *   @note the counter is not incremented if encoding/decoding resulted in an error.
         */
        int frame_number;
        /**
         * number of bytes per packet if constant and known or 0
         * Used by some WAV based audio codecs.
         */
        int block_align;
        //int cutoff; //Audio cutoff bandwidth (0 means "automatic")
        //uint64_t channel_layout;
    } audio_only;
    //from AVCodecContext
    class VideoOnly {
    public:
        //union member with ctor, dtor, copy ctor only works in c++11
        VideoOnly();
        int width, height;
        /**
         * Bitstream width / height, may be different from width/height if lowres enabled.
         * - encoding: unused
         * - decoding: Set by user before init if known. Codec should override / dynamically change if needed.
         */
        int coded_width, coded_height;
        /**
         * the number of pictures in a group of pictures, or 0 for intra_only
         */
        int gop_size;
        //enum AVPixelFormat pix_fmt; //TODO: new enum in QtAV
        /**
         * Motion estimation algorithm used for video coding.
         * 1 (zero), 2 (full), 3 (log), 4 (phods), 5 (epzs), 6 (x1), 7 (hex),
         * 8 (umh), 9 (iter), 10 (tesa) [7, 8, 10 are x264 specific, 9 is snow specific]
         */
        //int me_method;
    } video_only;
};

} //namespace QtAV

#endif // QTAV_STATISTICS_H
