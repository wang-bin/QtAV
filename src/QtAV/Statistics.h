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
#include <QtCore/QQueue>
#include <QtCore/QSharedData>

/*
 * time unit is s
 * TODO: frame counter, frame droped. see VLC
 */

/*!
 * values from functions are dynamically calculated
 */
namespace QtAV {

class Q_AV_EXPORT Statistics
{
public:
    Statistics();
    ~Statistics();
    void reset();

    QString url;
    int bit_rate;
    QString format;
    QTime start_time, duration;
    //TODO: filter, decoder, resampler info etc.
    class Common {
    public:
        Common();
        //TODO: dynamic bit rate compute

        bool available;
        QString codec, codec_long;
        QString decoder;
        //common audio/video info that may be used(visualize) by filters
        QTime current_time, total_time, start_time; //TODO: in AVFormatContext and AVStream, what's the difference?
        qreal bit_rate;
        qint64 frames; //AVStream.nb_frames. AVCodecContext.frame_number?
        qint64 size; //audio/video stream size. AVCodecContext.frame_size?

        //union member with ctor, dtor, copy ctor only works in c++11
        /*union {
            audio_only audio;
            video_only video;
        } only;*/
    private:
        class Private : public QSharedData {
        };
        QExplicitlySharedDataPointer<Private> d;
    } audio, video; //init them

    //from AVCodecContext
    class Q_AV_EXPORT AudioOnly {
    public:
        AudioOnly();
        int sample_rate; ///< samples per second
        int channels;    ///< number of audio channels
        QString channel_layout;
        QString sample_fmt;  ///< sample format
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
    private:
        class Private : public QSharedData {
        };
        QExplicitlySharedDataPointer<Private> d;
    } audio_only;
    //from AVCodecContext
    class Q_AV_EXPORT VideoOnly {
    public:
        //union member with ctor, dtor, copy ctor only works in c++11
        VideoOnly();
        // put frame to be renderer pts after decoded
        void putPts(qreal pts);
        // compute from pts history
        qreal currentDisplayFPS() const;
        //AVStream.avg_frame_rate may be 0, then use AVStream.r_frame_rate
        //http://libav-users.943685.n4.nabble.com/Libav-user-Reading-correct-frame-rate-fps-of-input-video-td4657666.html
        qreal fps_guess;
        qreal fps;
        // average fps frame AVStream
        qreal avg_frame_rate; //AVStream.avg_frame_rate Kps

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
        QString pix_fmt; //TODO: new enum in QtAV
        /**
         * Motion estimation algorithm used for video coding.
         * 1 (zero), 2 (full), 3 (log), 4 (phods), 5 (epzs), 6 (x1), 7 (hex),
         * 8 (umh), 9 (iter), 10 (tesa) [7, 8, 10 are x264 specific, 9 is snow specific]
         */
        //int me_method;
    private:
        class Private : public QSharedData {
        public:
            QQueue<qreal> ptsHistory; //compute fps
        };
        QExplicitlySharedDataPointer<Private> d;
    } video_only;
};

} //namespace QtAV

#endif // QTAV_STATISTICS_H
