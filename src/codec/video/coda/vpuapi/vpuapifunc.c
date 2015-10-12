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

#include "vpuapifunc.h"
#include "regdefine.h"



#ifndef MIN
#define MIN(a, b)       (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b)       (((a) > (b)) ? (a) : (b))
#endif
/******************************************************************************
    define value
******************************************************************************/

/******************************************************************************
    Codec Instance Slot Management
******************************************************************************/


RetCode InitCodecInstancePool(Uint32 coreIdx)
{
	int i;
	CodecInst * pCodecInst;
	vpu_instance_pool_t *vip;

	vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
	if (!vip)
		return RETCODE_INSUFFICIENT_RESOURCE;

	if (vip->instance_pool_inited==0)
	{
		for( i = 0; i < MAX_NUM_INSTANCE; i++)
		{
			pCodecInst = (CodecInst *)vip->codecInstPool[i];
			pCodecInst->instIndex = i;
			pCodecInst->inUse = 0;
		}
		vip->instance_pool_inited = 1;	
	}	
	return RETCODE_SUCCESS;
}
 
/*
 * GetCodecInstance() obtains a instance.
 * It stores a pointer to the allocated instance in *ppInst
 * and returns RETCODE_SUCCESS on success.
 * Failure results in 0(null pointer) in *ppInst and RETCODE_FAILURE.
 */

RetCode GetCodecInstance(Uint32 coreIdx, CodecInst ** ppInst)
{
	int i;
	CodecInst * pCodecInst = 0;
	vpu_instance_pool_t *vip;
	vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
	if (!vip)
		return RETCODE_INSUFFICIENT_RESOURCE;

	for (i = 0; i < MAX_NUM_INSTANCE; i++) {
		pCodecInst = (CodecInst *)vip->codecInstPool[i];

		if (!pCodecInst) {
			return RETCODE_FAILURE;
		}

		if (!pCodecInst->inUse) {
			break;
		}
	}

	if (i == MAX_NUM_INSTANCE) {
		*ppInst = 0;
		return RETCODE_INSUFFICIENT_RESOURCE;
	}

	pCodecInst->inUse = 1;
	pCodecInst->coreIdx = coreIdx;
	osal_memset((void*)&pCodecInst->CodecInfo, 0x00, sizeof(pCodecInst->CodecInfo));
	*ppInst = pCodecInst;

	if (vdi_open_instance(pCodecInst->coreIdx, pCodecInst->instIndex) < 0)
		return RETCODE_FAILURE;
	

	return RETCODE_SUCCESS;
}

void FreeCodecInstance(CodecInst * pCodecInst)
{

	pCodecInst->inUse = 0;
    pCodecInst->codecMode    = -1;
    pCodecInst->codecModeAux = -1;
   
	vdi_close_instance(pCodecInst->coreIdx, pCodecInst->instIndex);

}

RetCode CheckInstanceValidity(CodecInst * pCodecInst)
{
	int i;
	vpu_instance_pool_t *vip;

	vip = (vpu_instance_pool_t *)vdi_get_instance_pool(pCodecInst->coreIdx);
	if (!vip)
		return RETCODE_INSUFFICIENT_RESOURCE;

	for (i = 0; i < MAX_NUM_INSTANCE; i++) {
		if ((CodecInst *)vip->codecInstPool[i] == pCodecInst)
			return RETCODE_SUCCESS;
	}
	return RETCODE_INVALID_HANDLE;
}



/******************************************************************************
    API Subroutines
******************************************************************************/
RetCode BitLoadFirmware(Uint32 coreIdx, PhysicalAddress codeBase, const Uint16 *codeWord, int codeSize)
{
	int i;
	Uint32 data;

	BYTE code[8];


	for (i=0; i<codeSize; i+=4) {
		// 2byte little endian variable to 1byte big endian buffer
		code[0] = (BYTE)(codeWord[i+0]>>8);
		code[1] = (BYTE)codeWord[i+0];
		code[2] = (BYTE)(codeWord[i+1]>>8);
		code[3] = (BYTE)codeWord[i+1];
		code[4] = (BYTE)(codeWord[i+2]>>8);
		code[5] = (BYTE)codeWord[i+2];
		code[6] = (BYTE)(codeWord[i+3]>>8);
		code[7] = (BYTE)codeWord[i+3];
		VpuWriteMem(coreIdx, codeBase+i*2, (BYTE *)code, 8, VDI_BIG_ENDIAN);
	}

	VpuWriteReg(coreIdx, BIT_INT_ENABLE, 0);
	VpuWriteReg(coreIdx, BIT_CODE_RUN, 0);

	for (i=0; i<2048; ++i) {
		data = codeWord[i];
		VpuWriteReg(coreIdx, BIT_CODE_DOWN, (i << 16) | data);
	}

	vdi_set_bit_firmware_to_pm(coreIdx, codeWord); 

	return RETCODE_SUCCESS;
}



void BitIssueCommand(Uint32 coreIdx, CodecInst *inst, int cmd)
{
	int instIdx = 0;
	int cdcMode = 0;
	int auxMode = 0;
	

	
	if (inst != NULL) // command is specific to instance
	{
		instIdx = inst->instIndex;
		cdcMode = inst->codecMode;
		auxMode = inst->codecModeAux;
	}



	if (inst) {
		if (inst->codecMode < AVC_ENC)	{
		}
		else {
		}
	}	

	VpuWriteReg(coreIdx, BIT_BUSY_FLAG, 1);
	VpuWriteReg(coreIdx, BIT_RUN_INDEX, instIdx);
	VpuWriteReg(coreIdx, BIT_RUN_COD_STD, cdcMode);
	VpuWriteReg(coreIdx, BIT_RUN_AUX_STD, auxMode);
	if (inst && inst->loggingEnable)
		vdi_log(coreIdx, cmd, 1);
	VpuWriteReg(coreIdx, BIT_RUN_COMMAND, cmd);
}

RetCode CheckDecInstanceValidity(DecHandle handle)
{
	CodecInst * pCodecInst;
	RetCode ret;

	pCodecInst = handle;
	ret = CheckInstanceValidity(pCodecInst);
	if (ret != RETCODE_SUCCESS) {
		return RETCODE_INVALID_HANDLE;
	}
	if (!pCodecInst->inUse) {
		return RETCODE_INVALID_HANDLE;
	}
	if (pCodecInst->codecMode != AVC_DEC && 
		pCodecInst->codecMode != VC1_DEC &&
		pCodecInst->codecMode != MP2_DEC &&
		pCodecInst->codecMode != MP4_DEC &&
		pCodecInst->codecMode != DV3_DEC &&
		pCodecInst->codecMode != RV_DEC &&
		pCodecInst->codecMode != AVS_DEC &&
        pCodecInst->codecMode != VPX_DEC
        ) {


		return RETCODE_INVALID_HANDLE;
	}

	return RETCODE_SUCCESS;
}

RetCode CheckDecOpenParam(DecOpenParam * pop)
{
	if (pop == 0) {
		return RETCODE_INVALID_PARAM;
	}
	if (pop->bitstreamBuffer % 512) {
		return RETCODE_INVALID_PARAM;
	}

	if (pop->bitstreamBufferSize % 1024 ||
			pop->bitstreamBufferSize < 1024 ||
			pop->bitstreamBufferSize > (256*1024*1024-1) ) {
		return RETCODE_INVALID_PARAM;
	}

	if (pop->bitstreamFormat != STD_AVC
			&& pop->bitstreamFormat != STD_VC1
			&& pop->bitstreamFormat != STD_MPEG2
			&& pop->bitstreamFormat != STD_H263
			&& pop->bitstreamFormat != STD_MPEG4
			&& pop->bitstreamFormat != STD_DIV3
			&& pop->bitstreamFormat != STD_RV
            && pop->bitstreamFormat != STD_AVS
            && pop->bitstreamFormat != STD_THO
            && pop->bitstreamFormat != STD_VP3
			&& pop->bitstreamFormat != STD_VP8
	) {
		return RETCODE_INVALID_PARAM;
	}
	
	if( pop->mp4DeblkEnable == 1 && !(pop->bitstreamFormat == STD_MPEG4 || pop->bitstreamFormat == STD_MPEG2 || pop->bitstreamFormat == STD_DIV3)) {
		return RETCODE_INVALID_PARAM;
	}	

	if (pop->coreIdx > MAX_VPU_CORE_NUM) {
		return RETCODE_INVALID_PARAM;
	}

	return RETCODE_SUCCESS;
}

void  ConfigDecWPROTRegion(int coreIdx, DecInfo * pDecInfo)
{
	PhysicalAddress tempBuffer;
	vpu_instance_pool_t *vip;
	WriteMemProtectCfg *pCgf       = &pDecInfo->writeMemProtectCfg;	
	vpu_buffer_t	   *pvbWorkPs  = &pDecInfo->vbPs;
	

	vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
	if (!vip)
		return;
		
	tempBuffer = vip->vpu_common_buffer.phys_addr + CODE_BUF_SIZE;
	pCgf->decRegion[WPROT_DEC_WORK].enable = 1;
	pCgf->decRegion[WPROT_DEC_WORK].isSecondary = 0;
	pCgf->decRegion[WPROT_DEC_WORK].startAddress = tempBuffer;
	pCgf->decRegion[WPROT_DEC_WORK].endAddress = tempBuffer + WORK_BUF_SIZE + PARA_BUF_SIZE;

	if (pvbWorkPs->size)
	{
		pCgf->decRegion[WPROT_DEC_PS_SAVE].enable = 1;
		pCgf->decRegion[WPROT_DEC_PS_SAVE].isSecondary = 0;
		pCgf->decRegion[WPROT_DEC_PS_SAVE].startAddress = pvbWorkPs->phys_addr;
		pCgf->decRegion[WPROT_DEC_PS_SAVE].endAddress = pvbWorkPs->phys_addr + pvbWorkPs->size;

	}

}

void ConfigEncWPROTRegion(int coreIdx, EncInfo * pEncInfo, WriteMemProtectCfg *pCgf)
{
	int i;
	vpu_buffer_t *pvbScratch;
	PhysicalAddress tempBuffer;
	vpu_instance_pool_t *vip;

	// clear all 6 regions
	for (i=0; i<WPROT_ENC_MAX; i++)
	{
		pCgf->decRegion[i].enable = 0;
		pCgf->decRegion[i].isSecondary = 0;
	}

	
	pvbScratch = &pEncInfo->vbScratch;
	vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
	if (!vip)
		return;

	tempBuffer = vip->vpu_common_buffer.phys_addr + CODE_BUF_SIZE;

	pCgf->encRegion[WPROT_ENC_WORK].enable = 1;
	pCgf->encRegion[WPROT_ENC_WORK].isSecondary = 0;
	pCgf->encRegion[WPROT_ENC_WORK].startAddress =  tempBuffer;
	pCgf->encRegion[WPROT_ENC_WORK].endAddress = tempBuffer + WORK_BUF_SIZE + PARA_BUF_SIZE;
	if (pvbScratch->size)
	{
		pCgf->encRegion[WPROT_ENC_SCRATCH].enable = 1;
		pCgf->encRegion[WPROT_ENC_SCRATCH].isSecondary = 0;
		pCgf->encRegion[WPROT_ENC_SCRATCH].startAddress = pvbScratch->phys_addr;
		pCgf->encRegion[WPROT_ENC_SCRATCH].endAddress =  pvbScratch->phys_addr + pvbScratch->size;

	}
}

int DecBitstreamBufEmpty(DecInfo * pDecInfo)
{
	return (pDecInfo->streamRdPtr == pDecInfo->streamWrPtr);	
}

RetCode SetParaSet(DecHandle handle, int paraSetType, DecParamSet * para)
{
	CodecInst * pCodecInst;
	PhysicalAddress paraBuffer;
	int i;
	Uint32 * src;
	
	
	pCodecInst = handle;
	src = para->paraSet;

	EnterLock(pCodecInst->coreIdx);

	paraBuffer = VpuReadReg(pCodecInst->coreIdx, BIT_PARA_BUF_ADDR);
	for (i = 0; i < para->size; i += 4) {
		VpuWriteReg(pCodecInst->coreIdx, paraBuffer + i, *src++);
	}
	VpuWriteReg(pCodecInst->coreIdx, CMD_DEC_PARA_SET_TYPE, paraSetType); // 0: SPS, 1: PPS
	VpuWriteReg(pCodecInst->coreIdx, CMD_DEC_PARA_SET_SIZE, para->size);

	BitIssueCommand(pCodecInst->coreIdx, pCodecInst, DEC_PARA_SET);
	if (vdi_wait_vpu_busy(pCodecInst->coreIdx, VPU_BUSY_CHECK_TIMEOUT, BIT_BUSY_FLAG) == -1) {
		if (pCodecInst->loggingEnable)
			vdi_log(pCodecInst->coreIdx, DEC_PARA_SET, 2);
		LeaveLock(pCodecInst->coreIdx);
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}
	if (pCodecInst->loggingEnable)
		vdi_log(pCodecInst->coreIdx, DEC_PARA_SET, 0);
	
	LeaveLock(pCodecInst->coreIdx);
	return RETCODE_SUCCESS;
}

void DecSetHostParaAddr(Uint32 coreIdx, PhysicalAddress baseAddr, PhysicalAddress paraBuffer)
{
	BYTE tempBuf[8]={0,};					// 64bit bus & endian
	Uint32 val;
	
	val = paraBuffer;
	tempBuf[0] = 0;
	tempBuf[1] = 0;
	tempBuf[2] = 0;
	tempBuf[3] = 0;
	tempBuf[4] = (val >> 24) & 0xff;
	tempBuf[5] = (val >> 16) & 0xff;
	tempBuf[6] = (val >> 8) & 0xff;
	tempBuf[7] = (val >> 0) & 0xff;				
	VpuWriteMem(coreIdx, baseAddr, (BYTE *)tempBuf, 8, VDI_BIG_ENDIAN);				
}


RetCode CheckEncInstanceValidity(EncHandle handle)
{
	CodecInst * pCodecInst;
	RetCode ret;

	pCodecInst = handle;
	ret = CheckInstanceValidity(pCodecInst);
	if (ret != RETCODE_SUCCESS) {
		return RETCODE_INVALID_HANDLE;
	}
	if (!pCodecInst->inUse) {
		return RETCODE_INVALID_HANDLE;
	}

	if (pCodecInst->codecMode != MP4_ENC && 
		pCodecInst->codecMode != AVC_ENC) {
		return RETCODE_INVALID_HANDLE;
	}
	return RETCODE_SUCCESS;
}



RetCode CheckEncOpenParam(EncOpenParam * pop)
{
	int picWidth;
	int picHeight;

	if (pop == 0) {
		return RETCODE_INVALID_PARAM;
	}
	picWidth = pop->picWidth;
	picHeight = pop->picHeight;

	if (pop->bitstreamBuffer % 8) { 
		return RETCODE_INVALID_PARAM;
	}
	if (pop->bitstreamBufferSize % 1024 ||
			pop->bitstreamBufferSize < 1024 ||
			pop->bitstreamBufferSize > (256*1024*1024-1)) {
		return RETCODE_INVALID_PARAM;
	}

	if (pop->bitstreamFormat != STD_MPEG4 &&
			pop->bitstreamFormat != STD_H263 &&
			pop->bitstreamFormat != STD_AVC) {
		return RETCODE_INVALID_PARAM;
	}
	if (pop->frameRateInfo == 0) {
		return RETCODE_INVALID_PARAM;
	}
	{
		if (pop->bitRate > 32767 || pop->bitRate < 0) {
			return RETCODE_INVALID_PARAM;
		}
	}
	if (pop->bitRate !=0 && pop->initialDelay > 32767) {
		return RETCODE_INVALID_PARAM;
	}
	if (pop->bitRate !=0 && pop->initialDelay != 0 && pop->vbvBufferSize < 0) {
		return RETCODE_INVALID_PARAM;
	}
	if (pop->enableAutoSkip != 0 && pop->enableAutoSkip != 1) {
		return RETCODE_INVALID_PARAM;
	}


	if (pop->sliceMode.sliceMode != 0 && pop->sliceMode.sliceMode != 1) {
		return RETCODE_INVALID_PARAM;
	}
	if (pop->sliceMode.sliceMode == 1) {
		if (pop->sliceMode.sliceSizeMode != 0 && pop->sliceMode.sliceSizeMode != 1) {
			return RETCODE_INVALID_PARAM;
		}
		if (pop->sliceMode.sliceSizeMode == 1 && pop->sliceMode.sliceSize == 0 ) {
			return RETCODE_INVALID_PARAM;
		}
	}
	if (pop->intraRefresh < 0) {
		return RETCODE_INVALID_PARAM;
	}

	if (pop->MEUseZeroPmv != 0 && pop->MEUseZeroPmv != 1) {
		return RETCODE_INVALID_PARAM;
	}
	if (pop->intraCostWeight < 0 || pop->intraCostWeight >= 65535) {
		return RETCODE_INVALID_PARAM;
	}

	if (pop->MESearchRange < 0 || pop->MESearchRange >= 4) {
		return RETCODE_INVALID_PARAM;
	}
	if (pop->bitstreamFormat == STD_MPEG4) {
		EncMp4Param * param = &pop->EncStdParam.mp4Param;
		if (param->mp4DataPartitionEnable != 0 && param->mp4DataPartitionEnable != 1) {
			return RETCODE_INVALID_PARAM;
		}
		if (param->mp4DataPartitionEnable == 1) {
			if (param->mp4ReversibleVlcEnable != 0 && param->mp4ReversibleVlcEnable != 1) {
				return RETCODE_INVALID_PARAM;
			}
		}
		if (param->mp4IntraDcVlcThr < 0 || 7 < param->mp4IntraDcVlcThr) {
			return RETCODE_INVALID_PARAM;
		}

		if (picWidth < 96 || picWidth > MAX_ENC_PIC_WIDTH ) {
			return RETCODE_INVALID_PARAM;
		}

		if (picHeight < 16 || picHeight > MAX_ENC_PIC_HEIGHT ) {
			return RETCODE_INVALID_PARAM;
		}
	}
	else if (pop->bitstreamFormat == STD_H263) {
		EncH263Param * param = &pop->EncStdParam.h263Param;
		Uint32 frameRateInc, frameRateRes;
		if (param->h263AnnexJEnable != 0 && param->h263AnnexJEnable != 1) {
			return RETCODE_INVALID_PARAM;
		}
		if (param->h263AnnexKEnable != 0 && param->h263AnnexKEnable != 1) {
			return RETCODE_INVALID_PARAM;
		}
		if (param->h263AnnexTEnable != 0 && param->h263AnnexTEnable != 1) {
			return RETCODE_INVALID_PARAM;
		}

		if (picWidth < 64 || picWidth > MAX_ENC_PIC_WIDTH ) {
			return RETCODE_INVALID_PARAM;
		}
		if (picHeight < 16 || picHeight > MAX_ENC_PIC_HEIGHT ) {
			return RETCODE_INVALID_PARAM;
		}
		frameRateInc = ((pop->frameRateInfo>>16) &0xFFFF) + 1;
		frameRateRes = pop->frameRateInfo & 0xFFFF;
		if( (frameRateRes/frameRateInc) <15 )
			return RETCODE_INVALID_PARAM;

	}
	else if (pop->bitstreamFormat == STD_AVC) {
		EncAvcParam * param = &pop->EncStdParam.avcParam;
		if (param->constrainedIntraPredFlag != 0 && param->constrainedIntraPredFlag != 1) {
			return RETCODE_INVALID_PARAM;
		}
		if (param->disableDeblk != 0 && param->disableDeblk != 1 && param->disableDeblk != 2) {
			return RETCODE_INVALID_PARAM;
		}
		if (param->deblkFilterOffsetAlpha < -6 || 6 < param->deblkFilterOffsetAlpha) {
			return RETCODE_INVALID_PARAM;
		}
		if (param->deblkFilterOffsetBeta < -6 || 6 < param->deblkFilterOffsetBeta) {
			return RETCODE_INVALID_PARAM;
		}
		if (param->chromaQpOffset < -12 || 12 < param->chromaQpOffset) {
			return RETCODE_INVALID_PARAM;
		}
		if (param->audEnable != 0 && param->audEnable != 1) {
			return RETCODE_INVALID_PARAM;
		}
		if (param->frameCroppingFlag != 0 &&param->frameCroppingFlag != 1) {
			return RETCODE_INVALID_PARAM;
		}
		if (param->frameCropLeft & 0x01 ||
			param->frameCropRight & 0x01 ||
			param->frameCropTop & 0x01 ||
			param->frameCropBottom & 0x01) {
			return RETCODE_INVALID_PARAM;
		}
		if (picWidth < 64 || picWidth > MAX_ENC_PIC_WIDTH ) {
			return RETCODE_INVALID_PARAM;
		}
		if (picHeight < 16 || picHeight > MAX_ENC_PIC_HEIGHT ) {
			return RETCODE_INVALID_PARAM;
		}
	}

	if (pop->coreIdx > MAX_VPU_CORE_NUM) {
		return RETCODE_INVALID_PARAM;
	}
	return RETCODE_SUCCESS;
}


RetCode CheckEncParam(EncHandle handle, EncParam * param)
{
	CodecInst *pCodecInst;
	EncInfo *pEncInfo;
	int i;

	pCodecInst = handle;
	pEncInfo = &pCodecInst->CodecInfo.encInfo;

	if (param == 0) {
		return RETCODE_INVALID_PARAM;
	}
	if (param->skipPicture != 0 && param->skipPicture != 1) {
		return RETCODE_INVALID_PARAM;
	}
	if (param->skipPicture == 0) {
		if (param->sourceFrame == 0) {
			return RETCODE_INVALID_FRAME_BUFFER;
		}
		if (param->forceIPicture != 0 && param->forceIPicture != 1) {
			return RETCODE_INVALID_PARAM;
		}
	}
	if (pEncInfo->openParam.bitRate == 0) { // no rate control
		if (pCodecInst->codecMode == MP4_ENC) {
			if (param->quantParam < 1 || param->quantParam > 31) {
				return RETCODE_INVALID_PARAM;
			}
		}
		else { // AVC_ENC
			if (param->quantParam < 0 || param->quantParam > 51) {
				return RETCODE_INVALID_PARAM;
			}
		}
	}
	if (pEncInfo->ringBufferEnable == 0) {
		if (param->picStreamBufferAddr % 8 || param->picStreamBufferSize == 0) {
			return RETCODE_INVALID_PARAM;
		}
	}
	for (i=0; i<pEncInfo->numFrameBuffers; i++) {
		if (pEncInfo->frameBufPool[i].myIndex == param->sourceFrame->myIndex) {
			return RETCODE_INVALID_PARAM;
		}
	}	
	return RETCODE_SUCCESS;
}


/**
 * GetEncHeader() 
 *  1. Generate encoder header bitstream
 * @param handle         : encoder handle
 * @param encHeaderParam : encoder header parameter (buffer, size, type)
 * @return none
 */
RetCode GetEncHeader(EncHandle handle, EncHeaderParam * encHeaderParam)
{

	CodecInst * pCodecInst;
	EncInfo * pEncInfo;
	EncOpenParam *encOP;
	PhysicalAddress rdPtr;
	PhysicalAddress wrPtr;		
	int flag=0;
	int val = 0;

	pCodecInst = handle;
	pEncInfo = &pCodecInst->CodecInfo.encInfo;
	encOP = &(pEncInfo->openParam);

	EnterLock(pCodecInst->coreIdx);

	if (pEncInfo->ringBufferEnable == 0) {	

		if (pEncInfo->lineBufIntEn)
			val |= (0x1<<6);
		val |= (0x1<<5);
		val |= (0x1<<4);

	} 
	else {
		val |= (0x1<<3);
	}
	val |= pEncInfo->openParam.streamEndian;
	VpuWriteReg(pCodecInst->coreIdx, BIT_BIT_STREAM_CTRL, val);

	if (pEncInfo->ringBufferEnable == 0) {
		VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_HEADER_BB_START, encHeaderParam->buf);
		VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_HEADER_BB_SIZE, encHeaderParam->size/1024);
	}
	
	if ((encHeaderParam->headerType == SPS_RBSP || encHeaderParam->headerType == SPS_RBSP_MVC) && 
        pEncInfo->openParam.bitstreamFormat == STD_AVC) {
		Uint32 CropV, CropH;
		if (encOP->EncStdParam.avcParam.frameCroppingFlag == 1) {
			flag = 1;
			CropH = encOP->EncStdParam.avcParam.frameCropLeft << 16;
			CropH |= encOP->EncStdParam.avcParam.frameCropRight;
			CropV = encOP->EncStdParam.avcParam.frameCropTop << 16;
			CropV |= encOP->EncStdParam.avcParam.frameCropBottom;
			VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_HEADER_FRAME_CROP_H, CropH);
			VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_HEADER_FRAME_CROP_V, CropV);
		}
	}
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_HEADER_CODE, encHeaderParam->headerType | (flag << 3)); // 0: SPS, 1: PPS

	VpuWriteReg(pCodecInst->coreIdx, pEncInfo->streamRdPtrRegAddr, pEncInfo->streamRdPtr);
	VpuWriteReg(pCodecInst->coreIdx, pEncInfo->streamWrPtrRegAddr, pEncInfo->streamWrPtr);

	BitIssueCommand(pCodecInst->coreIdx, pCodecInst, ENCODE_HEADER);
	if (vdi_wait_vpu_busy(pCodecInst->coreIdx, VPU_BUSY_CHECK_TIMEOUT, BIT_BUSY_FLAG) == -1) {
		if (pCodecInst->loggingEnable)
			vdi_log(pCodecInst->coreIdx, ENCODE_HEADER, 2);
		LeaveLock(pCodecInst->coreIdx);
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}
	if (pCodecInst->loggingEnable)
		vdi_log(pCodecInst->coreIdx, ENCODE_HEADER, 0);
	
	if (pEncInfo->ringBufferEnable == 0) {

		rdPtr = encHeaderParam->buf;
		wrPtr = VpuReadReg(pCodecInst->coreIdx, pEncInfo->streamWrPtrRegAddr);
		{
			encHeaderParam->size = wrPtr - rdPtr;
		}
	}
	else {
		rdPtr = VpuReadReg(pCodecInst->coreIdx, pEncInfo->streamRdPtrRegAddr);
		wrPtr = VpuReadReg(pCodecInst->coreIdx, pEncInfo->streamWrPtrRegAddr);	
		encHeaderParam->buf = rdPtr;
		encHeaderParam->size       = wrPtr - rdPtr;
	}	

	pEncInfo->streamWrPtr = wrPtr;
	pEncInfo->streamRdPtr = rdPtr;

	LeaveLock(pCodecInst->coreIdx);
	return RETCODE_SUCCESS;

}

/**
 * EncParaSet() 
 *  1. Setting encoder header option
 *  2. Get RBSP format header in PARA_BUF
 * @param handle      : encoder handle
 * @param paraSetType : encoder header type >> SPS: 0, PPS: 1, VOS: 1, VO: 2, VOL: 0
 * @return none
 */
RetCode EncParaSet(EncHandle handle, int paraSetType)
{
	CodecInst * pCodecInst;
	EncInfo * pEncInfo;	
	int flag = 0;
	int encHeaderCode = paraSetType;
	EncOpenParam *encOP;

	pCodecInst = handle;
	pEncInfo = &pCodecInst->CodecInfo.encInfo;
	encOP = &(pEncInfo->openParam);

	EnterLock(pCodecInst->coreIdx);

	if( paraSetType == 0 && pEncInfo->openParam.bitstreamFormat == STD_AVC) {
		Uint32 CropV, CropH;

		if (encOP->EncStdParam.avcParam.frameCroppingFlag == 1) {
			flag = 1;
			CropH = encOP->EncStdParam.avcParam.frameCropLeft << 16;
			CropH |= encOP->EncStdParam.avcParam.frameCropRight;
			CropV = encOP->EncStdParam.avcParam.frameCropTop << 16;
			CropV |= encOP->EncStdParam.avcParam.frameCropBottom;
			VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_HEADER_FRAME_CROP_H, CropH );
			VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_HEADER_FRAME_CROP_V, CropV );			
		}	
	}
	encHeaderCode = paraSetType| (flag<<2); //paraSetType>> SPS: 0, PPS: 1, VOS: 1, VO: 2, VOL: 0

	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_PARA_SET_TYPE, encHeaderCode); 


	BitIssueCommand(pCodecInst->coreIdx, pCodecInst, ENC_PARA_SET);
	if (vdi_wait_vpu_busy(pCodecInst->coreIdx, VPU_BUSY_CHECK_TIMEOUT, BIT_BUSY_FLAG) == -1) {
		if (pCodecInst->loggingEnable)
			vdi_log(pCodecInst->coreIdx, ENC_PARA_SET, 2);
		LeaveLock(pCodecInst->coreIdx);
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}
	if (pCodecInst->loggingEnable)
		vdi_log(pCodecInst->coreIdx, ENC_PARA_SET, 0);

	LeaveLock(pCodecInst->coreIdx);
	return RETCODE_SUCCESS;
}



RetCode SetGopNumber(EncHandle handle, Uint32 *pGopNumber)
{
	CodecInst * pCodecInst;
	int data =0;
	Uint32 gopNumber = *pGopNumber;
	
	
	pCodecInst = handle;
	
	data = 1;

	EnterLock(pCodecInst->coreIdx);
	
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_PARA_CHANGE_ENABLE, data);
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_PARA_RC_GOP, gopNumber);

	BitIssueCommand(pCodecInst->coreIdx, pCodecInst, RC_CHANGE_PARAMETER);
	if (vdi_wait_vpu_busy(pCodecInst->coreIdx, VPU_BUSY_CHECK_TIMEOUT, BIT_BUSY_FLAG) == -1) {
		if (pCodecInst->loggingEnable)
			vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 2);
		LeaveLock(pCodecInst->coreIdx);
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}
	if (pCodecInst->loggingEnable)
		vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 0);
	
	LeaveLock(pCodecInst->coreIdx);
	return RETCODE_SUCCESS;
}

RetCode SetIntraQp(EncHandle handle, Uint32 *pIntraQp)
{
	CodecInst * pCodecInst;
	int data =0;
	Uint32 intraQp = *pIntraQp;
		
	pCodecInst = handle;
	

	data = 1<<1;

	EnterLock(pCodecInst->coreIdx);
	

	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_PARA_CHANGE_ENABLE, data);
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_PARA_RC_INTRA_QP, intraQp);

	BitIssueCommand(pCodecInst->coreIdx, pCodecInst, RC_CHANGE_PARAMETER);
	if (vdi_wait_vpu_busy(pCodecInst->coreIdx, VPU_BUSY_CHECK_TIMEOUT, BIT_BUSY_FLAG) == -1) {
		if (pCodecInst->loggingEnable)
			vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 2);
		LeaveLock(pCodecInst->coreIdx);
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}
	if (pCodecInst->loggingEnable)
		vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 0);
	LeaveLock(pCodecInst->coreIdx);
	return RETCODE_SUCCESS;
}

RetCode SetBitrate(EncHandle handle, Uint32 *pBitrate)
{
	CodecInst * pCodecInst;
	int data =0;
	Uint32 bitrate = *pBitrate;
		
	pCodecInst = handle;
	
	data = 1<<2;
	
	EnterLock(pCodecInst->coreIdx);
	

	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_PARA_CHANGE_ENABLE, data);
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_PARA_RC_BITRATE, bitrate);

	BitIssueCommand(pCodecInst->coreIdx, pCodecInst, RC_CHANGE_PARAMETER);
	if (vdi_wait_vpu_busy(pCodecInst->coreIdx, VPU_BUSY_CHECK_TIMEOUT, BIT_BUSY_FLAG) == -1) {
		if (pCodecInst->loggingEnable)
			vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 2);
		LeaveLock(pCodecInst->coreIdx);
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}
	if (pCodecInst->loggingEnable)
		vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 0);
	LeaveLock(pCodecInst->coreIdx);
	return RETCODE_SUCCESS;
}

RetCode SetFramerate(EncHandle handle, Uint32 *pFramerate)
{
	CodecInst * pCodecInst;
	int data =0;
	Uint32 frameRate = *pFramerate;
	
	
	pCodecInst = handle;
	

	data = 1<<3;

	EnterLock(pCodecInst->coreIdx);

	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_PARA_CHANGE_ENABLE, data);
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_PARA_RC_FRAME_RATE, frameRate);

	BitIssueCommand(pCodecInst->coreIdx, pCodecInst, RC_CHANGE_PARAMETER);
	if (vdi_wait_vpu_busy(pCodecInst->coreIdx, VPU_BUSY_CHECK_TIMEOUT, BIT_BUSY_FLAG) == -1) {
		if (pCodecInst->loggingEnable)
			vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 2);
		LeaveLock(pCodecInst->coreIdx);
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}
	if (pCodecInst->loggingEnable)
		vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 0);
	
	LeaveLock(pCodecInst->coreIdx);
	return RETCODE_SUCCESS;
}

RetCode SetIntraRefreshNum(EncHandle handle, Uint32 *pIntraRefreshNum)
{
	CodecInst * pCodecInst;
	Uint32 intraRefreshNum = *pIntraRefreshNum;
	int data = 0;

		
	pCodecInst = handle;
	
	data = 1<<4;

	EnterLock(pCodecInst->coreIdx);

	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_PARA_CHANGE_ENABLE, data);
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_PARA_INTRA_MB_NUM, intraRefreshNum);

	BitIssueCommand(pCodecInst->coreIdx, pCodecInst, RC_CHANGE_PARAMETER);
	if (vdi_wait_vpu_busy(pCodecInst->coreIdx, VPU_BUSY_CHECK_TIMEOUT, BIT_BUSY_FLAG) == -1) {
		if (pCodecInst->loggingEnable)
			vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 2);
		LeaveLock(pCodecInst->coreIdx);
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}
	if (pCodecInst->loggingEnable)
		vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 0);
	LeaveLock(pCodecInst->coreIdx);
	return RETCODE_SUCCESS;
}

RetCode SetSliceMode(EncHandle handle, EncSliceMode *pSliceMode)
{
	CodecInst * pCodecInst;
	Uint32 data = 0;
	int data2 = 0;
	
	pCodecInst = handle;

	EnterLock(pCodecInst->coreIdx);	
	
	data = pSliceMode->sliceSize<<2 | pSliceMode->sliceSizeMode<<1 | pSliceMode->sliceMode;
	data2 = 1<<5;

	

	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_PARA_CHANGE_ENABLE, data2);
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_PARA_SLICE_MODE, data);

	BitIssueCommand(pCodecInst->coreIdx, pCodecInst, RC_CHANGE_PARAMETER);
	if (vdi_wait_vpu_busy(pCodecInst->coreIdx, VPU_BUSY_CHECK_TIMEOUT, BIT_BUSY_FLAG) == -1) {
		if (pCodecInst->loggingEnable)
			vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 2);
		LeaveLock(pCodecInst->coreIdx);
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}
	if (pCodecInst->loggingEnable)
		vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 0);
	LeaveLock(pCodecInst->coreIdx);

	return RETCODE_SUCCESS;
}

RetCode SetHecMode(EncHandle handle, int mode)
{
	CodecInst * pCodecInst;
	Uint32 HecMode = mode;
	int data = 0;
	
	
	pCodecInst = handle;
	

	data = 1 << 6;

	EnterLock(pCodecInst->coreIdx);
	
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_PARA_CHANGE_ENABLE, data);
	VpuWriteReg(pCodecInst->coreIdx, CMD_ENC_SEQ_PARA_HEC_MODE, HecMode);

	BitIssueCommand(pCodecInst->coreIdx, pCodecInst, RC_CHANGE_PARAMETER);
	if (vdi_wait_vpu_busy(pCodecInst->coreIdx, VPU_BUSY_CHECK_TIMEOUT, BIT_BUSY_FLAG) == -1) {
		if (pCodecInst->loggingEnable)
			vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 2);
		LeaveLock(pCodecInst->coreIdx);
		return RETCODE_VPU_RESPONSE_TIMEOUT;
	}
	if (pCodecInst->loggingEnable)
		vdi_log(pCodecInst->coreIdx, RC_CHANGE_PARAMETER, 0);
	LeaveLock(pCodecInst->coreIdx);
	return RETCODE_SUCCESS;
}




void EncSetHostParaAddr(Uint32 coreIdx, PhysicalAddress baseAddr, PhysicalAddress paraAddr)
{
	BYTE tempBuf[8]={0,};					// 64bit bus & endian
	Uint32 val;
	
	val =  paraAddr;
	tempBuf[0] = 0;
	tempBuf[1] = 0;
	tempBuf[2] = 0;
	tempBuf[3] = 0;
	tempBuf[4] = (val >> 24) & 0xff;
	tempBuf[5] = (val >> 16) & 0xff;
	tempBuf[6] = (val >> 8) & 0xff;
	tempBuf[7] = (val >> 0) & 0xff;						
	VpuWriteMem(coreIdx, baseAddr, (BYTE *)tempBuf, 8, VDI_BIG_ENDIAN);
}


RetCode EnterDispFlagLock(Uint32 coreIdx)
{
    vdi_disp_lock(coreIdx);
    return RETCODE_SUCCESS;
}

RetCode LeaveDispFlagLock(Uint32 coreIdx)
{
    vdi_disp_unlock(coreIdx);
    return RETCODE_SUCCESS;
}


RetCode EnterLock(Uint32 coreIdx)
{	
	vdi_lock(coreIdx);
	SetClockGate(coreIdx, 1);
	return RETCODE_SUCCESS;

}

RetCode LeaveLock(Uint32 coreIdx)
{
	SetClockGate(coreIdx, 0);
	vdi_unlock(coreIdx);	
	return RETCODE_SUCCESS;
}

RetCode SetClockGate(Uint32 coreIdx, Uint32 on)
{
	CodecInst *inst;
	vpu_instance_pool_t *vip;

	vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
	if (!vip)
		return RETCODE_INSUFFICIENT_RESOURCE;
	
	inst = (CodecInst *)vip->pendingInst;
	if(inst && !on) 
		return RETCODE_SUCCESS;
	
	vdi_set_clock_gate(coreIdx, on);	
	
	return RETCODE_SUCCESS;
}

void SetPendingInst(Uint32 coreIdx, CodecInst *inst)
{
	vpu_instance_pool_t *vip;

	vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
	if (!vip)
		return;

	vip->pendingInst = inst;	

	if (inst)
		vip->lastInst = inst;
}

void ClearPendingInst(Uint32 coreIdx)
{
	vpu_instance_pool_t *vip;

	vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
	if (!vip)
		return;

	if(vip->pendingInst)
        vip->pendingInst = 0;	
}

CodecInst *GetPendingInst(Uint32 coreIdx)
{
	vpu_instance_pool_t *vip;

	vip = (vpu_instance_pool_t *)vdi_get_instance_pool(coreIdx);
	if (!vip)
		return NULL;

	return (CodecInst *)vip->pendingInst;
}


int GetLowDelayOutput(CodecInst *pCodecInst, DecOutputInfo *info)
{
	Uint32		val  = 0;
	Uint32		val2 = 0;
	Rect        rectInfo;
	DecInfo * pDecInfo;

	pDecInfo = &pCodecInst->CodecInfo.decInfo;

	info->indexFrameDisplay = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_DISPLAY_IDX);
	info->indexFrameDecoded = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_DECODED_IDX);
	if (!pDecInfo->reorderEnable)
		info->indexFrameDisplay = info->indexFrameDecoded;

	val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_SIZE); // decoding picture size
	info->decPicWidth  = (val>>16) & 0xFFFF;
	info->decPicHeight = (val) & 0xFFFF;


	if (info->indexFrameDecoded >= 0 && info->indexFrameDecoded < MAX_REG_FRAME)
	{
		//default value
		rectInfo.left   = 0;
		rectInfo.right  = info->decPicWidth;
		rectInfo.top    = 0;
		rectInfo.bottom = info->decPicHeight;

		if (pCodecInst->codecMode == AVC_DEC || pCodecInst->codecMode == MP2_DEC)
		{
			val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_CROP_LEFT_RIGHT);				// frame crop information(left, right)
			val2 = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_CROP_TOP_BOTTOM);			// frame crop information(top, bottom)

			if (val == (Uint32)-1 || val == 0)
			{
				rectInfo.left   = 0;
				rectInfo.right  = info->decPicWidth;
			} 
			else
			{
				rectInfo.left    = ((val>>16) & 0xFFFF);
				rectInfo.right   = info->decPicWidth - (val&0xFFFF);
			}
			if (val2 == (Uint32)-1 || val2 == 0)
			{
				rectInfo.top    = 0;
				rectInfo.bottom = info->decPicHeight;
			}
			else
			{
				rectInfo.top     = ((val2>>16) & 0xFFFF);
				rectInfo.bottom	= info->decPicHeight - (val2&0xFFFF);
			}
		}

		info->rcDecoded.left   =  pDecInfo->decOutInfo[info->indexFrameDecoded].rcDecoded.left   = rectInfo.left;    
		info->rcDecoded.right  =  pDecInfo->decOutInfo[info->indexFrameDecoded].rcDecoded.right  = rectInfo.right;   
		info->rcDecoded.top    =  pDecInfo->decOutInfo[info->indexFrameDecoded].rcDecoded.top    = rectInfo.top;     
		info->rcDecoded.bottom =  pDecInfo->decOutInfo[info->indexFrameDecoded].rcDecoded.bottom = rectInfo.bottom;
	}
	else
	{
		info->rcDecoded.left   = 0;  
		info->rcDecoded.right  = info->decPicWidth;	  
		info->rcDecoded.top    = 0;  
		info->rcDecoded.bottom = info->decPicHeight;	
	}

	if (info->indexFrameDisplay >= 0 && info->indexFrameDisplay < MAX_REG_FRAME)
	{
		if (pCodecInst->codecMode == VC1_DEC) // vc1 rotates decoded frame buffer region. the other std rotated whole frame buffer region.
		{
			if (pDecInfo->rotationEnable && (pDecInfo->rotationAngle==90 || pDecInfo->rotationAngle==270))
			{
				info->rcDisplay.left   = pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.top;                   
				info->rcDisplay.right  = pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.bottom;                
				info->rcDisplay.top    = pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.left;
				info->rcDisplay.bottom = pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.right;
			}
			else
			{
				info->rcDisplay.left   = pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.left;
				info->rcDisplay.right  = pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.right;
				info->rcDisplay.top    = pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.top;
				info->rcDisplay.bottom = pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.bottom;
			}
		}
		else
		{
			if (pDecInfo->rotationEnable)
			{
				switch(pDecInfo->rotationAngle)
				{
				case 90:
				case 270:
					info->rcDisplay.left   = pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.top;                   
					info->rcDisplay.right  = pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.bottom;                
					info->rcDisplay.top    = pDecInfo->rotatorOutput.height - pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.right;
					info->rcDisplay.bottom = pDecInfo->rotatorOutput.height - pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.left;
					break;
				default:
					info->rcDisplay.left   = pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.left;
					info->rcDisplay.right  = pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.right;
					info->rcDisplay.top    = pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.top;
					info->rcDisplay.bottom = pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.bottom;
					break;
				}

			}
			else
			{
				info->rcDisplay.left   = pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.left;
				info->rcDisplay.right  = pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.right;
				info->rcDisplay.top    = pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.top;
				info->rcDisplay.bottom = pDecInfo->decOutInfo[info->indexFrameDisplay].rcDecoded.bottom;
			}
		}

		if (info->indexFrameDisplay == info->indexFrameDecoded)
		{
			info->dispPicWidth =  info->decPicWidth;
			info->dispPicHeight = info->decPicHeight;
		}
		else
		{
			info->dispPicWidth = pDecInfo->decOutInfo[info->indexFrameDisplay].decPicWidth;
			info->dispPicHeight = pDecInfo->decOutInfo[info->indexFrameDisplay].decPicHeight;
		}
	}
	else
	{
		info->rcDisplay.left   = 0;  
		info->rcDisplay.right  = 0;  
		info->rcDisplay.top    = 0;  
		info->rcDisplay.bottom = 0;

		info->dispPicWidth = 0;
		info->dispPicHeight = 0;
	}

	val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_TYPE);
	info->interlacedFrame	= (val >> 18) & 0x1;
	info->topFieldFirst     = (val >> 21) & 0x0001;	// TopFieldFirst[21]
	if (info->interlacedFrame) {
		info->picTypeFirst = (val & 0x38) >> 3;	  // pic_type of 1st field
		info->picType = val & 0xff;

	}
	else {
		info->picTypeFirst   = PIC_TYPE_MAX;	// no meaning
		info->picType = val & 0xff;
	}

	info->pictureStructure  = (val >> 19) & 0x0003;	// MbAffFlag[17], FieldPicFlag[16]
	info->repeatFirstField  = (val >> 22) & 0x0001;
	info->progressiveFrame  = (val >> 23) & 0x0003;

	if( pCodecInst->codecMode == AVC_DEC)
	{
		info->nalRefIdc = (val >> 7) & 0x03;
		info->decFrameInfo = (val >> 15) & 0x0001;
		info->picStrPresent = (val >> 27) & 0x0001;
		info->picTimingStruct = (val >> 28) & 0x000f;
		info->avcNpfFieldInfo  = (val >> 16) & 0x0003;
		val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_HRD_INFO);
		info->avcHrdInfo.cpbMinus1 = val>>2;
		info->avcHrdInfo.vclHrdParamFlag = (val>>1)&1;
		info->avcHrdInfo.nalHrdParamFlag = val&1;

		val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_VUI_INFO);
		info->avcVuiInfo.fixedFrameRateFlag    = val &1;
		info->avcVuiInfo.timingInfoPresent     = (val>>1) & 0x01;
		info->avcVuiInfo.chromaLocBotField     = (val>>2) & 0x07;
		info->avcVuiInfo.chromaLocTopField     = (val>>5) & 0x07;
		info->avcVuiInfo.chromaLocInfoPresent  = (val>>8) & 0x01;
		info->avcVuiInfo.colorPrimaries        = (val>>16) & 0xff;
		info->avcVuiInfo.colorDescPresent      = (val>>24) & 0x01;
		info->avcVuiInfo.isExtSAR              = (val>>25) & 0x01;
		info->avcVuiInfo.vidFullRange          = (val>>26) & 0x01;
		info->avcVuiInfo.vidFormat             = (val>>27) & 0x07;
		info->avcVuiInfo.vidSigTypePresent     = (val>>30) & 0x01;
		info->avcVuiInfo.vuiParamPresent       = (val>>31) & 0x01;
		val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_VUI_PIC_STRUCT);
		info->avcVuiInfo.vuiPicStructPresent = (val & 0x1);
		info->avcVuiInfo.vuiPicStruct = (val>>1);
	}

	info->fRateNumerator    = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_FRATE_NR); //Frame rate, Aspect ratio can be changed frame by frame.
	info->fRateDenominator  = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_FRATE_DR);
	if (pCodecInst->codecMode == AVC_DEC && info->fRateDenominator > 0)
		info->fRateDenominator  *= 2;

	info->aspectRateInfo = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_ASPECT);

	// User Data
	if (pDecInfo->userDataEnable) {
		int userDataNum;
		int userDataSize;
		BYTE tempBuf[8] = {0,};

		VpuReadMem(pCodecInst->coreIdx, pDecInfo->userDataBufAddr + 0, tempBuf, 8,  VPU_USER_DATA_ENDIAN); 

		val =	((tempBuf[0]<<24) & 0xFF000000) |
			((tempBuf[1]<<16) & 0x00FF0000) |
			((tempBuf[2]<< 8) & 0x0000FF00) |
			((tempBuf[3]<< 0) & 0x000000FF);

		userDataNum = (val >> 16) & 0xFFFF;
		userDataSize = (val >> 0) & 0xFFFF;
		if (userDataNum == 0)
			userDataSize = 0;

		info->decOutputExtData.userDataNum = userDataNum;
		info->decOutputExtData.userDataSize = userDataSize;

		val =	((tempBuf[4]<<24) & 0xFF000000) |
			((tempBuf[5]<<16) & 0x00FF0000) |
			((tempBuf[6]<< 8) & 0x0000FF00) |
			((tempBuf[7]<< 0) & 0x000000FF);

		if (userDataNum == 0)
			info->decOutputExtData.userDataBufFull = 0;
		else
			info->decOutputExtData.userDataBufFull = (val >> 16) & 0xFFFF;

		info->decOutputExtData.activeFormat = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_ATSC_USER_DATA_INFO)&0xf;
	}

	info->numOfErrMBs = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_ERR_MB);

	val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_SUCCESS);
	info->decodingSuccess = val;
	info->sequenceChanged = ((val>>20) & 0x1);
	info->streamEndFlag = ((pDecInfo->streamEndflag>>2) & 0x01);

	
	if (info->indexFrameDisplay >= 0 && info->indexFrameDisplay < pDecInfo->numFrameBuffers) {
		info->dispFrame = pDecInfo->frameBufPool[info->indexFrameDisplay];
		if (pDecInfo->openParam.wtlEnable) {	// coda980 only
			info->dispFrame = pDecInfo->frameBufPool[info->indexFrameDisplay + pDecInfo->numFrameBuffers];
		}
	}


	if (pDecInfo->deringEnable || pDecInfo->mirrorEnable || pDecInfo->rotationEnable || pDecInfo->tiled2LinearEnable) {
		info->dispFrame = pDecInfo->rotatorOutput;
	}

	if (pCodecInst->codecMode == AVC_DEC && pCodecInst->codecModeAux == AVC_AUX_MVC)
	{
		int val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_MVC_REPORT);
		info->mvcPicInfo.viewIdxDisplay = (val>>0) & 1;
		info->mvcPicInfo.viewIdxDecoded = (val>>1) & 1;
	}

	if (pCodecInst->codecMode == AVC_DEC)
	{
		val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_AVC_FPA_SEI0);

		if (val == (Uint32)-1) {
			info->avcFpaSei.exist = 0;
		} 
		else {
			info->avcFpaSei.exist = 1;
			info->avcFpaSei.framePackingArrangementId = val;

			val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_AVC_FPA_SEI1);
			info->avcFpaSei.contentInterpretationType               = val&0x3F; // [5:0]
			info->avcFpaSei.framePackingArrangementType             = (val >> 6)&0x7F; // [12:6]
			info->avcFpaSei.framePackingArrangementExtensionFlag    = (val >> 13)&0x01; // [13]
			info->avcFpaSei.frame1SelfContainedFlag                 = (val >> 14)&0x01; // [14]
			info->avcFpaSei.frame0SelfContainedFlag                 = (val >> 15)&0x01; // [15]
			info->avcFpaSei.currentFrameIsFrame0Flag                = (val >> 16)&0x01; // [16]
			info->avcFpaSei.fieldViewsFlag                          = (val >> 17)&0x01; // [17]
			info->avcFpaSei.frame0FlippedFlag                       = (val >> 18)&0x01; // [18]
			info->avcFpaSei.spatialFlippingFlag                     = (val >> 19)&0x01; // [19]
			info->avcFpaSei.quincunxSamplingFlag                    = (val >> 20)&0x01; // [20]
			info->avcFpaSei.framePackingArrangementCancelFlag       = (val >> 21)&0x01; // [21]

			val = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_AVC_FPA_SEI2);
			info->avcFpaSei.framePackingArrangementRepetitionPeriod = val&0x7FFF;       // [14:0]
			info->avcFpaSei.frame1GridPositionY                     = (val >> 16)&0x0F; // [19:16]
			info->avcFpaSei.frame1GridPositionX                     = (val >> 20)&0x0F; // [23:20]
			info->avcFpaSei.frame0GridPositionY                     = (val >> 24)&0x0F; // [27:24]
			info->avcFpaSei.frame0GridPositionX                     = (val >> 28)&0x0F; // [31:28]
		}      


		info->avcPocTop = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_POC_TOP);
		info->avcPocBot = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_POC_BOT);

		if (info->interlacedFrame)
		{
			if (info->avcPocTop > info->avcPocBot) {
				info->avcPocPic = info->avcPocBot;
			} else {
				info->avcPocPic = info->avcPocTop;
			}
		}
		else
			info->avcPocPic = VpuReadReg(pCodecInst->coreIdx, RET_DEC_PIC_POC);
	}

	//pDecInfo->streamRdPtr //NA
	pDecInfo->frameDisplayFlag = VpuReadReg(pCodecInst->coreIdx, pDecInfo->frameDisplayFlagRegAddr);
	//info->consumedByte //NA
	//info->notSufficientSliceBuffer; // NA
	//info->notSufficientPsBuffer;  // NA
	//info->bytePosFrameStart //NA
	//info->bytePosFrameEnd   //NA
	//info->rdPtr //NA
	//info->wrPtr //NA
	//info->frameCycle  //NA
	//Vp8ScaleInfo vp8ScaleInfo; //NA
	//Vp8PicInfo vp8PicInfo; //NA
	//MvcPicInfo mvcPicInfo; ////NA
	//info->wprotErrReason; avcVuiInfo
	//PhysicalAddress wprotErrAddress; avcVuiInfo

	// Report Information
	//info->frameDct; //NA
	//info->progressiveSequence; //NA
	//info->mp4TimeIncrement; //NA
	//info->mp4ModuloTimeBase; //NA
	

	return 1;
}



RetCode AllocateFrameBufferArray(int coreIdx, FrameBuffer *frambufArray, vpu_buffer_t *pvbFrame, int mapType, int interleave, int framebufFormat, int num, int stride, int memHeight, int gdiIndex, int fbType, PhysicalAddress tiledBaseAddr, DRAMConfig *pDramCfg)
{
	int chr_hscale, chr_vscale;
	int size_dpb_lum, size_dpb_chr, size_dpb_all;
	int alloc_by_user;		
	int chr_size_y, chr_size_x;
	int i, width, height;
	PhysicalAddress addrY;
	int size_dpb_lum_4k, size_dpb_chr_4k, size_dpb_all_4k;
	

	
	alloc_by_user = (frambufArray[0].bufCb == (PhysicalAddress)-1 && frambufArray[0].bufCr == (PhysicalAddress)-1);

	height = memHeight;
	width = stride;
    
	switch (framebufFormat)
	{
	case FORMAT_420:
		height = (memHeight+1)/2*2;
		width = (stride+1)/2*2;
		break;
	case FORMAT_224:
		height = (memHeight+1)/2*2;
		break;
	case FORMAT_422:
		width = (stride+1)/2*2;
		break;
	case FORMAT_444:
		break;
	case FORMAT_400:
		height = (memHeight+1)/2*2;
		width = (stride+1)/2*2;	
		break;
	default:
		return RETCODE_INVALID_PARAM;
	}

	chr_hscale = framebufFormat == FORMAT_420 || framebufFormat == FORMAT_422 ? 2 : 1;
	chr_vscale = framebufFormat == FORMAT_420 || framebufFormat == FORMAT_224 ? 2 : 1;

	if (mapType == LINEAR_FRAME_MAP) { // AllocateFrameBuffer

		chr_size_y = (height/chr_hscale); 
		chr_size_x = (width/chr_vscale);

		size_dpb_lum   = width * height;
		size_dpb_chr   = chr_size_y * chr_size_x;
		size_dpb_all   = size_dpb_lum + (size_dpb_chr*2);

		pvbFrame->size     = size_dpb_all*num;
		if (alloc_by_user) {		
		}
		else {
			if (vdi_allocate_dma_memory(coreIdx, pvbFrame)<0)
				return RETCODE_INSUFFICIENT_RESOURCE;
		}
		addrY = pvbFrame->phys_addr;
		for (i=0; i<num; i++)
		{	
			if (alloc_by_user)
				addrY = frambufArray[i].bufY;

			frambufArray[i].myIndex = i+gdiIndex;
			frambufArray[i].mapType = mapType;
			frambufArray[i].height = memHeight;
			frambufArray[i].bufY    = addrY;
			frambufArray[i].bufCb   = frambufArray[i].bufY + size_dpb_lum;
			if (!interleave)
				frambufArray[i].bufCr = frambufArray[i].bufY + size_dpb_lum + size_dpb_chr;
            frambufArray[i].stride         = width;		
            frambufArray[i].cbcrInterleave = interleave;
			frambufArray[i].sourceLBurstEn = 0;
			addrY += size_dpb_all;			
		}
		
	}
	else if (mapType == TILED_FRAME_MB_RASTER_MAP || mapType == TILED_FIELD_MB_RASTER_MAP) { //AllocateMBRasterTiled
		chr_size_x = width/chr_hscale;
		chr_size_y = height/chr_hscale;
		size_dpb_lum   = width * height;
		size_dpb_chr   = chr_size_y * chr_size_x;

		// aligned to 8192*2 (0x4000) for top/bot field
		// use upper 20bit address only
		size_dpb_lum_4k	=  ((size_dpb_lum + 16383)/16384)*16384;
		size_dpb_chr_4k	=  ((size_dpb_chr + 16383)/16384)*16384;
		size_dpb_all_4k =  size_dpb_lum_4k + 2*size_dpb_chr_4k;

		if (mapType == TILED_FIELD_MB_RASTER_MAP)
		{
			size_dpb_lum_4k = ((size_dpb_lum_4k+(0x8000-1))&~(0x8000-1));
			size_dpb_chr_4k = ((size_dpb_chr_4k+(0x8000-1))&~(0x8000-1));
			size_dpb_all_4k =  size_dpb_lum_4k + 2*size_dpb_chr_4k;
		}

		pvbFrame->size     = size_dpb_all_4k*num;
		if (alloc_by_user) {			
		}
		else 
		{
			if (vdi_allocate_dma_memory(coreIdx, pvbFrame)<0)
				return RETCODE_INSUFFICIENT_RESOURCE;
		}

		addrY = ((pvbFrame->phys_addr+(16384-1))&~(16384-1));

		for (i=0; i<num; i++)
		{
			int  lum_top_base;
			int  lum_bot_base;
			int  chr_top_base;
			int  chr_bot_base;
			
			//-------------------------------------
			// use tiled map
			//-------------------------------------
			if (alloc_by_user)
				addrY = ((frambufArray[i].bufY+(16384-1))&~(16384-1));

			lum_top_base = addrY;
			lum_bot_base = addrY + size_dpb_lum_4k/2;
			chr_top_base = addrY + size_dpb_lum_4k;
			chr_bot_base = addrY + size_dpb_lum_4k + size_dpb_chr_4k; // cbcr is interleaved

			lum_top_base = (lum_top_base>>12) & 0xfffff;
			lum_bot_base = (lum_bot_base>>12) & 0xfffff;
			chr_top_base = (chr_top_base>>12) & 0xfffff;
			chr_bot_base = (chr_bot_base>>12) & 0xfffff;
			
			frambufArray[i].myIndex = i+gdiIndex;
			frambufArray[i].mapType = mapType;
			frambufArray[i].height = memHeight;
			// {AddrY,AddrCb,AddrCr} = {lum_top_base[31:12],chr_top_base[31:12],lum_bot_base[31:12],chr_bot_base[31:12],16'b0}
			// AddrY  = {lum_top_base[31:12],chr_top_base[31:20]} : 20 + 12 bit
			// AddrCb = {chr_top_base[19:12],lum_bot_base[31:20],chr_bot_base[31:28]} : 8 + 20 + 4 bit
			// AddrCr = {chr_bot_base[27:12],16'b0} : 16 bit
			frambufArray[i].bufY  = ( lum_top_base           << 12) | (chr_top_base >> 8);
			frambufArray[i].bufCb = ((chr_top_base & 0xff  ) << 24) | (lum_bot_base << 4) | (chr_bot_base >> 16);
			frambufArray[i].bufCr = ((chr_bot_base & 0xffff) << 16) ;
            frambufArray[i].stride         = width;		
            frambufArray[i].cbcrInterleave = interleave;
			frambufArray[i].sourceLBurstEn = 0;
			addrY += size_dpb_all_4k;
		}

	}
	else {
		PhysicalAddress addrYRas;
		int ChrSizeYField;

		int  VerSizePerRas,HorSizePerRas,RasLowBitsForHor; 
		int  ChrFieldRasSize,ChrFrameRasSize,LumFieldRasSize,LumFrameRasSize;
		int  LumRasTop,LumRasBot,ChrRasTop,ChrRasBot;     

		if (mapType == TILED_FRAME_V_MAP) {
			if (pDramCfg->casBit == 9 && pDramCfg->bankBit == 2 && pDramCfg->rasBit == 13)	// CNN setting 
			{
				VerSizePerRas = 64;
				HorSizePerRas = 256;
				RasLowBitsForHor = 3;
			}
			else if(pDramCfg->casBit == 10 && pDramCfg->bankBit == 3 && pDramCfg->rasBit == 13)
			{
				VerSizePerRas = 64;
				HorSizePerRas = 512;
				RasLowBitsForHor = 2;
			}
			else
				return RETCODE_INVALID_PARAM;			
		} else if (mapType == TILED_FRAME_H_MAP) {
			if (pDramCfg->casBit == 9 && pDramCfg->bankBit == 2 && pDramCfg->rasBit == 13)	// CNN setting 
			{
				VerSizePerRas = 64;
				HorSizePerRas = 256;
				RasLowBitsForHor = 3;
			}
			else if(pDramCfg->casBit == 10 && pDramCfg->bankBit == 3 && pDramCfg->rasBit == 13)
			{
				VerSizePerRas = 64;
				HorSizePerRas = 512;
				RasLowBitsForHor = 2;
			}
			else
				return RETCODE_INVALID_PARAM;			
		} else if (mapType == TILED_FIELD_V_MAP) {
			if (pDramCfg->casBit == 9 && pDramCfg->bankBit == 2 && pDramCfg->rasBit == 13)	// CNN setting 
			{
				VerSizePerRas = 64;
				HorSizePerRas = 256;
				RasLowBitsForHor = 3;
			}
			else if(pDramCfg->casBit == 10 && pDramCfg->bankBit == 3 && pDramCfg->rasBit == 13)
			{
				VerSizePerRas = 64;
				HorSizePerRas = 512;
				RasLowBitsForHor = 2;
			}
			else
				return RETCODE_INVALID_PARAM;			
		} else {
			if (pDramCfg->casBit == 9 && pDramCfg->bankBit == 2 && pDramCfg->rasBit == 13)	// CNN setting 
			{
				VerSizePerRas = 64;
				HorSizePerRas = 256;
				RasLowBitsForHor = 3;
			}
			else if(pDramCfg->casBit == 10 && pDramCfg->bankBit == 3 && pDramCfg->rasBit == 13)
			{
				VerSizePerRas = 64;
				HorSizePerRas = 512;
				RasLowBitsForHor = 2;
			}
			else
				return RETCODE_INVALID_PARAM;
		} 

		UNREFERENCED_PARAMETER(HorSizePerRas);
		
		chr_size_y = height/chr_hscale;
		ChrSizeYField = ((chr_size_y+1)>>1);
		ChrFieldRasSize = ((ChrSizeYField + (VerSizePerRas-1))/VerSizePerRas) << RasLowBitsForHor;
		ChrFrameRasSize = ChrFieldRasSize *2;
		LumFieldRasSize = ChrFrameRasSize;
		LumFrameRasSize = LumFieldRasSize *2;

		size_dpb_all         = (LumFrameRasSize + ChrFrameRasSize) << (pDramCfg->bankBit+pDramCfg->casBit+pDramCfg->busBit);
		pvbFrame->size     = size_dpb_all*num;
		if (alloc_by_user)
		{
			pvbFrame->phys_addr = frambufArray[0].bufY;
			pvbFrame->phys_addr = GetTiledFrameBase(coreIdx, &frambufArray[0], num);
		}
		else 
		{
			if (vdi_allocate_dma_memory(coreIdx, pvbFrame)<0)
				return RETCODE_INSUFFICIENT_RESOURCE;
		}

		if (fbType == FB_TYPE_PPU) {
			addrY = pvbFrame->phys_addr - tiledBaseAddr;
		}
		else {
			SetTiledFrameBase(coreIdx, pvbFrame->phys_addr);			
			addrY = 0;
			tiledBaseAddr = pvbFrame->phys_addr;
		}
	
		for (i=0; i<num; i++)
		{
			if (alloc_by_user) {
				addrY = frambufArray[i].bufY - tiledBaseAddr;				
			}	

			// align base_addr to RAS boundary
			addrYRas  = (addrY + ((1<<(pDramCfg->bankBit+pDramCfg->casBit+pDramCfg->busBit))-1)) >> (pDramCfg->bankBit+pDramCfg->casBit+pDramCfg->busBit);
			// round up RAS lower 3(or 4) bits
			addrYRas  = ((addrYRas + ((1<<(RasLowBitsForHor))-1)) >> RasLowBitsForHor) << RasLowBitsForHor;

			LumRasTop = addrYRas;
			LumRasBot = LumRasTop  + LumFieldRasSize;
			ChrRasTop = LumRasTop  + LumFrameRasSize;
			ChrRasBot = ChrRasTop  + ChrFieldRasSize;
			
			frambufArray[i].myIndex = i+gdiIndex;
			frambufArray[i].mapType = mapType;
			frambufArray[i].height = memHeight;
			frambufArray[i].bufY  = (LumRasBot << 16) + LumRasTop;
			frambufArray[i].bufCb = (ChrRasBot << 16) + ChrRasTop;
            frambufArray[i].stride         = width;
            frambufArray[i].cbcrInterleave = interleave;
			frambufArray[i].sourceLBurstEn = 0;
			if (RasLowBitsForHor == 4)
				frambufArray[i].bufCr  = ((((ChrRasBot>>4)<<4) + 8) << 16) + (((ChrRasTop>>4)<<4) + 8);
			else if (RasLowBitsForHor == 3)
				frambufArray[i].bufCr  = ((((ChrRasBot>>3)<<3) + 4) << 16) + (((ChrRasTop>>3)<<3) + 4);
			else if (RasLowBitsForHor == 2)
				frambufArray[i].bufCr  = ((((ChrRasBot>>2)<<2) + 2) << 16) + (((ChrRasTop>>2)<<2) + 2);
			else if (RasLowBitsForHor == 1)
				frambufArray[i].bufCr  = ((((ChrRasBot>>1)<<1) + 1) << 16) + (((ChrRasTop>>1)<<1) + 1);
			else
				return RETCODE_INSUFFICIENT_RESOURCE; // Invalid RasLowBit value

			addrYRas = (addrYRas + LumFrameRasSize + ChrFrameRasSize);		
			addrY    = ((addrYRas) << (pDramCfg->bankBit+pDramCfg->casBit+pDramCfg->busBit));
		}
	}



	return RETCODE_SUCCESS;
}

int ConfigSecAXI(Uint32 coreIdx, CodStd codStd, SecAxiInfo *sa, int width, int height, int profile)
{
	vpu_buffer_t vb;
	int offset;
	int MbNumX = ((width & 0xFFFF) + 15) / 16;
	int MbNumY = ((height & 0xFFFF) + 15) / 16;
	//int MbNumY = ((height & 0xFFFF) + 31) / 32; //field??


	if (vdi_get_sram_memory(coreIdx, &vb) < 0)
		return 0;

	if (!vb.size)
	{
		sa->bufSize = 0;
		sa->useBitEnable = 0;
		sa->useIpEnable = 0;
		sa->useDbkYEnable = 0;
		sa->useDbkCEnable = 0;
		sa->useOvlEnable = 0;
		sa->useBtpEnable = 0;
		return 0;
	}
	
	offset = 0;
	//BIT
	if (sa->useBitEnable)
	{
		sa->useBitEnable = 1;
		sa->bufBitUse = vb.phys_addr + offset;

		switch (codStd) 
		{
		case STD_AVC:
			offset = offset + MbNumX * 144; 
			break; // AVC
		case STD_RV:
			offset = offset + MbNumX * 128;
			break;
		case STD_VC1:
			offset = offset + MbNumX *  64;
			break;
		case STD_AVS:
			offset = offset + ((MbNumX + 3) & ~3) *  32; 
			break;
		case STD_MPEG2:
			offset = offset + MbNumX * 0; 
			break;
		case STD_VP8:
			offset = offset + MbNumX * 128; 
			break;
		default:
			offset = offset + MbNumX *  16; 
			break; // MPEG-4, Divx3
		}

		if (offset > vb.size)
		{
			sa->bufSize = 0;
			return 0;
		}	

	}

	//Intra Prediction, ACDC
	if (sa->useIpEnable)
	{
		sa->bufIpAcDcUse = vb.phys_addr + offset;
		sa->useIpEnable = 1;

		switch (codStd) 
		{
		case STD_AVC:
			offset = offset + MbNumX * 64; 
			break; // AVC
		case STD_RV:
			offset = offset + MbNumX * 64;
			break;
		case STD_VC1:
			offset = offset + MbNumX * 128;
			break;
		case STD_AVS:
			offset = offset + MbNumX * 64;
			break;
		case STD_MPEG2:
			offset = offset + MbNumX * 0; 
			break;
		case STD_VP8:
			offset = offset + MbNumX * 64; 
			break;
		default:
			offset = offset + MbNumX * 128; 
			break; // MPEG-4, Divx3
		}

		if (offset > vb.size)
		{
			sa->bufSize = 0;
			return 0;
		}		
	}


	//Deblock Chroma
	if (sa->useDbkCEnable)
	{
		sa->bufDbkCUse = vb.phys_addr + offset;
		sa->useDbkCEnable = 1;
		switch (codStd) 
		{
		case STD_AVC:
			offset = profile==0 ? offset + (MbNumX * 64) : offset + (MbNumX * 128);			
			break; // AVC
		case STD_RV:
			offset = offset + MbNumX * 128;
			break;
		case STD_VC1:
			offset = profile==2 ? offset + MbNumX * 256 : offset + MbNumX * 128;
			break;
		case STD_AVS:
			offset = offset + MbNumX * 128;
			break;
		case STD_MPEG2:
			offset = offset + MbNumX * 64; 
			break;
		case STD_VP8:
			offset = offset + MbNumX * 128; 
			break;
		default:
			offset = offset + MbNumX * 64; 
			break;
		}

		if (offset > vb.size)
		{
			sa->bufSize = 0;
			return 0;
		}		
	}

	//Deblock Luma
	if (sa->useDbkYEnable)
	{
		sa->bufDbkYUse = vb.phys_addr + offset;
		sa->useDbkYEnable = 1;

		switch (codStd) 
		{
		case STD_AVC:
			offset = profile==0 ? offset+ (MbNumX * 64) : offset + (MbNumX * 128);
			break; // AVC
		case STD_RV:
			offset = offset + MbNumX * 128;
			break;
		case STD_VC1:
			offset = profile==2 ? offset + MbNumX * 256 : offset + MbNumX * 128;			
			break;
		case STD_AVS:
			offset = offset + MbNumX * 128;
			break;
		case STD_MPEG2:
			offset = offset + MbNumX * 128; 
			break;
		case STD_VP8:
			offset = offset + MbNumX * 128; 
			break;
		default:
			offset = offset + MbNumX * 128; 
			break;
		}

		if (offset > vb.size)
		{
			sa->bufSize = 0;
			return 0;
		}		
	}

	//VC1 Bit-plane
	if (sa->useBtpEnable)
	{
		if (codStd != STD_VC1)
		{
			sa->useBtpEnable = 0;			
		}
		else
		{
			int oneBTP;

			offset = ((offset+255)&~255);			
			sa->bufBtpUse = vb.phys_addr + offset;
			sa->useBtpEnable = 1;

			oneBTP  = (((MbNumX+15)/16) * MbNumY + 1) * 2;
			oneBTP  = (oneBTP%256) ? ((oneBTP/256)+1)*256 : oneBTP;

			offset = offset + oneBTP * 3;
			//offset = ((offset+255)&~255);

			if (offset > vb.size)
			{
				sa->bufSize = 0;
				return 0;
			}	
		}
	}

	//VC1 Overlap
	if (sa->useOvlEnable)
	{
		if (codStd != STD_VC1)
		{
			sa->useOvlEnable = 0;			
		}
		else
		{
			sa->bufOvlUse = vb.phys_addr + offset;
			sa->useOvlEnable = 1;

			offset = offset + MbNumX *  80;

			if (offset > vb.size)
			{
				sa->bufSize = 0;
				return 0;
			}		
		}		
	}

	sa->bufSize = offset;

	return 1;
}


PhysicalAddress GetTiledFrameBase(Uint32 coreIdx, FrameBuffer *frame, int num)
{
	PhysicalAddress baseAddr;
	int i;

	coreIdx = 0;	// as a unreferenced parameter
	baseAddr = frame[0].bufY;
	for (i=0; i<num; i++)
	{
		if (frame[i].bufY < baseAddr)
			baseAddr = frame[i].bufY;
	}

	return baseAddr;
}

void SetTiledFrameBase(Uint32 coreIdx, PhysicalAddress baseAddr)
{
	VpuWriteReg(coreIdx, GDI_TILEDBUF_BASE, baseAddr);
}
int SetTiledMapType(Uint32 coreIdx, TiledMapConfig *pMapCfg, DRAMConfig *dramCfg, int stride, int mapType)
{

#define XY2CONFIG(A,B,C,D,E,F,G,H,I) ((A)<<20 | (B)<<19 | (C)<<18 | (D)<<17 | (E)<<16 | (F)<<12 | (G)<<8 | (H)<<4 | (I))
#define XY2(A,B,C,D)                 ((A)<<12 | (B)<<8 | (C)<<4 | (D))
#define XY2BANK(A,B,C,D,E,F)         ((A)<<13 | (B)<<12 | (C)<<8 | (D)<<5 | (E)<<4 | (F))
#define RBC(A,B,C,D)                 ((A)<<10 | (B)<< 6 | (C)<<4 | (D))
#define RBC_SAME(A,B)                ((A)<<10 | (B)<< 6 | (A)<<4 | (B))
#define X_SEL 0
#define Y_SEL 1
#define CA_SEL 0
#define BA_SEL 1
#define RA_SEL 2
#define Z_SEL 3
	int ret; 
	int luma_map; 
	int chro_map;
	int i;	

	UNREFERENCED_PARAMETER(stride);
	pMapCfg->mapType = mapType;
	//         inv = 1'b0, zero = 1'b1 , tbxor = 1'b0, xy = 1'b0, bit = 4'd0
	luma_map = 64;
	chro_map = 64;

	for (i=0; i<16 ; i=i+1) {
		pMapCfg->xy2caMap[i] = luma_map << 8 | chro_map;
	}

	for (i=0; i<4;  i=i+1) {
		pMapCfg->xy2baMap[i] = luma_map << 8 | chro_map;
	}

	for (i=0; i<16; i=i+1) {
		pMapCfg->xy2raMap[i] = luma_map << 8 | chro_map;
	}

	ret = stride; // this will be removed after map size optimizing.
	ret = 0;
	switch(mapType)
	{
		case LINEAR_FRAME_MAP:
			pMapCfg->xy2rbcConfig = 0;
			ret = 1;
 			break;
		case TILED_FRAME_V_MAP:
			if (dramCfg->casBit == 9 && dramCfg->bankBit == 2 && dramCfg->rasBit == 13)	// CNN setting 
			{
				//cas
				pMapCfg->xy2caMap[0] = XY2(Y_SEL, 0, Y_SEL, 0);
				pMapCfg->xy2caMap[1] = XY2(Y_SEL, 1, Y_SEL, 1);
				pMapCfg->xy2caMap[2] = XY2(Y_SEL, 2, Y_SEL, 2);
				pMapCfg->xy2caMap[3] = XY2(Y_SEL, 3, Y_SEL, 3);
				pMapCfg->xy2caMap[4] = XY2(X_SEL, 3, X_SEL, 3);
				pMapCfg->xy2caMap[5] = XY2(X_SEL, 4, X_SEL, 4);
				pMapCfg->xy2caMap[6] = XY2(X_SEL, 5, X_SEL, 5);
				pMapCfg->xy2caMap[7] = XY2(X_SEL, 6, X_SEL, 6);
				pMapCfg->xy2caMap[8] = XY2(Y_SEL, 4, Y_SEL, 5);

				//bank
				pMapCfg->xy2baMap[0] = XY2BANK(0,X_SEL, 7, 4, X_SEL, 7);
				pMapCfg->xy2baMap[1] = XY2BANK(0,Y_SEL, 5, 4, Y_SEL, 4);

				//ras
				pMapCfg->xy2raMap[ 0] = XY2(X_SEL, 8, X_SEL, 8);
				pMapCfg->xy2raMap[ 1] = XY2(X_SEL, 9, X_SEL, 9);
				pMapCfg->xy2raMap[ 2] = XY2(X_SEL,10, X_SEL,10);
				pMapCfg->xy2raMap[ 3] = XY2(Y_SEL, 6, Y_SEL, 6);
				pMapCfg->xy2raMap[ 4] = XY2(Y_SEL, 7, Y_SEL, 7);
				pMapCfg->xy2raMap[ 5] = XY2(Y_SEL, 8, Y_SEL, 8);
				pMapCfg->xy2raMap[ 6] = XY2(Y_SEL, 9, Y_SEL, 9);
				pMapCfg->xy2raMap[ 7] = XY2(Y_SEL,10, Y_SEL,10);
				pMapCfg->xy2raMap[ 8] = XY2(Y_SEL,11, Y_SEL,11);
				pMapCfg->xy2raMap[ 9] = XY2(Y_SEL,12, Y_SEL,12);
				pMapCfg->xy2raMap[10] = XY2(Y_SEL,13, Y_SEL,13);
				pMapCfg->xy2raMap[11] = XY2(Y_SEL,14, Y_SEL,14);
				pMapCfg->xy2raMap[12] = XY2(Y_SEL,15, Y_SEL,15);
				
			}
			else if(dramCfg->casBit == 10 && dramCfg->bankBit == 3 && dramCfg->rasBit == 13)
			{
				//cas
				pMapCfg->xy2caMap[0] = XY2(Z_SEL, 0, Z_SEL, 0);
				pMapCfg->xy2caMap[1] = XY2(Y_SEL, 0, Y_SEL, 0);
				pMapCfg->xy2caMap[2] = XY2(Y_SEL, 1, Y_SEL, 1);
				pMapCfg->xy2caMap[3] = XY2(Y_SEL, 2, Y_SEL, 2);
				pMapCfg->xy2caMap[4] = XY2(Y_SEL, 3, Y_SEL, 3);
				pMapCfg->xy2caMap[5] = XY2(X_SEL, 3, X_SEL, 3);
				pMapCfg->xy2caMap[6] = XY2(X_SEL, 4, X_SEL, 4);
				pMapCfg->xy2caMap[7] = XY2(X_SEL, 5, X_SEL, 5);
				pMapCfg->xy2caMap[8] = XY2(X_SEL, 6, X_SEL, 6);
				pMapCfg->xy2caMap[9] = XY2(Y_SEL, 4, Y_SEL, 4);

				//bank
				pMapCfg->xy2baMap[0] = XY2BANK(0,X_SEL, 7, 4, X_SEL, 7); 
				pMapCfg->xy2baMap[1] = XY2BANK(0,X_SEL, 8, 4, X_SEL, 8); 
				pMapCfg->xy2baMap[2] = XY2BANK(0,Y_SEL, 5, 4, Y_SEL, 5); 

				//ras
					pMapCfg->xy2raMap[ 0] = XY2(X_SEL, 9, X_SEL, 9);
					pMapCfg->xy2raMap[ 1] = XY2(X_SEL,10, X_SEL,10);
					pMapCfg->xy2raMap[ 2] = XY2(Y_SEL, 6, Y_SEL, 6);
					pMapCfg->xy2raMap[ 3] = XY2(Y_SEL, 7, Y_SEL, 7);
					pMapCfg->xy2raMap[ 4] = XY2(Y_SEL, 8, Y_SEL, 8);
					pMapCfg->xy2raMap[ 5] = XY2(Y_SEL, 9, Y_SEL, 9);
					pMapCfg->xy2raMap[ 6] = XY2(Y_SEL,10, Y_SEL,10);
					pMapCfg->xy2raMap[ 7] = XY2(Y_SEL,11, Y_SEL,11);
					pMapCfg->xy2raMap[ 8] = XY2(Y_SEL,12, Y_SEL,12);
					pMapCfg->xy2raMap[ 9] = XY2(Y_SEL,13, Y_SEL,13);
					pMapCfg->xy2raMap[10] = XY2(Y_SEL,14, Y_SEL,14);
					pMapCfg->xy2raMap[11] = XY2(Y_SEL,15, Y_SEL,15);
			}


			//xy2rbcConfig
			pMapCfg->xy2rbcConfig = XY2CONFIG(0,0,0,1,1,15,0,15,0);
			break;
		case TILED_FRAME_H_MAP:
			if (dramCfg->casBit == 9 && dramCfg->bankBit == 2 && dramCfg->rasBit == 13) // CNN setting 
			{
				//cas
				pMapCfg->xy2caMap[0] = XY2(X_SEL, 3, X_SEL, 3);
				pMapCfg->xy2caMap[1] = XY2(X_SEL, 4, X_SEL, 4);
				pMapCfg->xy2caMap[2] = XY2(X_SEL, 5, X_SEL, 5);
				pMapCfg->xy2caMap[3] = XY2(X_SEL, 6, X_SEL, 6);
				pMapCfg->xy2caMap[4] = XY2(Y_SEL, 0, Y_SEL, 0);
				pMapCfg->xy2caMap[5] = XY2(Y_SEL, 1, Y_SEL, 1);
				pMapCfg->xy2caMap[6] = XY2(Y_SEL, 2, Y_SEL, 2);
				pMapCfg->xy2caMap[7] = XY2(Y_SEL, 3, Y_SEL, 3);
				pMapCfg->xy2caMap[8] = XY2(Y_SEL, 4, Y_SEL, 5);

				//bank
				pMapCfg->xy2baMap[0] = XY2BANK(0,X_SEL, 7, 4,X_SEL, 7);
				pMapCfg->xy2baMap[1] = XY2BANK(0,Y_SEL, 5, 4,Y_SEL, 4);

				//ras
				pMapCfg->xy2raMap[ 0] = XY2(X_SEL, 8, X_SEL, 8);
				pMapCfg->xy2raMap[ 1] = XY2(X_SEL, 9, X_SEL, 9);
				pMapCfg->xy2raMap[ 2] = XY2(X_SEL,10, X_SEL,10);
				pMapCfg->xy2raMap[ 3] = XY2(Y_SEL, 6, Y_SEL, 6);
				pMapCfg->xy2raMap[ 4] = XY2(Y_SEL, 7, Y_SEL, 7);
				pMapCfg->xy2raMap[ 5] = XY2(Y_SEL, 8, Y_SEL, 8);
				pMapCfg->xy2raMap[ 6] = XY2(Y_SEL, 9, Y_SEL, 9);
				pMapCfg->xy2raMap[ 7] = XY2(Y_SEL,10, Y_SEL,10);
				pMapCfg->xy2raMap[ 8] = XY2(Y_SEL,11, Y_SEL,11);
				pMapCfg->xy2raMap[ 9] = XY2(Y_SEL,12, Y_SEL,12);
				pMapCfg->xy2raMap[10] = XY2(Y_SEL,13, Y_SEL,13);
				pMapCfg->xy2raMap[11] = XY2(Y_SEL,14, Y_SEL,14);
				pMapCfg->xy2raMap[12] = XY2(Y_SEL,15, Y_SEL,15);
				
			}
			else if(dramCfg->casBit == 10 && dramCfg->bankBit == 3 && dramCfg->rasBit == 13) 
			{
				//cas
				pMapCfg->xy2caMap[0] = XY2(Z_SEL, 0, Z_SEL, 0);
				pMapCfg->xy2caMap[1] = XY2(X_SEL, 3, X_SEL, 3);
				pMapCfg->xy2caMap[2] = XY2(X_SEL, 4, X_SEL, 4);
				pMapCfg->xy2caMap[3] = XY2(X_SEL, 5, X_SEL, 5);
				pMapCfg->xy2caMap[4] = XY2(X_SEL, 6, X_SEL, 6);
				pMapCfg->xy2caMap[5] = XY2(Y_SEL, 0, Y_SEL, 0);
				pMapCfg->xy2caMap[6] = XY2(Y_SEL, 1, Y_SEL, 1);
				pMapCfg->xy2caMap[7] = XY2(Y_SEL, 2, Y_SEL, 2);
				pMapCfg->xy2caMap[8] = XY2(Y_SEL, 3, Y_SEL, 3);
				pMapCfg->xy2caMap[9] = XY2(Y_SEL, 4, Y_SEL, 4);

				//bank
				pMapCfg->xy2baMap[0] = XY2BANK(0,X_SEL, 7, 4, X_SEL, 7); 
				pMapCfg->xy2baMap[1] = XY2BANK(0,X_SEL, 8, 4, X_SEL, 8); 
				pMapCfg->xy2baMap[2] = XY2BANK(0,Y_SEL, 5, 4, Y_SEL, 5); 
					pMapCfg->xy2raMap[ 0] = XY2(X_SEL, 9, X_SEL, 9);
					pMapCfg->xy2raMap[ 1] = XY2(X_SEL,10, X_SEL,10);
					pMapCfg->xy2raMap[ 2] = XY2(Y_SEL, 6, Y_SEL, 6);
					pMapCfg->xy2raMap[ 3] = XY2(Y_SEL, 7, Y_SEL, 7);
					pMapCfg->xy2raMap[ 4] = XY2(Y_SEL, 8, Y_SEL, 8);
					pMapCfg->xy2raMap[ 5] = XY2(Y_SEL, 9, Y_SEL, 9);
					pMapCfg->xy2raMap[ 6] = XY2(Y_SEL,10, Y_SEL,10);
					pMapCfg->xy2raMap[ 7] = XY2(Y_SEL,11, Y_SEL,11);
					pMapCfg->xy2raMap[ 8] = XY2(Y_SEL,12, Y_SEL,12);
					pMapCfg->xy2raMap[ 9] = XY2(Y_SEL,13, Y_SEL,13);
					pMapCfg->xy2raMap[10] = XY2(Y_SEL,14, Y_SEL,14);
					pMapCfg->xy2raMap[11] = XY2(Y_SEL,15, Y_SEL,15);
			}
			//xy2rbcConfig
			pMapCfg->xy2rbcConfig = XY2CONFIG(0,0,0,1,0,15,15,15,15);
			break;
		case TILED_FIELD_V_MAP:
			if (dramCfg->casBit == 9 && dramCfg->bankBit == 2 && dramCfg->rasBit == 13) // CNN setting 
			{
				//cas
				pMapCfg->xy2caMap[0] = XY2(Y_SEL, 0, Y_SEL, 0);
				pMapCfg->xy2caMap[1] = XY2(Y_SEL, 1, Y_SEL, 1);
				pMapCfg->xy2caMap[2] = XY2(Y_SEL, 2, Y_SEL, 2);
				pMapCfg->xy2caMap[3] = XY2(Y_SEL, 3, Y_SEL, 3);
				pMapCfg->xy2caMap[4] = XY2(X_SEL, 3, X_SEL, 3);
				pMapCfg->xy2caMap[5] = XY2(X_SEL, 4, X_SEL, 4);
				pMapCfg->xy2caMap[6] = XY2(X_SEL, 5, X_SEL, 5);
				pMapCfg->xy2caMap[7] = XY2(X_SEL, 6, X_SEL, 6);
				pMapCfg->xy2caMap[8] = XY2(Y_SEL, 4, Y_SEL, 5);

				//bank
				pMapCfg->xy2baMap[0] = XY2BANK(0,X_SEL, 7, 4,X_SEL, 7);
				pMapCfg->xy2baMap[1] = XY2BANK(1,Y_SEL, 5, 5,Y_SEL, 4);

				//ras
				pMapCfg->xy2raMap[ 0] = XY2(X_SEL, 8, X_SEL, 8);
				pMapCfg->xy2raMap[ 1] = XY2(X_SEL, 9, X_SEL, 9);
				pMapCfg->xy2raMap[ 2] = XY2(X_SEL,10, X_SEL,10);
				pMapCfg->xy2raMap[ 3] = XY2(Y_SEL, 6, Y_SEL, 6);
				pMapCfg->xy2raMap[ 4] = XY2(Y_SEL, 7, Y_SEL, 7);
				pMapCfg->xy2raMap[ 5] = XY2(Y_SEL, 8, Y_SEL, 8);
				pMapCfg->xy2raMap[ 6] = XY2(Y_SEL, 9, Y_SEL, 9);
				pMapCfg->xy2raMap[ 7] = XY2(Y_SEL,10, Y_SEL,10);
				pMapCfg->xy2raMap[ 8] = XY2(Y_SEL,11, Y_SEL,11);
				pMapCfg->xy2raMap[ 9] = XY2(Y_SEL,12, Y_SEL,12);
				pMapCfg->xy2raMap[10] = XY2(Y_SEL,13, Y_SEL,13);
				pMapCfg->xy2raMap[11] = XY2(Y_SEL,14, Y_SEL,14);
				pMapCfg->xy2raMap[12] = XY2(Y_SEL,15, Y_SEL,15);
			}
			else if(dramCfg->casBit == 10 && dramCfg->bankBit == 3 && dramCfg->rasBit == 13)
			{
				//cas
				pMapCfg->xy2caMap[0] = XY2(Z_SEL, 0, Z_SEL, 0);
				pMapCfg->xy2caMap[1] = XY2(Y_SEL, 0, Y_SEL, 0);
				pMapCfg->xy2caMap[2] = XY2(Y_SEL, 1, Y_SEL, 1);
				pMapCfg->xy2caMap[3] = XY2(Y_SEL, 2, Y_SEL, 2);
				pMapCfg->xy2caMap[4] = XY2(Y_SEL, 3, Y_SEL, 3);
				pMapCfg->xy2caMap[5] = XY2(X_SEL, 3, X_SEL, 3);
				pMapCfg->xy2caMap[6] = XY2(X_SEL, 4, X_SEL, 4);
				pMapCfg->xy2caMap[7] = XY2(X_SEL, 5, X_SEL, 5);
				pMapCfg->xy2caMap[8] = XY2(X_SEL, 6, X_SEL, 6);
				pMapCfg->xy2caMap[9] = XY2(Y_SEL, 4, Y_SEL, 4);

				//bank
				pMapCfg->xy2baMap[0] = XY2BANK(0,X_SEL, 7, 4, X_SEL, 7); 
				pMapCfg->xy2baMap[1] = XY2BANK(0,X_SEL, 8, 4, X_SEL, 8); 
				pMapCfg->xy2baMap[2] = XY2BANK(0,Y_SEL, 5, 4, Y_SEL, 5); 
					pMapCfg->xy2raMap[ 0] = XY2(X_SEL, 9, X_SEL, 9);
					pMapCfg->xy2raMap[ 1] = XY2(X_SEL,10, X_SEL,10);
					pMapCfg->xy2raMap[ 2] = XY2(Y_SEL, 6, Y_SEL, 6);
					pMapCfg->xy2raMap[ 3] = XY2(Y_SEL, 7, Y_SEL, 7);
					pMapCfg->xy2raMap[ 4] = XY2(Y_SEL, 8, Y_SEL, 8);
					pMapCfg->xy2raMap[ 5] = XY2(Y_SEL, 9, Y_SEL, 9);
					pMapCfg->xy2raMap[ 6] = XY2(Y_SEL,10, Y_SEL,10);
					pMapCfg->xy2raMap[ 7] = XY2(Y_SEL,11, Y_SEL,11);
					pMapCfg->xy2raMap[ 8] = XY2(Y_SEL,12, Y_SEL,12);
					pMapCfg->xy2raMap[ 9] = XY2(Y_SEL,13, Y_SEL,13);
					pMapCfg->xy2raMap[10] = XY2(Y_SEL,14, Y_SEL,14);
					pMapCfg->xy2raMap[11] = XY2(Y_SEL,15, Y_SEL,15);
			}
			

			//xy2rbcConfig
			pMapCfg->xy2rbcConfig = XY2CONFIG(0,1,1,1,1,15,15,15,15);
			break;
		case TILED_MIXED_V_MAP:
			//cas
			if (dramCfg->casBit == 9 && dramCfg->bankBit == 2 && dramCfg->rasBit == 13) // CNN setting 
			{
				pMapCfg->xy2caMap[0] = XY2(Y_SEL, 1, Y_SEL, 1);
				pMapCfg->xy2caMap[1] = XY2(Y_SEL, 2, Y_SEL, 2);
				pMapCfg->xy2caMap[2] = XY2(Y_SEL, 3, Y_SEL, 3);
				pMapCfg->xy2caMap[3] = XY2(Y_SEL, 0, Y_SEL, 0);
				pMapCfg->xy2caMap[4] = XY2(X_SEL, 3, X_SEL, 3);
				pMapCfg->xy2caMap[5] = XY2(X_SEL, 4, X_SEL, 4);
				pMapCfg->xy2caMap[6] = XY2(X_SEL, 5, X_SEL, 5);
				pMapCfg->xy2caMap[7] = XY2(X_SEL, 6, X_SEL, 6);
				pMapCfg->xy2caMap[8] = XY2(Y_SEL, 4, Y_SEL, 5);

				pMapCfg->xy2baMap[0] = XY2BANK(0,X_SEL, 7, 4,X_SEL, 7);
				pMapCfg->xy2baMap[1] = XY2BANK(0,Y_SEL, 5, 4,Y_SEL, 4);

				pMapCfg->xy2raMap[ 0] = XY2(X_SEL, 8, X_SEL, 8);
				pMapCfg->xy2raMap[ 1] = XY2(X_SEL, 9, X_SEL, 9);
				pMapCfg->xy2raMap[ 2] = XY2(X_SEL,10, X_SEL,10);
				pMapCfg->xy2raMap[ 3] = XY2(Y_SEL, 6, Y_SEL, 6);
				pMapCfg->xy2raMap[ 4] = XY2(Y_SEL, 7, Y_SEL, 7);
				pMapCfg->xy2raMap[ 5] = XY2(Y_SEL, 8, Y_SEL, 8);
				pMapCfg->xy2raMap[ 6] = XY2(Y_SEL, 9, Y_SEL, 9);
				pMapCfg->xy2raMap[ 7] = XY2(Y_SEL,10, Y_SEL,10);
				pMapCfg->xy2raMap[ 8] = XY2(Y_SEL,11, Y_SEL,11);
				pMapCfg->xy2raMap[ 9] = XY2(Y_SEL,12, Y_SEL,12);
				pMapCfg->xy2raMap[10] = XY2(Y_SEL,13, Y_SEL,13);
				pMapCfg->xy2raMap[11] = XY2(Y_SEL,14, Y_SEL,14);
				pMapCfg->xy2raMap[12] = XY2(Y_SEL,15, Y_SEL,15);
			}
			else if(dramCfg->casBit == 10 && dramCfg->bankBit == 3 && dramCfg->rasBit == 13)
			{
				//cas
				pMapCfg->xy2caMap[0] = XY2(Z_SEL, 0, Z_SEL, 0);
				pMapCfg->xy2caMap[1] = XY2(Y_SEL, 1, Y_SEL, 1);
				pMapCfg->xy2caMap[2] = XY2(Y_SEL, 2, Y_SEL, 2);
				pMapCfg->xy2caMap[3] = XY2(Y_SEL, 3, Y_SEL, 3);
				pMapCfg->xy2caMap[4] = XY2(Y_SEL, 0, Y_SEL, 0);
				pMapCfg->xy2caMap[5] = XY2(X_SEL, 3, X_SEL, 3);
				pMapCfg->xy2caMap[6] = XY2(X_SEL, 4, X_SEL, 4);
				pMapCfg->xy2caMap[7] = XY2(X_SEL, 5, X_SEL, 5);
				pMapCfg->xy2caMap[8] = XY2(X_SEL, 6, X_SEL, 6);
				pMapCfg->xy2caMap[9] = XY2(Y_SEL, 4, Y_SEL, 4);
				
				//bank
				pMapCfg->xy2baMap[0] = XY2BANK(0,X_SEL, 7, 4, X_SEL, 7); 
				pMapCfg->xy2baMap[1] = XY2BANK(0,X_SEL, 8, 4, X_SEL, 8); 
				pMapCfg->xy2baMap[2] = XY2BANK(0,Y_SEL, 5, 4, Y_SEL, 5); 
					pMapCfg->xy2raMap[ 0] = XY2(X_SEL, 9, X_SEL, 9);
					pMapCfg->xy2raMap[ 1] = XY2(X_SEL,10, X_SEL,10);
					pMapCfg->xy2raMap[ 2] = XY2(Y_SEL, 6, Y_SEL, 6);
					pMapCfg->xy2raMap[ 3] = XY2(Y_SEL, 7, Y_SEL, 7);
					pMapCfg->xy2raMap[ 4] = XY2(Y_SEL, 8, Y_SEL, 8);
					pMapCfg->xy2raMap[ 5] = XY2(Y_SEL, 9, Y_SEL, 9);
					pMapCfg->xy2raMap[ 6] = XY2(Y_SEL,10, Y_SEL,10);
					pMapCfg->xy2raMap[ 7] = XY2(Y_SEL,11, Y_SEL,11);
					pMapCfg->xy2raMap[ 8] = XY2(Y_SEL,12, Y_SEL,12);
					pMapCfg->xy2raMap[ 9] = XY2(Y_SEL,13, Y_SEL,13);
					pMapCfg->xy2raMap[10] = XY2(Y_SEL,14, Y_SEL,14);
					pMapCfg->xy2raMap[11] = XY2(Y_SEL,15, Y_SEL,15);
			}
		
			//xy2rbcConfig
			pMapCfg->xy2rbcConfig = XY2CONFIG(0,0,1,1,1,7,7,7,7);
			break;
		case TILED_FRAME_MB_RASTER_MAP:
			//cas
			pMapCfg->xy2caMap[0] = XY2(Y_SEL, 0, Y_SEL, 0);
			pMapCfg->xy2caMap[1] = XY2(Y_SEL, 1, Y_SEL, 1);
			pMapCfg->xy2caMap[2] = XY2(Y_SEL, 2, Y_SEL, 2);
			pMapCfg->xy2caMap[3] = XY2(Y_SEL, 3, X_SEL, 3);
			pMapCfg->xy2caMap[4] = XY2(X_SEL, 3, 4    , 0);

			//xy2rbcConfig
			pMapCfg->xy2rbcConfig = XY2CONFIG(0,0,0,1,1,15,0,7,0);
			break;
		case TILED_FIELD_MB_RASTER_MAP:
			//cas
			pMapCfg->xy2caMap[0] = XY2(Y_SEL, 0, Y_SEL, 0);
			pMapCfg->xy2caMap[1] = XY2(Y_SEL, 1, Y_SEL, 1);
			pMapCfg->xy2caMap[2] = XY2(Y_SEL, 2, X_SEL, 3);
			pMapCfg->xy2caMap[3] = XY2(X_SEL, 3, 4    , 0);

			//xy2rbcConfig
			pMapCfg->xy2rbcConfig = XY2CONFIG(0,1,1,1,1,7,7,3,3);	
			break;
		default:
			return 0;
			break;
	}
	
	if (mapType == TILED_FRAME_MB_RASTER_MAP)
	{
		pMapCfg->rbc2axiMap[ 0] = RBC( Z_SEL, 0,  Z_SEL, 0);
		pMapCfg->rbc2axiMap[ 1] = RBC( Z_SEL, 0,  Z_SEL, 0);
		pMapCfg->rbc2axiMap[ 2] = RBC( Z_SEL, 0,  Z_SEL, 0);
		pMapCfg->rbc2axiMap[ 3] = RBC(CA_SEL, 0, CA_SEL, 0);
		pMapCfg->rbc2axiMap[ 4] = RBC(CA_SEL, 1, CA_SEL, 1);
		pMapCfg->rbc2axiMap[ 5] = RBC(CA_SEL, 2, CA_SEL, 2);
		pMapCfg->rbc2axiMap[ 6] = RBC(CA_SEL, 3, CA_SEL, 3);
		pMapCfg->rbc2axiMap[ 7] = RBC(CA_SEL, 4, CA_SEL, 8);
		pMapCfg->rbc2axiMap[ 8] = RBC(CA_SEL, 8, CA_SEL, 9);
		pMapCfg->rbc2axiMap[ 9] = RBC(CA_SEL, 9, CA_SEL,10);
		pMapCfg->rbc2axiMap[10] = RBC(CA_SEL,10, CA_SEL,11);
		pMapCfg->rbc2axiMap[11] = RBC(CA_SEL,11, CA_SEL,12);
		pMapCfg->rbc2axiMap[12] = RBC(CA_SEL,12, CA_SEL,13);
		pMapCfg->rbc2axiMap[13] = RBC(CA_SEL,13, CA_SEL,14);
		pMapCfg->rbc2axiMap[14] = RBC(CA_SEL,14, CA_SEL,15);
		pMapCfg->rbc2axiMap[15] = RBC(CA_SEL,15, RA_SEL, 0);
		pMapCfg->rbc2axiMap[16] = RBC(RA_SEL, 0, RA_SEL, 1);
		pMapCfg->rbc2axiMap[17] = RBC(RA_SEL, 1, RA_SEL, 2);
		pMapCfg->rbc2axiMap[18] = RBC(RA_SEL, 2, RA_SEL, 3);
		pMapCfg->rbc2axiMap[19] = RBC(RA_SEL, 3, RA_SEL, 4);
		pMapCfg->rbc2axiMap[20] = RBC(RA_SEL, 4, RA_SEL, 5);
		pMapCfg->rbc2axiMap[21] = RBC(RA_SEL, 5, RA_SEL, 6);
		pMapCfg->rbc2axiMap[22] = RBC(RA_SEL, 6, RA_SEL, 7);
		pMapCfg->rbc2axiMap[23] = RBC(RA_SEL, 7, RA_SEL, 8);
		pMapCfg->rbc2axiMap[24] = RBC(RA_SEL, 8, RA_SEL, 9);
		pMapCfg->rbc2axiMap[25] = RBC(RA_SEL, 9, RA_SEL,10);
		pMapCfg->rbc2axiMap[26] = RBC(RA_SEL,10, RA_SEL,11);
		pMapCfg->rbc2axiMap[27] = RBC(RA_SEL,11, RA_SEL,12);
		pMapCfg->rbc2axiMap[28] = RBC(RA_SEL,12, RA_SEL,13);
		pMapCfg->rbc2axiMap[29] = RBC(RA_SEL,13, RA_SEL,14);
		pMapCfg->rbc2axiMap[30] = RBC(RA_SEL,14, RA_SEL,15);
		pMapCfg->rbc2axiMap[31] = RBC(RA_SEL,15,  Z_SEL, 0);
		ret = 1;
	} 
	else if (mapType == TILED_FIELD_MB_RASTER_MAP) 
	{
		pMapCfg->rbc2axiMap[ 0] = RBC(Z_SEL ,0  ,Z_SEL , 0);
		pMapCfg->rbc2axiMap[ 1] = RBC(Z_SEL ,0  ,Z_SEL , 0);
		pMapCfg->rbc2axiMap[ 2] = RBC(Z_SEL ,0  ,Z_SEL , 0);
		pMapCfg->rbc2axiMap[ 3] = RBC(CA_SEL,0  ,CA_SEL, 0);
		pMapCfg->rbc2axiMap[ 4] = RBC(CA_SEL,1  ,CA_SEL, 1);
		pMapCfg->rbc2axiMap[ 5] = RBC(CA_SEL,2  ,CA_SEL, 2);
		pMapCfg->rbc2axiMap[ 6] = RBC(CA_SEL,3  ,CA_SEL, 8);
		pMapCfg->rbc2axiMap[ 7] = RBC(CA_SEL,8,  CA_SEL, 9);
		pMapCfg->rbc2axiMap[ 8] = RBC(CA_SEL,9,  CA_SEL,10);
		pMapCfg->rbc2axiMap[ 9] = RBC(CA_SEL,10 ,CA_SEL,11);
		pMapCfg->rbc2axiMap[10] = RBC(CA_SEL,11 ,CA_SEL,12);
		pMapCfg->rbc2axiMap[11] = RBC(CA_SEL,12 ,CA_SEL,13);
		pMapCfg->rbc2axiMap[12] = RBC(CA_SEL,13 ,CA_SEL,14);
		pMapCfg->rbc2axiMap[13] = RBC(CA_SEL,14 ,CA_SEL,15);
		pMapCfg->rbc2axiMap[14] = RBC(CA_SEL,15 ,RA_SEL, 0);

		pMapCfg->rbc2axiMap[15] = RBC(RA_SEL,0  ,RA_SEL, 1);
		pMapCfg->rbc2axiMap[16] = RBC(RA_SEL,1  ,RA_SEL, 2);
		pMapCfg->rbc2axiMap[17] = RBC(RA_SEL,2  ,RA_SEL, 3);
		pMapCfg->rbc2axiMap[18] = RBC(RA_SEL,3  ,RA_SEL, 4);
		pMapCfg->rbc2axiMap[19] = RBC(RA_SEL,4  ,RA_SEL, 5);
		pMapCfg->rbc2axiMap[20] = RBC(RA_SEL,5  ,RA_SEL, 6);
		pMapCfg->rbc2axiMap[21] = RBC(RA_SEL,6  ,RA_SEL, 7);
		pMapCfg->rbc2axiMap[22] = RBC(RA_SEL,7  ,RA_SEL, 8);
		pMapCfg->rbc2axiMap[23] = RBC(RA_SEL,8  ,RA_SEL, 9);
		pMapCfg->rbc2axiMap[24] = RBC(RA_SEL,9  ,RA_SEL,10);
		pMapCfg->rbc2axiMap[25] = RBC(RA_SEL,10 ,RA_SEL,11);
		pMapCfg->rbc2axiMap[26] = RBC(RA_SEL,11 ,RA_SEL,12);
		pMapCfg->rbc2axiMap[27] = RBC(RA_SEL,12 ,RA_SEL,13);
		pMapCfg->rbc2axiMap[28] = RBC(RA_SEL,13 ,RA_SEL,14);
		pMapCfg->rbc2axiMap[29] = RBC(RA_SEL,14 ,RA_SEL,15);
		pMapCfg->rbc2axiMap[30] = RBC(RA_SEL,15 , Z_SEL, 0);
		pMapCfg->rbc2axiMap[31] = RBC(Z_SEL , 0 , Z_SEL, 0);
		ret = 1;
	} 
	else 
	{
		if (dramCfg->casBit == 9 && dramCfg->bankBit == 2 && dramCfg->rasBit == 13)
		{
			pMapCfg->rbc2axiMap[ 0] = RBC(Z_SEL,0, Z_SEL,0);
			pMapCfg->rbc2axiMap[ 1] = RBC(Z_SEL,0, Z_SEL,0);
			pMapCfg->rbc2axiMap[ 2] = RBC(Z_SEL,0, Z_SEL,0);
			pMapCfg->rbc2axiMap[ 3] = RBC(CA_SEL,0,CA_SEL,0);
			pMapCfg->rbc2axiMap[ 4] = RBC(CA_SEL,1,CA_SEL,1);
			pMapCfg->rbc2axiMap[ 5] = RBC(CA_SEL,2,CA_SEL,2);
			pMapCfg->rbc2axiMap[ 6] = RBC(CA_SEL,3,CA_SEL,3);
			pMapCfg->rbc2axiMap[ 7] = RBC(CA_SEL,4,CA_SEL,4);
			pMapCfg->rbc2axiMap[ 8] = RBC(CA_SEL,5,CA_SEL,5);
			pMapCfg->rbc2axiMap[ 9] = RBC(CA_SEL,6,CA_SEL,6);
			pMapCfg->rbc2axiMap[10] = RBC(CA_SEL,7,CA_SEL,7);
			pMapCfg->rbc2axiMap[11] = RBC(CA_SEL,8,CA_SEL,8);
	
			pMapCfg->rbc2axiMap[12] = RBC(BA_SEL,0, BA_SEL,0);
			pMapCfg->rbc2axiMap[13] = RBC(BA_SEL,1, BA_SEL,1);
		
			pMapCfg->rbc2axiMap[14] = RBC(RA_SEL,0, RA_SEL, 0);
			pMapCfg->rbc2axiMap[15] = RBC(RA_SEL,1, RA_SEL, 1);
			pMapCfg->rbc2axiMap[16] = RBC(RA_SEL,2 ,RA_SEL, 2);
			pMapCfg->rbc2axiMap[17] = RBC(RA_SEL,3 ,RA_SEL, 3);
			pMapCfg->rbc2axiMap[18] = RBC(RA_SEL,4 ,RA_SEL, 4);
			pMapCfg->rbc2axiMap[19] = RBC(RA_SEL,5 ,RA_SEL, 5);
			pMapCfg->rbc2axiMap[20] = RBC(RA_SEL,6 ,RA_SEL, 6);
			pMapCfg->rbc2axiMap[21] = RBC(RA_SEL,7 ,RA_SEL, 7);
			pMapCfg->rbc2axiMap[22] = RBC(RA_SEL,8 ,RA_SEL, 8);
			pMapCfg->rbc2axiMap[23] = RBC(RA_SEL,9 ,RA_SEL, 9);
			pMapCfg->rbc2axiMap[24] = RBC(RA_SEL,10,RA_SEL,10);
			pMapCfg->rbc2axiMap[25] = RBC(RA_SEL,11,RA_SEL,11);
			pMapCfg->rbc2axiMap[26] = RBC(RA_SEL,12,RA_SEL,12);
			pMapCfg->rbc2axiMap[27] = RBC(RA_SEL,13,RA_SEL,13);
			pMapCfg->rbc2axiMap[28] = RBC(RA_SEL,14,RA_SEL,14);
			pMapCfg->rbc2axiMap[29] = RBC(RA_SEL,15,RA_SEL,15);
			pMapCfg->rbc2axiMap[30] = RBC(Z_SEL , 0, Z_SEL, 0);
			pMapCfg->rbc2axiMap[31] = RBC(Z_SEL , 0, Z_SEL, 0);

			ret = 1;
		}
		else if(dramCfg->casBit == 10 && dramCfg->bankBit == 3 && dramCfg->rasBit == 13)
		{
			pMapCfg->rbc2axiMap[ 0] = RBC(Z_SEL, 0, Z_SEL,0);
			pMapCfg->rbc2axiMap[ 1] = RBC(Z_SEL, 0, Z_SEL,0);
			pMapCfg->rbc2axiMap[ 2] = RBC(CA_SEL,0,CA_SEL,0);
			pMapCfg->rbc2axiMap[ 3] = RBC(CA_SEL,1,CA_SEL,1);
			pMapCfg->rbc2axiMap[ 4] = RBC(CA_SEL,2,CA_SEL,2);
			pMapCfg->rbc2axiMap[ 5] = RBC(CA_SEL,3,CA_SEL,3);
			pMapCfg->rbc2axiMap[ 6] = RBC(CA_SEL,4,CA_SEL,4);
			pMapCfg->rbc2axiMap[ 7] = RBC(CA_SEL,5,CA_SEL,5);
			pMapCfg->rbc2axiMap[ 8] = RBC(CA_SEL,6,CA_SEL,6);
			pMapCfg->rbc2axiMap[ 9] = RBC(CA_SEL,7,CA_SEL,7);
			pMapCfg->rbc2axiMap[10] = RBC(CA_SEL,8,CA_SEL,8);
			pMapCfg->rbc2axiMap[11] = RBC(CA_SEL,9,CA_SEL,9);

			pMapCfg->rbc2axiMap[12] = RBC(BA_SEL,0, BA_SEL,0);
			pMapCfg->rbc2axiMap[13] = RBC(BA_SEL,1, BA_SEL,1);
			pMapCfg->rbc2axiMap[14] = RBC(BA_SEL,2, BA_SEL,2);

			pMapCfg->rbc2axiMap[15] = RBC(RA_SEL, 0, RA_SEL, 0);
			pMapCfg->rbc2axiMap[16] = RBC(RA_SEL, 1 ,RA_SEL, 1);
			pMapCfg->rbc2axiMap[17] = RBC(RA_SEL, 2 ,RA_SEL, 2);
			pMapCfg->rbc2axiMap[18] = RBC(RA_SEL, 3 ,RA_SEL, 3);
			pMapCfg->rbc2axiMap[19] = RBC(RA_SEL, 4 ,RA_SEL, 4);
			pMapCfg->rbc2axiMap[20] = RBC(RA_SEL, 5 ,RA_SEL, 5);
			pMapCfg->rbc2axiMap[21] = RBC(RA_SEL, 6 ,RA_SEL, 6);
			pMapCfg->rbc2axiMap[22] = RBC(RA_SEL, 7 ,RA_SEL, 7);
			pMapCfg->rbc2axiMap[23] = RBC(RA_SEL, 8 ,RA_SEL, 8);
			pMapCfg->rbc2axiMap[24] = RBC(RA_SEL, 9, RA_SEL, 9);
			pMapCfg->rbc2axiMap[25] = RBC(RA_SEL,10, RA_SEL,10);
			pMapCfg->rbc2axiMap[26] = RBC(RA_SEL,11, RA_SEL,11);
			pMapCfg->rbc2axiMap[27] = RBC(RA_SEL,12, RA_SEL,12);
			pMapCfg->rbc2axiMap[28] = RBC(Z_SEL , 0, Z_SEL , 0);
			pMapCfg->rbc2axiMap[29] = RBC(Z_SEL , 0, Z_SEL , 0);
			pMapCfg->rbc2axiMap[30] = RBC(Z_SEL , 0, Z_SEL , 0);
			pMapCfg->rbc2axiMap[31] = RBC(Z_SEL , 0, Z_SEL , 0);

			ret = 1;
		}
	}
		
	for (i=0; i<16; i++) { //xy2ca_map		
		VpuWriteReg(coreIdx, GDI_XY2_CAS_0 + 4*i, pMapCfg->xy2caMap[i]);	
		//printf("WriteReg(0x%x,0x%04x);\n",GDI_XY2_CAS_0 + 4*i,pMapCfg->xy2ca_map[i]);
	}

	for (i=0; i<4; i++) { //xy2baMap		
		VpuWriteReg(coreIdx, GDI_XY2_BA_0  + 4*i, pMapCfg->xy2baMap[i]);
		//printf("WriteReg(0x%x,0x%04x);\n",GDI_XY2_BA_0 + 4*i,pMapCfg->xy2baMap[i]);
	}

	for (i=0; i<16; i++) { //xy2raMap
		VpuWriteReg(coreIdx, GDI_XY2_RAS_0 + 4*i, pMapCfg->xy2raMap[i]);		
		//printf("WriteReg(0x%x,0x%04x);\n",GDI_XY2_RAS_0 + 4*i,pMapCfg->xy2raMap[i]);
	}

	//xy2rbcConfig
	VpuWriteReg(coreIdx, GDI_XY2_RBC_CONFIG,pMapCfg->xy2rbcConfig);
	//printf("WriteReg(0x%x,0x%04x);\n",GDI_XY2_RBC_CONFIG,pMapCfg->xy2rbcConfig);
	//// fast access for reading
	pMapCfg->tbSeparateMap  = (pMapCfg->xy2rbcConfig >> 19) & 0x1;
	pMapCfg->topBotSplit    = (pMapCfg->xy2rbcConfig >> 18) & 0x1;
	pMapCfg->tiledMap			= (pMapCfg->xy2rbcConfig >> 17) & 0x1;
	pMapCfg->caIncHor       = (pMapCfg->xy2rbcConfig >> 16) & 0x1;

	// RAS, BA, CAS -> Axi Addr
	for (i=0; i<32; i++) {
		VpuWriteReg(coreIdx, GDI_RBC2_AXI_0 + 4*i ,pMapCfg->rbc2axiMap[i]);
		//printf("WriteReg(0x%x,0x%04x);\n",GDI_RBC2_AXI_0 + 4*i,pMapCfg->rbc2axiMap[i]);
	}
	
	return ret;

}



static int GetXY2RBCLogic(int map_val,int xpos,int ypos, int tb)
{
	int invert;
	int assign_zero;
	int tbxor;
	int xysel;
	int bitsel;

	int xypos,xybit,xybit_st1,xybit_st2,xybit_st3;

	invert		= map_val >> 7;
	assign_zero = (map_val & 0x78) >> 6;
	tbxor		= (map_val & 0x3C) >> 5;
	xysel		= (map_val & 0x1E) >> 4;
	bitsel		= map_val & 0x0f; 

	xypos     = (xysel) ? ypos : xpos;
	xybit     = (xypos >> bitsel) & 0x01;
	xybit_st1 = (tbxor)       ? xybit^tb : xybit;
	xybit_st2 = (assign_zero) ? 0 : xybit_st1;
	xybit_st3 = (invert)      ? !xybit_st2 : xybit_st2;

	return xybit_st3;
}

static int rbc2axi_logic(int map_val , int ra_in, int ba_in, int ca_in)
{
	int rbc;
	int rst_bit ;
	int rbc_sel = map_val >> 4;
	int bit_sel = map_val & 0x0f;


	if (rbc_sel == 0)
		rbc = ca_in;
	else if (rbc_sel == 1)
		rbc = ba_in;
	else if (rbc_sel == 2)
		rbc = ra_in;
	else 
		rbc = 0;

	rst_bit = ((rbc >> bit_sel) & 1);

	return rst_bit;
}


int GetXY2AXIAddr(TiledMapConfig *pMapCfg, int ycbcr, int posY, int posX, int stride, FrameBuffer *fb)
{
	int ypos_mod;
	int temp;
	int temp_bit;
	int i;	
	int tb;
	int ra_base;
	int ras_base;
	int ra_conv,ba_conv,ca_conv;

	int pix_addr;

	int lum_top_base,chr_top_base;
	int lum_bot_base,chr_bot_base;

	int mbx,mby,mb_addr;
	int temp_val12bit, temp_val6bit;
	int Addr;
	int mb_raster_base;

	if (!pMapCfg)
		return -1;

	pix_addr       = 0;
	mb_raster_base = 0;
	ra_conv        = 0;
	ba_conv        = 0;
	ca_conv        = 0;

	tb = posY & 0x1;

	ypos_mod =  pMapCfg->tbSeparateMap ? posY >> 1 : posY;

	Addr = ycbcr == 0 ? fb->bufY  :
		ycbcr == 2 ? fb->bufCb : fb->bufCr;

	if (fb->mapType == LINEAR_FRAME_MAP) 
		return ((posY * stride) + posX) + Addr;

	// 20bit = AddrY [31:12]
	lum_top_base =   fb->bufY >> 12; 

	// 20bit = AddrY [11: 0], AddrCb[31:24]
	chr_top_base = ((fb->bufY  & 0xfff) << 8) | ((fb->bufCb >> 24) & 0xff);  //12bit +  (32-24) bit

	// 20bit = AddrCb[23: 4]
	lum_bot_base =  (fb->bufCb >> 4) & 0xfffff;

	// 20bit = AddrCb[ 3: 0], AddrCr[31:16]
	chr_bot_base =  ((fb->bufCb & 0xf) << 16) | ((fb->bufCr >> 16) & 0xffff);


	if (fb->mapType == TILED_FRAME_MB_RASTER_MAP || fb->mapType == TILED_FIELD_MB_RASTER_MAP)
	{
		if (ycbcr == 0)
		{
			mbx = posX/16;
			mby = posY/16;
		}
		else //always interleave
		{
			mbx = posX/16;
			mby = posY/8;
		}

		mb_addr = (stride/16) * mby + mbx;		

		// ca[7:0]
		for (i=0 ; i<8; i++)
		{
			if (ycbcr==2 || ycbcr == 3)
				temp = pMapCfg->xy2caMap[i] & 0xff; 
			else 
				temp = pMapCfg->xy2caMap[i] >> 8;				
			temp_bit = GetXY2RBCLogic(temp,posX,ypos_mod,tb);
			ca_conv  = ca_conv + (temp_bit << i);
		}

		// ca[15:8]
		ca_conv      = ca_conv + ((mb_addr & 0xff) << 8);

		// ra[15:0]
		ra_conv      = mb_addr >> 8;


		// ra,ba,ca -> axi
		for (i=0; i<32; i++) {

			temp_val12bit = pMapCfg->rbc2axiMap[i];
			temp_val6bit  = (ycbcr == 0 ) ? (temp_val12bit >> 6) : (temp_val12bit & 0x3f);

			temp_bit = rbc2axi_logic(temp_val6bit,ra_conv,ba_conv,ca_conv);

			pix_addr =  pix_addr + (temp_bit<<i);
		}

		if (pMapCfg->tbSeparateMap ==1 && tb ==1)
			mb_raster_base = ycbcr == 0 ? lum_bot_base : chr_bot_base;
		else
			mb_raster_base = ycbcr == 0 ? lum_top_base : chr_top_base;

		pix_addr = pix_addr + (mb_raster_base << 12);
	}
	else	
	{			
		// ca
		for (i=0 ; i<16; i++)
		{
			if (ycbcr==0 || ycbcr==1)  // clair : there are no case ycbcr = 1
				temp = pMapCfg->xy2caMap[i] >> 8;
			else 
				temp = pMapCfg->xy2caMap[i] & 0xff;	

			temp_bit = GetXY2RBCLogic(temp,posX,ypos_mod,tb);
			ca_conv  = ca_conv + (temp_bit << i);
		}

		// ba
		for (i=0 ; i<4; i++)
		{
			if (ycbcr==2 || ycbcr == 3)
				temp = pMapCfg->xy2baMap[i] & 0xff;	
			else
				temp = pMapCfg->xy2baMap[i] >> 8;

			temp_bit = GetXY2RBCLogic(temp,posX,ypos_mod,tb);
			ba_conv  = ba_conv + (temp_bit << i);
		}

		// ras
		for (i=0 ; i<16; i++)
		{
			if (ycbcr==2 || ycbcr == 3)
				temp = pMapCfg->xy2raMap[i] & 0xff;	
			else
				temp = pMapCfg->xy2raMap[i] >> 8;

			temp_bit = GetXY2RBCLogic(temp,posX,ypos_mod,tb);
			ra_conv  = ra_conv + (temp_bit << i);
		}

		if (pMapCfg->tbSeparateMap == 1 && tb == 1)
			ras_base = Addr >> 16;
		else
			ras_base = Addr & 0xffff;

		ra_base  = ra_conv + ras_base;
		pix_addr = 0;

		// ra,ba,ca -> axi
		for (i=0; i<32; i++) {

			temp_val12bit = pMapCfg->rbc2axiMap[i];
			temp_val6bit  = (ycbcr == 0 ) ? (temp_val12bit >> 6) : (temp_val12bit & 0x3f);

			temp_bit = rbc2axi_logic(temp_val6bit,ra_base,ba_conv,ca_conv);

			pix_addr = pix_addr + (temp_bit<<i);
		}
		pix_addr += pMapCfg->tiledBaseAddr;
	}

	//printf("ycbcr[%d], intlv[%d], ypos[%d], xpos[%d], stride[%d], base_addr[%x], pix_addr[%x], ca[%x], ba[%x], ra[%x], ra_base[%x]",
	//	ycbcr, 0, posY , posX ,stride ,Addr ,pix_addr, ca_conv, ba_conv, ra_conv, ra_base);
	return pix_addr;
}






