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
//		This file should be modified by some customers according to their SOC configuration.
//--=========================================================================--

#ifndef _VPU_CONFIG_H_
#define _VPU_CONFIG_H_

#include "../config.h"
#include "vputypes.h"



#define MAX_INST_HANDLE_SIZE	        (32*1024)

#define BODA7503_CODE       0x7503
#define CODA7542_CODE       0x7542
#define CODA851_CODE        0x8510
#define BODA950_CODE        0x9500
#define CODA960_CODE        0x9600
#define CODA980_CODE        0x9800
#define WAVE320_CODE        0x3200

#	define MAX_ENC_PIC_WIDTH        1920
#	define MAX_ENC_PIC_HEIGHT       1088
#	define MAX_ENC_AVC_PIC_WIDTH        1920
#	define MAX_ENC_AVC_PIC_HEIGHT       1088  
#	define MAX_DEC_PIC_WIDTH   1920
#	define MAX_DEC_PIC_HEIGHT  1088

// MAX_NUM_INSTANCE can set any value as many as memory size of system. the bellow setting is dedicated for C&M FPGA board.
//#define MAX_NUM_INSTANCE 4
#define MAX_NUM_INSTANCE 16
#define MAX_NUM_VPU_CORE 1

#if MAX_DEC_PIC_WIDTH > 1920		// 4K		

#define USE_WTL          0          // not support 4K WTL
#define MAX_EXTRA_FRAME_BUFFER_NUM      1
#define MAX_PP_SRC_NUM                  2   //PPU buffer(decoding case) or Source buffer(encoding case)

#define MAX_DPB_MBS      110400     // 32768 for level 5

#elif MAX_DEC_PIC_WIDTH > 720

#define USE_WTL         0           // for WTL (needs MAX_DPB_NUM*2 frame)
#define MAX_EXTRA_FRAME_BUFFER_NUM      4
#define MAX_PP_SRC_NUM                  2   //PPU buffer(decoding case) or Source buffer(encoding case)

#define MAX_DPB_MBS      32768      // 32768 for level 4.1

#else

#define USE_WTL          0          // for WTL (needs MAX_DPB_NUM*2 frame)
#define MAX_EXTRA_FRAME_BUFFER_NUM      4
#define MAX_PP_SRC_NUM                  2   //PPU buffer(decoding case) or Source buffer(encoding case)

#define MAX_DPB_MBS      32768      // 32768 for level 4.1

#endif


// codec specific configuration
#define VPU_REORDER_ENABLE  1   // it can be set to 1 to handle reordering DPB in host side.
#define VPU_GMC_PROCESS_METHOD  1
#define VPU_AVC_X264_SUPPORT 1

// DRAM configuration for TileMap access
#ifdef CNM_FPGA_PLATFORM
#define EM_RAS 13
#define EM_BANK  2
#define EM_CAS   9
#define EM_WIDTH 3
#else
#define EM_RAS 13
#define EM_BANK  3
#define EM_CAS   10
#define EM_WIDTH 2
#endif


//  Application specific configuration
#define VPU_BUSY_CHECK_TIMEOUT       5000

#ifdef CNM_FPGA_PLATFORM
#define VPU_FRAME_ENDIAN      VDI_BIG_ENDIAN
#define VPU_STREAM_ENDIAN     VDI_BIG_ENDIAN
#else
#define VPU_FRAME_ENDIAN      VDI_LITTLE_ENDIAN
#define VPU_STREAM_ENDIAN     VDI_LITTLE_ENDIAN
#endif
#define VPU_USER_DATA_ENDIAN  VDI_LITTLE_ENDIAN

//#define CBCR_INTERLEAVE			1 //[default 1 for BW checking with CnMViedo Conformance] 0 (chroma separate mode), 1 (chroma interleave mode) // if the type of tiledmap uses the kind of MB_RASTER_MAP. must set to enable CBCR_INTERLEAVE
#define CBCR_INTERLEAVE                       0
#define VPU_ENABLE_BWB			1 
//#define VPU_ENABLE_BWB                        0
#define VPU_REPORT_USERDATA		0
//#define VPU_REPORT_USERDATA             1

#if 0
#define USE_BIT_INTERNAL_BUF	1
#define USE_IP_INTERNAL_BUF		1
#define USE_DBKY_INTERNAL_BUF	1
#define USE_DBKC_INTERNAL_BUF	1
#define USE_OVL_INTERNAL_BUF	1
#define USE_BTP_INTERNAL_BUF	1
#define USE_ME_INTERNAL_BUF		1
#endif
#define USE_BIT_INTERNAL_BUF    0 
#define USE_IP_INTERNAL_BUF             0
#define USE_DBKY_INTERNAL_BUF   0 
#define USE_DBKC_INTERNAL_BUF   0 
#define USE_OVL_INTERNAL_BUF    0 
#define USE_BTP_INTERNAL_BUF    0
#define USE_ME_INTERNAL_BUF             0

#define STREAM_FULL_EMPTY_CHECK_DISABLE 0

#define VPU_GBU_SIZE    1024	//No modification required
#define JPU_GBU_SIZE	512		//No modification required

//********************************************//
//      External Memory Map Table - No modification required
//********************************************//


#	define CODE_BUF_SIZE      (160*1024)
#   define MINI_PIPPEN_SCALER_COEFFCIENT_ARRAY_SIZE  256   /** the real size is 152 bytes, please refer to: ScalerCoeff_c */
	#	define WORK_BUF_SIZE      (512*1024) + (MAX_NUM_INSTANCE*48*1024) + (MAX_DEC_PIC_WIDTH*MAX_DEC_PIC_HEIGHT*3/4)


#define PARA_BUF_SIZE     ( 10 * 1024 )
//#define CODE_BUF_SIZE                   (152*1024)
//#define WORK_BUF_SIZE                   (512*1024) + (MAX_NUM_INSTANCE*41*1024)
                
//=====1. VPU COMMON MEMORY ======================//
#define SIZE_COMMON                     (CODE_BUF_SIZE + WORK_BUF_SIZE + PARA_BUF_SIZE)

//=====2. VPU BITSTREAM MEMORY ===================//
#define MAX_STREAM_BUF_SIZE             0x300000  // max bitstream size

//===== 3. VPU PPS Save Size ===================//
//----- SPS/PPS save buffer --------------------//
#define PS_SAVE_SIZE          (320*1024)
//----- VP8 MB save buffer -------------------//
#define VP8_MB_SAVE_SIZE                (17*4*(MAX_DEC_PIC_WIDTH*MAX_DEC_PIC_HEIGHT/256))    // MB information + split MVs)*4*MbNumbyte
//----- slice save buffer --------------------//
#define SLICE_SAVE_SIZE                 (MAX_DEC_PIC_WIDTH*MAX_DEC_PIC_HEIGHT*3/4)          // this buffer for ASO/FMO


//=====4. VPU REPORT MEMORY  ======================//
#define SIZE_REPORT_BUF                 (0x34000)

//=====5. VPU Encoder Scratch buffer ssize ======================//
#define SIZE_AVCENC_QMAT_TABLE          ((16*6)+(64*2)) // 4x4 6, 8x8 2
#define SIZE_MP4ENC_DATA_PARTITION      (MAX_ENC_PIC_WIDTH*MAX_ENC_PIC_HEIGHT*3/4)

//=====6. Check VPU required memory size =======================//
#define MAX_FRM_SIZE                    ((MAX_DEC_PIC_WIDTH*MAX_DEC_PIC_HEIGHT*3)/2)
#define MAX_DEC_FRAME_BUFFERING         (MAX_DPB_MBS/((MAX_DEC_PIC_WIDTH*MAX_DEC_PIC_HEIGHT)/256))
#define MAX_DPB_NUM                     (16 < MAX_DEC_FRAME_BUFFERING ? (16+2+MAX_EXTRA_FRAME_BUFFER_NUM) : (MAX_DEC_FRAME_BUFFERING+2+MAX_EXTRA_FRAME_BUFFER_NUM)) //  (+2 for current and display_delay)
#define MAX_DPB_SIZE                    (((MAX_FRM_SIZE+0x3fff)&(~0x3fff))*MAX_DPB_NUM)    // frame buffer for codec (MAX_DPB_NUM)
#define MAX_MV_COL_SIZE                 (((MAX_FRM_SIZE+4)/5)*MAX_DPB_NUM)
#define MAX_PP_SRC_SIZE					(((MAX_FRM_SIZE+0x3fff)&(~0x3fff))*MAX_PP_SRC_NUM)

#define CODEC_FRAME_BASE                ((((MAX_STREAM_BUF_SIZE*MAX_NUM_INSTANCE)) + 0xff) & (~0xff))
#define REQUIRED_VPU_MEMORY_SIZE		(CODEC_FRAME_BASE+((MAX_DPB_SIZE*(1+USE_WTL))+MAX_MV_COL_SIZE+MAX_PP_SRC_SIZE)*MAX_NUM_INSTANCE)


#endif  /* _VPU_CONFIG_H_ */

