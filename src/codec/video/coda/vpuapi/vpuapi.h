//--=========================================================================--
//  This file is a part of VPU Reference API project
//-----------------------------------------------------------------------------
//
//       This confidential and proprietary software may be used only
//     as authorized by a licensing agreement from Chips&Media Inc.
//     In the event of publication, the following notice is applicable:
//
//            (C) COPYRIGHT 2006 - 2011  CHIPS&MEDIA INC.
//                      ALL RIGHTS RESERVED
//
//       The entire notice above must be reproduced on all authorized
//       copies.
//
//--=========================================================================--

#ifndef VPUAPI_H_INCLUDED
#define VPUAPI_H_INCLUDED

#include "vpuconfig.h"



#include "../vdi/vdi.h"
#include "../vdi/vdi_osal.h"


//#define FEAT_MINI_PIPPEN
//#define FEAT_OUTPUT_FORMAT

//------------------------------------------------------------------------------
// common struct and definition
//------------------------------------------------------------------------------
typedef enum {
	STD_AVC,
	STD_VC1,
	STD_MPEG2,
	STD_MPEG4,
	STD_H263,
	STD_DIV3,
	STD_RV,
	STD_AVS,
	STD_THO = 9,
	STD_VP3,
	STD_VP8
} CodStd;

typedef enum {
	RETCODE_SUCCESS,
	RETCODE_FAILURE,
	RETCODE_INVALID_HANDLE,
	RETCODE_INVALID_PARAM,
	RETCODE_INVALID_COMMAND,
	RETCODE_ROTATOR_OUTPUT_NOT_SET,
	RETCODE_ROTATOR_STRIDE_NOT_SET,
	RETCODE_FRAME_NOT_COMPLETE,
	RETCODE_INVALID_FRAME_BUFFER,
	RETCODE_INSUFFICIENT_FRAME_BUFFERS,
	RETCODE_INVALID_STRIDE,
	RETCODE_WRONG_CALL_SEQUENCE,
	RETCODE_CALLED_BEFORE,
	RETCODE_NOT_INITIALIZED,
	RETCODE_USERDATA_BUF_NOT_SET,
	RETCODE_MEMORY_ACCESS_VIOLATION,
	RETCODE_VPU_RESPONSE_TIMEOUT,
	RETCODE_INSUFFICIENT_RESOURCE,
	RETCODE_NOT_FOUND_BITCODE_PATH
} RetCode;

typedef enum {
	ENABLE_ROTATION,
	DISABLE_ROTATION,
	ENABLE_MIRRORING,
	DISABLE_MIRRORING,
	SET_MIRROR_DIRECTION,
	SET_ROTATION_ANGLE,
	SET_ROTATOR_OUTPUT,
	SET_ROTATOR_STRIDE,
	DEC_SET_SPS_RBSP,
	DEC_SET_PPS_RBSP,
	ENABLE_DERING,
	DISABLE_DERING,
	SET_SEC_AXI,
	SET_DRAM_CONFIG,
	GET_DRAM_CONFIG,
	ENABLE_REP_USERDATA,
	DISABLE_REP_USERDATA,
	SET_ADDR_REP_USERDATA,
	SET_VIRT_ADDR_REP_USERDATA,
	SET_SIZE_REP_USERDATA,
	SET_USERDATA_REPORT_MODE,
	GET_TILEDMAP_CONFIG,
	SET_LOW_DELAY_CONFIG,
	DEC_GET_FIELD_PIC_TYPE,
    SET_DECODE_FLUSH,
    DEC_SET_FRAME_DELAY,
    DEC_GET_DISPLAY_OUTPUT_INFO,
	DEC_ENABLE_REORDER,
	DEC_DISABLE_REORDER,
	DEC_FREE_FRAME_BUFFER,
	DEC_SET_DISPLAY_FLAG,
	//vpu put header stream to bitstream buffer
	ENC_PUT_VIDEO_HEADER,
	ENC_PUT_MP4_HEADER,
	ENC_PUT_AVC_HEADER,
	//host generate header bitstream
	ENC_GET_VIDEO_HEADER,
	ENC_SET_INTRA_MB_REFRESH_NUMBER,
	ENC_ENABLE_HEC,
	ENC_DISABLE_HEC,
	ENC_SET_SLICE_INFO,
	ENC_SET_GOP_NUMBER,
	ENC_SET_INTRA_QP,
	ENC_SET_BITRATE,
	ENC_SET_FRAME_RATE,
	GET_LOW_DELAY_OUTPUT,
	ENABLE_LOGGING,
	DISABLE_LOGGING,
    CMD_END
} CodecCommand;

typedef enum {
	CBCR_ORDER_NORMAL,
	CBCR_ORDER_REVERSED
} CbCrOrder;


typedef enum {
	MIRDIR_NONE,
	MIRDIR_VER,
	MIRDIR_HOR,
	MIRDIR_HOR_VER
} MirrorDirection;

typedef enum {
	FORMAT_420              = 0,
	FORMAT_422              = 1,
	FORMAT_224              = 2,
	FORMAT_444              = 3,
	FORMAT_400              = 4
}FrameBufferFormat;

typedef enum{
	YUV_FORMAT_I420,
	YUV_FORMAT_NV12,
	YUV_FORMAT_NV21,
	YUV_FORMAT_I422,
	YUV_FORMAT_NV16,
	YUV_FORMAT_NV61,
	YUV_FORMAT_UYVY,
	YUV_FORMAT_YUYV,
	
}EYUVFORMAT;

typedef enum {
	INT_BIT_INIT            = 0,
	INT_BIT_SEQ_INIT        = 1,
	INT_BIT_SEQ_END         = 2,
	INT_BIT_PIC_RUN         = 3,
	INT_BIT_FRAMEBUF_SET    = 4,
	INT_BIT_ENC_HEADER      = 5,
	INT_BIT_DEC_PARA_SET    = 7,
	INT_BIT_DEC_BUF_FLUSH   = 8,
	INT_BIT_USERDATA        = 9,
	INT_BIT_DEC_FIELD       = 10,
	INT_BIT_DEC_MB_ROWS     = 13,
	INT_BIT_BIT_BUF_EMPTY   = 14,
	INT_BIT_BIT_BUF_FULL    = 15
}InterruptBit;

typedef enum {
	PIC_TYPE_I            = 0,
	PIC_TYPE_P            = 1,
	PIC_TYPE_B            = 2,
	PIC_TYPE_VC1_BI       = 2,
	PIC_TYPE_VC1_B        = 3,
	PIC_TYPE_D            = 3,    // D picture in mpeg2, and is only composed of DC codfficients
	PIC_TYPE_S            = 3,    // S picture in mpeg4, and is an acronym of Sprite. and used for GMC
	PIC_TYPE_VC1_P_SKIP   = 4,
	PIC_TYPE_MP4_P_SKIP_NOT_CODED = 4, // Not Coded P Picture at mpeg4 packed mode
    PIC_TYPE_IDR          = 5,    // H.264 IDR frame
    PIC_TYPE_MAX
}PicType;

typedef enum {
    PAIRED_FIELD          = 0,
    TOP_FIELD_MISSING     = 1,
    BOT_FIELD_MISSING     = 2,
    TOP_BOT_FIELD_MISSING = 3,
}AvcNpfFieldInfo;

typedef enum {
	FF_NONE                 = 0,
	FF_FRAME                = 1,
	FF_FIELD                = 2
}FrameFlag;

typedef enum {
	BS_MODE_INTERRUPT,
	BS_MODE_ROLLBACK,
	BS_MODE_PIC_END
}BitStreamMode;

typedef enum {
	SW_RESET_SAFETY,
	SW_RESET_FORCE,
	SW_RESET_ON_BOOT
}SWResetMode;






typedef struct {
	int  xy2caMap[16];
	int  xy2baMap[16];
	int  xy2raMap[16];
	int  rbc2axiMap[32];
	int  mapType;
	int  xy2rbcConfig;

	unsigned long  tiledBaseAddr;
	int  tbSeparateMap;
	int  topBotSplit;
	int  tiledMap;
	int  caIncHor;
	int  convLinear;
} TiledMapConfig;


typedef struct {
	int  rasBit;
	int  casBit;
	int  bankBit;
	int  busBit;	
} DRAMConfig;



typedef enum {
	LINEAR_FRAME_MAP  = 0,
	TILED_FRAME_V_MAP = 1,
	TILED_FRAME_H_MAP = 2,
	TILED_FIELD_V_MAP = 3,
	TILED_MIXED_V_MAP = 4,
	TILED_FRAME_MB_RASTER_MAP = 5,
	TILED_FIELD_MB_RASTER_MAP = 6,
	TILED_MAP_TYPE_MAX
} TiledMapType;


typedef struct {
    PhysicalAddress bufY;
    PhysicalAddress bufCb;
    PhysicalAddress bufCr;
    int endian;
    int cbcrInterleave;
    int myIndex;
    int mapType;
    int stride;
    int height;
    int sourceLBurstEn;
} FrameBuffer;

typedef enum {
	FB_TYPE_CODEC,
	FB_TYPE_PPU,
}FramebufferAllocType;

typedef struct {
    int mapType;
    int cbcrInterleave;
    int format;
    int stride;
    int height;
    int endian;
    int num;
    int type;
} FrameBufferAllocInfo;

#if defined(SUPPORT_DEC_SLICE_BUFFER)
typedef struct{
    PhysicalAddress bufferBase;
    int bufferSize;
} ExtBufCfg;
#endif

#if defined(SUPPORT_DEC_SLICE_BUFFER) || defined(SUPPORT_DEC_RESOLUTION_CHANGE)
typedef struct {
#ifdef SUPPORT_DEC_SLICE_BUFFER
	ExtBufCfg avcSliceBufInfo;	
#endif
#ifdef SUPPORT_DEC_RESOLUTION_CHANGE
    int maxDecMbX;
	int maxDecMbY;
    int maxDecMbNum;
#endif
} DecBufInfo;
#endif
typedef struct {
	Uint32 left;
	Uint32 top;
	Uint32 right;
	Uint32 bottom;
} Rect;

//Theora specific display information
typedef struct {
	int frameWidth;
	int frameHeight;
	int picWidth;
	int picHeight;
	int picOffsetX;
	int picOffsetY;
} ThoScaleInfo;

// VP8 specific display information
typedef struct {
	unsigned hScaleFactor   : 2;
	unsigned vScaleFactor   : 2;
	unsigned picWidth       : 14;
	unsigned picHeight      : 14;
} Vp8ScaleInfo;



typedef struct {
	int lowDelayEn;
	int numRows;
} LowDelayInfo;

typedef struct {
	int useBitEnable;
	int useIpEnable;
	int useDbkYEnable;
	int useDbkCEnable;
	int useOvlEnable;
	int useBtpEnable;
} SecAxiUse;

typedef struct CacheSizeCfg {
	unsigned BufferSize     : 8;
	unsigned PageSizeX      : 4;
	unsigned PageSizeY      : 4;
	unsigned CacheSizeX     : 4;
	unsigned CacheSizeY     : 4;
	unsigned Reserved       : 8;
} CacheSizeCfg;

typedef struct {
	union {
		Uint32 word;
		CacheSizeCfg cfg;
	} luma;
	union {
		Uint32 word;
		CacheSizeCfg cfg;
	} chroma;
	unsigned Bypass         : 1;
	unsigned DualConf       : 1;
	unsigned PageMerge      : 2;
} MaverickCacheConfig;

typedef struct {
	Uint32 * paraSet;
	int     size;
} DecParamSet;

typedef enum {
	WPROT_DEC_WORK,
	WPROT_DEC_PS_SAVE,
	WPROT_DEC_PIC_SAVE,
	WPROT_DEC_REPORT,
	WPROT_DEC_FRAME,
	WPROT_DEC_SEC_AXI,
	WPROT_DEC_MAX
} WriteMemProtectDecType;

typedef enum {
	WPROT_ENC_WORK,
	WPROT_ENC_BITSTREAM,
	WPROT_ENC_SCRATCH,
	WPROT_ENC_FRAME,
	WPROT_ENC_SEC_AXI,
	WPROT_ENC_MAX
} WriteMemProtectEncType;
typedef struct {
	int enable;
	int isSecondary;
	PhysicalAddress startAddress;
	PhysicalAddress endAddress;
} WriteMemProtectRegion;

typedef struct {
	WriteMemProtectRegion decRegion[WPROT_DEC_MAX];
	WriteMemProtectRegion encRegion[WPROT_ENC_MAX];
} WriteMemProtectCfg;

struct CodecInst;

//------------------------------------------------------------------------------
// decode struct and definition
//------------------------------------------------------------------------------

typedef struct CodecInst DecInst;
typedef DecInst * DecHandle;

typedef struct {
	int fixedFrameRateFlag;
	int timingInfoPresent;
	int chromaLocBotField;
	int chromaLocTopField;
	int chromaLocInfoPresent;
	int colorPrimaries;
	int colorDescPresent;
	int isExtSAR;
	int vidFullRange;
	int vidFormat;
	int vidSigTypePresent;
	int vuiParamPresent;
	int vuiPicStructPresent;
	int vuiPicStruct;
} AvcVuiInfo;

typedef struct {
	CodStd bitstreamFormat;
	PhysicalAddress bitstreamBuffer;
	int bitstreamBufferSize;
	int mp4DeblkEnable;	
	int avcExtension;
	int mp4Class;
	int tiled2LinearEnable;
    int wtlEnable;

	int cbcrInterleave;
	int cbcrOrder;	
	int bwbEnable;
	int frameEndian;
	int streamEndian;
	int bitstreamMode;
#ifdef FEAT_OUTPUT_FORMAT
	int outputFormat;
#endif
	Uint32 coreIdx;	
} DecOpenParam;

typedef struct {
	int picWidth;
	int picHeight;
	int fRateNumerator;
	int fRateDenominator;
	Rect picCropRect;
	int mp4DataPartitionEnable;
	int mp4ReversibleVlcEnable;
	int mp4ShortVideoHeader;
	int h263AnnexJEnable;
	int minFrameBufferCount;
	int frameBufDelay;
	int normalSliceSize;
	int worstSliceSize;

	// Report Information
	int profile;
	int level;
	int interlace;
	int constraint_set_flag[4];
	int direct8x8Flag;
	int vc1Psf;
    int avcIsExtSAR;
	int maxNumRefFrmFlag;
	int aspectRateInfo;
	int bitRate;
	ThoScaleInfo thoScaleInfo;
	Vp8ScaleInfo vp8ScaleInfo;
    int mp2LowDelay;
    int mp2DispVerSize;
    int mp2DispHorSize;

	int userDataNum;
	int userDataSize;
	int userDataBufFull;
	int seqInitErrReason;	// please refer to [Appendix D : ERROR REASONS IN DECODING SEQUENCE HEADERS chapter in programmers guide] to know the meaning of the value for detail.
	PhysicalAddress rdPtr;
	PhysicalAddress wrPtr;
	AvcVuiInfo avcVuiInfo;
} DecInitialInfo;


// Report Information

typedef struct {
	int iframeSearchEnable;
	int skipframeMode;
    union {
        int mp2PicFlush;
        int rvDbkMode;
    } DecStdParam;
} DecParam;

// Report Information
typedef struct {
	int userDataNum;        // User Data
	int userDataSize;
	int userDataBufFull;
    int activeFormat;
} DecOutputExtData;
// VP8 specific header information
typedef struct {
	unsigned showFrame      : 1;
	unsigned versionNumber  : 3;
	unsigned refIdxLast     : 8;
	unsigned refIdxAltr     : 8;
	unsigned refIdxGold     : 8;
} Vp8PicInfo;

// MVC specific picture information
typedef struct {
	int viewIdxDisplay;
	int viewIdxDecoded;
} MvcPicInfo;

// AVC specific SEI information (frame packing arrangement SEI)
typedef struct {
	unsigned exist;
	unsigned framePackingArrangementId;
	unsigned framePackingArrangementCancelFlag;
	unsigned quincunxSamplingFlag;
	unsigned spatialFlippingFlag;
	unsigned frame0FlippedFlag;
	unsigned fieldViewsFlag;
	unsigned currentFrameIsFrame0Flag;
	unsigned frame0SelfContainedFlag;
	unsigned frame1SelfContainedFlag;
	unsigned framePackingArrangementExtensionFlag;
	unsigned framePackingArrangementType;
	unsigned contentInterpretationType;
	unsigned frame0GridPositionX;
	unsigned frame0GridPositionY;
	unsigned frame1GridPositionX;
	unsigned frame1GridPositionY;
	unsigned framePackingArrangementRepetitionPeriod;
} AvcFpaSei;

// AVC specific HRD information 
typedef struct {
    int cpbMinus1;
    int vclHrdParamFlag;
    int nalHrdParamFlag;
} AvcHrdInfo;



typedef struct {
	int indexFrameDisplay;
	int indexFrameDecoded;
	int picType;
	int picTypeFirst;
	int numOfErrMBs;
	int notSufficientSliceBuffer;
	int notSufficientPsBuffer;
	int decodingSuccess;
	int interlacedFrame;
	int chunkReuseRequired;
    Rect rcDisplay;
	int dispPicWidth;
	int dispPicHeight;
    Rect rcDecoded;
	int decPicWidth;
	int decPicHeight;
	int aspectRateInfo;
	int fRateNumerator;
	int fRateDenominator;
	Vp8ScaleInfo vp8ScaleInfo;
	Vp8PicInfo vp8PicInfo;
	MvcPicInfo mvcPicInfo;
	AvcFpaSei avcFpaSei;
    AvcHrdInfo avcHrdInfo;
    AvcVuiInfo avcVuiInfo;
    int avcNpfFieldInfo;
	int avcPocPic;
	int avcPocTop;
	int avcPocBot;
	int wprotErrReason;
	PhysicalAddress wprotErrAddress;
	// Report Information
	int pictureStructure;
	int topFieldFirst;
	int repeatFirstField;
	int progressiveFrame;
	int fieldSequence;
    int frameDct;
	int nalRefIdc;
    int decFrameInfo;
    int picStrPresent;
	int picTimingStruct;
	int progressiveSequence;
	int mp4TimeIncrement;
	int mp4ModuloTimeBase;
	DecOutputExtData decOutputExtData;
	int consumedByte;
	int rdPtr;
	int wrPtr;
    int bytePosFrameStart;
    int bytePosFrameEnd;
	FrameBuffer dispFrame;
	int mp2DispVerSize;
	int mp2DispHorSize;
	int mp2NpfFieldInfo;
	int	frameDisplayFlag;	
	int sequenceChanged;
	int streamEndFlag;
	int frameCycle;
} DecOutputInfo;




//------------------------------------------------------------------------------
// encode struct and definition
//------------------------------------------------------------------------------

typedef struct CodecInst EncInst;
typedef EncInst * EncHandle;

typedef struct {
	int mp4DataPartitionEnable;
	int mp4ReversibleVlcEnable;
	int mp4IntraDcVlcThr;
	int mp4HecEnable;
	int mp4Verid;
} EncMp4Param;

typedef struct {
	int h263AnnexIEnable;
	int h263AnnexJEnable;
	int h263AnnexKEnable;
	int h263AnnexTEnable;
} EncH263Param;




typedef struct {
	int ppsId;
	int entropyCodingMode;  // 0 : CAVLC, 1 : CABAC, 2: CAVLC/CABAC select according to PicType
	int cabacInitIdc;
	int transform8x8Mode;   // 0 : disable(BP), 1 : enable(HP)
} AvcPpsParam;

typedef struct {
	int constrainedIntraPredFlag;
	int disableDeblk;
	int deblkFilterOffsetAlpha;
	int deblkFilterOffsetBeta;
	int chromaQpOffset;
	int audEnable;
	int frameCroppingFlag;
	int frameCropLeft;
	int frameCropRight;
	int frameCropTop;
	int frameCropBottom;

	int level;
} EncAvcParam;
typedef struct{
	int sliceMode;
	int sliceSizeMode;
	int sliceSize;
} EncSliceMode;



typedef struct {
	PhysicalAddress bitstreamBuffer;
	Uint32 bitstreamBufferSize;
	CodStd bitstreamFormat;
	int ringBufferEnable;
	int picWidth;
	int picHeight;
	int linear2TiledEnable;
	int frameRateInfo;
	int MESearchRange;
	int bitRate;
	int initialDelay;
	int vbvBufferSize;
	int enableAutoSkip;
	int gopSize;
	int idrInterval;
	int meBlkMode;
	EncSliceMode sliceMode;
	int intraRefresh;
    int ConscIntraRefreshEnable;
	int userQpMax;

	//h.264 only
    int maxIntraSize;
	int userMaxDeltaQp;
	int userQpMin;
	int userMinDeltaQp;
	int rcEnable;           // rate control enable (0:constant QP, 1:CBR, 2:ABR)
	int MEUseZeroPmv;       // 0: PMV_ENABLE, 1: PMV_DISABLE
	int intraCostWeight;    // Additional weight of Intra Cost for mode decision to reduce Intra MB density

	//mp4 only
	int rcIntraQp;
	int userGamma;
	int rcIntervalMode;     // 0:normal, 1:frame_level, 2:slice_level, 3: user defined Mb_level
	int mbInterval;         // use when RcintervalMode is 3

	union {
		EncMp4Param mp4Param;
		EncH263Param h263Param;
		EncAvcParam avcParam;
	} EncStdParam;


	// Maverick-II Cache Configuration
	int cacheBypass;
	int cacheDual;
	int cbcrInterleave;
	int cbcrOrder;
	int frameEndian;
	int streamEndian;
	int bwbEnable;
	int lineBufIntEn;


	Uint32 coreIdx;	
} EncOpenParam;


typedef struct {
	int minFrameBufferCount;
} EncInitialInfo;


typedef struct {
	FrameBuffer *sourceFrame;
	int forceIPicture;
	int skipPicture;
	int quantParam;
	PhysicalAddress picStreamBufferAddr;
	int picStreamBufferSize;
} EncParam;

typedef struct {
	PhysicalAddress bitstreamBuffer;
	Uint32 bitstreamSize;
	int bitstreamWrapAround;
	int picType;
	int numOfSlices;
	int reconFrameIndex;
	FrameBuffer reconFrame;
	int rdPtr;
	int wrPtr;
	int frameCycle;
} EncOutputInfo;

typedef struct {
	PhysicalAddress paraSet;
	int size;
} EncParamSet;


typedef struct {
	PhysicalAddress buf;
	BYTE *pBuf;
	int size;
	int headerType;
} EncHeaderParam;

typedef enum {
	VOL_HEADER,
	VOS_HEADER,
	VIS_HEADER
} Mp4HeaderType;

typedef enum {
	SPS_RBSP,
	PPS_RBSP,
	SPS_RBSP_MVC,
	PPS_RBSP_MVC,
} AvcHeaderType;


#ifdef __cplusplus
extern "C" {
#endif

	Uint32  VPU_IsBusy(
		Uint32 coreIdx
		);

	Uint32  VPU_WaitInterrupt(
		Uint32 coreIdx,
		int timeout
		);

	Uint32  VPU_IsInit(
		Uint32 coreIdx
		);

	RetCode VPU_Init(
		Uint32 coreIdx
		);

	RetCode VPU_InitWithBitcode(
		Uint32 coreIdx,
		const Uint16 *bitcode,
		Uint32 size
		);

	void    VPU_DeInit(
		Uint32 coreIdx
		);


	Uint32  VPU_GetOpenInstanceNum(
		Uint32 coreIdx
		);
	RetCode VPU_GetVersionInfo(
		Uint32 coreIdx,
		Uint32 *versionInfo,
		Uint32 *revision,
		Uint32 *productId
		);

	void    VPU_ClearInterrupt(
		Uint32 coreIdx
		);

	RetCode VPU_SWReset(
		Uint32 coreIdx,
		int resetMode,
		void *pendingInst
		);

	RetCode VPU_HWReset(
		Uint32 coreIdx
		);

	RetCode VPU_SleepWake(
		Uint32 coreIdx,
		int iSleepWake
		);	
	int VPU_GetMvColBufSize(
		CodStd codStd, 
		int width, 
		int height, 
		int num
		);
	int     VPU_GetFrameBufSize(
		int width,
		int height,
		int mapType,
		int format,
		DRAMConfig *pDramCfg
		);
	// function for decode
	RetCode VPU_DecOpen(
		DecHandle *pHandle,
		DecOpenParam *pop
		);

	RetCode VPU_DecClose(
		DecHandle handle
		);

	RetCode VPU_DecSetEscSeqInit(
		DecHandle handle, 
		int escape);

	RetCode VPU_DecGetInitialInfo(
		DecHandle handle,
		DecInitialInfo *info
		);

	RetCode VPU_DecIssueSeqInit(
		DecHandle handle
		);

	RetCode VPU_DecCompleteSeqInit(
		DecHandle handle,
		DecInitialInfo *info
		);
#if defined(SUPPORT_DEC_SLICE_BUFFER) || defined(SUPPORT_DEC_RESOLUTION_CHANGE)
RetCode VPU_DecRegisterFrameBuffer(
		DecHandle handle,
		FrameBuffer * bufArray,
		int num,
		int stride,
		int height,
		int mapType,
		DecBufInfo * pBufInfo);
#else
	RetCode VPU_DecRegisterFrameBuffer(
		DecHandle handle,
		FrameBuffer *bufArray,
		int num,
		int stride,
		int height,
		int mapType
		);
#endif
	RetCode VPU_DecGetFrameBuffer(
		DecHandle handle,
		int frameIdx,
		FrameBuffer *frameBuf
		);

	RetCode VPU_DecGetBitstreamBuffer(
		DecHandle handle,
		PhysicalAddress *prdPrt,
		PhysicalAddress *pwrPtr,
		int *size
		);
	RetCode VPU_DecUpdateBitstreamBuffer(
		DecHandle handle,
		int size
		);

	RetCode VPU_DecStartOneFrame(
		DecHandle handle,
		DecParam *param
		);

	RetCode VPU_DecGetOutputInfo(
		DecHandle handle,
		DecOutputInfo * info
		);

	RetCode VPU_DecFrameBufferFlush(
		DecHandle handle
		);
	RetCode VPU_DecBitBufferFlush(
		DecHandle handle
		);
	RetCode VPU_DecSetRdPtr(
		DecHandle handle,
		PhysicalAddress addr,
		int updateWrPtr
		);
	RetCode VPU_DecClrDispFlag(
		DecHandle handle,
		int index
		);

	RetCode VPU_DecGiveCommand(
		DecHandle handle,
		CodecCommand cmd,
		void * parameter
		);

	RetCode VPU_DecAllocateFrameBuffer(
		DecHandle handle,
		FrameBufferAllocInfo info,
		FrameBuffer *frameBuffer
		);
	// function for encode
	RetCode VPU_EncOpen(
		EncHandle *handle,
		EncOpenParam *encOpParam
		);

	RetCode VPU_EncClose(
		EncHandle handle
		);

	RetCode VPU_EncGetInitialInfo(
		EncHandle handle,
		EncInitialInfo *encInitInfo
		);

#ifdef SUPPORT_ENC_COARSE_ME
	RetCode VPU_EncRegisterFrameBuffer(
		EncHandle handle,
		FrameBuffer * bufArray,
		int num,
		int stride,
		int height,
		int mapType,
		PhysicalAddress subSampBaseA,
		PhysicalAddress subSampBaseB
		);
#else
	RetCode VPU_EncRegisterFrameBuffer(
		EncHandle handle,
		FrameBuffer * bufArray,
		int num,
		int stride,
		int height,
		int mapType
		);
#endif

	RetCode VPU_EncGetFrameBuffer(
		EncHandle handle,
		int frameIdx,
		FrameBuffer *frameBuf
		);

	RetCode VPU_EncGetBitstreamBuffer(
		EncHandle handle,
		PhysicalAddress *prdPrt,
		PhysicalAddress *pwrPtr,
		int * size
		);

	RetCode VPU_EncUpdateBitstreamBuffer(
		EncHandle handle,
		int size
		);

	RetCode VPU_EncStartOneFrame(
		EncHandle handle,
		EncParam * param
		);

	RetCode VPU_EncGetOutputInfo(
		EncHandle handle,
		EncOutputInfo * info
		);

	RetCode VPU_EncGiveCommand(
		EncHandle handle,
		CodecCommand cmd,
		void * parameter
		);


	RetCode VPU_EncAllocateFrameBuffer(
		EncHandle handle,
		FrameBufferAllocInfo info,
		FrameBuffer *frameBuffer
		);


#ifdef __cplusplus
}
#endif

#endif

