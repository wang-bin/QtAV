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

#include "QtAV/VideoDecoder.h"
#include "QtAV/private/VideoDecoder_p.h"
#include "QtAV/Packet.h"
#include "QtAV/private/AVCompat.h"
#include "QtAV/private/prepost.h"
#include <libavcodec/avcodec.h>
extern "C"
{
	#include <libcedarv/libcedarv.h>
}
#include "utils/Logger.h"

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

#ifndef NO_NEON_OPT //Don't HAVE_NEON

static void map32x32_to_yuv_Y_neon(unsigned char* srcY,unsigned char* tarY,unsigned int coded_width,unsigned int coded_height)
{
	unsigned int i,j,l,m,n;
	unsigned int mb_width,mb_height,twomb_line;
	unsigned long offset;
	unsigned char *ptr;
	unsigned char *dst_asm,*src_asm;

	ptr = srcY;
	mb_width = (coded_width+15)>>4;
	mb_height = (coded_height+15)>>4;
	twomb_line = (mb_height+1)>>1;

	for(i=0;i<twomb_line;i++)
	{
		for(j=0;j<mb_width/2;j++)
		{
			for(l=0;l<32;l++)
			{
				//first mb
				m=i*32 + l;
				n= j*32;
				offset = m*coded_width + n;
				//memcpy(tarY+offset,ptr,32);
				dst_asm = tarY+offset;
				src_asm = ptr;
				asm volatile (
				        "vld1.8         {d0 - d3}, [%[src_asm]]              \n\t"
				        "vst1.8         {d0 - d3}, [%[dst_asm]]              \n\t"
				        : [dst_asm] "+r" (dst_asm), [src_asm] "+r" (src_asm)
				        :  //[srcY] "r" (srcY)
				        : "cc", "memory", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23", "d24", "d28", "d29", "d30", "d31"
				        );

				ptr += 32;
			}
		}

		//LOGV("mb_width:%d",mb_width);
		if(mb_width & 1)
		{
			j = mb_width-1;
			for(l=0;l<32;l++)
			{
				//first mb
				m=i*32 + l;
				n= j*16;
				if(m<coded_height && n<coded_width)
				{
					offset = m*coded_width + n;
					//memcpy(tarY+offset,ptr,16);
					dst_asm = tarY+offset;
					src_asm = ptr;
					asm volatile (
					        "vld1.8         {d0 - d1}, [%[src_asm]]              \n\t"
					        "vst1.8         {d0 - d1}, [%[dst_asm]]              \n\t"
					        : [dst_asm] "+r" (dst_asm), [src_asm] "+r" (src_asm)
					        :  //[srcY] "r" (srcY)
					        : "cc", "memory", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23", "d24", "d28", "d29", "d30", "d31"
					        );
				}

				ptr += 16;
				ptr += 16;
			}
		}
	}
}

static void map32x32_to_yuv_C_neon(unsigned char* srcC,unsigned char* tarCb,unsigned char* tarCr,unsigned int coded_width,unsigned int coded_height)
{
	unsigned int i,j,l,m,n,k;
	unsigned int mb_width,mb_height,fourmb_line;
	unsigned long offset;
	unsigned char *ptr;
	unsigned char *dst0_asm,*dst1_asm,*src_asm;
	unsigned char line[16];
	int dst_stride = (coded_width + 15) & (~15);

	ptr = srcC;
	mb_width = (coded_width+7)>>3;
	mb_height = (coded_height+7)>>3;
	fourmb_line = (mb_height+3)>>2;

	for(i=0;i<fourmb_line;i++)
	{
		for(j=0;j<mb_width/2;j++)
		{
			for(l=0;l<32;l++)
			{
				//first mb
				m=i*32 + l;
				n= j*16;
				if(m<coded_height && n<coded_width)
				{
					offset = m*dst_stride + n;

					dst0_asm = tarCb + offset;
					dst1_asm = tarCr+offset;
					src_asm = ptr;
//					for(k=0;k<16;k++)
//					{
//						dst0_asm[k] = src_asm[2*k];
//						dst1_asm[k] = src_asm[2*k+1];
//					}
					asm volatile (
					        "vld1.8         {d0 - d3}, [%[src_asm]]              \n\t"
							"vuzp.8         d0, d1              \n\t"
							"vuzp.8         d2, d3              \n\t"
							"vst1.8         {d0}, [%[dst0_asm]]!              \n\t"
							"vst1.8         {d2}, [%[dst0_asm]]!              \n\t"
							"vst1.8         {d1}, [%[dst1_asm]]!              \n\t"
							"vst1.8         {d3}, [%[dst1_asm]]!              \n\t"
					         : [dst0_asm] "+r" (dst0_asm), [dst1_asm] "+r" (dst1_asm), [src_asm] "+r" (src_asm)
					         :  //[srcY] "r" (srcY)
					         : "cc", "memory", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23", "d24", "d28", "d29", "d30", "d31"
					         );
				}

				ptr += 32;
			}
		}

		if(mb_width & 1)
		{
			j= mb_width-1;
			for(l=0;l<32;l++)
			{
				m=i*32 + l;
				n= j*8;

				if(m<coded_height && n<coded_width)
				{
					offset = m*dst_stride + n;
					memcpy(line,ptr,16);
					for(k=0;k<8;k++)
					{
						*(tarCb + offset + k) = line[2*k];
						*(tarCr + offset + k) = line[2*k+1];
					}
				}

				ptr += 16;
				ptr += 16;
			}
		}
	}
}

#endif

namespace QtAV {

class VideoDecoderCedarvPrivate;
class VideoDecoderCedarv : public VideoDecoder
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(VideoDecoderCedarv)
#ifndef NO_NEON_OPT //Don't HAVE_NEON
    Q_PROPERTY(bool neon READ neon WRITE setNeon NOTIFY neonChanged)
#endif
public:
	VideoDecoderCedarv();
    virtual VideoDecoderId id() const;
    virtual QString description() const{
        return "Allwinner A10 CedarX video hardware acceleration";
    }
    bool prepare();
    bool decode(const QByteArray &encoded) Q_DECL_FINAL;
    bool decode(const Packet& packet) Q_DECL_FINAL;
	VideoFrame frame();

    //properties
    void setNeon(bool value);
    bool neon() const;
Q_SIGNALS:
    void neonChanged();
};

extern VideoDecoderId VideoDecoderId_Cedarv;
FACTORY_REGISTER_ID_AUTO(VideoDecoder, Cedarv, "Cedarv")

void RegisterVideoDecoderCedarv_Man()
{
	FACTORY_REGISTER_ID_MAN(VideoDecoder, Cedarv, "Cedarv")
}

class VideoDecoderCedarvPrivate : public VideoDecoderPrivate
{
public:
    VideoDecoderCedarvPrivate()
        : VideoDecoderPrivate()
        , map_y(map32x32_to_yuv_Y)
        , map_c(map32x32_to_yuv_C)
    {
	   cedarv = 0;
	}

	~VideoDecoderCedarvPrivate() {
		//TODO:
	}

	CEDARV_DECODER *cedarv;
	cedarv_picture_t cedarPicture;
    typedef void (*map_y_t)(unsigned char*, unsigned char*, unsigned int, unsigned int);
    map_y_t map_y;
    typedef void (*map_c_t)(unsigned char*,unsigned char*, unsigned char*, unsigned int,unsigned int);
    map_c_t map_c;
};

VideoDecoderCedarv::VideoDecoderCedarv()
    : VideoDecoder(*new VideoDecoderCedarvPrivate())
{
}

VideoDecoderId VideoDecoderCedarv::id() const
{
    return VideoDecoderId_Cedarv;
}

void VideoDecoderCedarv::setNeon(bool value)
{
    if (value == neon())
        return;
    DPTR_D(VideoDecoderCedarv);
    if (value) {
#ifndef NO_NEON_OPT //Don't HAVE_NEON
        d.map_y = map32x32_to_yuv_Y_neon;
        d.map_c = map32x32_to_yuv_C_neon;
#endif
    } else {
        d.map_y = map32x32_to_yuv_Y;
        d.map_c = map32x32_to_yuv_C;
    }
    emit neonChanged();
}

bool VideoDecoderCedarv::neon() const
{
    return d_func().map_y != map32x32_to_yuv_Y;
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
        case QTAV_CODEC_ID(H264):
			cedarStreamInfo.format = CEDARV_STREAM_FORMAT_H264;
			break;
        case QTAV_CODEC_ID(VP8):
			cedarStreamInfo.format = CEDARV_STREAM_FORMAT_VP8;
			break;
        case QTAV_CODEC_ID(VC1):
			cedarStreamInfo.format = CEDARV_STREAM_FORMAT_VC1;
			break;
        case QTAV_CODEC_ID(MPEG4):
			cedarStreamInfo.format = CEDARV_STREAM_FORMAT_MPEG4;
			cedarStreamInfo.sub_format = CEDARV_MPEG4_SUB_FORMAT_XVID;
			break;
        case QTAV_CODEC_ID(MPEG2VIDEO):
			cedarStreamInfo.format = CEDARV_STREAM_FORMAT_MPEG2;
			break;
        case QTAV_CODEC_ID(RV40):
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

    if (encoded.isEmpty())
        return true;
	//d.cedarv->ioctrl(d.cedarv, CEDARV_COMMAND_JUMP, 0);
	u32 bufsize0, bufsize1;
	u8 *buf0, *buf1;

    if (d.cedarv->request_write(d.cedarv, encoded.size(), &buf0, &bufsize0, &buf1, &bufsize1) >= 0) {
        memcpy(buf0, encoded.constData(), bufsize0);
        if ((u32)encoded.size() > bufsize0) {
            memcpy(buf1, encoded.constData() + bufsize0, bufsize1);
		}
		cedarv_stream_data_info_t stream_data_info;
		stream_data_info.type = 0;
        stream_data_info.lengh = encoded.size();
        stream_data_info.pts = 0; //packet.pts;
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

bool VideoDecoderCedarv::decode(const Packet &packet)
{
    DPTR_D(VideoDecoderCedarv);

    if (packet.data.isEmpty())
        return true;
    //d.cedarv->ioctrl(d.cedarv, CEDARV_COMMAND_JUMP, 0);
    u32 bufsize0, bufsize1;
    u8 *buf0, *buf1;

    if (d.cedarv->request_write(d.cedarv, packet.data.size(), &buf0, &bufsize0, &buf1, &bufsize1) >= 0) {
        memcpy(buf0, packet.data.constData(), bufsize0);
        if ((u32)packet.data.size() > bufsize0) {
            memcpy(buf1, packet.data.constData() + bufsize0, bufsize1);
        }
        cedarv_stream_data_info_t stream_data_info;
        stream_data_info.type = 0; // TODO
        stream_data_info.lengh = packet.data.size();
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
	unsigned int size_y = d.cedarPicture.size_y;
	unsigned int size_u = d.cedarPicture.size_u / 2;
	unsigned int size_v = d.cedarPicture.size_u / 2;
	unsigned int offset_y = 0;
	unsigned int offset_u = offset_y + size_y;
	unsigned int offset_v = offset_u + size_u;
	QByteArray buf(size_y + size_u + size_v, '\0');
	buf.resize(size_y + size_u + size_v);
	unsigned char *dst = reinterpret_cast<unsigned char *>(buf.data());

	int bitsPerLine_Y = d.cedarPicture.size_y / d.cedarPicture.height;
	int bitsPerRow_Y = d.cedarPicture.size_y / bitsPerLine_Y;

    d.map_y(d.cedarPicture.y, dst + offset_y, bitsPerLine_Y, bitsPerRow_Y);
    d.map_c(d.cedarPicture.u, dst + offset_u, dst + offset_v, bitsPerLine_Y / 2, bitsPerRow_Y / 2);

	uint8_t *pp_plane[3];
	pp_plane[0] = dst + offset_y;
	pp_plane[1] = dst + offset_u;
	pp_plane[2] = dst + offset_v;

	int pi_pitch[3];
	pi_pitch[0] = d.cedarPicture.size_y / d.cedarPicture.height;
	pi_pitch[1] = bitsPerLine_Y / 2;
	pi_pitch[2] = bitsPerLine_Y / 2;
	VideoFrame frame = VideoFrame(buf, d.cedarPicture.width, d.cedarPicture.height, VideoFormat(VideoFormat::Format_YUV420P));
	frame.setBits(pp_plane);
	frame.setBytesPerLine(pi_pitch);
    // TODO: timestamp
	d.cedarv->display_release(d.cedarv, d.cedarPicture.id);
	d.cedarPicture.id = 0;
	return frame;
}


} // namespace QtAV

#include "VideoDecoderCedarv.moc"
