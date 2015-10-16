/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013-2015 Wang Bin <wbsecg1@gmail.com>
    Miroslav Bendik <miroslav.bendik@gmail.com>

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
#include "QtAV/private/AVDecoder_p.h"
#include "QtAV/Packet.h"
#include "QtAV/private/AVCompat.h"
#include "QtAV/private/factory.h"
#include <libavcodec/avcodec.h>
#define SUPPORT_FFMPEG_DEMUX 1
extern "C" {
#include "coda/vpuapi/vpuapi.h"
#include "coda/vpuapi/regdefine.h"
#include "coda/include/mixer.h"
#include "coda/include/vpuio.h"
#include "coda/include/vpuhelper.h"
#include "coda/vpuapi/vpuapifunc.h"
}
#include "utils/Logger.h"

//avformat ctx: flag CODEC_FLAG_TRUNCATED
// PPU disabled

//#define ENC_SOURCE_FRAME_DISPLAY
#define ENC_RECON_FRAME_DISPLAY
#define VPU_ENC_TIMEOUT       5000
#define VPU_DEC_TIMEOUT       5000
#define VPU_WAIT_TIME_OUT	10		//should be less than normal decoding time to give a chance to fill stream. if this value happens some problem. we should fix VPU_WaitInterrupt function
#define PARALLEL_VPU_WAIT_TIME_OUT 1 	//the value of timeout is 1 means we want to keep a waiting time to give a chance of an interrupt of the next core.
//#define PARALLEL_VPU_WAIT_TIME_OUT 0 	//the value of timeout is 0 means we just check interrupt flag. do not wait any time to give a chance of an interrupt of the next core.
#if PARALLEL_VPU_WAIT_TIME_OUT > 0
#undef VPU_DEC_TIMEOUT
#define VPU_DEC_TIMEOUT       1000
#endif
#define MAX_CHUNK_HEADER_SIZE 1024
#define MAX_DYNAMIC_BUFCOUNT	3
#define NUM_FRAME_BUF			19
#define MAX_ROT_BUF_NUM			2
#define EXTRA_FRAME_BUFFER_NUM	1
#define ENC_SRC_BUF_NUM			2
#define STREAM_BUF_SIZE		 0x300000  // max bitstream size
//#define STREAM_FILL_SIZE    (512 * 16)  //  4 * 1024 | 512 | 512+256( wrap around test )
#define STREAM_FILL_SIZE    0x2000  //  4 * 1024 | 512 | 512+256( wrap around test )
#define STREAM_END_SIZE			0
#define STREAM_END_SET_FLAG		0
#define STREAM_END_CLEAR_FLAG	-1
#define STREAM_READ_SIZE    (512 * 16)

#define FORCE_SET_VSYNC_FLAG

#define VPU_ENSURE(x, ...) \
    do { \
        RetCode ret = x; \
        if (ret != RETCODE_SUCCESS) { \
            qWarning("Coda VPU error@%d. " #x ": %#x", __LINE__, ret); \
            return __VA_ARGS__; \
        } \
    } while(0)
#define VPU_WARN(a) \
do { \
  RetCode res = a; \
  if(res != RETCODE_SUCCESS) \
    qWarning("Coda VPU error@%d. " #x ": %#x", __LINE__, ret); \
} while(0);

namespace QtAV {

class VideoDecoderVPUPrivate;
class VideoDecoderVPU : public VideoDecoder
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(VideoDecoderVPU)
public:
    VideoDecoderVPU();
    VideoDecoderId id() const Q_DECL_OVERRIDE;
    QString description() const Q_DECL_OVERRIDE;
    bool decode(const Packet& packet) Q_DECL_OVERRIDE;
    void flush() Q_DECL_OVERRIDE;
    VideoFrame frame() Q_DECL_OVERRIDE;
};
extern VideoDecoderId VideoDecoderId_VPU;
FACTORY_REGISTER(VideoDecoder, VPU, "VPU")

int x;
class VideoDecoderVPUPrivate Q_DECL_FINAL: public VideoDecoderPrivate
{
public:
    VideoDecoderVPUPrivate()
        : VideoDecoderPrivate()
    {}
    ~VideoDecoderVPUPrivate() {
    }
    QString getVPUInfo(quint32 core_idx) {
        Uint32 version;
        Uint32 revision;
        Uint32 productId;
        VPU_GetVersionInfo(core_idx, &version, &revision, &productId);
        return QString("VPU core@%1. projectId:%2, version:%3.%4.%5 r%6, hw version:%7, API:%8")
                .arg(core_idx)
                .arg(version>>16, 0, 16).arg((version>>12)&0x0f).arg((version>>8)&0x0f).arg(version&0xff).arg(revision)
                .arg(productId).arg(API_VERSION);
    }

    bool open() Q_DECL_OVERRIDE;
    void close() Q_DECL_OVERRIDE;
    bool flush();
    bool flushBuffer(); // do decode
    bool initSeq();
    bool processHeaders(QByteArray* chunkData, int* seqHeaderSize, int* picHeaderSize);
    // user config
    int instIdx, coreIdx; //0
    bool useRot; //false
    bool useDering; //false
    bool tiled2LinearEnable;
    TiledMapConfig mapType;

    // vpu status
    bool chunkReuseRequired; //false
    bool seqInited; //false
    bool seqFilled; //false
    int bsfillSize; //0
    bool decodefinish; //false
    int int_reason; //0
    int totalNumofErrMbs; //0
    bool ppuEnable; // depends on useRot, useDering
    vpu_buffer_t vbStream;
    DecHandle handle; // 0
    DecOpenParam decOP;
    DecOutputInfo outputInfo;
    int framebufWidth, framebufHeight;
    int framebufStride; // align 16
    FrameBufferFormat framebufFormat;
    SecAxiUse secAxiUse;

 #define MAX_PPU_SRC_NUM 2
    FrameBuffer fbPPU[MAX_PPU_SRC_NUM];

    frame_queue_item_t *display_queue; //TODO: FrameBuffer queue

    QByteArray seqHeader;
    QByteArray picHeader;
    QString description;
};

// codec_id in vpuhelper.c is wrong, so copy it here
typedef struct {
    CodStd codStd;
    int mp4Class;
    int codec_id;
    unsigned int fourcc;
} CodStdTab;
#ifndef MKTAG
#define MKTAG(a,b,c,d) (a | (b << 8) | (c << 16) | (d << 24))
#endif
const CodStdTab codstd_tab[] = {
    { STD_AVC,		0, AV_CODEC_ID_H264,		MKTAG('H', '2', '6', '4') },
    { STD_AVC,		0, AV_CODEC_ID_H264,		MKTAG('X', '2', '6', '4') },
    { STD_AVC,		0, AV_CODEC_ID_H264,		MKTAG('A', 'V', 'C', '1') },
    { STD_AVC,		0, AV_CODEC_ID_H264,		MKTAG('V', 'S', 'S', 'H') },
    { STD_H263,		0, AV_CODEC_ID_H263,		MKTAG('H', '2', '6', '3') },
    { STD_H263,		0, AV_CODEC_ID_H263,		MKTAG('X', '2', '6', '3') },
    { STD_H263,		0, AV_CODEC_ID_H263,		MKTAG('T', '2', '6', '3') },
    { STD_H263,		0, AV_CODEC_ID_H263,		MKTAG('L', '2', '6', '3') },
    { STD_H263,		0, AV_CODEC_ID_H263,		MKTAG('V', 'X', '1', 'K') },
    { STD_H263,		0, AV_CODEC_ID_H263,		MKTAG('Z', 'y', 'G', 'o') },
    { STD_H263,		0, AV_CODEC_ID_H263,		MKTAG('H', '2', '6', '3') },
    { STD_H263,		0, AV_CODEC_ID_H263,		MKTAG('I', '2', '6', '3') },	/* intel h263 */
    { STD_H263,		0, AV_CODEC_ID_H263,		MKTAG('H', '2', '6', '1') },
    { STD_H263,		0, AV_CODEC_ID_H263,		MKTAG('U', '2', '6', '3') },
    { STD_H263,		0, AV_CODEC_ID_H263,		MKTAG('V', 'I', 'V', '1') },
    { STD_MPEG4,	0, AV_CODEC_ID_MPEG4,		MKTAG('F', 'M', 'P', '4') },
    { STD_MPEG4,	5, AV_CODEC_ID_MPEG4,		MKTAG('D', 'I', 'V', 'X') },	// DivX 4
    { STD_MPEG4,	1, AV_CODEC_ID_MPEG4,		MKTAG('D', 'X', '5', '0') },
    { STD_MPEG4,	2, AV_CODEC_ID_MPEG4,		MKTAG('X', 'V', 'I', 'D') },
    { STD_MPEG4,	0, AV_CODEC_ID_MPEG4,		MKTAG('M', 'P', '4', 'S') },
    { STD_MPEG4,	0, AV_CODEC_ID_MPEG4,		MKTAG('M', '4', 'S', '2') },	//MPEG-4 version 2 simple profile
    { STD_MPEG4,	0, AV_CODEC_ID_MPEG4,		MKTAG( 4 ,  0 ,  0 ,  0 ) },	/* some broken avi use this */
    { STD_MPEG4,	0, AV_CODEC_ID_MPEG4,		MKTAG('D', 'I', 'V', '1') },
    { STD_MPEG4,	0, AV_CODEC_ID_MPEG4,		MKTAG('B', 'L', 'Z', '0') },
    { STD_MPEG4,	0, AV_CODEC_ID_MPEG4,		MKTAG('M', 'P', '4', 'V') },
    { STD_MPEG4,	0, AV_CODEC_ID_MPEG4,		MKTAG('U', 'M', 'P', '4') },
    { STD_MPEG4,	0, AV_CODEC_ID_MPEG4,		MKTAG('W', 'V', '1', 'F') },
    { STD_MPEG4,	0, AV_CODEC_ID_MPEG4,		MKTAG('S', 'E', 'D', 'G') },
    { STD_MPEG4,	0, AV_CODEC_ID_MPEG4,		MKTAG('R', 'M', 'P', '4') },
    { STD_MPEG4,	0, AV_CODEC_ID_MPEG4,		MKTAG('3', 'I', 'V', '2') },
    { STD_MPEG4,	0, AV_CODEC_ID_MPEG4,		MKTAG('F', 'F', 'D', 'S') },
    { STD_MPEG4,	0, AV_CODEC_ID_MPEG4,		MKTAG('F', 'V', 'F', 'W') },
    { STD_MPEG4,	0, AV_CODEC_ID_MPEG4,		MKTAG('D', 'C', 'O', 'D') },
    { STD_MPEG4,	0, AV_CODEC_ID_MPEG4,		MKTAG('M', 'V', 'X', 'M') },
    { STD_MPEG4,	0, AV_CODEC_ID_MPEG4,		MKTAG('P', 'M', '4', 'V') },
    { STD_MPEG4,	0, AV_CODEC_ID_MPEG4,		MKTAG('S', 'M', 'P', '4') },
    { STD_MPEG4,	0, AV_CODEC_ID_MPEG4,		MKTAG('D', 'X', 'G', 'M') },
    { STD_MPEG4,	0, AV_CODEC_ID_MPEG4,		MKTAG('V', 'I', 'D', 'M') },
    { STD_MPEG4,	0, AV_CODEC_ID_MPEG4,		MKTAG('M', '4', 'T', '3') },
    { STD_MPEG4,	0, AV_CODEC_ID_MPEG4,		MKTAG('G', 'E', 'O', 'X') },
    { STD_MPEG4,	0, AV_CODEC_ID_MPEG4,		MKTAG('H', 'D', 'X', '4') }, /* flipped video */
    { STD_MPEG4,	0, AV_CODEC_ID_MPEG4,		MKTAG('D', 'M', 'K', '2') },
    { STD_MPEG4,	0, AV_CODEC_ID_MPEG4,		MKTAG('D', 'I', 'G', 'I') },
    { STD_MPEG4,	0, AV_CODEC_ID_MPEG4,		MKTAG('I', 'N', 'M', 'C') },
    { STD_MPEG4,	0, AV_CODEC_ID_MPEG4,		MKTAG('E', 'P', 'H', 'V') }, /* Ephv MPEG-4 */
    { STD_MPEG4,	0, AV_CODEC_ID_MPEG4,		MKTAG('E', 'M', '4', 'A') },
    { STD_MPEG4,	0, AV_CODEC_ID_MPEG4,		MKTAG('M', '4', 'C', 'C') }, /* Divio MPEG-4 */
    { STD_MPEG4,	0, AV_CODEC_ID_MPEG4,		MKTAG('S', 'N', '4', '0') },
    { STD_MPEG4,	0, AV_CODEC_ID_MPEG4,		MKTAG('V', 'S', 'P', 'X') },
    { STD_MPEG4,	0, AV_CODEC_ID_MPEG4,		MKTAG('U', 'L', 'D', 'X') },
    { STD_MPEG4,	0, AV_CODEC_ID_MPEG4,		MKTAG('G', 'E', 'O', 'V') },
    { STD_MPEG4,	0, AV_CODEC_ID_MPEG4,		MKTAG('S', 'I', 'P', 'P') }, /* Samsung SHR-6040 */
    { STD_DIV3,		0, AV_CODEC_ID_MSMPEG4V3,	MKTAG('D', 'I', 'V', '3') }, /* default signature when using MSMPEG4 */
    { STD_DIV3,		0, AV_CODEC_ID_MSMPEG4V3,	MKTAG('M', 'P', '4', '3') },
    { STD_DIV3,		0, AV_CODEC_ID_MSMPEG4V3,	MKTAG('M', 'P', 'G', '3') },
    { STD_MPEG4,	1, AV_CODEC_ID_MSMPEG4V3,	MKTAG('D', 'I', 'V', '5') },
    { STD_MPEG4,	1, AV_CODEC_ID_MSMPEG4V3,	MKTAG('D', 'I', 'V', '6') },
    { STD_MPEG4,	5, AV_CODEC_ID_MSMPEG4V3,	MKTAG('D', 'I', 'V', '4') },
    { STD_DIV3,		0, AV_CODEC_ID_MSMPEG4V3,	MKTAG('D', 'V', 'X', '3') },
    { STD_DIV3,		0, AV_CODEC_ID_MSMPEG4V3,	MKTAG('A', 'P', '4', '1') },	//Another hacked version of Microsoft's MP43 codec.
    { STD_MPEG4,	0, AV_CODEC_ID_MSMPEG4V3,	MKTAG('C', 'O', 'L', '1') },
    { STD_MPEG4,	0, AV_CODEC_ID_MSMPEG4V3,	MKTAG('C', 'O', 'L', '0') },	// not support ms mpeg4 v1, 2
    { STD_MPEG4,  256, AV_CODEC_ID_FLV1,		MKTAG('F', 'L', 'V', '1') }, /* Sorenson spark */
    { STD_VC1,		0, AV_CODEC_ID_WMV1,		MKTAG('W', 'M', 'V', '1') },
    { STD_VC1,		0, AV_CODEC_ID_WMV2,		MKTAG('W', 'M', 'V', '2') },
    { STD_MPEG2,	0, AV_CODEC_ID_MPEG1VIDEO,	MKTAG('M', 'P', 'G', '1') },
    { STD_MPEG2,	0, AV_CODEC_ID_MPEG1VIDEO,	MKTAG('M', 'P', 'G', '2') },
    { STD_MPEG2,	0, AV_CODEC_ID_MPEG2VIDEO,	MKTAG('M', 'P', 'G', '2') },
    { STD_MPEG2,	0, AV_CODEC_ID_MPEG2VIDEO,	MKTAG('M', 'P', 'E', 'G') },
    { STD_MPEG2,	0, AV_CODEC_ID_MPEG1VIDEO,	MKTAG('M', 'P', '2', 'V') },
    { STD_MPEG2,	0, AV_CODEC_ID_MPEG1VIDEO,	MKTAG('P', 'I', 'M', '1') },
    { STD_MPEG2,	0, AV_CODEC_ID_MPEG2VIDEO,	MKTAG('P', 'I', 'M', '2') },
    { STD_MPEG2,	0, AV_CODEC_ID_MPEG1VIDEO,	MKTAG('V', 'C', 'R', '2') },
    { STD_MPEG2,	0, AV_CODEC_ID_MPEG1VIDEO,	MKTAG( 1 ,  0 ,  0 ,  16) },
    { STD_MPEG2,	0, AV_CODEC_ID_MPEG2VIDEO,	MKTAG( 2 ,  0 ,  0 ,  16) },
    { STD_MPEG4,	0, AV_CODEC_ID_MPEG4,		MKTAG( 4 ,  0 ,  0 ,  16) },
    { STD_MPEG2,	0, AV_CODEC_ID_MPEG2VIDEO,	MKTAG('D', 'V', 'R', ' ') },
    { STD_MPEG2,	0, AV_CODEC_ID_MPEG2VIDEO,	MKTAG('M', 'M', 'E', 'S') },
    { STD_MPEG2,	0, AV_CODEC_ID_MPEG2VIDEO,	MKTAG('L', 'M', 'P', '2') }, /* Lead MPEG2 in avi */
    { STD_MPEG2,	0, AV_CODEC_ID_MPEG2VIDEO,	MKTAG('S', 'L', 'I', 'F') },
    { STD_MPEG2,	0, AV_CODEC_ID_MPEG2VIDEO,	MKTAG('E', 'M', '2', 'V') },
    { STD_VC1,		0, AV_CODEC_ID_WMV3,		MKTAG('W', 'M', 'V', '3') },
    { STD_VC1,		0, AV_CODEC_ID_VC1,		MKTAG('W', 'V', 'C', '1') },
    { STD_VC1,		0, AV_CODEC_ID_VC1,		MKTAG('W', 'M', 'V', 'A') },


    { STD_RV,		0, AV_CODEC_ID_RV30,		MKTAG('R','V','3','0') },
    { STD_RV,		0, AV_CODEC_ID_RV40,		MKTAG('R','V','4','0') },

    { STD_AVS,		0, AV_CODEC_ID_CAVS,		MKTAG('C','A','V','S') },
    { STD_AVS,		0, AV_CODEC_ID_AVS,		MKTAG('A','V','S','2') },
    { STD_VP3,		0, AV_CODEC_ID_VP3,		MKTAG('V', 'P', '3', '0') },
    { STD_VP3,		0, AV_CODEC_ID_VP3,		MKTAG('V', 'P', '3', '1') },
    { STD_THO,		0, AV_CODEC_ID_THEORA,		MKTAG('T', 'H', 'E', 'O') },
    { STD_VP8,		0, AV_CODEC_ID_VP8,		MKTAG('V', 'P', '8', '0') }
    // 	{ STD_VP6,		0, AV_CODEC_ID_VP6,		MKTAG('V', 'P', '6', '0') },
    // 	{ STD_VP6,		0, AV_CODEC_ID_VP6,		MKTAG('V', 'P', '6', '1') },
    // 	{ STD_VP6,		0, AV_CODEC_ID_VP6,		MKTAG('V', 'P', '6', '2') },
    // 	{ STD_VP6,		0, AV_CODEC_ID_VP6F,		MKTAG('V', 'P', '6', 'F') },
    // 	{ STD_VP6,		0, AV_CODEC_ID_VP6F,		MKTAG('F', 'L', 'V', '4') },

};

int fourCCToMp4Class(unsigned int fourcc)
{
    char str[5];
    str[0] = toupper((char)fourcc);
    str[1] = toupper((char)(fourcc>>8));
    str[2] = toupper((char)(fourcc>>16));
    str[3] = toupper((char)(fourcc>>24));
    str[4] = '\0';
    for(int i=0; i<sizeof(codstd_tab)/sizeof(codstd_tab[0]); i++)
        if (codstd_tab[i].fourcc == (unsigned int)MKTAG(str[0], str[1], str[2], str[3]))
            return codstd_tab[i].mp4Class;
    return -1;
}

int fourCCToCodStd(unsigned int fourcc)
{
    char str[5];
    str[0] = toupper((char)fourcc);
    str[1] = toupper((char)(fourcc>>8));
    str[2] = toupper((char)(fourcc>>16));
    str[3] = toupper((char)(fourcc>>24));
    str[4] = '\0';
    for(int i=0; i<sizeof(codstd_tab)/sizeof(codstd_tab[0]); i++)
        if (codstd_tab[i].fourcc == (unsigned int)MKTAG(str[0], str[1], str[2], str[3]))
            return codstd_tab[i].codStd;
    return -1;
}

int codecIdToMp4Class(int codec_id)
{
    for(int i=0; i<sizeof(codstd_tab)/sizeof(codstd_tab[0]); i++)
        if (codstd_tab[i].codec_id == codec_id)
            return codstd_tab[i].mp4Class;
    return -1;

}
int codecIdToCodStd(int codec_id)
{
    for(int i=0; i<sizeof(codstd_tab)/sizeof(codstd_tab[0]); i++)
        if (codstd_tab[i].codec_id == codec_id)
            return codstd_tab[i].codStd;
    return -1;
}

int codecIdToFourcc(int codec_id)
{
    for(int i=0; i<sizeof(codstd_tab)/sizeof(codstd_tab[0]); i++)
        if (codstd_tab[i].codec_id == codec_id)
            return codstd_tab[i].fourcc;
    return 0;
}

VideoDecoderVPU::VideoDecoderVPU()
    : VideoDecoder(*new VideoDecoderVPUPrivate())
{
}

VideoDecoderId VideoDecoderVPU::id() const
{
    return VideoDecoderId_VPU;
}

QString VideoDecoderVPU::description() const
{
    return d_func().description;
}

bool VideoDecoderVPUPrivate::open()
{
    if (codecIdToCodStd(codec_ctx->codec_id) < 0) {
        qWarning("Unsupported codec");
        return false;
    }

    RetCode ret = VPU_Init(coreIdx);
#ifndef BIT_CODE_FILE_PATH
#endif
    if (ret != RETCODE_SUCCESS &&
        ret != RETCODE_CALLED_BEFORE) {
        qWarning("VPU_Init failed Error code is 0x%x \n", ret );
        return false;
    }
    decodefinish = false;
    int_reason = 0;
    totalNumofErrMbs = 0;
    description = getVPUInfo(coreIdx);

    instIdx = VPU_GetOpenInstanceNum(pVpu->coreIdx); //omx use it. vpurun set from other
    if (instIdx > MAX_NUM_INSTANCE) {
        qDebug("exceed the opened instance num than %d num", MAX_NUM_INSTANCE);
        return false;
    }
    //omx_vpudec_component_vpuLibInit
    memset(&outputInfo, 0, sizeof(outputInfo));
    memset(&decOP, 0, sizeof(decOP));
    decOP.bitstreamFormat = fourCCToCodStd(codec_ctx->codec_tag);
    if (decOP.bitstreamFormat < 0)
        decOP.bitstreamFormat = codecIdToCodStd(codec_ctx->codec_id);
    if (decOP.bitstreamFormat < 0) {
        qWarning("can not support video format in VPU tag=%c%c%c%c, codec_id=0x%x \n", ctxVideo->codec_tag>>0, ctxVideo->codec_tag>>8, ctxVideo->codec_tag>>16, ctxVideo->codec_tag>>24, ctxVideo->codec_id );
        return false;
    }
    decOP.mp4Class = fourCCToMp4Class(codec_ctx->codec_tag);
    if (decOP.mp4Class < 0)
        decOP.mp4Class = codecIdToMp4Class(codec_ctx->codec_id);
//!USE_OMX_HEADER_BUFFER_AS_DECODER_BITSTREAM_BUFFER
    memset(&vbStream, 0, sizeof(vbStream));
    vbStream.size = STREAM_BUF_SIZE;
    vbStream.size = FFALIGN(vbStream.size, 1024);
    if (vdi_allocate_dma_memory(coreIdx, &vbStream) < 0) {
        qWarning("fail to allocate bitstream buffer\n" );
        return false;
    }
    decOP.bitstreamBuffer = vbStream.phys_addr;
    decOP.bitstreamBufferSize = vbStream.size;
    decOP.mp4DeblkEnable = 0;


    if(decOP.bitstreamFormat == STD_THO || decOP.bitstreamFormat == STD_VP3) {
    }
    decOP.tiled2LinearEnable = tiled2LinearEnable;
#if 0 //not in omx
    if (mapType) {
        //decOP.wtlEnable = decConfig.wtlEnable; //TODO:
        if (decOP.wtlEnable) {
            //decConfig.rotAngle;
            //decConfig.mirDir;
            //useRot = false;
            //useDering = false;
            decOP.mp4DeblkEnable = 0;
            decOP.tiled2LinearEnable = 0;
        }
    }
#endif
    decOP.cbcrInterleave = CBCR_INTERLEAVE;
    if (mapType == TILED_FRAME_MB_RASTER_MAP ||
        mapType == TILED_FIELD_MB_RASTER_MAP) {
            decOP.cbcrInterleave = 1;
    }
    decOP.cbcrOrder = CBCR_ORDER_NORMAL;
    if (true) { //output nv12
        decOP.cbcrInterleave = 1;
        decOP.cbcrOrder = CBCR_ORDER_NORMAL;
    } else { //output yuv420p
        decOP.cbcrInterleave = 0;
#ifdef ANDROID
        decOP.cbcrOrder = CBCR_ORDER_REVERSED;
#else
        decOP.cbcrOrder = CBCR_ORDER_NORMAL;
#endif
    }
    decOP.bwbEnable = VPU_ENABLE_BWB;
    decOP.frameEndian  = VPU_FRAME_ENDIAN;
    decOP.streamEndian = VPU_STREAM_ENDIAN;
    decOP.bitstreamMode = BS_MODE_PIC_END; //omx

    ppuEnable = (useRot || useDering || decOP.tiled2LinearEnable);
    seqHeader = QByteArray(codec_ctx->extradata_size + MAX_CHUNK_HEADER_SIZE);	// allocate more buffer to fill the vpu specific header.
    picHeader = QByteArray(MAX_CHUNK_HEADER_SIZE);

    VPU_ENSURE(VPU_DecOpen(&handle, &decOP), false);
    display_queue = frame_queue_init(MAX_REG_FRAME);
    init_VSYNC_flag();

    // TODO: omx GET_DRAM_CONFIG is not in open
    DRAMConfig dramCfg = {0};
    VPU_ENSURE(VPU_DecGiveCommand(handle, GET_DRAM_CONFIG, &dramCfg), false);
    seqInited = false;
    seqFilled = false;
    bsfillSize = 0;
    chunkReuseRequired = false;

    return true;
}

void VideoDecoderVPUPrivate::close()
{
    if (!handle)
        return;
    RetCode ret = VPU_DecClose(handle);
    if (ret == RETCODE_FRAME_NOT_COMPLETE) {
        VPU_DecGetOutputInfo(handle, &outputInfo);
        VPU_DecClose(handle);
    }
    if (vbStream.size)
        vdi_free_dma_memory(coreIdx, &vbStream);
    seqHeader.clear();
    picHeader.clear();
    if (display_queue) {
        frame_queue_dequeue_all(display_queue);
        frame_queue_deinit(display_queue);
    }
}

bool VideoDecoderVPUPrivate::flush()
{
    VPU_DecUpdateBitstreamBuffer(handle, STREAM_END_SIZE);	//tell VPU to reach the end of stream. starting flush decoded output in VPU
}

bool VideoDecoderVPUPrivate::initSeq()
{
    if (seqInited)
        return true;
    ConfigSeqReport(coreIdx, handle, decOP.bitstreamFormat); // TODO: remove
    DecInitialInfo initialInfo = {0};
    if (decOP.bitstreamMode == BS_MODE_PIC_END) { //TODO: no check in omx (always BS_MODE_PIC_END)
        ret = VPU_DecGetInitialInfo(handle, &initialInfo);
        if (ret != RETCODE_SUCCESS) {
            if (ret == RETCODE_MEMORY_ACCESS_VIOLATION)
                PrintMemoryAccessViolationReason(coreIdx, NULL);
            qWarning("VPU_DecGetInitialInfo failed Error code is %#x", ret);
            return false; //TODO: omx wait for next chunk
        }
        VPU_ClearInterrupt(coreIdx);
    } else {
        // d.int_reason
        if ((int_reason & (1<<INT_BIT_BIT_BUF_EMPTY)) != (1<<INT_BIT_BIT_BUF_EMPTY)) {
            VPU_ENSURE(VPU_DecIssueSeqInit(handle), false);
        } else {
            // After VPU generate the BIT_EMPTY interrupt. HOST should feed the bitstream up to 1024 in case of seq_init
            if (bsfillSize < VPU_GBU_SIZE*2)
                return true;
        }
        while (osal_kbhit() == 0) {
            int_reason = VPU_WaitInterrupt(d.coreIdx, VPU_DEC_TIMEOUT);
            if (int_reason)
                VPU_ClearInterrupt(coreIdx);
            if(int_reason & (1<<INT_BIT_BIT_BUF_EMPTY))
                break;
            CheckUserDataInterrupt(d.coreIdx, d.handle, 1, decOP.bitstreamFormat, int_reason);
            if (int_reason && (int_reason & (1<<INT_BIT_SEQ_INIT))) {
                seqInited = 1;
                break;
            }
        }
        if (int_reason & (1<<INT_BIT_BIT_BUF_EMPTY) || int_reason == -1) {
            bsfillSize = 0;
            return true; // go to take next chunk.
        }
        if (seqInited) {
            ret = VPU_DecCompleteSeqInit(handle, &initialInfo);
            if (ret != RETCODE_SUCCESS) {
                if (ret == RETCODE_MEMORY_ACCESS_VIOLATION)
                    PrintMemoryAccessViolationReason(coreIdx, NULL);
                if (initialInfo.seqInitErrReason & (1<<31)) // this case happened only ROLLBACK mode
                    VLOG(ERR, "Not enough header : Parser has to feed right size of a sequence header  \n");
                VLOG(ERR, "VPU_DecCompleteSeqInit failed Error code is 0x%x \n", ret );
                goto ERR_DEC_OPEN;
            }
        } else {
            VLOG(ERR, "VPU_DecGetInitialInfo failed Error code is 0x%x \n", ret);
            goto ERR_DEC_OPEN;
        }
    }
    SaveSeqReport(coreIdx, handle, &initialInfo, decOP.bitstreamFormat);
    if (decOP.bitstreamFormat == STD_VP8) {
        // For VP8 frame upsampling infomration
        static const int scale_factor_mul[4] = {1, 5, 5, 2};
        static const int scale_factor_div[4] = {1, 4, 3, 1};
        const int hScaleFactor = initialInfo.vp8ScaleInfo.hScaleFactor;
        const int vScaleFactor = initialInfo.vp8ScaleInfo.vScaleFactor;
        const int scaledWidth = initialInfo.picWidth * scale_factor_mul[hScaleFactor] / scale_factor_div[hScaleFactor];
        const int scaledHeight = initialInfo.picHeight * scale_factor_mul[vScaleFactor] / scale_factor_div[vScaleFactor];
        framebufWidth = FFALIGN(scaledWidth, 16);
        if (IsSupportInterlaceMode(decOP.bitstreamFormat, &initialInfo))
            framebufHeight = FFALIGN(scaledHeight, 32); // framebufheight must be aligned by 31 because of the number of MB height would be odd in each filed picture.
        else
            framebufHeight = FFALIGN(scaledHeight, 16);
#ifdef SUPPORT_USE_VPU_ROTATOR
        rotbufWidth = (decConfig.rotAngle == 90 || decConfig.rotAngle == 270) ?
            FFALIGN(scaledHeight, 16) : FFALIGN(scaledWidth, 16);
        rotbufHeight = (decConfig.rotAngle == 90 || decConfig.rotAngle == 270) ?
            FFALIGN(scaledWidth, 16) : FFALIGN(scaledHeight, 16);
#endif
    } else {
        framebufWidth = FFALIGN(initialInfo.picWidth, 16);
        // TODO: vpurun decConfig.maxHeight
        // TODO: omx fbHeight align 16
        if (IsSupportInterlaceMode(decOP.bitstreamFormat, &initialInfo))
            framebufHeight = FFALIGN(initialInfo.picHeight, 32); // framebufheight must be aligned by 31 because of the number of MB height would be odd in each filed picture.
        else
            framebufHeight = FFALIGN(initialInfo.picHeight, 16);
#ifdef SUPPORT_USE_VPU_ROTATOR
        rotbufWidth = (decConfig.rotAngle == 90 || decConfig.rotAngle == 270) ?
            FFALIGN(initialInfo.picHeight, 16) : FFALIGN(initialInfo.picWidth, 16);
        rotbufHeight = (decConfig.rotAngle == 90 || decConfig.rotAngle == 270) ?
            FFALIGN(initialInfo.picWidth, 16) : FFALIGN(initialInfo.picHeight, 16);
#endif
    }
#ifdef SUPPORT_USE_VPU_ROTATOR
    rotStride = rotbufWidth;
#endif
    framebufStride = framebufWidth;
    framebufFormat = FORMAT_420;
    // vpurun: allocate out buffer of size framebufSize for rendering. we do not need it
    //framebufSize = VPU_GetFrameBufSize(framebufStride, framebufHeight, mapType, framebufFormat, &dramCfg);
/*
    sw_mixer_close((coreIdx*MAX_NUM_VPU_CORE)+instIdx);
    if (!ppuEnable)
        sw_mixer_open((coreIdx*MAX_NUM_VPU_CORE)+instIdx, framebufStride, framebufHeight);
    else
        sw_mixer_open((coreIdx*MAX_NUM_VPU_CORE)+instIdx, rotStride, rotbufHeight);
*/
    // the size of pYuv should be aligned 8 byte. because of C&M HPI bus system constraint.

    secAxiUse.useBitEnable  = USE_BIT_INTERNAL_BUF;
    secAxiUse.useIpEnable   = USE_IP_INTERNAL_BUF;
    secAxiUse.useDbkYEnable = USE_DBKY_INTERNAL_BUF;
    secAxiUse.useDbkCEnable = USE_DBKC_INTERNAL_BUF;
    secAxiUse.useBtpEnable  = USE_BTP_INTERNAL_BUF;
    secAxiUse.useOvlEnable  = USE_OVL_INTERNAL_BUF;

    VPU_DecGiveCommand(handle, SET_SEC_AXI, &secAxiUse);
#ifdef SUPPORT_DEC_RESOLUTION_CHANGE
    decBufInfo.maxDecMbX = framebufWidth/16;
    decBufInfo.maxDecMbY = FFALIGN(framebufHeight, 32)/16;
    decBufInfo.maxDecMbNum = decBufInfo.maxDecMbX*decBufInfo.maxDecMbY;
#endif
    FrameBuffer* fbUserFrame = NULL; //it means VPU_DecRegisterFrameBuffer should allocation frame buffer by using VDI and vpu device driver.
    int regFrameBufCount = initialInfo.minFrameBufferCount + EXTRA_FRAME_BUFFER_NUM;
#if defined(SUPPORT_DEC_SLICE_BUFFER) || defined(SUPPORT_DEC_RESOLUTION_CHANGE)
    // Register frame buffers requested by the decoder.
    VPU_ENSURE(VPU_DecRegisterFrameBuffer(handle, fbUserFrame, regFrameBufCount, framebufStride, framebufHeight, mapType, &decBufInfo), false); // frame map type (can be changed before register frame buffer)
#else
    // Register frame buffers requested by the decoder.
    VPU_ENSURE(VPU_DecRegisterFrameBuffer(handle, fbUserFrame, regFrameBufCount, framebufStride, framebufHeight, mapType), false); // frame map type (can be changed before register frame buffer)
#endif
    //omx check PrintMemoryAccessViolationReason(pVpu->coreIdx, NULL);
    TiledMapConfig mapCfg = {{0}};
    VPU_ENSURE(VPU_DecGiveCommand(handle, GET_TILEDMAP_CONFIG, &mapCfg), false);
    if (ppuEnable) {
#if 0
        ppIdx = 0;
        fbAllocInfo.format = framebufFormat;
        fbAllocInfo.cbcrInterleave = decOP.cbcrInterleave;
        if (decOP.tiled2LinearEnable)
            fbAllocInfo.mapType = LINEAR_FRAME_MAP;
        else
            fbAllocInfo.mapType = mapType;

        fbAllocInfo.stride  = rotStride;
        fbAllocInfo.height  = rotbufHeight;
        fbAllocInfo.num     = MAX_ROT_BUF_NUM;
        fbAllocInfo.endian  = decOP.frameEndian;
        fbAllocInfo.type    = FB_TYPE_PPU;
        VPU_ENSURE(VPU_DecAllocateFrameBuffer(handle, fbAllocInfo, fbPPU), false);

        ppIdx = 0;

        if (decConfig.useRot) {
            VPU_DecGiveCommand(handle, SET_ROTATION_ANGLE, &(decConfig.rotAngle));
            VPU_DecGiveCommand(handle, SET_MIRROR_DIRECTION, &(decConfig.mirDir));
        }
        if (decConfig.useDering)
            VPU_DecGiveCommand(handle, ENABLE_DERING, 0);
        VPU_DecGiveCommand(handle, SET_ROTATOR_STRIDE, &rotStride);
#endif
    }
    seqInited = true;
}

bool VideoDecoderVPUPrivate::processHeaders(QByteArray *chunkData, int *seqHeaderSize, int *picHeaderSize)
{
    if (!seqInited && !seqFilled) {
        // FIXME: copy from omx or vpuhelper
        *seqHeaderSize = BuildSeqHeader((BYTE*)seqHeader.constData(), decOP.bitstreamFormat, ic->streams[idxVideo]);	// make sequence data as reference file header to support VPU decoder.
        if (headerSize < 0) {// indicate the stream dose not support in VPU.
            qWarning("BuildSeqHeader the stream does not support in VPU");
            return false;
        }
        // TODO: vpurun check STD_THO, STD_VP3
        if (*seqHeaderSize > 0) { //write to vbStream
            const int size = WriteBsBufFromBufHelper(coreIdx, handle, &vbStream, seqHeader, *seqHeaderSize, decOP.streamEndian);
            if (size < 0) {
                qWarning("WriteBsBufFromBufHelper failed Error code is 0x%x", size);
                return false;
            }
            bsfillSize += size;
        }
        seqFilled = true;
    }
    // Build and Fill picture Header data which is dedicated for VPU
    //picHeaderSize is also used in FLUSH_BUFFER
    *picHeaderSize = BuildPicHeader((BYTE*)picHeader.constData(), decOP.bitstreamFormat, ic->streams[idxVideo], pkt); //FIXME
    if (*picHeaderSize > 0) { // TODO: no check in vpurun
        switch(decOP.bitstreamFormat) {
        case STD_THO:
        case STD_VP3:
            break;
        default: // write to vbStream
            const int size = WriteBsBufFromBufHelper(coreIdx, handle, &vbStream, picHeader.constData(), *picHeaderSize, decOP.streamEndian);
            if (size < 0) {
                qWarning("WriteBsBufFromBufHelper failed Error code is 0x%x", size);
                return false;
            }
            bsfillSize += size; // TODO: not in omx
            break;
        }
    }
    // Fill VCL data
    switch(decOP.bitstreamFormat) {
    case STD_VP3:
    case STD_THO:
        //const int size = WriteBsBufFromBufHelper(coreIdx, handle, &vbStream, picHeader, picHeaderSize, decOP.streamEndian);
        break;
    default: {
        if (decOP.bitstreamFormat == STD_RV) {
            chunkData->remove(0, 1+(cSlice*8));
        }
        const int size = WriteBsBufFromBufHelper(coreIdx, handle, &vbStream, chunkData->constData(), chunkData->size(), decOP.streamEndian);
        if (size <0) {
            qWarning("WriteBsBufFromBufHelper failed Error code is 0x%x", size);
            return false;
        }
        bsfillSize += size;
    }
    break;
    }

    chunkIdx++;
    if (!initSeq())
        return false;
    return true;
}

bool VideoDecoderVPU::decode(const Packet &packet)
{
    DPTR_D(VideoDecoderVPU);
    if (d.decodefinish) {
        qWarning("decode finished");
        return false;
    }
    if (packet.data.isEmpty())
        return true;
    if (packet.isEOF()) { //decode the trailing frames
        if (!d.flush()) {
            qDebug("Error decode EOS"); // when?
            return false;
        }
        goto FLUSH_BUFFER;
    }
    QByteArray chunkData(packet.data);
    DecOpenParam &decOP = d.decOP;
    if (decOP.bitstreamMode == BS_MODE_PIC_END) { //TODO: no check in omx. it's always true here
        if (!d.chunkReuseRequired) {
            VPU_DecSetRdPtr(handle, decOP.bitstreamBuffer, 1);
        }
    }
    int seqHeaderSize = 0; //also used in FLUSH_BUFFER
    int picHeaderSize = 0;
    if (!d.chunkReuseRequired) {
        if (!d.processHeaders(&chunkData, &seqHeaderSize, &picHeaderSize))
            return false;
    }
    d.chunkReuseRequired = false;

//FLUSH_BUFFER:
    if(!(int_reason & (1<<INT_BIT_BIT_BUF_EMPTY)) && !(int_reason & (1<<INT_BIT_DEC_FIELD))) {
        if (ppuEnable) {
#if 0
            VPU_DecGiveCommand(handle, SET_ROTATOR_OUTPUT, &fbPPU[ppIdx]);
            if (decConfig.useRot) {
                VPU_DecGiveCommand(handle, ENABLE_ROTATION, 0);
                VPU_DecGiveCommand(handle, ENABLE_MIRRORING, 0);
            }
            if (decConfig.useDering)
                VPU_DecGiveCommand(handle, ENABLE_DERING, 0);
#endif
        }

        ConfigDecReport(d.coreIdx, d.handle, decOP.bitstreamFormat);
        // Start decoding a frame.
        DecParam decParam = {0};
        VPU_ENSURE(VPU_DecStartOneFrame(handle, &decParam), false);
    } else { // TODO: omx always run here
        if(int_reason & (1<<INT_BIT_DEC_FIELD)) {
            d.int_reason = 0;
            VPU_ClearInterrupt(d.coreIdx);
        }
        // After VPU generate the BIT_EMPTY interrupt. HOST should feed the bitstreams than 512 byte.
        if (decOP.bitstreamMode != BS_MODE_PIC_END) {
            if (d.bsfillSize < VPU_GBU_SIZE)
                return true; //continue
        }
    }
    // TODO: omx always call the following
    //VPU_DecSetRdPtr(d.handle, d.vbStream.phys_addr, 0);	// CODA851 can't support a exact rdptr. so after SEQ_INIT, RdPtr should be rewinded.
    // Start decoding a frame.
    //VPU_ENSURE(VPU_DecStartOneFrame(handle, &decParam), false);

    while (osal_kbhit() == 0) { //TODO: omx while(1)
        d.int_reason = VPU_WaitInterrupt(d.coreIdx, VPU_DEC_TIMEOUT);
        if (d.int_reason == (Uint32)-1 ) {// timeout
            VPU_SWReset(d.coreIdx, SW_RESET_SAFETY, d.handle);
            qWarning("VPU_WaitInterrupt timeout");
            break;
        }
        CheckUserDataInterrupt(d.coreIdx, d.handle, d.outputInfo.indexFrameDecoded, decOP.bitstreamFormat, d.int_reason);
        if (d.int_reason & (1<<INT_BIT_DEC_FIELD)) {
            if (decOP.bitstreamMode == BS_MODE_PIC_END) {
                PhysicalAddress rdPtr, wrPtr;
                int room;
                VPU_DecGetBitstreamBuffer(d.handle, &rdPtr, &wrPtr, &room);
                // TODO: omx
                if (rdPtr-decOP.bitstreamBuffer <
                        (PhysicalAddress)(chunkData.size()+picHeaderSize+seqHeaderSize-8)
                        ) // there is full frame data in chunk data.
                    VPU_DecSetRdPtr(d.handle, rdPtr, 0);		//set rdPtr to the position of next field data.
                else // do not clear interrupt until feeding next field picture.
                    break;
            }
        }
        if (d.int_reason)
            VPU_ClearInterrupt(d.coreIdx);
        if (d.int_reason & (1<<INT_BIT_BIT_BUF_EMPTY)) { // TODO: omx no check and below
            if (decOP.bitstreamMode == BS_MODE_PIC_END) {
                VLOG(ERR, "Invalid operation is occurred in pic_end mode \n");
                return false;
            }
            break;
        }
        if (d.int_reason & (1<<INT_BIT_PIC_RUN))
            break;
    }

    // TODO: omx no this code ---NOT IN OMX BEGIN
    if ((d.int_reason & (1<<INT_BIT_BIT_BUF_EMPTY))
            || (d.int_reason & (1<<INT_BIT_DEC_FIELD))) {
        d.bsfillSize = 0;
        return true;//continue; // go to take next chunk.
    }
    // ---NOT IN OMX END

    RetCode ret = VPU_DecGetOutputInfo(d.handle, &d.outputInfo);
    if (ret != RETCODE_SUCCESS) {
        VLOG(ERR,  "VPU_DecGetOutputInfo failed Error code is 0x%x \n", ret);
        if (ret == RETCODE_MEMORY_ACCESS_VIOLATION)
            PrintMemoryAccessViolationReason(d.coreIdx, &d.outputInfo);
        goto ERR_DEC_OPEN;
    }
    if ((d.outputInfo.decodingSuccess & 0x01) == 0) { //OMX no 0x01
        VLOG(ERR, "VPU_DecGetOutputInfo decode fail framdIdx %d \n", frameIdx);
        VLOG(TRACE, "#%d, indexFrameDisplay %d || picType %d || indexFrameDecoded %d\n",
            frameIdx, d.outputInfo.indexFrameDisplay, d.outputInfo.picType, d.outputInfo.indexFrameDecoded );
    }
    VLOG(TRACE, "#%d:%d, indexDisplay %d || picType %d || indexDecoded %d || rdPtr=0x%x || wrPtr=0x%x || chunkSize = %d, consume=%d\n",
        d.instIdx, frameIdx, d.outputInfo.indexFrameDisplay, d.outputInfo.picType, d.outputInfo.indexFrameDecoded, d.outputInfo.rdPtr, d.outputInfo.wrPtr, chunkData.size()+picHeaderSize, d.outputInfo.consumedByte);

    SaveDecReport(d.coreIdx, d.handle, &d.outputInfo, decOP.bitstreamFormat, ((initialInfo.picWidth+15)&~15)/16); ///TODO:
    if (d.outputInfo.chunkReuseRequired) {// reuse previous chunk. that would be true once framebuffer is full.
        d.chunkReuseRequired = true;
        d.undecoded_size = chunkData.size(); // decode previous chunk again!!
        qDebug("chunkReuseRequired");
    } else {
        // must always set undecoded_size if it was set!!!
        d.undecoded_size = 0;
    }

    if (d.outputInfo.indexFrameDisplay == -1)
        d.decodefinish = true;
    if (d.decodefinish) {
        if (!ppuEnable || decodeIdx == 0)
            return !packet.isEOF(); // EOS
    }
    if (d.outputInfo.indexFrameDisplay == -3 ||
        d.outputInfo.indexFrameDisplay == -2 ) // BIT doesn't have picture to be displayed
    {
        if (check_VSYNC_flag()) {
            clear_VSYNC_flag();
            if (frame_queue_dequeue(display_queue, &dispDoneIdx) == 0)
                VPU_DecClrDispFlag(handle, dispDoneIdx);
        }
#if defined(CNM_FPGA_PLATFORM) && defined(FPGA_LX_330)
#else
        if (d.outputInfo.indexFrameDecoded == -1)	// VPU did not decode a picture because there is not enough frame buffer to continue decoding
        {
            // if you can't get VSYN interrupt on your sw layer. this point is reasonable line to set VSYN flag.
            // but you need fine tune EXTRA_FRAME_BUFFER_NUM value not decoder to write being display buffer.
            if (frame_queue_count(display_queue) > 0)
                set_VSYNC_flag();
        }
#endif
        return !packet.isEOF();// more packet required
    }
#if 0
    if (ppuEnable) {
        if (decodeIdx == 0) // if PP has been enabled, the first picture is saved at next time.
        {
            // save rotated dec width, height to display next decoding time.
            if (d.outputInfo.indexFrameDisplay >= 0)
                frame_queue_enqueue(display_queue, outputInfo.indexFrameDisplay);
            rcPrevDisp = outputInfo.rcDisplay;
            decodeIdx++;
            continue;

        }
    }
    decodeIdx++;
#endif
    if (d.outputInfo.indexFrameDisplay >= 0)
        frame_queue_enqueue(d.display_queue, d.outputInfo.indexFrameDisplay);

// TODO: to be continue here
    if (!saveImage) {
        if (!ppuEnable) { // TODO: fill out buffer. used by VideoFrame and XImage (GPU scale)
            vdi_read_memory(coreIdx, d.outputInfo.dispFrame.bufY, (BYTE *)dst,  (stride*picHeight*3/2), endian);
            //OmxSaveYuvImageBurstFormat(d.coreIdx, &d.outputInfo.dispFrame, framebufStride, framebufHeight);
        } else {
            ppIdx = (ppIdx+1)%MAX_ROT_BUF_NUM;
            OmxSaveYuvImageBurstFormat(&d.outputInfo.dispFrame, rotStride, rotbufHeight);
        }
#ifdef FORCE_SET_VSYNC_FLAG
        set_VSYNC_flag();
#endif
    }

    if (check_VSYNC_flag()) {
        clear_VSYNC_flag();
        if (frame_queue_dequeue(display_queue, &dispDoneIdx) == 0) // omx: != -1
            VPU_DecClrDispFlag(d.handle, dispDoneIdx);
    }
    // save rotated dec width, height to display next decoding time.
    rcPrevDisp = d.outputInfo.rcDisplay;

    if (d.outputInfo.numOfErrMBs) {
        d.totalNumofErrMbs += d.outputInfo.numOfErrMBs;
        VLOG(ERR, "Num of Error Mbs : %d, in Frame : %d \n", d.outputInfo.numOfErrMBs, frameIdx);
    }

    frameIdx++;

    return true;
}

void VideoDecoderVPU::flush()
{
    VPU_DecFrameBufferFlush(handle);
}

VideoFrame VideoDecoderVPU::frame()
{
    DPTR_D(VideoDecoderVPU);
    int dispIdx = -1;
    // 0 on success
    if (frame_queue_dequeue(d.display_queue, &dispIdx) < 0) {
        qDebug("VPU has no frame decoded");
        return VideoFrame();
    }
    // TODO: timestamp is packet pts in OMX!
    VideoFrame frame;
    frame.setMetaData(QStringLiteral("vpu_disp"), dispIdx);
    return frame;
}

} // namespace QtAV
#include "VideoDecoderVPU.moc"
