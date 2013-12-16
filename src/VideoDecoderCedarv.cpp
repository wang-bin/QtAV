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

#include "QtAV/VideoDecoderFFmpeg.h"
#include "private/VideoDecoderFFmpeg_p.h"
#include <QtAV/Packet.h>
#include <QtAV/QtAV_Compat.h>
#include "prepost.h"

#include <QDebug>

#include <libavcodec/avcodec.h>
extern "C"
{
	#include <libcedarv/libcedarv.h>
}

static void map32x32_to_yuv_Y(unsigned char* srcY, unsigned char* tarY, unsigned int coded_width, unsigned int coded_height)
{
	unsigned int i,j,l,m,n;
	unsigned int mb_width,mb_height,twomb_line,recon_width;
	unsigned long offset;
	unsigned char *ptr;

	ptr = srcY;
	mb_width = (coded_width+15)>>4;
	mb_height = (coded_height+15)>>4;
	twomb_line = (mb_height+1)>>1;
	recon_width = (mb_width+1)&0xfffffffe;

	for(i=0;i<twomb_line;i++)
	{
		for(j=0;j<recon_width;j+=2)
		{
			for(l=0;l<32;l++)
			{
				//first mb
				m=i*32 + l;
				n= j*16;
				if(m<coded_height && n<coded_width)
				{
					offset = m*coded_width + n;
					memcpy(tarY+offset,ptr,16);
					ptr += 16;
				}
				else
					ptr += 16;

				//second mb
				n= j*16+16;
				if(m<coded_height && n<coded_width)
				{
					offset = m*coded_width + n;
					memcpy(tarY+offset,ptr,16);
					ptr += 16;
				}
				else
					ptr += 16;
			}
		}
	}
}

static void map32x32_to_yuv_C(unsigned char* srcC,unsigned char* tarCb,unsigned char* tarCr,unsigned int coded_width,unsigned int coded_height)
{
	/*
	unsigned int i,j,k;

	for (i = 0; i < coded_width; ++i) {
		k = i * coded_width;
		for (j = 0; j < coded_height; ++j) {
			*(tarCb + k) = srcC[2*k];
			*(tarCr + k) = srcC[2*k+1];
			k++;
		}
	}
	*/
	unsigned int i,j,l,m,n,k;
	unsigned int mb_width,mb_height,fourmb_line,recon_width;
	unsigned char line[16];
	unsigned long offset;
	unsigned char *ptr;

	ptr = srcC;
	mb_width = (coded_width+7)>>3;
	mb_height = (coded_height+7)>>3;
	fourmb_line = (mb_height+3)>>2;
	recon_width = (mb_width+1)&0xfffffffe;

	for(i=0;i<fourmb_line;i++)
	{
		for(j=0;j<recon_width;j+=2)
		{
			for(l=0;l<32;l++)
			{
				//first mb
				m=i*32 + l;
				n= j*8;
				if(m<coded_height && n<coded_width)
				{
					offset = m*coded_width + n;
					memcpy(line,ptr,16);
					for(k=0;k<8;k++)
					{
						*(tarCb + offset + k) = line[2*k];
						*(tarCr + offset + k) = line[2*k+1];
					}
					ptr += 16;
				}
				else
					ptr += 16;

				//second mb
				n= j*8+8;
				if(m<coded_height && n<coded_width)
				{
					offset = m*coded_width + n;
					memcpy(line,ptr,16);
					for(k=0;k<8;k++)
					{
						*(tarCb + offset + k) = line[2*k];
						*(tarCr + offset + k) = line[2*k+1];
					}
					ptr += 16;
				}
				else
					ptr += 16;
			}
		}
	}
}

namespace QtAV {

class VideoDecoderCedarvPrivate;
class VideoDecoderCedarv : public VideoDecoderFFmpeg
{
	DPTR_DECLARE_PRIVATE(VideoDecoderCedarv)
public:
	VideoDecoderCedarv();
	bool prepare();
	bool decode(const QByteArray &encoded);
	VideoFrame frame();
};

extern VideoDecoderId VideoDecoderId_Cedarv;
FACTORY_REGISTER_ID_AUTO(VideoDecoder, Cedarv, "Cedarv")

void RegisterVideoDecoderCedarv_Man()
{
	FACTORY_REGISTER_ID_MAN(VideoDecoder, Cedarv, "Cedarv")
}

class VideoDecoderCedarvPrivate : public VideoDecoderFFmpegPrivate
{
public:
	VideoDecoderCedarvPrivate() {
	   cedarv = 0;
	}

	~VideoDecoderCedarvPrivate() {
		//TODO:
	}

	CEDARV_DECODER *cedarv;
	cedarv_picture_t cedarPicture;
	QByteArray y;
	QByteArray u;
	QByteArray v;
};

VideoDecoderCedarv::VideoDecoderCedarv()
	: VideoDecoderFFmpeg(*new VideoDecoderCedarvPrivate())
{
}


bool VideoDecoderCedarv::prepare()
{
	DPTR_D(VideoDecoderCedarv);
	if (!d.cedarv) {
		int ret;
		d.cedarv = libcedarv_init(&ret);
		if (ret < 0 || d.cedarv == NULL)
			return false;
	}

	d.codec_ctx->opaque = &d; //is it ok?

	cedarv_stream_info_t cedarStreamInfo;
	memset(&cedarStreamInfo, 0, sizeof cedarStreamInfo);

	switch (d.codec_ctx->codec_id) {
		case AV_CODEC_ID_H264:
			cedarStreamInfo.format = CEDARV_STREAM_FORMAT_H264;
			break;
		case AV_CODEC_ID_VP8:
			cedarStreamInfo.format = CEDARV_STREAM_FORMAT_VP8;
			break;
		case AV_CODEC_ID_VC1:
			cedarStreamInfo.format = CEDARV_STREAM_FORMAT_VC1;
			break;
		case AV_CODEC_ID_MPEG4:
			cedarStreamInfo.format = CEDARV_STREAM_FORMAT_MPEG4;
			cedarStreamInfo.sub_format = CEDARV_MPEG4_SUB_FORMAT_XVID;
			break;
		case AV_CODEC_ID_MPEG2VIDEO:
			cedarStreamInfo.format = CEDARV_STREAM_FORMAT_MPEG2;
			break;
		case AV_CODEC_ID_RV40:
			cedarStreamInfo.format = CEDARV_STREAM_FORMAT_REALVIDEO;
			break;
		default:
			return false;
	}
	cedarStreamInfo.video_width = d.codec_ctx->width;
	cedarStreamInfo.video_height = d.codec_ctx->height;
	if (d.codec_ctx->extradata_size) {
		cedarStreamInfo.init_data = d.codec_ctx->extradata;
		cedarStreamInfo.init_data_len = d.codec_ctx->extradata_size;
	}

	int cedarvRet;
	cedarvRet = d.cedarv->set_vstream_info(d.cedarv, &cedarStreamInfo);
	if (cedarvRet < 0)
		return false;
	cedarvRet = d.cedarv->open(d.cedarv);
	if (cedarvRet < 0)
		return false;

	d.cedarv->ioctrl(d.cedarv, CEDARV_COMMAND_PLAY, 0);

	return true;
}

bool VideoDecoderCedarv::decode(const QByteArray &encoded)
{
	DPTR_D(VideoDecoderCedarv);

	//d.cedarv->ioctrl(d.cedarv, CEDARV_COMMAND_JUMP, 0);

	AVPacket packet;
	av_new_packet(&packet, encoded.size());
	memcpy(packet.data, encoded.data(), encoded.size());

	if (packet.size == 0) {
		return true;
	}

	u32 bufsize0, bufsize1;
	u8 *buf0, *buf1;

	if (d.cedarv->request_write(d.cedarv, packet.size, &buf0, &bufsize0, &buf1, &bufsize1) >= 0) {
		memcpy(buf0, packet.data, bufsize0);
		if ((u32)packet.size > bufsize0) {
			memcpy(buf1, packet.data + bufsize0, bufsize1);
		}
		cedarv_stream_data_info_t stream_data_info;
		stream_data_info.type = 0;
		stream_data_info.lengh = packet.size;
		stream_data_info.pts = packet.pts;
		stream_data_info.flags = CEDARV_FLAG_FIRST_PART | CEDARV_FLAG_LAST_PART | CEDARV_FLAG_PTS_VALID;
		d.cedarv->update_data(d.cedarv, &stream_data_info);
		if (d.cedarv->decode(d.cedarv) >= 0 && !d.cedarv->display_request(d.cedarv, &d.cedarPicture)) {
		}
		else {
			if (d.cedarPicture.id) {
				d.cedarv->display_release(d.cedarv, d.cedarPicture.id);
				d.cedarPicture.id = 0;
			}
		}
	}
	return true;
}


VideoFrame VideoDecoderCedarv::frame()
{
	DPTR_D(VideoDecoderCedarv);
	if (!d.cedarPicture.id) {
		return VideoFrame();
	}
	VideoFrame frame = VideoFrame(d.cedarPicture.width, d.cedarPicture.height, VideoFormat(VideoFormat::Format_YUV420P));
	if ((unsigned int)d.y.size() != d.cedarPicture.size_y) {
		d.y.resize(d.cedarPicture.size_y);
	}
	if ((unsigned int)d.u.size() != d.cedarPicture.size_u / 2) {
		d.u.resize(d.cedarPicture.size_u / 2);
	}
	if ((unsigned int)d.v.size() != d.cedarPicture.size_u / 2) {
		d.v.resize(d.cedarPicture.size_u / 2);
	}
	int bitsPerLine_Y = d.cedarPicture.size_y / d.cedarPicture.height;
	int bitsPerRow_Y = d.cedarPicture.size_y / bitsPerLine_Y;
	map32x32_to_yuv_Y(d.cedarPicture.y, (uchar *)d.y.data(), bitsPerLine_Y, bitsPerRow_Y);
	map32x32_to_yuv_C(d.cedarPicture.u, (uchar *)d.u.data(), (uchar *)d.v.data(), bitsPerLine_Y / 2, bitsPerRow_Y / 2);
	frame.setBits((uchar *)d.y.data(), 0);
	frame.setBytesPerLine(d.cedarPicture.size_y / d.cedarPicture.height, 0);
	frame.setBits((uchar *)d.u.data(), 1);
	frame.setBytesPerLine(bitsPerLine_Y / 2, 1);
	frame.setBits((uchar *)d.v.data(), 2);
	frame.setBytesPerLine(bitsPerLine_Y / 2, 2);

	d.cedarv->display_release(d.cedarv, d.cedarPicture.id);
	d.cedarPicture.id = 0;
	return frame;
}


} // namespace QtAV
