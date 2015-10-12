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
#include "../config.h"

#ifndef REGDEFINE_H_INCLUDED
#define REGDEFINE_H_INCLUDED

//------------------------------------------------------------------------------
// REGISTER BASE
//------------------------------------------------------------------------------
#define BIT_BASE            0x0000
#define GDMA_BASE          	0x1000
#define MBC_BASE            0x0400
#define ME_BASE				0x0600
#define DBK_BASE            0x0800
#define MC_BASE				0x0C00
#define DMAC_BASE			0x2000

#define BW_BASE             0x03000000
//------------------------------------------------------------------------------
// HARDWARE REGISTER
//------------------------------------------------------------------------------
#define BIT_CODE_RUN                (BIT_BASE + 0x000)
#define BIT_CODE_DOWN               (BIT_BASE + 0x004)
#define BIT_INT_REQ                 (BIT_BASE + 0x008)
#define BIT_INT_CLEAR               (BIT_BASE + 0x00C)
#define BIT_INT_STS					(BIT_BASE + 0x010)
#define BIT_CODE_RESET				(BIT_BASE + 0x014)
#define BIT_CUR_PC                  (BIT_BASE + 0x018)
#define BIT_SW_RESET				(BIT_BASE + 0x024)
#define BIT_SW_RESET_STATUS			(BIT_BASE + 0x034)

//------------------------------------------------------------------------------
// GLOBAL REGISTER
//------------------------------------------------------------------------------
#define BIT_CODE_BUF_ADDR           (BIT_BASE + 0x100)
#define BIT_WORK_BUF_ADDR           (BIT_BASE + 0x104)
#define BIT_PARA_BUF_ADDR           (BIT_BASE + 0x108)
#define BIT_BIT_STREAM_CTRL         (BIT_BASE + 0x10C)
#define BIT_FRAME_MEM_CTRL          (BIT_BASE + 0x110)
#define	BIT_BIT_STREAM_PARAM		(BIT_BASE + 0x114)

#define BIT_RD_PTR					(BIT_BASE + 0x120)
#define BIT_WR_PTR					(BIT_BASE + 0x124)


#define BIT_AXI_SRAM_USE            (BIT_BASE + 0x140)
#define BIT_BYTE_POS_FRAME_START    (BIT_BASE + 0x144)
#define BIT_BYTE_POS_FRAME_END		(BIT_BASE + 0x148)
#define BIT_FRAME_CYCLE             (BIT_BASE + 0x14C)

#define	BIT_FRM_DIS_FLG				(BIT_BASE + 0x150)

#define BIT_BUSY_FLAG               (BIT_BASE + 0x160)
#define BIT_RUN_COMMAND             (BIT_BASE + 0x164)
#define BIT_RUN_INDEX               (BIT_BASE + 0x168)
#define BIT_RUN_COD_STD             (BIT_BASE + 0x16C)
#define BIT_INT_ENABLE              (BIT_BASE + 0x170)
#define BIT_INT_REASON              (BIT_BASE + 0x174)
#define BIT_RUN_AUX_STD             (BIT_BASE + 0x178)

// MSG REGISTER ADDRESS changed
#define BIT_MSG_0                   (BIT_BASE + 0x130)
#define BIT_MSG_1                   (BIT_BASE + 0x134)
#define BIT_MSG_2                   (BIT_BASE + 0x138)
#define BIT_MSG_3                   (BIT_BASE + 0x13C)

#define MBC_BUSY                	(MBC_BASE + 0x040)
#define MC_BUSY                 	(MC_BASE + 0x004)



//------------------------------------------------------------------------------
// [ENC SEQ INIT] COMMAND
//------------------------------------------------------------------------------
#define CMD_ENC_SEQ_BB_START        (BIT_BASE + 0x180)
#define CMD_ENC_SEQ_BB_SIZE         (BIT_BASE + 0x184)
#define CMD_ENC_SEQ_OPTION          (BIT_BASE + 0x188)          // HecEnable,ConstIntraQp, FMO, QPREP, AUD, SLICE, MB BIT
#define CMD_ENC_SEQ_COD_STD         (BIT_BASE + 0x18C)
#define CMD_ENC_SEQ_SRC_SIZE        (BIT_BASE + 0x190)
#define CMD_ENC_SEQ_SRC_F_RATE      (BIT_BASE + 0x194)
#define CMD_ENC_SEQ_MP4_PARA        (BIT_BASE + 0x198)
#define CMD_ENC_SEQ_263_PARA        (BIT_BASE + 0x19C)
#define CMD_ENC_SEQ_264_PARA        (BIT_BASE + 0x1A0)
#define CMD_ENC_SEQ_SLICE_MODE      (BIT_BASE + 0x1A4)
#define CMD_ENC_SEQ_GOP_NUM         (BIT_BASE + 0x1A8)
#define CMD_ENC_SEQ_RC_PARA         (BIT_BASE + 0x1AC)
#define CMD_ENC_SEQ_RC_BUF_SIZE     (BIT_BASE + 0x1B0)
#define CMD_ENC_SEQ_INTRA_REFRESH   (BIT_BASE + 0x1B4)
#define CMD_ENC_SEQ_INTRA_QP		(BIT_BASE + 0x1C4)
#define CMD_ENC_SEQ_RC_QP_MAX			(BIT_BASE + 0x1C8)		
#define CMD_ENC_SEQ_RC_GAMMA			(BIT_BASE + 0x1CC)		
#define CMD_ENC_SEQ_RC_INTERVAL_MODE	(BIT_BASE + 0x1D0)		// mbInterval[32:2], rcIntervalMode[1:0]
#define CMD_ENC_SEQ_INTRA_WEIGHT		(BIT_BASE + 0x1D4)
#define CMD_ENC_SEQ_ME_OPTION			(BIT_BASE + 0x1D8)
#define CMD_ENC_SEQ_RC_PARA2			(BIT_BASE + 0x1DC)
#define CMD_ENC_SEQ_QP_RANGE_SET		(BIT_BASE + 0x1E0)
#define CMD_ENC_SEQ_RC_MAX_INTRA_SIZE   (BIT_BASE + 0x1F0)

#define CMD_ENC_SEQ_FIRST_MBA           (BIT_BASE + 0x1E4)
#define CMD_ENC_SEQ_HEIGHT_IN_MAP_UNITS (BIT_BASE + 0x1E8)
#define CMD_ENC_SEQ_OVERLAP_CLIP_SIZE   (BIT_BASE + 0x1EC)

//------------------------------------------------------------------------------
// [ENC SEQ END] COMMAND
//------------------------------------------------------------------------------
#define RET_ENC_SEQ_END_SUCCESS         (BIT_BASE + 0x1C0)


//------------------------------------------------------------------------------
// [DEC][MINI-PIPPEN] COMMAND
//------------------------------------------------------------------------------
#define	CMD_DEC_PIC_SCL_TGT_SIZE		(BIT_BASE + 0x1B8)
#define	CMD_DEC_PIC_SCL_ADDR_Y  		(BIT_BASE + 0x128)
#define	CMD_DEC_PIC_SCL_ADDR_CB 		(BIT_BASE + 0x12C)
#define	CMD_DEC_PIC_SCL_ADDR_CR 		(BIT_BASE + 0x154)


//------------------------------------------------------------------------------
// [ENC PIC RUN] COMMAND
//------------------------------------------------------------------------------

#define CMD_ENC_PIC_SRC_INDEX       (BIT_BASE + 0x180)
#define CMD_ENC_PIC_SRC_STRIDE      (BIT_BASE + 0x184)
#define CMD_ENC_PIC_SRC_ADDR_Y		(BIT_BASE + 0x1A8)
#define CMD_ENC_PIC_SRC_ADDR_CB		(BIT_BASE + 0x1AC)
#define CMD_ENC_PIC_SRC_ADDR_CR		(BIT_BASE + 0x1B0)
#define CMD_ENC_PIC_QS              (BIT_BASE + 0x18C)
#define CMD_ENC_PIC_ROT_MODE        (BIT_BASE + 0x190)
#define CMD_ENC_PIC_OPTION          (BIT_BASE + 0x194)
#define CMD_ENC_PIC_BB_START        (BIT_BASE + 0x198)
#define CMD_ENC_PIC_BB_SIZE        	(BIT_BASE + 0x19C)
#define CMD_ENC_PIC_PARA_BASE_ADDR	(BIT_BASE + 0x1A0)
#define CMD_ENC_PIC_SUB_FRAME_SYNC  (BIT_BASE + 0x1A4)



#define RET_ENC_PIC_FRAME_NUM       (BIT_BASE + 0x1C0)
#define RET_ENC_PIC_TYPE            (BIT_BASE + 0x1C4)
#define RET_ENC_PIC_FRAME_IDX       (BIT_BASE + 0x1C8)
#define RET_ENC_PIC_SLICE_NUM       (BIT_BASE + 0x1CC)
#define RET_ENC_PIC_FLAG			(BIT_BASE + 0x1D0)
#define RET_ENC_PIC_SUCCESS         (BIT_BASE + 0x1D8)

//------------------------------------------------------------------------------
// [ENC ROI INIT] COMMAND
//------------------------------------------------------------------------------
#define CMD_ENC_ROI_MODE            (BIT_BASE + 0x180)      // [1]: RoiType, [0]: RoiEn
#define CMD_ENC_ROI_NUM             (BIT_BASE + 0x184)      // [3:0] RoiNum

#define CMD_ENC_ROI_POS_0           (BIT_BASE + 0x188)      // [31:24]: MbyEnd, [23:16]: MbyStart, [15:8]: MbxEnd, [7:0]: MbxStart
#define CMD_ENC_ROI_QP_0            (BIT_BASE + 0x18C)      // [05:04]: RoiQp
#define CMD_ENC_ROI_POS_1           (BIT_BASE + 0x190)
#define CMD_ENC_ROI_QP_1            (BIT_BASE + 0x194)
#define CMD_ENC_ROI_POS_2           (BIT_BASE + 0x198)
#define CMD_ENC_ROI_QP_2            (BIT_BASE + 0x19C)
#define CMD_ENC_ROI_POS_3           (BIT_BASE + 0x1A0)
#define CMD_ENC_ROI_QP_3            (BIT_BASE + 0x1A4)
#define CMD_ENC_ROI_POS_4           (BIT_BASE + 0x1A8)
#define CMD_ENC_ROI_QP_4            (BIT_BASE + 0x1AC)
#define CMD_ENC_ROI_POS_5           (BIT_BASE + 0x1B0)
#define CMD_ENC_ROI_QP_5            (BIT_BASE + 0x1B4)
#define CMD_ENC_ROI_POS_6           (BIT_BASE + 0x1B8)
#define CMD_ENC_ROI_QP_6            (BIT_BASE + 0x1BC)
#define CMD_ENC_ROI_POS_7           (BIT_BASE + 0x1C0)
#define CMD_ENC_ROI_QP_7            (BIT_BASE + 0x1C4)
#define CMD_ENC_ROI_POS_8           (BIT_BASE + 0x1C8)
#define CMD_ENC_ROI_QP_8            (BIT_BASE + 0x1CC)
#define CMD_ENC_ROI_POS_9           (BIT_BASE + 0x1D0)
#define CMD_ENC_ROI_QP_9            (BIT_BASE + 0x1D4)

#define RET_ENC_ROI_SUCCESS         (BIT_BASE + 0x1D8)


//------------------------------------------------------------------------------
// [ENC SET FRAME BUF] COMMAND
//------------------------------------------------------------------------------
#define CMD_SET_FRAME_SUBSAMP_A    (BIT_BASE + 0x188)
#define CMD_SET_FRAME_SUBSAMP_B    (BIT_BASE + 0x18C)

#define CMD_SET_FRAME_DP_BUF_BASE  (BIT_BASE + 0x1B0)
#define CMD_SET_FRAME_DP_BUF_SIZE  (BIT_BASE + 0x1B4)

//------------------------------------------------------------------------------
// [ENC HEADER] COMMAND
//------------------------------------------------------------------------------
#define CMD_ENC_HEADER_CODE         (BIT_BASE + 0x180)
#define CMD_ENC_HEADER_BB_START     (BIT_BASE + 0x184)
#define CMD_ENC_HEADER_BB_SIZE      (BIT_BASE + 0x188)
#define CMD_ENC_HEADER_FRAME_CROP_H (BIT_BASE + 0x18C)
#define CMD_ENC_HEADER_FRAME_CROP_V (BIT_BASE + 0x190)


#define RET_ENC_HEADER_SUCCESS      (BIT_BASE + 0x1C0)

//------------------------------------------------------------------------------
// [ENC_PARA_SET] COMMAND
//------------------------------------------------------------------------------
#define CMD_ENC_PARA_SET_TYPE       (BIT_BASE + 0x180)
#define RET_ENC_PARA_SET_SIZE       (BIT_BASE + 0x1c0)
#define RET_ENC_PARA_SET_SUCCESS    (BIT_BASE + 0x1C4)

//------------------------------------------------------------------------------
// [ENC PARA CHANGE] COMMAND :
//------------------------------------------------------------------------------
#define CMD_ENC_SEQ_PARA_CHANGE_ENABLE	(BIT_BASE + 0x180)		// FrameRateEn[3], BitRateEn[2], IntraQpEn[1], GopEn[0]
#define CMD_ENC_SEQ_PARA_RC_GOP			(BIT_BASE + 0x184)
#define CMD_ENC_SEQ_PARA_RC_INTRA_QP	(BIT_BASE + 0x188)      
#define CMD_ENC_SEQ_PARA_RC_BITRATE     (BIT_BASE + 0x18C)
#define CMD_ENC_SEQ_PARA_RC_FRAME_RATE  (BIT_BASE + 0x190)
#define	CMD_ENC_SEQ_PARA_INTRA_MB_NUM	(BIT_BASE + 0x194)		// update param
#define CMD_ENC_SEQ_PARA_SLICE_MODE		(BIT_BASE + 0x198)		// update param
#define CMD_ENC_SEQ_PARA_HEC_MODE		(BIT_BASE + 0x19C)		// update param
#define CMD_ENC_SEQ_PARA_CABAC_MODE		(BIT_BASE + 0x1A0)		// entropyCodingMode==2
#define CMD_ENC_SEQ_PARA_PPS_ID  		(BIT_BASE + 0x1B4)		// entropyCodingMode==2

#define RET_ENC_SEQ_PARA_CHANGE_SECCESS	(BIT_BASE + 0x1C0)

//------------------------------------------------------------------------------
// [DEC SEQ INIT] COMMAND
//------------------------------------------------------------------------------
#define CMD_DEC_SEQ_BB_START        (BIT_BASE + 0x180)
#define CMD_DEC_SEQ_BB_SIZE         (BIT_BASE + 0x184)
#define CMD_DEC_SEQ_OPTION          (BIT_BASE + 0x188)
#define CMD_DEC_SEQ_SRC_SIZE        (BIT_BASE + 0x18C)
#define CMD_DEC_SEQ_START_BYTE		(BIT_BASE + 0x190)

#define CMD_DEC_SEQ_PS_BB_START     (BIT_BASE + 0x194)
#define CMD_DEC_SEQ_PS_BB_SIZE      (BIT_BASE + 0x198)

#define CMD_DEC_SEQ_MP4_ASP_CLASS   (BIT_BASE + 0x19C)
#define CMD_DEC_SEQ_VC1_STREAM_FMT  (BIT_BASE + 0x19C)
#define CMD_DEC_SEQ_X264_MV_EN		(BIT_BASE + 0x19C)
#define CMD_DEC_SEQ_SPP_CHUNK_SIZE  (BIT_BASE + 0x1A0)

// For MPEG2 only
#define CMD_DEC_SEQ_USER_DATA_OPTION		(BIT_BASE + 0x194)
#define CMD_DEC_SEQ_USER_DATA_BASE_ADDR		(BIT_BASE + 0x1AC)
#define CMD_DEC_SEQ_USER_DATA_BUF_SIZE		(BIT_BASE + 0x1B0)	


#define CMD_DEC_SEQ_INIT_ESCAPE		(BIT_BASE + 0x114)


#define RET_DEC_SEQ_BIT_RATE		(BIT_BASE + 0x1B4)
#define RET_DEC_SEQ_EXT_INFO        (BIT_BASE + 0x1B8)
#define RET_DEC_SEQ_SUCCESS         (BIT_BASE + 0x1C0)
#define RET_DEC_SEQ_SRC_SIZE        (BIT_BASE + 0x1C4)
#define RET_DEC_SEQ_ASPECT          (BIT_BASE + 0x1C8)
#define RET_DEC_SEQ_FRAME_NEED      (BIT_BASE + 0x1CC)
#define RET_DEC_SEQ_FRAME_DELAY     (BIT_BASE + 0x1D0)
#define RET_DEC_SEQ_INFO            (BIT_BASE + 0x1D4)
#define RET_DEC_SEQ_VP8_SCALE_INFO  (BIT_BASE + 0x1D4)

#define RET_DEC_SEQ_CROP_LEFT_RIGHT (BIT_BASE + 0x1D8)
#define RET_DEC_SEQ_CROP_TOP_BOTTOM (BIT_BASE + 0x1DC)
#define RET_DEC_SEQ_SEQ_ERR_REASON  (BIT_BASE + 0x1E0)

#define RET_DEC_SEQ_FRATE_NR        (BIT_BASE + 0x1E4)
#define RET_DEC_SEQ_FRATE_DR        (BIT_BASE + 0x1E8)
#define RET_DEC_SEQ_HEADER_REPORT   (BIT_BASE + 0x1EC)
#define RET_DEC_SEQ_VUI_INFO		(BIT_BASE + 0x18C)
#define RET_DEC_SEQ_VUI_PIC_STRUCT		(BIT_BASE + 0x1A8)




//------------------------------------------------------------------------------
// [DEC SEQ END] COMMAND
//------------------------------------------------------------------------------
#define RET_DEC_SEQ_END_SUCCESS     (BIT_BASE + 0x1C0)

//------------------------------------------------------------------------------
// [DEC PIC RUN] COMMAND
//----------------------------------------------------
#define CMD_DEC_PIC_ROT_MODE        (BIT_BASE + 0x180)
#define CMD_DEC_PIC_ROT_INDEX       (BIT_BASE + 0x184)
#define CMD_DEC_PIC_ROT_ADDR_Y      (BIT_BASE + 0x188)
#define CMD_DEC_PIC_ROT_ADDR_CB     (BIT_BASE + 0x18C)
#define CMD_DEC_PIC_ROT_ADDR_CR     (BIT_BASE + 0x190)
#define CMD_DEC_PIC_ROT_STRIDE      (BIT_BASE + 0x1B8)


#define CMD_DEC_PIC_OPTION			(BIT_BASE + 0x194)

#define	CMD_DEC_PIC_CHUNK_SIZE		(BIT_BASE + 0x19C)
#define	CMD_DEC_PIC_BB_START		(BIT_BASE + 0x1A0)
#define CMD_DEC_PIC_START_BYTE		(BIT_BASE + 0x1A4)

#define CMD_DEC_PIC_USER_DATA_BASE_ADDR		(BIT_BASE + 0x1AC)
#define CMD_DEC_PIC_USER_DATA_BUF_SIZE		(BIT_BASE + 0x1B0)

#define CMD_DEC_PIC_NUM_ROWS		        (BIT_BASE + 0x1B4)
#define CMD_DEC_PIC_THO_PIC_PARA            (BIT_BASE + 0x198)
#define CMD_DEC_PIC_THO_QMAT_ADDR           (BIT_BASE + 0x1A0)
#define CMD_DEC_PIC_THO_MB_PARA_ADDR        (BIT_BASE + 0x1A4)
#define RET_DEC_PIC_VUI_PIC_STRUCT			(BIT_BASE + 0x1A8)
#define RET_DEC_PIC_AVC_FPA_SEI0			(BIT_BASE + 0x19C)
#define RET_DEC_PIC_AVC_FPA_SEI1			(BIT_BASE + 0x1A0)
#define RET_DEC_PIC_AVC_FPA_SEI2			(BIT_BASE + 0x1A4)
#define RET_DEC_NUM_MB_ROWS					(BIT_BASE + 0x1B4) // it will be update
#define RET_DEC_PIC_HRD_INFO				(BIT_BASE + 0x1B8)
#define RET_DEC_PIC_SIZE					(BIT_BASE + 0x1BC)
#define RET_DEC_PIC_FRAME_NUM				(BIT_BASE + 0x1C0)
#define RET_DEC_PIC_FRAME_IDX				(BIT_BASE + 0x1C4)
#define RET_DEC_PIC_DISPLAY_IDX				(BIT_BASE + 0x1C4)
#define RET_DEC_PIC_ERR_MB					(BIT_BASE + 0x1C8)
#define RET_DEC_PIC_TYPE					(BIT_BASE + 0x1CC)
#define RET_DEC_PIC_POST					(BIT_BASE + 0x1D0) // for VC1
#define RET_DEC_PIC_MVC_REPORT				(BIT_BASE + 0x1D0) // for MVC
#define RET_DEC_PIC_OPTION					(BIT_BASE + 0x1D4)
#define RET_DEC_PIC_SUCCESS					(BIT_BASE + 0x1D8)
#define RET_DEC_PIC_CUR_IDX					(BIT_BASE + 0x1DC)
#define RET_DEC_PIC_DECODED_IDX				(BIT_BASE + 0x1DC)
#define	RET_DEC_PIC_CROP_LEFT_RIGHT			(BIT_BASE + 0x1E0) // for AVC, MPEG-2
#define RET_DEC_PIC_CROP_TOP_BOTTOM			(BIT_BASE + 0x1E4) // for AVC, MPEG-2
#define	RET_DEC_PIC_MODULO_TIME_BASE		(BIT_BASE + 0x1E0) // for MP4
#define RET_DEC_PIC_VOP_TIME_INCREMENT		(BIT_BASE + 0x1E4) // for MP4
#define RET_DEC_PIC_RV_TR					(BIT_BASE + 0x1E8)
#define RET_DEC_PIC_VP8_PIC_REPORT			(BIT_BASE + 0x1E8)
#define RET_DEC_PIC_ATSC_USER_DATA_INFO		(BIT_BASE + 0x1E8) // H.264, MEPEG2
#define RET_DEC_PIC_VUI_INFO				(BIT_BASE + 0x1EC)
#define RET_DEC_PIC_ASPECT					(BIT_BASE + 0x1F0)
#define RET_DEC_PIC_VP8_SCALE_INFO			(BIT_BASE + 0x1F0)
#define RET_DEC_PIC_FRATE_NR				(BIT_BASE + 0x1F4)
#define RET_DEC_PIC_FRATE_DR				(BIT_BASE + 0x1F8)
#define RET_DEC_PIC_POC_TOP					(BIT_BASE + 0x1AC)
#define RET_DEC_PIC_POC_BOT					(BIT_BASE + 0x1B0)
#define RET_DEC_PIC_POC  					(BIT_BASE + 0x1B0)
#define RET_DEC_PIC_SEQ_EXT_INFO			(BIT_BASE + 0x1D0) // for MP2

//------------------------------------------------------------------------------
// [DEC SET FRAME BUF] COMMAND
//------------------------------------------------------------------------------
#define CMD_SET_FRAME_BUF_NUM       (BIT_BASE + 0x180)
#define CMD_SET_FRAME_BUF_STRIDE    (BIT_BASE + 0x184)

#define CMD_SET_FRAME_SLICE_BB_START	(BIT_BASE + 0x188)
#define CMD_SET_FRAME_SLICE_BB_SIZE		(BIT_BASE + 0x18C)
#define CMD_SET_FRAME_AXI_BIT_ADDR		(BIT_BASE + 0x190)
#define CMD_SET_FRAME_AXI_IPACDC_ADDR	(BIT_BASE + 0x194)
#define CMD_SET_FRAME_AXI_DBKY_ADDR		(BIT_BASE + 0x198)
#define CMD_SET_FRAME_AXI_DBKC_ADDR		(BIT_BASE + 0x19C)
#define CMD_SET_FRAME_AXI_OVL_ADDR		(BIT_BASE + 0x1A0)
#define CMD_SET_FRAME_AXI_BTP_ADDR		(BIT_BASE + 0x1A4)

#define CMD_SET_FRAME_CACHE_SIZE        (BIT_BASE + 0x1A8)
#define CMD_SET_FRAME_CACHE_CONFIG      (BIT_BASE + 0x1AC)
#define CMD_SET_FRAME_MB_BUF_BASE       (BIT_BASE + 0x1B0)

#define CMD_SET_FRAME_MAX_DEC_SIZE		(BIT_BASE + 0x1A8)
#define CMD_SET_FRAME_DELAY     		(BIT_BASE + 0x1BC)

#define RET_SET_FRAME_SUCCESS           (BIT_BASE + 0x1C0)

//------------------------------------------------------------------------------
// [DEC_PARA_SET] COMMAND
//------------------------------------------------------------------------------
#define CMD_DEC_PARA_SET_TYPE       (BIT_BASE + 0x180)
#define CMD_DEC_PARA_SET_SIZE       (BIT_BASE + 0x184)

#define RET_DEC_PARA_SET_SUCCESS        (BIT_BASE + 0x1C0)

//------------------------------------------------------------------------------
// [DEC_BUF_FLUSH] COMMAND
//------------------------------------------------------------------------------
#define RET_DEC_BUF_FLUSH_SUCCESS       (BIT_BASE + 0x1C0)

//------------------------------------------------------------------------------
// [SLEEP/WAKE] COMMAND
//------------------------------------------------------------------------------
#define RET_SLEEP_WAKE_SUCCESS          (BIT_BASE + 0x1C0)


//------------------------------------------------------------------------------
// [SET PIC INFO] COMMAND
//------------------------------------------------------------------------------
#define GDI_PRI_RD_PRIO_L   		(GDMA_BASE + 0x000)
#define GDI_PRI_RD_PRIO_H   		(GDMA_BASE + 0x004)
#define GDI_PRI_WR_PRIO_L   		(GDMA_BASE + 0x008)
#define GDI_PRI_WR_PRIO_H   		(GDMA_BASE + 0x00c)
#define GDI_PRI_RD_LOCK_CNT 		(GDMA_BASE + 0x010)
#define GDI_PRI_WR_LOCK_CNT 		(GDMA_BASE + 0x014)
#define GDI_SEC_RD_PRIO_L   		(GDMA_BASE + 0x018)
#define GDI_SEC_RD_PRIO_H   		(GDMA_BASE + 0x01c)
#define GDI_SEC_WR_PRIO_L   		(GDMA_BASE + 0x020)
#define GDI_SEC_WR_PRIO_H   		(GDMA_BASE + 0x024)
#define GDI_SEC_RD_LOCK_CNT 		(GDMA_BASE + 0x028)
#define GDI_SEC_WR_LOCK_CNT 		(GDMA_BASE + 0x02c)
#define GDI_SEC_CLIENT_EN   		(GDMA_BASE + 0x030)
#define GDI_CONTROL                 (GDMA_BASE + 0x034)
#define GDI_PIC_INIT_HOST           (GDMA_BASE + 0x038)

#define GDI_HW_VERINFO            	(GDMA_BASE + 0x050)
#define GDI_PINFO_REQ               (GDMA_BASE + 0x060)
#define GDI_PINFO_ACK               (GDMA_BASE + 0x064)
#define GDI_PINFO_ADDR              (GDMA_BASE + 0x068)
#define GDI_PINFO_DATA              (GDMA_BASE + 0x06c)
#define GDI_BWB_ENABLE              (GDMA_BASE + 0x070)
#define GDI_BWB_SIZE                (GDMA_BASE + 0x074)
#define GDI_BWB_STD_STRUCT          (GDMA_BASE + 0x078)
#define GDI_BWB_STATUS              (GDMA_BASE + 0x07c)

#define GDI_STATUS					(GDMA_BASE + 0x080)

#define GDI_DEBUG_0                 (GDMA_BASE + 0x084)
#define GDI_DEBUG_1                 (GDMA_BASE + 0x088)
#define GDI_DEBUG_2                 (GDMA_BASE + 0x08c)
#define GDI_DEBUG_3                 (GDMA_BASE + 0x090)
#define GDI_DEBUG_PROBE_ADDR        (GDMA_BASE + 0x094)
#define GDI_DEBUG_PROBE_DATA        (GDMA_BASE + 0x098)



// write protect
#define GDI_WPROT_ERR_CLR			(GDMA_BASE + 0x0A0)
#define GDI_WPROT_ERR_RSN			(GDMA_BASE + 0x0A4)
#define GDI_WPROT_ERR_ADR			(GDMA_BASE + 0x0A8)
#define GDI_WPROT_RGN_EN			(GDMA_BASE + 0x0AC)
#define GDI_WPROT_RGN0_STA			(GDMA_BASE + 0x0B0)
#define GDI_WPROT_RGN0_END			(GDMA_BASE + 0x0B4)
#define GDI_WPROT_RGN1_STA			(GDMA_BASE + 0x0B8)
#define GDI_WPROT_RGN1_END			(GDMA_BASE + 0x0BC)
#define GDI_WPROT_RGN2_STA			(GDMA_BASE + 0x0C0)
#define GDI_WPROT_RGN2_END			(GDMA_BASE + 0x0C4)
#define GDI_WPROT_RGN3_STA			(GDMA_BASE + 0x0C8)
#define GDI_WPROT_RGN3_END			(GDMA_BASE + 0x0CC)	
#define GDI_WPROT_RGN4_STA			(GDMA_BASE + 0x0D0)
#define GDI_WPROT_RGN4_END			(GDMA_BASE + 0x0D4)
#define GDI_WPROT_RGN5_STA			(GDMA_BASE + 0x0D8)
#define GDI_WPROT_RGN5_END			(GDMA_BASE + 0x0DC)

#define GDI_BUS_CTRL				(GDMA_BASE + 0x0F0)
#define GDI_BUS_STATUS				(GDMA_BASE + 0x0F4)



#define GDI_SIZE_ERR_FLAG           (GDMA_BASE + 0x0e0)
#define GDI_ADR_RQ_SIZE_ERR_PRI0	(GDMA_BASE + 0x100)
#define GDI_ADR_RQ_SIZE_ERR_PRI1	(GDMA_BASE + 0x104)
#define GDI_ADR_RQ_SIZE_ERR_PRI1	(GDMA_BASE + 0x104)
#define GDI_ADR_RQ_SIZE_ERR_PRI2	(GDMA_BASE + 0x108)
#define GDI_ADR_WQ_SIZE_ERR_PRI0	(GDMA_BASE + 0x10c)
#define GDI_ADR_WQ_SIZE_ERR_PRI1	(GDMA_BASE + 0x110)
#define GDI_ADR_WQ_SIZE_ERR_PRI2	(GDMA_BASE + 0x114)

#define GDI_ADR_RQ_SIZE_ERR_SEC0	(GDMA_BASE + 0x118)
#define GDI_ADR_RQ_SIZE_ERR_SEC1	(GDMA_BASE + 0x11c)
#define GDI_ADR_RQ_SIZE_ERR_SEC2	(GDMA_BASE + 0x120)

#define GDI_ADR_WQ_SIZE_ERR_SEC0	(GDMA_BASE + 0x124)
#define GDI_ADR_WQ_SIZE_ERR_SEC1	(GDMA_BASE + 0x128)
#define GDI_ADR_WQ_SIZE_ERR_SEC2	(GDMA_BASE + 0x12c)

#define GDI_INFO_CONTROL			(GDMA_BASE + 0x400)
#define GDI_INFO_PIC_SIZE			(GDMA_BASE + 0x404)
#define GDI_INFO_BASE_Y				(GDMA_BASE + 0x408)
#define GDI_INFO_BASE_CB			(GDMA_BASE + 0x40C)
#define GDI_INFO_BASE_CR			(GDMA_BASE + 0x410)

#define GDI_XY2_CAS_0				(GDMA_BASE + 0x800)
#define GDI_XY2_CAS_F				(GDMA_BASE + 0x83C)

#define GDI_XY2_BA_0				(GDMA_BASE + 0x840)
#define GDI_XY2_BA_1				(GDMA_BASE + 0x844)
#define GDI_XY2_BA_2				(GDMA_BASE + 0x848)
#define GDI_XY2_BA_3				(GDMA_BASE + 0x84C)

#define GDI_XY2_RAS_0				(GDMA_BASE + 0x850)
#define GDI_XY2_RAS_F				(GDMA_BASE + 0x88C)

#define GDI_XY2_RBC_CONFIG			(GDMA_BASE + 0x890)
#define GDI_RBC2_AXI_0				(GDMA_BASE + 0x8A0)
#define GDI_RBC2_AXI_1F				(GDMA_BASE + 0x91C)
#define GDI_TILEDBUF_BASE			(GDMA_BASE + 0x920)

//------------------------------------------------------------------------------
// Product, Reconfiguration Information
//------------------------------------------------------------------------------
#define DBG_CONFIG_REPORT_0         (GDMA_BASE + 0x040)    //product name and version
#define DBG_CONFIG_REPORT_1         (GDMA_BASE + 0x044)    //interface configuration, hardware definition
#define DBG_CONFIG_REPORT_2         (GDMA_BASE + 0x048)    //standard definition
#define DBG_CONFIG_REPORT_3         (GDMA_BASE + 0x04C)    //standard detail definition
#define DBG_CONFIG_REPORT_4         (GDMA_BASE + 0x050)    //definition in cnm_define
#define DBG_CONFIG_REPORT_5         (GDMA_BASE + 0x054)
#define DBG_CONFIG_REPORT_6         (GDMA_BASE + 0x058)
#define DBG_CONFIG_REPORT_7         (GDMA_BASE + 0x05C)

//------------------------------------------------------------------------------
// MEMORY COPY MODULE REGISTER
//------------------------------------------------------------------------------
#define	ADDR_DMAC_PIC_RUN			(DMAC_BASE+0x000)
#define	ADDR_DMAC_PIC_STATUS		(DMAC_BASE+0x004)
#define	ADDR_DMAC_PIC_OP_MODE		(DMAC_BASE+0x008)
#define	ADDR_DMAC_ID				(DMAC_BASE+0x00c)	//the result muse be 0x4d435059
                                    
#define	ADDR_DMAC_SRC_BASE_Y		(DMAC_BASE+0x010)
#define	ADDR_DMAC_SRC_BASE_CB		(DMAC_BASE+0x014)
#define	ADDR_DMAC_SRC_BASE_CR		(DMAC_BASE+0x018)
#define	ADDR_DMAC_SRC_STRIDE		(DMAC_BASE+0x01c)
                                    
#define	ADDR_DMAC_DST_BASE_Y		(DMAC_BASE+0x020)
#define	ADDR_DMAC_DST_BASE_CB		(DMAC_BASE+0x024)
#define	ADDR_DMAC_DST_BASE_CR		(DMAC_BASE+0x028)
#define	ADDR_DMAC_DST_STRIDE		(DMAC_BASE+0x02c)
                                    
#define	ADDR_DMAC_SRC_MB_POS_X		(DMAC_BASE+0x030)
#define	ADDR_DMAC_SRC_MB_POS_Y		(DMAC_BASE+0x034)
#define	ADDR_DMAC_SRC_MB_BLK_X		(DMAC_BASE+0x038)
#define	ADDR_DMAC_SRC_MB_BLK_Y		(DMAC_BASE+0x03c)
                                    
#define	ADDR_DMAC_DST_MB_POS_X		(DMAC_BASE+0x040)
#define	ADDR_DMAC_DST_MB_POS_Y		(DMAC_BASE+0x044)
#define	ADDR_DMAC_DST_MB_BLK_X		(DMAC_BASE+0x048)
#define	ADDR_DMAC_DST_MB_BLK_Y		(DMAC_BASE+0x04c)
                                    
#define	ADDR_DMAC_SET_COLOR_Y		(DMAC_BASE+0x050)
#define	ADDR_DMAC_SET_COLOR_CB		(DMAC_BASE+0x054)
#define	ADDR_DMAC_SET_COLOR_CR		(DMAC_BASE+0x058)
                                    
#define	ADDR_DMAC_SUB_SAMPLE_X		(DMAC_BASE+0x060)
#define	ADDR_DMAC_SUB_SAMPLE_Y		(DMAC_BASE+0x064)



//------------------------------------------------------------------------------
// DMAC
//------------------------------------------------------------------------------
#define DMAC_DMAC_RUN		    (DMAC_BASE + 0x00)
#define DMAC_SOFT_RESET		    (DMAC_BASE + 0x04)
#define DMAC_DMAC_MODE		    (DMAC_BASE + 0x08)
#define DMAC_DESC_ADDR		    (DMAC_BASE + 0x0c)
#define DMAC_DESC0				(DMAC_BASE + 0x10)
#define DMAC_DESC1				(DMAC_BASE + 0x14)
#define DMAC_DESC2				(DMAC_BASE + 0x18)
#define DMAC_DESC3				(DMAC_BASE + 0x1c)
#define DMAC_DESC4				(DMAC_BASE + 0x20)
#define DMAC_DESC5				(DMAC_BASE + 0x24)
#define DMAC_DESC6				(DMAC_BASE + 0x28)
#define DMAC_DESC7				(DMAC_BASE + 0x2c)


//------------------------------------------------------------------------------
// [FIRMWARE VERSION] COMMAND  
// [32:16] project number => 
// [16:0]  version => xxxx.xxxx.xxxxxxxx 
//------------------------------------------------------------------------------
#define RET_FW_VER_NUM					(BIT_BASE + 0x1c0)
#define RET_FW_CODE_REV             (BIT_BASE + 0x1c4)


#endif

