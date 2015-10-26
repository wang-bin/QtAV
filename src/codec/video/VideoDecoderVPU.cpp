/******************************************************************************
******************************************************************************/

#include "QtAV/VideoDecoder.h"
#include "QtAV/private/AVDecoder_p.h"
#include "QtAV/Packet.h"
#include "QtAV/private/AVCompat.h"
#include "QtAV/private/factory.h"
#include "QtAV/private/mkid.h"
#include "utils/ring.h"
//extern "C" {
#include "coda/vpuapi/vpuapi.h"
#include "coda/vpuapi/vpuapifunc.h"
//}
#include "SurfaceInteropGAL.h"
#include <QtCore/QMetaType>
#include "utils/Logger.h"

//avformat ctx: flag CODEC_FLAG_TRUNCATED
// VPU_DecClrDispFlag(index) after rendering? like destroy surface?

#define VPU_DEC_TIMEOUT       5000
#define VPU_WAIT_TIME_OUT	10		//should be less than normal decoding time to give a chance to fill stream. if this value happens some problem. we should fix VPU_WaitInterrupt function
#define PARALLEL_VPU_WAIT_TIME_OUT 1 	//the value of timeout is 1 means we want to keep a waiting time to give a chance of an interrupt of the next core.
//#define PARALLEL_VPU_WAIT_TIME_OUT 0 	//the value of timeout is 0 means we just check interrupt flag. do not wait any time to give a chance of an interrupt of the next core.
#if PARALLEL_VPU_WAIT_TIME_OUT > 0
#undef VPU_DEC_TIMEOUT
#define VPU_DEC_TIMEOUT       1000
#endif
#define MAX_CHUNK_HEADER_SIZE 1024
#define MAX_ROT_BUF_NUM			2
#define EXTRA_FRAME_BUFFER_NUM	1
#define STREAM_BUF_SIZE		 0x300000  // max bitstream size
#define STREAM_END_SIZE			0

#define VPU_ENSURE(x, ...) \
    do { \
        RetCode ret = x; \
        if (ret != RETCODE_SUCCESS) { \
            qWarning("Coda VPU error@%d. " #x ": %#x", __LINE__, ret); \
            return __VA_ARGS__; \
        } \
    } while(0)
#define VPU_WARN(x) \
do { \
  RetCode res = x; \
  if(res != RETCODE_SUCCESS) \
    qWarning("Coda VPU error@%d. " #x ": %#x", __LINE__, ret); \
} while(0);

namespace QtAV {
static int BuildSeqHeader(BYTE *pbHeader, const CodStd codStd, const AVCodecContext *avctx, int fps_num, int fps_den, int nb_index_entries);
static int BuildPicHeader(BYTE *pbHeader, const CodStd codStd, const AVCodecContext *avctx, const Packet& pkt);
static bool IsSupportInterlaceMode(CodStd bitstreamFormat, DecInitialInfo *pSeqInfo);
static int WriteBsBufFromBufHelper(Uint32 core_idx, DecHandle handle, vpu_buffer_t *pVbStream, BYTE *pChunk,  int chunkSize, int endian);
static void PrintMemoryAccessViolationReason(Uint32 core_idx, DecOutputInfo *out);

static const VideoDecoderId VideoDecoderId_VPU = mkid::id32base36_3<'V', 'P', 'U'>::value;
class VideoDecoderVPUPrivate;
class VideoDecoderVPU : public VideoDecoder
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(VideoDecoderVPU)
public:
    VideoDecoderVPU();
    VideoDecoderId id() const Q_DECL_OVERRIDE {return VideoDecoderId_VPU;}
    QString description() const Q_DECL_OVERRIDE;
    bool decode(const Packet& packet) Q_DECL_OVERRIDE;
    void flush() Q_DECL_OVERRIDE;
    VideoFrame frame() Q_DECL_OVERRIDE;
};

class VideoDecoderVPUPrivate Q_DECL_FINAL: public VideoDecoderPrivate
{
public:
    VideoDecoderVPUPrivate()
        : VideoDecoderPrivate()
        , coreIdx(0)
        , tiled2LinearEnable(false) //?
        , mapType(LINEAR_FRAME_MAP) //?
        , instIdx(0)
        , chunkReuseRequired(false)
        , seqInited(false), seqFilled(false)
        , bsfillSize(0)
        , decodefinish(false)
        , int_reason(0)
        , totalNumofErrMbs(0)
        , ppuEnable(false)
        , handle(0)
        , framebufWidth(0), framebufHeight(0), framebufStride(0)
        , framebufFormat(FORMAT_420)
        , display_queue(ring<FBSurfacePtr>(0))
    {}
    ~VideoDecoderVPUPrivate() {}
    QString getVPUInfo(quint32 core_idx) {
        Uint32 version, revision, productId;
        VPU_GetVersionInfo(core_idx, &version, &revision, &productId);
        return QString("VPU core@%1. projectId:%2, version:%3.%4.%5 r%6, hw version:%7, API:%8")
                .arg(core_idx)
                .arg(version>>16, 0, 16).arg((version>>12)&0x0f).arg((version>>8)&0x0f).arg(version&0xff).arg(revision)
                .arg(productId).arg(API_VERSION);
    }
    bool open() Q_DECL_OVERRIDE;
    void close() Q_DECL_OVERRIDE;
    bool initSeq();
    bool processHeaders(int* seqHeaderSize, int* picHeaderSize, const Packet& pkt, int fps_num, int fps_den, int nb_index_entries);
    // user config
    int coreIdx; // MAX_NUM_VPU_CORE = 1
    bool useRot, useDering; //false
    bool tiled2LinearEnable;
    TiledMapType mapType;
    // vpu status
    int instIdx;
    bool chunkReuseRequired; //false
    bool seqInited, seqFilled; //false
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
    DecInitialInfo initialInfo;
#if defined(SUPPORT_DEC_SLICE_BUFFER) || defined(SUPPORT_DEC_RESOLUTION_CHANGE)
    DecBufInfo decBufInfo;
#endif
 #define MAX_PPU_SRC_NUM 2
    FrameBuffer fbPPU[MAX_PPU_SRC_NUM];

    int frameIdx;
    ring<FBSurfacePtr> display_queue;
    QByteArray seqHeader;
    QByteArray picHeader;
    QString description;
    vpu::InteropResourcePtr interop_res; //may be still used in video frames when decoder is destroyed
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

static inline unsigned int mktag(unsigned int fourcc)
{
    return (unsigned int)MKTAG(toupper((char)fourcc), toupper((char)(fourcc>>8)), toupper((char)(fourcc>>16)), toupper((char)(fourcc>>24)));
}

int fourCCToMp4Class(unsigned int fourcc)
{
    for (size_t i = 0; i < sizeof(codstd_tab)/sizeof(codstd_tab[0]); i++)
        if (codstd_tab[i].fourcc == mktag(fourcc))
            return codstd_tab[i].mp4Class;
    return -1;
}
CodStd fourCCToCodStd(unsigned int fourcc)
{
    for (size_t i = 0; i < sizeof(codstd_tab)/sizeof(codstd_tab[0]); i++)
        if (codstd_tab[i].fourcc == mktag(fourcc))
            return codstd_tab[i].codStd;
    return (CodStd)-1;
}
int codecIdToMp4Class(int codec_id)
{
    for (size_t i = 0; i < sizeof(codstd_tab)/sizeof(codstd_tab[0]); i++)
        if (codstd_tab[i].codec_id == codec_id)
            return codstd_tab[i].mp4Class;
    return -1;

}
CodStd codecIdToCodStd(int codec_id)
{
    for (size_t i = 0; i < sizeof(codstd_tab)/sizeof(codstd_tab[0]); i++)
        if (codstd_tab[i].codec_id == codec_id)
            return codstd_tab[i].codStd;
    return (CodStd)-1;
}
int codecIdToFourcc(int codec_id)
{
    for (size_t i = 0; i < sizeof(codstd_tab)/sizeof(codstd_tab[0]); i++)
        if (codstd_tab[i].codec_id == codec_id)
            return codstd_tab[i].fourcc;
    return 0;
}

VideoDecoderVPU::VideoDecoderVPU()
    : VideoDecoder(*new VideoDecoderVPUPrivate())
{
}

QString VideoDecoderVPU::description() const
{
    return d_func().description;
}

bool VideoDecoderVPUPrivate::open()
{
    if (QtAV::codecIdToCodStd(codec_ctx->codec_id) < 0) {
        qWarning("Unsupported codec");
        return false;
    }
#ifdef HAVE_REPORT
    VpuReportConfig_t reportCfg;
    memset(&reportCfg, 0x00, sizeof(reportCfg));
    reportCfg.userDataEnable = VPU_REPORT_USERDATA;
    reportCfg.userDataReportMode = 0;
    OpenDecReport(coreIdx, &reportCfg);
#endif //HAVE_REPORT
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

    instIdx = VPU_GetOpenInstanceNum(coreIdx); //omx use it. vpurun set from other
    if (instIdx > MAX_NUM_INSTANCE) {
        qDebug("exceed the opened instance num than %d num", MAX_NUM_INSTANCE);
        return false;
    }
    //omx_vpudec_component_vpuLibInit
    memset(&outputInfo, 0, sizeof(outputInfo));
    memset(&decOP, 0, sizeof(decOP));
    decOP.bitstreamFormat = QtAV::fourCCToCodStd(codec_ctx->codec_tag);
    if (decOP.bitstreamFormat < 0)
        decOP.bitstreamFormat = QtAV::codecIdToCodStd(codec_ctx->codec_id);
    if (decOP.bitstreamFormat < 0) {
        qWarning("can not support video format in VPU tag=%c%c%c%c, codec_id=0x%x \n", codec_ctx->codec_tag>>0, codec_ctx->codec_tag>>8, codec_ctx->codec_tag>>16, codec_ctx->codec_tag>>24, codec_ctx->codec_id );
        return false;
    }
    decOP.mp4Class = QtAV::fourCCToMp4Class(codec_ctx->codec_tag);
    if (decOP.mp4Class < 0)
        decOP.mp4Class = QtAV::codecIdToMp4Class(codec_ctx->codec_id);
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
    decOP.cbcrInterleave = CBCR_INTERLEAVE; //0
    if (mapType == TILED_FRAME_MB_RASTER_MAP ||
        mapType == TILED_FIELD_MB_RASTER_MAP) {
            decOP.cbcrInterleave = 1;
    }
    decOP.cbcrOrder = CBCR_ORDER_NORMAL;
    if (false) { //output nv12. viv gal api does not support nv12
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
    seqHeader = QByteArray(codec_ctx->extradata_size + MAX_CHUNK_HEADER_SIZE, 0);	// allocate more buffer to fill the vpu specific header.
    picHeader = QByteArray(MAX_CHUNK_HEADER_SIZE, 0);

    VPU_ENSURE(VPU_DecOpen(&handle, &decOP), false);
    display_queue = ring<FBSurfacePtr>(MAX_REG_FRAME);

    // TODO: omx GET_DRAM_CONFIG is not in open
    DRAMConfig dramCfg;
    VPU_ENSURE(VPU_DecGiveCommand(handle, GET_DRAM_CONFIG, &dramCfg), false);
    seqInited = false;
    seqFilled = false;
    bsfillSize = 0;
    chunkReuseRequired = false;
    frameIdx = 0;
    interop_res = vpu::InteropResourcePtr(new vpu::InteropResource());
    return true;
}

void VideoDecoderVPUPrivate::close()
{
    if (!handle)
        return;
    RetCode ret = VPU_DecClose(handle);
    if (ret == RETCODE_FRAME_NOT_COMPLETE) {
qDebug("RETCODE_FRAME_NOT_COMPLETE");
        VPU_DecGetOutputInfo(handle, &outputInfo);
        VPU_DecClose(handle);
    }
    if (vbStream.size)
        vdi_free_dma_memory(coreIdx, &vbStream);
    seqHeader.clear();
    picHeader.clear();
    display_queue = ring<FBSurfacePtr>(0);
#ifdef HAVE_REPORT
    CloseDecReport(coreIdx);
#endif
}

bool VideoDecoderVPUPrivate::initSeq()
{
qDebug("initSeq: %d", seqInited);
    if (seqInited)
        return true;
#ifdef HAVE_REPORT
    ConfigSeqReport(coreIdx, handle, decOP.bitstreamFormat); // TODO: remove
#endif
    memset(&initialInfo, 0, sizeof(initialInfo));
    if (decOP.bitstreamMode == BS_MODE_PIC_END) { //TODO: no check in omx (always BS_MODE_PIC_END)
        RetCode ret = VPU_DecGetInitialInfo(handle, &initialInfo);
        if (ret != RETCODE_SUCCESS) {
            if (ret == RETCODE_MEMORY_ACCESS_VIOLATION)
                PrintMemoryAccessViolationReason(coreIdx, NULL);
            qWarning("VPU_DecGetInitialInfo failed Error code is %#x", ret);
            return false; //TODO: omx wait for next chunk
        }
        VPU_ClearInterrupt(coreIdx);
    } else {
        qDebug("!BS_MODE_PIC_END");
        // d.int_reason
        if ((int_reason & (1<<INT_BIT_BIT_BUF_EMPTY)) != (1<<INT_BIT_BIT_BUF_EMPTY)) {
            VPU_ENSURE(VPU_DecIssueSeqInit(handle), false);
        } else {
            // After VPU generate the BIT_EMPTY interrupt. HOST should feed the bitstream up to 1024 in case of seq_init
            if (bsfillSize < VPU_GBU_SIZE*2)
                return true;
        }
        while (osal_kbhit() == 0) {
            int_reason = VPU_WaitInterrupt(coreIdx, VPU_DEC_TIMEOUT);
            if (int_reason)
                VPU_ClearInterrupt(coreIdx);
            if(int_reason & (1<<INT_BIT_BIT_BUF_EMPTY))
                break;
            //CheckUserDataInterrupt(coreIdx, handle, 1, decOP.bitstreamFormat, int_reason);
            if (int_reason && (int_reason & (1<<INT_BIT_SEQ_INIT))) {
                seqInited = 1;
                break;
            }
        }
        if (int_reason & (1<<INT_BIT_BIT_BUF_EMPTY) || int_reason == -1) {
            bsfillSize = 0;
	                qDebug("bsfillSize = 0 INT_BIT_BIT_BUF_EMPTY|int_reason-1");

            return true; // go to take next chunk.
        }
        if (seqInited) {
            RetCode ret = VPU_DecCompleteSeqInit(handle, &initialInfo);
            if (ret != RETCODE_SUCCESS) {
                if (ret == RETCODE_MEMORY_ACCESS_VIOLATION)
                    PrintMemoryAccessViolationReason(coreIdx, NULL);
                if (initialInfo.seqInitErrReason & (1<<31)) // this case happened only ROLLBACK mode
                    qWarning("Not enough header : Parser has to feed right size of a sequence header");
                qWarning("VPU_DecCompleteSeqInit failed Error code is 0x%x", ret);
                return false;
            }
        } else {
            //qWarning("VPU_DecGetInitialInfo failed Error code is 0x%x", ret);
            return false;
        }
    }
#ifdef HAVE_REPORT
    SaveSeqReport(coreIdx, handle, &initialInfo, decOP.bitstreamFormat);
#endif
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
    } else {
        framebufWidth = FFALIGN(initialInfo.picWidth, 16);
        // TODO: omx fbHeight align 16
        if (IsSupportInterlaceMode(decOP.bitstreamFormat, &initialInfo))
            framebufHeight = FFALIGN(initialInfo.picHeight, 32); // framebufheight must be aligned by 31 because of the number of MB height would be odd in each filed picture.
        else
            framebufHeight = FFALIGN(initialInfo.picHeight, 16);
    }
    framebufStride = framebufWidth;
    framebufFormat = FORMAT_420;
    // vpurun: allocate out buffer of size framebufSize for rendering. we do not need it
    //framebufSize = VPU_GetFrameBufSize(framebufStride, framebufHeight, mapType, framebufFormat, &dramCfg);
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
    //omx check PrintMemoryAccessViolationReason(coreIdx, NULL);
    TiledMapConfig mapCfg;
    memset(&mapCfg, 0, sizeof(mapCfg));
    VPU_ENSURE(VPU_DecGiveCommand(handle, GET_TILEDMAP_CONFIG, &mapCfg), false);
    // ppu here
qDebug("framebufStride: %d h: %d, initialInfo.picWidth/Height:%dx%d", framebufStride, framebufWidth, initialInfo.picWidth, initialInfo.picHeight);
    seqInited = true;
    return true;
}

bool VideoDecoderVPUPrivate::processHeaders(int *seqHeaderSize, int *picHeaderSize, const Packet &pkt, int fps_num, int fps_den, int nb_index_entries)
{
    if (!seqInited && !seqFilled) {
qDebug("BuildSeqHeader");
        // FIXME: copy from omx or vpuhelper
        *seqHeaderSize = BuildSeqHeader((BYTE*)seqHeader.constData(), decOP.bitstreamFormat, codec_ctx, fps_num, fps_den, nb_index_entries);	// make sequence data as reference file header to support VPU decoder.
        if (*seqHeaderSize < 0) {// indicate the stream dose not support in VPU.
            qWarning("BuildSeqHeader the stream does not support in VPU");
            return false;
        }
        // TODO: vpurun check STD_THO, STD_VP3
        if (*seqHeaderSize > 0) { //write to vbStream
            const int size = WriteBsBufFromBufHelper(coreIdx, handle, &vbStream, (BYTE*)seqHeader.constData(), *seqHeaderSize, decOP.streamEndian);
            if (size < 0) {
                qWarning("WriteBsBufFromBufHelper failed Error code is 0x%x", size);
                return false;
            }
            bsfillSize += size;
        }
        seqFilled = true;
    }
qDebug("BuildPicHeader");
    // Build and Fill picture Header data which is dedicated for VPU
    //picHeaderSize is also used in FLUSH_BUFFER
    *picHeaderSize = BuildPicHeader((BYTE*)picHeader.constData(), decOP.bitstreamFormat, codec_ctx, pkt); //FIXME
    if (*picHeaderSize > 0) { // TODO: no check in vpurun
        switch(decOP.bitstreamFormat) {
        case STD_THO:
        case STD_VP3:
            break;
        default: // write to vbStream
            const int size = WriteBsBufFromBufHelper(coreIdx, handle, &vbStream, (BYTE*)picHeader.constData(), *picHeaderSize, decOP.streamEndian);
            if (size < 0) {
                qWarning("WriteBsBufFromBufHelper failed Error code is 0x%x", size);
                return false;
            }
            bsfillSize += size; // TODO: not in omx
            break;
        }
    }
    return true;
}

bool VideoDecoderVPU::decode(const Packet &packet)
{
    DPTR_D(VideoDecoderVPU);
    if (d.decodefinish) {
        qWarning("decode finished");
        return false;
    }
    QByteArray chunkData(packet.data);
    DecOpenParam &decOP = d.decOP;
    int seqHeaderSize = 0; //also used in FLUSH_BUFFER
    int picHeaderSize = 0;
    if (packet.isEOF()) {
        VPU_ENSURE(VPU_DecUpdateBitstreamBuffer(d.handle, STREAM_END_SIZE), false);	//tell VPU to reach the end of stream. starting flush decoded output in VPU
        chunkData = QByteArray(); //decode the trailing frames
qDebug("eof packet");
    } else {
        if (decOP.bitstreamMode == BS_MODE_PIC_END) { //TODO: no check in omx. it's always true here
            if (!d.chunkReuseRequired) {
    qDebug("!chunkReuseRequired VPU_DecSetRdPtr");
                VPU_DecSetRdPtr(d.handle, decOP.bitstreamBuffer, 1);
            }
        }
        if (!d.chunkReuseRequired) {
            if (!d.processHeaders(&seqHeaderSize, &picHeaderSize, packet, property("fps_num").toInt(), property("fps_den").toInt(), property("nb_index_entries").toInt())) {
        qWarning("procesHeader error");
                return false;
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
            const int cSlice = chunkData.at(0) + 1;
                chunkData.remove(0, 1+(cSlice*8));
            }
            const int size = WriteBsBufFromBufHelper(d.coreIdx, d.handle, &d.vbStream, (BYTE*)chunkData.constData(), chunkData.size(), decOP.streamEndian);
            if (size <0) {
                qWarning("WriteBsBufFromBufHelper failed Error code is 0x%x", size);
                return false;
            }
            d.bsfillSize += size;
        }
        break;
        }

    qDebug("bsfillSize: %d", d.bsfillSize);
        //chunkIdx++;
        if (!d.initSeq())
            return false;
        d.chunkReuseRequired = false;
    }
//FLUSH_BUFFER:
    if(!(d.int_reason & (1<<INT_BIT_BIT_BUF_EMPTY)) && !(d.int_reason & (1<<INT_BIT_DEC_FIELD))) {
        // ppu here
#ifdef HAVE_REPORT
        ConfigDecReport(d.coreIdx, d.handle, decOP.bitstreamFormat);
#endif
        // Start decoding a frame.
        DecParam decParam; //TODO: can set frame drop here
        memset(&decParam, 0, sizeof(decParam));
        VPU_ENSURE(VPU_DecStartOneFrame(d.handle, &decParam), false);
    } else { // TODO: omx always run here
qDebug("INT_BIT_BIT_BUF_EMPTY|INT_BIT_DEC_FIELD");
        if(d.int_reason & (1<<INT_BIT_DEC_FIELD)) {
            d.int_reason = 0;
qDebug("INT_BIT_DEC_FIELD");
            VPU_ClearInterrupt(d.coreIdx);
        }
        // After VPU generate the BIT_EMPTY interrupt. HOST should feed the bitstreams than 512 byte.
        if (decOP.bitstreamMode != BS_MODE_PIC_END) {
qDebug("!BS_MODE_PIC_END");
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
        if (d.int_reason == -1 ) {// timeout
            VPU_SWReset(d.coreIdx, SW_RESET_SAFETY, d.handle);
            qWarning("VPU_WaitInterrupt timeout");
            break;
        }
        //CheckUserDataInterrupt(d.coreIdx, d.handle, d.outputInfo.indexFrameDecoded, decOP.bitstreamFormat, d.int_reason);
        if (d.int_reason & (1<<INT_BIT_DEC_FIELD)) {
            qDebug("INT_BIT_DEC_FIELD");
            if (decOP.bitstreamMode == BS_MODE_PIC_END) {
qDebug("VPU_DecGetBitstreamBuffer");
                PhysicalAddress rdPtr, wrPtr;
                int room;
                VPU_DecGetBitstreamBuffer(d.handle, &rdPtr, &wrPtr, &room);
                // TODO: omx
                if (rdPtr-decOP.bitstreamBuffer <
                        (PhysicalAddress)(chunkData.size()+picHeaderSize+seqHeaderSize-8)
                        ) {// there is full frame data in chunk data.
			qDebug("VPU_DecSetRdPtr full frame data in chunk data");
                    VPU_DecSetRdPtr(d.handle, rdPtr, 0);		//set rdPtr to the position of next field data.
		    }
                else // do not clear interrupt until feeding next field picture.
                    break;
            }
        }
        if (d.int_reason) {
qDebug("VPU_ClearInterrupt");
            VPU_ClearInterrupt(d.coreIdx);
	    }
        if (d.int_reason & (1<<INT_BIT_BIT_BUF_EMPTY)) { // TODO: omx no check and below
            if (decOP.bitstreamMode == BS_MODE_PIC_END) {
                qWarning("Invalid operation is occurred in pic_end mode");
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
        qDebug("bsfillSize = 0 INT_BIT_BIT_BUF_EMPTY|INT_BIT_DEC_FIELD");
        return true;//continue; // go to take next chunk.
    }
    // ---NOT IN OMX END
    RetCode ret = VPU_DecGetOutputInfo(d.handle, &d.outputInfo);
    if (ret != RETCODE_SUCCESS) {
        qWarning( "VPU_DecGetOutputInfo failed Error code is 0x%x", ret);
        if (ret == RETCODE_MEMORY_ACCESS_VIOLATION)
            PrintMemoryAccessViolationReason(d.coreIdx, &d.outputInfo);
        return false;
    }
    if ((d.outputInfo.decodingSuccess & 0x01) == 0) { //OMX no 0x01
        qWarning("VPU_DecGetOutputInfo decode fail");
        qWarning("indexFrameDisplay %d || picType %d || indexFrameDecoded %d",
             d.outputInfo.indexFrameDisplay, d.outputInfo.picType, d.outputInfo.indexFrameDecoded );
    }
    //qDebug("#%d:%d, indexDisplay %d || picType %d || indexDecoded %d || rdPtr=0x%x || wrPtr=0x%x || chunkSize = %d, consume=%d",
      //  d.instIdx, d.frameIdx, d.outputInfo.indexFrameDisplay, d.outputInfo.picType, d.outputInfo.indexFrameDecoded, d.outputInfo.rdPtr, d.outputInfo.wrPtr, chunkData.size()+picHeaderSize, d.outputInfo.consumedByte);
#ifdef HAVE_REPORT
    SaveDecReport(d.coreIdx, d.handle, &d.outputInfo, decOP.bitstreamFormat, ((d.initialInfo.picWidth+15)&~15)/16); ///TODO:
#endif
    if (d.outputInfo.chunkReuseRequired) {// reuse previous chunk. that would be true once framebuffer is full.
        d.chunkReuseRequired = true;
        d.undecoded_size = chunkData.size(); // decode previous chunk again!!
        qDebug("chunkReuseRequired");
    } else {
        // must always set undecoded_size if it was set!!!
        d.undecoded_size = 0;
    }

    if (d.outputInfo.indexFrameDisplay == -1) {
        qDebug("decodefinish. indexFrameDisplay==-1");
        d.decodefinish = true;
    }
    if (d.decodefinish) { //ppu here
        return !packet.isEOF();
    }
    if (d.outputInfo.indexFrameDisplay == -3 ||
        d.outputInfo.indexFrameDisplay == -2 ) // BIT doesn't have picture to be displayed, or frame drop
    {
        qDebug("BIT doesn't have picture to be displayed, or frame drop. indexFrameDisplay: %d", d.outputInfo.indexFrameDisplay);
        if (!d.display_queue.empty()) { //?
            d.display_queue.pop_front();
        }
        if (d.outputInfo.indexFrameDecoded == -1)	// VPU did not decode a picture because there is not enough frame buffer to continue decoding
        {
            qDebug("VPU did not decode a picture because there is not enough frame buffer to continue decoding");
            // TODO: doc says VPU_DecClrDispFlag() + VPU_DecStartOneFrame()
            // if you can't get VSYN interrupt on your sw layer. this point is reasonable line to set VSYN flag.
            // but you need fine tune EXTRA_FRAME_BUFFER_NUM value not decoder to write being display buffer.

        }
        return !packet.isEOF();// more packet required
    }
    // ppu here
    if (d.outputInfo.indexFrameDisplay >= 0) {
        FBSurfacePtr surf(new FBSurface(d.handle));
        surf->index = d.outputInfo.indexFrameDisplay;
        surf->fb = d.outputInfo.dispFrame;
        d.display_queue.push_back(surf);
        qDebug("push_back FBSurfacePtr. queue.size: %lu", d.display_queue.size());
    }
    if (d.outputInfo.numOfErrMBs) {
        d.totalNumofErrMbs += d.outputInfo.numOfErrMBs;
        qWarning("Num of Error Mbs : %d", d.outputInfo.numOfErrMBs);
    }
    d.frameIdx++;
    return true;
}

void VideoDecoderVPU::flush()
{
    DPTR_D(VideoDecoderVPU);
    VPU_DecFrameBufferFlush(d.handle);
    //d.display_queue = ring<FBSurfacePtr>(0); //FIXME: why decode error and block (framebuffer space not enough)?
}

VideoFrame VideoDecoderVPU::frame()
{
    DPTR_D(VideoDecoderVPU);
    if (d.display_queue.empty()) {
        qDebug("VPU has no frame decoded");
        return VideoFrame();
    }
    FBSurfacePtr surf(d.display_queue.front());
    d.display_queue.pop_front();
    // TODO: timestamp is packet pts in OMX!
    // TODO: use Format_YUV420P so that we can capture yuv frame. but we have to change code somewhere else
    VideoFrame frame(d.codec_ctx->width, d.codec_ctx->height, VideoFormat::Format_RGB32);
    //frame.setTimestamp();
    vpu::SurfaceInteropGAL *interop = new vpu::SurfaceInteropGAL(d.interop_res);
    interop->setSurface(surf, d.codec_ctx->width, d.codec_ctx->height);
    frame.setMetaData(QStringLiteral("surface_interop"), QVariant::fromValue(VideoSurfaceInteropPtr(interop)));
    return frame;
}

#define PUT_BYTE(_p, _b) *_p++ = (unsigned char)_b; 
#define PUT_BUFFER(_p, _buf, _len) \
  memcpy(_p, _buf, _len); \
   _p += _len;
#define PUT_LE32(_p, _var) \
*_p++ = (unsigned char)((_var)>>0);  \
*_p++ = (unsigned char)((_var)>>8);  \
*_p++ = (unsigned char)((_var)>>16); \
*_p++ = (unsigned char)((_var)>>24); 
#define PUT_BE32(_p, _var) \
*_p++ = (unsigned char)((_var)>>24);  \
*_p++ = (unsigned char)((_var)>>16);  \
*_p++ = (unsigned char)((_var)>>8); \
*_p++ = (unsigned char)((_var)>>0); 
#define PUT_LE16(_p, _var) \
*_p++ = (unsigned char)((_var)>>0);  \
*_p++ = (unsigned char)((_var)>>8);  
#define PUT_BE16(_p, _var) *_p++ = (unsigned char)((_var)>>8);  \
 *_p++ = (unsigned char)((_var)>>0);  

int BuildSeqHeader(BYTE *pbHeader, const CodStd codStd, const AVCodecContext *avctx, int fps_num, int fps_den, int nb_index_entries)
{
    BYTE *pbMetaData = avctx->extradata;
    int nMetaData = avctx->extradata_size;
    BYTE* p = pbMetaData;
    BYTE *a = p + 4 - ((long) p & 3);
    BYTE* t = pbHeader;
    int sps, pps, i, nal;

    int fourcc = avctx->codec_tag;
    if (!fourcc)
        fourcc = QtAV::codecIdToFourcc(avctx->codec_id);
    int size = 0;
    if (codStd == STD_AVC || codStd == STD_AVS) {
        // check mov/mo4 file format stream
        if (nMetaData > 1 && pbMetaData && pbMetaData[0] == 0x01) {
            p += 5;
            sps = (*p & 0x1f); // Number of sps
            p++;
            for (i = 0; i < sps; i++) {
                nal = (*p << 8) + *(p + 1) + 2;
                PUT_BYTE(t, 0x00);
                PUT_BYTE(t, 0x00);
                PUT_BYTE(t, 0x00);
                PUT_BYTE(t, 0x01);
                PUT_BUFFER(t, p+2, nal-2);
                p += nal;
                size += (nal - 2 + 4); // 4 => length of start code to be inserted
            }
            pps = *(p++); // number of pps
            for (i = 0; i < pps; i++) {
                nal = (*p << 8) + *(p + 1) + 2;
                PUT_BYTE(t, 0x00);
                PUT_BYTE(t, 0x00);
                PUT_BYTE(t, 0x00);
                PUT_BYTE(t, 0x01);
                PUT_BUFFER(t, p+2, nal-2);
                p += nal;
                size += (nal - 2 + 4); // 4 => length of start code to be inserted
            }
        } else if (nMetaData > 3) {
            size = -1;// return to meaning of invalid stream data;
            for (; p < a; p++) {
                if (p[0] == 0 && p[1] == 0 && p[2] == 1) {// find startcode
                    size = avctx->extradata_size;
                    PUT_BUFFER(pbHeader, pbMetaData, size);
                    break;
                }
            }
        }
    } else if (codStd == STD_VC1) {
        if (!fourcc)
            return -1;
        //VC AP
        if (fourcc == MKTAG('W', 'V', 'C', '1') || fourcc == MKTAG('W', 'M', 'V', 'A'))	{
            size = nMetaData;
            PUT_BUFFER(pbHeader, pbMetaData, size);
            //if there is no seq startcode in pbMetatData. VPU will be failed at seq_init stage.
        } else {
#ifdef RCV_V2
            PUT_LE32(pbHeader, ((0xC5 << 24)|0));
            size += 4; //version
            PUT_LE32(pbHeader, nMetaData);
            size += 4;
            PUT_BUFFER(pbHeader, pbMetaData, nMetaData);
            size += nMetaData;
            PUT_LE32(pbHeader, avctx->height);
            size += 4;
            PUT_LE32(pbHeader, avctx->width);
            size += 4;
            PUT_LE32(pbHeader, 12);
            size += 4;
            PUT_LE32(pbHeader, 2 << 29 | 1 << 28 | 0x80 << 24 | 1 << 0);
            size += 4; // STRUCT_B_FRIST (LEVEL:3|CBR:1:RESERVE:4:HRD_BUFFER|24)
            PUT_LE32(pbHeader, avctx->bit_rate);
            size += 4; // hrd_rate
            PUT_LE32(pbHeader, (int)((double)fps_num/(double)fps_den));
            size += 4; // frameRate
#else	//RCV_V1
            PUT_LE32(pbHeader, (0x85 << 24) | 0x00);
            size += 4; //frames count will be here
            PUT_LE32(pbHeader, nMetaData);
            size += 4;
            PUT_BUFFER(pbHeader, pbMetaData, nMetaData);
            size += nMetaData;
            PUT_LE32(pbHeader, avctx->height);
            size += 4;
            PUT_LE32(pbHeader, avctx->width);
            size += 4;
#endif
        }
    } else if (codStd == STD_RV) {
        int st_size =0;
        if (!fourcc)
            return -1;
        if (fourcc != MKTAG('R','V','3','0') && fourcc != MKTAG('R','V','4','0'))
            return -1;
        size = 26 + nMetaData;
        PUT_BE32(pbHeader, size); //Length
        PUT_LE32(pbHeader, MKTAG('V', 'I', 'D', 'O')); //MOFTag
        PUT_LE32(pbHeader, fourcc); //SubMOFTagl
        PUT_BE16(pbHeader, avctx->width);
        PUT_BE16(pbHeader, avctx->height);
        PUT_BE16(pbHeader, 0x0c); //BitCount;
        PUT_BE16(pbHeader, 0x00); //PadWidth;
        PUT_BE16(pbHeader, 0x00); //PadHeight;

        PUT_LE32(pbHeader, (int)((double)fps_num/(double)fps_den));
        PUT_BUFFER(pbHeader, pbMetaData, nMetaData); //OpaqueDatata
        size += st_size; //add for startcode pattern.
    } else if (codStd == STD_DIV3) {
        // not implemented yet
        if (!nMetaData) {
            PUT_LE32(pbHeader, MKTAG('C', 'N', 'M', 'V')); //signature 'CNMV'
            PUT_LE16(pbHeader, 0x00);                      //version
            PUT_LE16(pbHeader, 0x20);                      //length of header in bytes
            PUT_LE32(pbHeader, MKTAG('D', 'I', 'V', '3')); //codec FourCC
            PUT_LE16(pbHeader, avctx->width);                //width
            PUT_LE16(pbHeader, avctx->height);               //height
            PUT_LE32(pbHeader, fps_num);      //frame rate
            PUT_LE32(pbHeader, fps_den);      //time scale(?)
            PUT_LE32(pbHeader, nb_index_entries);      //number of frames in file
            PUT_LE32(pbHeader, 0); //unused
            size += 32;
            return size;
        }
        PUT_BE32(pbHeader, nMetaData);
        size += 4;
        PUT_BUFFER(pbHeader, pbMetaData, nMetaData);
        size += nMetaData;
    } else if (codStd == STD_VP8) { ///FIXME
        PUT_LE32(pbHeader, MKTAG('D', 'K', 'I', 'F')); //signature 'DKIF'
        PUT_LE16(pbHeader, 0x00);                      //version
        PUT_LE16(pbHeader, 0x20);                      //length of header in bytes
        PUT_LE32(pbHeader, MKTAG('V', 'P', '8', '0')); //codec FourCC
        PUT_LE16(pbHeader, avctx->width);                //width
        PUT_LE16(pbHeader, avctx->height);               //height
        PUT_LE32(pbHeader, fps_num);      //frame rate
        PUT_LE32(pbHeader, fps_den);      //time scale(?)
        PUT_LE32(pbHeader, nb_index_entries);      //number of frames in file
        PUT_LE32(pbHeader, 0); //unused
        size += 32;
    } else {
        PUT_BUFFER(pbHeader, pbMetaData, nMetaData);
        size = nMetaData;
    }
    return size;
}

int BuildPicHeader(BYTE *pbHeader, const CodStd codStd, const AVCodecContext *avctx, const Packet& pkt)
{
    int size = 0;
    int fourcc = avctx->codec_tag;
    if (!fourcc)
        fourcc = QtAV::codecIdToFourcc(avctx->codec_id);
    BYTE *pbChunk = (BYTE*)pkt.data.constData();
    if (codStd == STD_VC1) {
        if (!fourcc)
            return -1;
        if (fourcc == MKTAG('W', 'V', 'C', '1') || fourcc == MKTAG('W', 'M', 'V', 'A'))	{//VC AP
            if (pbChunk[0] != 0 || pbChunk[1] != 0 || pbChunk[2] != 1) {// check start code as prefix (0x00, 0x00, 0x01)
                pbHeader[0] = 0x00;
                pbHeader[1] = 0x00;
                pbHeader[2] = 0x01;
                pbHeader[3] = 0x0D;	// replace to the correct picture header to indicate as frame
                size += 4;
            }
        } else {
            PUT_LE32(pbHeader, pkt.data.size() | (pkt.hasKeyFrame ? 0x80000000 : 0));
            size += 4;
#ifdef RCV_V2
            if (pkt.pts == 0) { //NO_PTS
                PUT_LE32(pbHeader, 0);
            } else {
                PUT_LE32(pbHeader, (int)((double)(pkt->pts*1000.0))); // milli_sec
            }
            size += 4;
#endif
        }
    } else if (codStd == STD_RV) {
        int st_size = 0;
        if (!fourcc)
            return -1;
        if (fourcc != MKTAG('R','V','3','0') && fourcc != MKTAG('R','V','4','0')) // RV version 8, 9 , 10
            return -1;
        int cSlice = pbChunk[0] + 1;
        int nSlice =  pkt.data.size() - 1 - (cSlice * 8);
        size = 20 + (cSlice*8);
        PUT_BE32(pbHeader, nSlice);
        if (pkt.pts == 0) {
            PUT_LE32(pbHeader, 0);
        } else {
            PUT_LE32(pbHeader, (int)((double)(pkt.pts*1000.0))); // milli_sec
        }
        PUT_BE16(pbHeader, avctx->frame_number);
        PUT_BE16(pbHeader, 0x02); //Flags
        PUT_BE32(pbHeader, 0x00); //LastPacket
        PUT_BE32(pbHeader, cSlice); //NumSegments
        int offset = 1;
        for (int i = 0; i < (int) cSlice; i++) {
            int val = (pbChunk[offset+3] << 24) | (pbChunk[offset+2] << 16) | (pbChunk[offset+1] << 8) | pbChunk[offset];
            PUT_BE32(pbHeader, val); //isValid
            offset += 4;
            val = (pbChunk[offset+3] << 24) | (pbChunk[offset+2] << 16) | (pbChunk[offset+1] << 8) | pbChunk[offset];
            PUT_BE32(pbHeader, val); //Offset
            offset += 4;
        }
        size += st_size;
#if 0
        PUT_BUFFER(pbChunk, pkt->data+(1+(cSlice*8)), nSlice);
        size += nSlice;
#endif
    } else if (codStd == STD_AVC) {
        if(pkt.data.size() < 5)
            return 0;
        int has_st_code = 0;
        if (!(avctx->extradata_size > 1 && avctx->extradata && avctx->extradata[0] == 0x01)) {
            const Uint8 *pbEnd = pbChunk + 4 - ((intptr_t)pbChunk & 3);
            for (; pbChunk < pbEnd ; pbChunk++) {
                if (pbChunk[0] == 0 && pbChunk[1] == 0 && pbChunk[2] == 1) {
                    has_st_code = 1;
                    break;
                }
            }
        }
        // check sequence metadata if the stream is mov/mo4 file format.
        if ((!has_st_code && avctx->extradata[0] == 0x01) || (avctx->extradata_size > 1 && avctx->extradata && avctx->extradata[0] == 0x01)) {
            pbChunk = (BYTE*)pkt.data.constData();
            int offset = 0;
            while (offset < pkt.data.size()) {
                int nSlice = pbChunk[offset] << 24 | pbChunk[offset+1] << 16 | pbChunk[offset+2] << 8 | pbChunk[offset+3];
                pbChunk[offset] = 0x00;
                pbChunk[offset+1] = 0x00;
                pbChunk[offset+2] = 0x00;
                pbChunk[offset+3] = 0x01;		//replace size to startcode
                offset += 4;
                switch (pbChunk[offset]&0x1f) /* NAL unit */
                {
                case 6: /* SEI */
                case 7: /* SPS */
                case 8: /* PPS */
                case 9: /* AU */
                    /* check next */
                    break;
                }
                offset += nSlice;
            }
        }
    } else if(codStd == STD_AVS) {
        const Uint8 *pbEnd;
        if(pkt.data.size() < 5)
            return 0;
        pbEnd = pbChunk + 4 - ((intptr_t)pbChunk & 3);
        int has_st_code = 0;
        for (; pbChunk < pbEnd ; pbChunk++) {
            if (pbChunk[0] == 0 && pbChunk[1] == 0 && pbChunk[2] == 1) {
                has_st_code = 1;
                break;
            }
        }
        if(has_st_code == 0) {
            pbChunk = (BYTE*)pkt.data.constData();
            int offset = 0;
            while (offset < pkt.data.size()) {
                int nSlice = pbChunk[offset] << 24 | pbChunk[offset+1] << 16 | pbChunk[offset+2] << 8 | pbChunk[offset+3];
                pbChunk[offset] = 0x00;
                pbChunk[offset+1] = 0x00;
                pbChunk[offset+2] = 0x00;
                pbChunk[offset+3] = 0x00;		//replace size to startcode
                pbChunk[offset+4] = 0x01;
                offset += 4;
                switch (pbChunk[offset]&0x1f) /* NAL unit */
                {
                case 6: /* SEI */
                case 7: /* SPS */
                case 8: /* PPS */
                case 9: /* AU */
                    /* check next */
                    break;
                }
                offset += nSlice;
            }
        }
    } else if (codStd == STD_DIV3) {
        PUT_LE32(pbHeader,pkt.data.size());
        PUT_LE32(pbHeader,0);
        PUT_LE32(pbHeader,0);
        size += 12;
    } else if (codStd == STD_VP8) {
        PUT_LE32(pbHeader,pkt.data.size());
        PUT_LE32(pbHeader,0);
        PUT_LE32(pbHeader,0);
        size += 12;
    }
    return size;
}

bool IsSupportInterlaceMode(CodStd bitstreamFormat, DecInitialInfo *pSeqInfo)
{
    bool bSupport = false;
    int profile = pSeqInfo->profile;
    switch(bitstreamFormat) {
    case STD_AVC:
        profile = (pSeqInfo->profile==66) ? 0 : (pSeqInfo->profile==77) ? 1 : (pSeqInfo->profile==88) ? 2 : (pSeqInfo->profile==100) ? 3 : 4;
        bSupport = !!profile; // BP: profile==0
        break;
    case STD_MPEG4:
        if (pSeqInfo->level & 0x80) {
            pSeqInfo->level &= 0x7F;
            if (pSeqInfo->level == 8 && pSeqInfo->profile == 0) {
                profile = 0; // Simple
            } else {
                switch (pSeqInfo->profile) {
                case 0xB:	profile = 2; break;
                case 0xF:
                    if ((pSeqInfo->level&8) == 0)
                        profile = 1;
                    else
                        profile = 5;
                    break;
                case 0x0:	profile = 0; break;
                default :	profile = 5; break;
                }
            }
        } else {// Vol Header Only
            switch(pSeqInfo->profile) {
            case  0x1: profile = 0; break; // simple object
            case  0xC: profile = 2; break; // advanced coding efficiency object
            case 0x11: profile = 1; break; // advanced simple object
            default  : profile = 5; break; // reserved
            }
        }
        bSupport = !!profile; //SP: profile == 0
        break;
    case STD_VC1:
        profile = pSeqInfo->profile;
        bSupport = !(profile == 0 || profile == 1);	// SP	// MP
        break;
    case STD_RV:
        profile = pSeqInfo->profile - 8;
        bSupport = !!profile;
        break;
    case STD_MPEG2:
    case STD_AVS:
        bSupport = true;
        break;
    case STD_H263:
    case STD_DIV3:
        bSupport = false;
        break;
    case STD_THO:
    case STD_VP3:
    case STD_VP8:
        bSupport = false;
        break;
    }
    return bSupport;
}

int WriteBsBufFromBufHelper(Uint32 core_idx, DecHandle handle, vpu_buffer_t *pVbStream, BYTE *pChunk, int chunkSize, int endian)
{
    if (chunkSize < 1)
        return 0;
    if (chunkSize > pVbStream->size)
        return -1;
    int size = 0;
    PhysicalAddress paRdPtr, paWrPtr, targetAddr;
    VPU_ENSURE(VPU_DecGetBitstreamBuffer(handle, &paRdPtr, &paWrPtr, &size), -1);
    if(size < chunkSize)
        return 0; // no room for feeding bitstream. it will take a change to fill stream
    targetAddr = paWrPtr;
    if ((targetAddr+chunkSize) >  (pVbStream->phys_addr+pVbStream->size)) {
        int room = (pVbStream->phys_addr+pVbStream->size) - targetAddr;
        //write to physical address
        vdi_write_memory(core_idx, targetAddr, pChunk, room, endian);
        vdi_write_memory(core_idx, pVbStream->phys_addr, pChunk+room, chunkSize-room, endian);
    } else {
        //write to physical address
        vdi_write_memory(core_idx, targetAddr, pChunk, chunkSize, endian);
    }
    VPU_ENSURE(VPU_DecUpdateBitstreamBuffer(handle, chunkSize), -1);
    return chunkSize;
}

void PrintMemoryAccessViolationReason(Uint32 core_idx, DecOutputInfo *out)
{
    Uint32 err_reason;
    Uint32 err_addr;
    Uint32 err_size;
    Uint32 err_size1;
    Uint32 err_size2;
    Uint32 err_size3;
    if (out) {
        err_reason = out->wprotErrReason;
        err_addr = out->wprotErrAddress;
        err_size = VpuReadReg(core_idx, GDI_SIZE_ERR_FLAG);
    } else {
        err_reason = VpuReadReg(core_idx, GDI_WPROT_ERR_RSN);
        err_addr = VpuReadReg(core_idx, GDI_WPROT_ERR_ADR);
        err_size = VpuReadReg(core_idx, GDI_SIZE_ERR_FLAG);
    }
    //vdi_log(core_idx, 0x10, 1);
    if (err_size) {
        VLOG(ERR, "~~~~~~~ GDI rd/wr zero request size violation ~~~~~~~ \n");
        VLOG(ERR, "err_size = %0x%x\n",   err_size);
        err_size1 = VpuReadReg(core_idx, GDI_ADR_RQ_SIZE_ERR_PRI0);
        err_size2 = VpuReadReg(core_idx, GDI_ADR_RQ_SIZE_ERR_PRI1);
        err_size3 = VpuReadReg(core_idx, GDI_ADR_RQ_SIZE_ERR_PRI2);
        VLOG(ERR, "err_size1 = 0x%x || err_size2 = 0x%x || err_size3 = 0x%x \n", err_size1, err_size2, err_size3);
    } else {
        VLOG(ERR, "~~~~~~~ Memory write access violation ~~~~~~~ \n");
        VLOG(ERR, "pri/sec = %d\n",   (err_reason>>8) & 1);
        VLOG(ERR, "awid    = %d\n",   (err_reason>>4) & 0xF);
        VLOG(ERR, "awlen   = %d\n",   (err_reason>>0) & 0xF);
        VLOG(ERR, "awaddr  = 0x%X\n", err_addr);
        VLOG(ERR, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ \n");
    }
    //---------------------------------------------+
    //| Primary AXI interface (A)WID               |
    //+----+----------------------------+----------+
    //| ID |                            | sec use  |
    //+----+----------------------------+----------+
    //| 00 |  BPU MIB primary           | NA       |
    //| 01 |  Overlap filter primary    | NA       |
    //| 02 |  Deblock write-back        | NA       |
    //| 03 |  PPU                       | NA       |
    //| 04 |  Deblock sub-sampling      | NA       |
    //| 05 |  Reconstruction            | possible |
    //| 06 |  BPU MIB secondary         | possible |
    //| 07 |  BPU SPP primary           | NA       |
    //| 08 |  Spatial prediction        | possible |
    //| 09 |  Overlap filter secondary  | possible |
    //| 10 |  Deblock Y secondary       | possible |
    //| 11 |  Deblock C secondary       | possible |
    //| 12 |  JPEG write-back or Stream | NA       |
    //| 13 |  JPEG secondary            | possible |
    //| 14 |  DMAC write                | NA       |
    //| 15 |  BPU SPP secondary         | possible |
    //+----+----------------------------+----------
}

FACTORY_REGISTER(VideoDecoder, VPU, "VPU")
} // namespace QtAV
Q_DECLARE_METATYPE(QtAV::FBSurfacePtr)
#include "VideoDecoderVPU.moc"
