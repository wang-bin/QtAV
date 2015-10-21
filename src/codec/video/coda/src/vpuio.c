//------------------------------------------------------------------------------
// File: vpuio.c
//
// Copyright (c) 2006, Chips & Media.  All rights reserved.
//------------------------------------------------------------------------------

#include "vpuapi.h"
#include "vpuapifunc.h"
#include "vpuio.h"

//#define FILL_GRAY

//////////////////// DRAM Read/Write helper Function ////////////////////////////
void LoadYuvImageBurstFormat(Uint32 core_idx, Uint8 * src, int picWidth, int picHeight,
    FrameBuffer *fb, int stride, int interLeave, int format, int endian )

{
    int i, y, nY, nCb, nCr;
    int addr;
    int lumaSize, chromaSize, chromaStride, chromaWidth, delta_luma_size, delta_chroma_size;
    Uint8 * puc;
    Uint8 * temp_data;
    BYTE *  pTemp;
    BYTE * srcAddrCb;
    BYTE * srcAddrCr;

    switch (format)
    {
    case FORMAT_420:
        nY = picHeight;
        nCb = nCr = picHeight / 2;
        chromaSize = picWidth * picHeight / 4;
        delta_chroma_size = (((picWidth+15)&~15) * ((picHeight+15)&~15))/4 - chromaSize;
        chromaStride = stride / 2;
        chromaWidth = picWidth / 2;
        break;
    case FORMAT_224:
        nY = picHeight;
        nCb = nCr = picHeight / 2;
        chromaSize = picWidth * picHeight / 2;
        delta_chroma_size = (((picWidth+15)&~15) * ((picHeight+15)&~15))/2 - chromaSize;
        chromaStride = stride;
        chromaWidth = picWidth;
        break;
    case FORMAT_422:
        nY = picHeight;
        nCb = nCr = picHeight;
        chromaSize = picWidth * picHeight / 2;
        delta_chroma_size = (((picWidth+15)&~15) * ((picHeight+15)&~15))/2 - chromaSize;
        chromaStride = stride / 2;
        chromaWidth = picWidth / 2;
        break;
    case FORMAT_444:
        nY = picHeight;
        nCb = nCr = picHeight;
        chromaSize = picWidth * picHeight;
        delta_chroma_size = (((picWidth+15)&~15) * ((picHeight+15)&~15)) - chromaSize;
        chromaStride = stride;
        chromaWidth = picWidth;
        break;
    case FORMAT_400:
        nY = picHeight;
        nCb = nCr = 0;
        chromaSize = picWidth * picHeight / 4;
        delta_chroma_size = (((picWidth+15)&~15) * ((picHeight+15)&~15))/4 - chromaSize;
        chromaStride = stride / 2;
        chromaWidth = picWidth / 2;
        break;
    }


    puc = src;
    addr = fb->bufY;
    lumaSize = picWidth * picHeight;

    delta_luma_size = ((picWidth+15)&~15) * ((picHeight+15)&~15) - lumaSize;

    if( picWidth == stride) // for fast write
    {
        vdi_write_memory(core_idx, addr, (Uint8 *)( puc ), lumaSize, endian);

        if(delta_luma_size!=0)
        {
            temp_data = osal_malloc(delta_luma_size);
            for(i=0; i<delta_luma_size ; i++)
                temp_data[i] =0;
            vdi_write_memory(core_idx, addr+lumaSize,  temp_data , delta_luma_size, endian);
            osal_free(temp_data);
        }

        if( format == FORMAT_400)
            return;

        if( interLeave == 1 )
        {
            addr = fb->bufCb;
            srcAddrCb = puc + lumaSize;
            srcAddrCr = puc + lumaSize + chromaSize;

            pTemp = osal_malloc(chromaStride*2);
            if (!pTemp)
                return;

            for (y=0; y<nCb; y++) {
                for (i=0; i<chromaStride*2; i+=8) {
                    pTemp[i  ] = *srcAddrCb++;
                    pTemp[i+2] = *srcAddrCb++;
                    pTemp[i+4] = *srcAddrCb++;
                    pTemp[i+6] = *srcAddrCb++;
                    pTemp[i+1] = *srcAddrCr++;
                    pTemp[i+3] = *srcAddrCr++;
                    pTemp[i+5] = *srcAddrCr++;
                    pTemp[i+7] = *srcAddrCr++;
                }
                vdi_write_memory(core_idx, addr+2*chromaStride*y, (Uint8 *)pTemp, chromaStride*2, endian);
            }

            if(delta_chroma_size !=0)
            {
                temp_data = osal_malloc(delta_chroma_size*2);
                for(i=0; i<delta_chroma_size*2 ; i++)
                    temp_data[i] =128;
                vdi_write_memory(core_idx, addr+2*chromaStride*nCb,  temp_data , delta_chroma_size*2, endian);
                osal_free(temp_data);
            }

            osal_free(pTemp);
        }
        else
        {			
            puc = src + lumaSize;
            addr = fb->bufCb;
            vdi_write_memory(core_idx, addr, (Uint8 *)puc, chromaSize, endian);

            temp_data = osal_malloc(delta_chroma_size);
            for(i=0; i<delta_chroma_size ; i++)
                temp_data[i] =128;
            vdi_write_memory(core_idx, addr+chromaSize ,  temp_data , delta_chroma_size, endian);
            osal_free(temp_data);

            puc = src + lumaSize + chromaSize;
            addr = fb->bufCr;
            vdi_write_memory(core_idx, addr, (Uint8 *)puc, chromaSize, endian);

            temp_data = osal_malloc(delta_chroma_size);
            for(i=0; i<delta_chroma_size ; i++)
                temp_data[i] =128;
            vdi_write_memory(core_idx, addr+chromaSize ,  temp_data , delta_chroma_size, endian);
            osal_free(temp_data);
        }
    }
    else
    {
        for (y = 0; y < nY; ++y) {
            vdi_write_memory(core_idx, addr + stride * y, (Uint8 *)(puc + y * picWidth), picWidth, endian);
        }


        if( format == FORMAT_400)
            return;

        if( interLeave == 1 )
        {
            addr = fb->bufCb;
            puc = src + lumaSize;
            srcAddrCb = puc + lumaSize;
            srcAddrCr = puc + lumaSize + chromaSize;

            pTemp = (BYTE*)osal_malloc(chromaWidth*2);
            if (!pTemp)
                return;

            for (y=0; y<nCb; y++) {
                for (i=0; i<chromaWidth*2; i+=8) {
                    pTemp[i  ] = *srcAddrCb++;
                    pTemp[i+2] = *srcAddrCb++;
                    pTemp[i+4] = *srcAddrCb++;
                    pTemp[i+6] = *srcAddrCb++;
                    pTemp[i+1] = *srcAddrCr++;
                    pTemp[i+3] = *srcAddrCr++;
                    pTemp[i+5] = *srcAddrCr++;
                    pTemp[i+7] = *srcAddrCr++;
                }
                vdi_write_memory(core_idx, addr+2*chromaStride*y, (Uint8 *)pTemp, chromaWidth*2, endian);
            }
            osal_free(pTemp);
        }
        else
        {
            puc = src + lumaSize;
            addr = fb->bufCb;
            for (y = 0; y < nCb; ++y) {
                vdi_write_memory(core_idx, addr + chromaStride * y, (Uint8 *)(puc + y * chromaWidth), chromaWidth, endian);
            }

            puc = src + lumaSize + chromaSize;
            addr = fb->bufCr;
            for (y = 0; y < nCr; ++y) {
                vdi_write_memory(core_idx, addr + chromaStride * y, (Uint8 *)(puc + y * chromaWidth), chromaWidth, endian);
            }	
        }
    }
}




int  LoadTiledImageYuvBurst(Uint32 core_idx, BYTE *pYuv, int picWidth, int picHeight, FrameBuffer *fb, TiledMapConfig mapCfg, int stride,int interLeave, int format, int endian)
{   
	BYTE *pSrc;
	int  divX, divY;
	int  pix_addr;
	int  rrow, ccol;
	int  offsetX,offsetY;
	int  stride_c;
    int  enc_pic_width;
    int  enc_pic_height;
    Uint8* temp_data;
    int  i;

	offsetX = offsetY    = 0;

	divX = format == FORMAT_420 || format == FORMAT_422 ? 2 : 1;
	divY = format == FORMAT_420 || format == FORMAT_224 ? 2 : 1;

	switch (format) {
	case FORMAT_400:
		stride_c = 0;
		break;
	case FORMAT_420:
	case FORMAT_422:
		stride_c = stride / 2;
		break;
	case FORMAT_224:
	case FORMAT_444:
		stride_c = stride;
		break;
	}

	// Y
	pSrc    = pYuv;
    enc_pic_height = ((picHeight+15)&~15);
    enc_pic_width = ((picWidth+15)&~15);

	// no opt code
	for (rrow=0; rrow <picHeight; rrow=rrow+1) 
	{
		for (ccol=0; ccol<picWidth; ccol=ccol+DRAM_BUS_WIDTH)
		{    
			pix_addr = GetXY2AXIAddr(&mapCfg, 0/*luma*/, rrow +offsetY, ccol + offsetX, stride, fb);
			vdi_write_memory(core_idx, pix_addr, pSrc+rrow*picWidth+ccol, 8, endian);
		}
	}

    // MB align
    temp_data = osal_malloc(DRAM_BUS_WIDTH);
    for (rrow =picHeight; rrow < enc_pic_height; rrow = rrow+1)
    {
        for(ccol=0; ccol<enc_pic_width; ccol = ccol +DRAM_BUS_WIDTH)
        {
            pix_addr = GetXY2AXIAddr(&mapCfg, 0/*luma*/, rrow +offsetY, ccol + offsetX, stride, fb);
            for(i=0; i < DRAM_BUS_WIDTH ; i++)
                temp_data[i] =0;
			vdi_write_memory(core_idx, pix_addr, temp_data, 8, endian);
        }
    }
    osal_free(temp_data);

	if (format == FORMAT_400) {
		return 1;
	}

	if (interLeave == 0) 
	{ 
		// CB
		pSrc = pYuv + picWidth*picHeight;

		for (rrow=0; rrow <(picHeight/divY) ; rrow=rrow+1) 
		{
			for (ccol=0; ccol<(picWidth/divX); ccol=ccol+DRAM_BUS_WIDTH) 
			{
				pix_addr = GetXY2AXIAddr(&mapCfg, 2, rrow + offsetY, ccol +offsetX, stride_c, fb);
				vdi_write_memory(core_idx, pix_addr, pSrc+rrow*picWidth/divX+ccol, 8, endian);
			}
		}


        temp_data = osal_malloc(DRAM_BUS_WIDTH);
        for(i=0; i < DRAM_BUS_WIDTH ; i++)
            temp_data[i] =128;
        for (rrow =(picHeight/divY); rrow < (enc_pic_height/divY); rrow = rrow+1)
        {
            for(ccol=0 ; ccol<(enc_pic_width/divX) ; ccol = ccol +DRAM_BUS_WIDTH)
            {
                pix_addr = GetXY2AXIAddr(&mapCfg, 2, rrow + offsetY, ccol +offsetX, stride_c, fb);
                vdi_write_memory(core_idx, pix_addr, temp_data, 8, endian);
            }
        }
		// CR

		pSrc = pYuv + picWidth*picHeight+ (picWidth/divX)*(picHeight/divY);

		for (rrow=0; rrow <picHeight/divY ; rrow=rrow+1) 
		{
			for (ccol=0; ccol<picWidth/divX; ccol=ccol+DRAM_BUS_WIDTH) 
			{
				pix_addr = GetXY2AXIAddr(&mapCfg, 3, rrow  + offsetY ,ccol +offsetX, stride_c, fb);
				vdi_write_memory(core_idx, pix_addr, pSrc+rrow*picWidth/divX+ccol, 8, endian);
			}
		}

        for (rrow =(picHeight/divY); rrow < (enc_pic_height/divY); rrow = rrow+1)
        {
            for(ccol=0 ; ccol<(enc_pic_width/divX) ; ccol = ccol +DRAM_BUS_WIDTH)
            {
                pix_addr = GetXY2AXIAddr(&mapCfg, 3, rrow  + offsetY ,ccol +offsetX, stride_c, fb);
                vdi_write_memory(core_idx, pix_addr, temp_data, 8, endian);
            }
        }
        osal_free(temp_data);
	}
	else 
	{

		BYTE * pTemp;
		BYTE * srcAddrCb;
		BYTE * srcAddrCr;

		int cbcr_y;
		int cbcr_x;

		switch( format) {
		case FORMAT_444 : 
			cbcr_y = picHeight; 
			cbcr_x = picWidth*2;
			break; 
		case FORMAT_420 : 
			cbcr_y = picHeight/divY; 
			cbcr_x = picWidth  ; 
			break;
		case FORMAT_422 : 
			cbcr_y = picHeight/divY; 
			cbcr_x = picWidth  ;
			break;
		case FORMAT_224 : 
			cbcr_y = picHeight/divY; 
			cbcr_x = picWidth*2;
			break;
		default: break;
		}

		cbcr_y = cbcr_y;
		stride = stride_c * 2;

		srcAddrCb = pYuv + picWidth*picHeight;
		srcAddrCr = pYuv + picWidth*picHeight + picWidth/divX*picHeight/divY;

		pTemp = osal_malloc(sizeof(char)*8);
		if (!pTemp) {
			return 0;
		}

		for (rrow=0; rrow <picHeight/divY; rrow=rrow+1) {
			for (ccol=0; ccol<cbcr_x ; ccol=ccol+DRAM_BUS_WIDTH) {     

				pTemp[0  ] = *srcAddrCb++;
				pTemp[0+2] = *srcAddrCb++;
				pTemp[0+4] = *srcAddrCb++;
				pTemp[0+6] = *srcAddrCb++;
				pTemp[0+1] = *srcAddrCr++;
				pTemp[0+3] = *srcAddrCr++;
				pTemp[0+5] = *srcAddrCr++;
				pTemp[0+7] = *srcAddrCr++;

				pix_addr = GetXY2AXIAddr(&mapCfg, 2, rrow + offsetY ,ccol + (offsetX*2), stride, fb);  
				vdi_write_memory(core_idx, pix_addr, (unsigned char *)pTemp, 8, endian);
			}
		}
		osal_free(pTemp);

        temp_data = osal_malloc(DRAM_BUS_WIDTH);
        for(i=0; i < DRAM_BUS_WIDTH ; i++)
            temp_data[i] =128;

        for (rrow =(picHeight/divY); rrow < (enc_pic_height/divY); rrow = rrow+1)
        {
            for(ccol=0 ; ccol<cbcr_x ; ccol = ccol +DRAM_BUS_WIDTH)
            {
                pix_addr = GetXY2AXIAddr(&mapCfg, 2, rrow + offsetY ,ccol + (offsetX*2), stride, fb);  
                vdi_write_memory(core_idx, pix_addr, temp_data, 8, endian);
            }
        }
        osal_free(temp_data);

	}

	return 1;
}


void StoreYuvImageBurstFormat(Uint32 core_idx, FrameBuffer *fbSrc, TiledMapConfig mapCfg, Uint8 *pDst, Rect cropRect, int interLeave, int format, int endian)
{
    int y, x;
    int	pix_addr, div_x, div_y, chroma_stride;
    Uint8 * puc;
#ifdef USE_CROP_INFO
    int copy_height = cropRect.bottom - cropRect.top;
#else
    int copy_height = fbSrc->height;
#endif
    int stride      = fbSrc->stride;
    int height      = fbSrc->height;

    div_x = format == FORMAT_420 || format == FORMAT_422 ? 2 : 1;
    div_y = format == FORMAT_420 || format == FORMAT_224 ? 2 : 1;

    chroma_stride = (stride / div_x);
#ifdef USE_CROP_INFO
    puc = pDst + cropRect.top*stride;
#else
    puc = pDst;
#endif

    for ( y=0 ; y<copy_height ; y+=1 ) 
    {
        for ( x=0 ; x<stride ; x+=DRAM_BUS_WIDTH )
        {  
#ifdef USE_CROP_INFO
            pix_addr = GetXY2AXIAddr(&mapCfg, 0, y+cropRect.top, x, stride, fbSrc);
#else
            pix_addr = GetXY2AXIAddr(&mapCfg, 0, y, x, stride, fbSrc);
#endif
            vdi_read_memory(core_idx, pix_addr, (Uint8 *)( puc ) + y * stride + x, DRAM_BUS_WIDTH,  endian);			
        }
    }
#ifdef USE_CROP_INFO
#ifdef FILL_GRAY
    //top
    osal_memset(pDst, 0x80, cropRect.top*stride);
    puc = pDst + cropRect.top*stride;
    for ( y=0 ; y<copy_height ; y+=1 ) 
    {
        //left
        osal_memset(puc, 0x80, cropRect.left);
        //right
        osal_memset(puc + cropRect.right, 0x80, stride - cropRect.right);
        puc += stride;
    }
    //bottom
    osal_memset(puc, 0x80, (height-cropRect.bottom)*stride);
#endif
#endif
    if (format == FORMAT_400)
        return;

    copy_height /= div_y;
    if (interLeave == 1)
    {
        Uint8 * pTemp;
        Uint8 * dstAddrCb;
        Uint8 * dstAddrCr;
        int  cbcr_per_2pix=1;
#ifdef USE_CROP_INFO
        dstAddrCb = pDst + stride*height + (cropRect.top/div_y)*chroma_stride;
        dstAddrCr = pDst + stride*height + chroma_stride*(height/div_y) + cropRect.top/div_y*chroma_stride;
#else
        dstAddrCb = pDst + stride*height;
        dstAddrCr = pDst + stride*height + chroma_stride*(height/div_y);
#endif

        pTemp = osal_malloc(DRAM_BUS_WIDTH);
        if (!pTemp) {
            VLOG(ERR, "malloc() failed \n");
            return;
        }

        cbcr_per_2pix = (format==FORMAT_224||format==FORMAT_444) ? 2 : 1;

        for ( y = 0 ; y < copy_height ; ++y ) 
        {
            for ( x = 0 ; x < stride*cbcr_per_2pix ; x += DRAM_BUS_WIDTH )
            {
#ifdef USE_CROP_INFO
                pix_addr = GetXY2AXIAddr(&mapCfg, 2, y+(cropRect.top/div_y), x, stride, fbSrc);
#else
                pix_addr = GetXY2AXIAddr(&mapCfg, 2, y, x, stride, fbSrc);
#endif
                vdi_read_memory(core_idx, pix_addr,  pTemp, DRAM_BUS_WIDTH,  endian); 
                *dstAddrCb++ = pTemp[0];
                *dstAddrCb++ = pTemp[2];
                *dstAddrCb++ = pTemp[4];
                *dstAddrCb++ = pTemp[6];
                *dstAddrCr++ = pTemp[1];
                *dstAddrCr++ = pTemp[3];
                *dstAddrCr++ = pTemp[5];
                *dstAddrCr++ = pTemp[7];
            }
        }
        osal_free(pTemp);
    }
    else
    {	
#ifdef USE_CROP_INFO
        puc = pDst + stride*height + (cropRect.top/div_y)*chroma_stride;
#else
        puc = pDst + stride*height;
#endif

        for ( y = 0 ; y < copy_height ; y += 1 ) 
        {
            for ( x = 0 ; x < chroma_stride; x += DRAM_BUS_WIDTH ) 
            {
#ifdef USE_CROP_INFO
                pix_addr = GetXY2AXIAddr(&mapCfg, 2, y+(cropRect.top/div_y), x, chroma_stride, fbSrc);
#else
                pix_addr = GetXY2AXIAddr(&mapCfg, 2, y, x, chroma_stride, fbSrc);
#endif
                vdi_read_memory(core_idx, pix_addr, (Uint8 *)(puc + ((y*chroma_stride)+x)), DRAM_BUS_WIDTH,  endian);
            }
        }


#ifdef USE_CROP_INFO
        puc = pDst + stride*height + chroma_stride*(height/div_y) + cropRect.top/div_y*chroma_stride;
#else
        puc = pDst + stride*height + chroma_stride*(height/div_y);
#endif
        for ( y = 0 ; y < copy_height ; y += 1 ) 
        {
            for ( x = 0 ; x < chroma_stride ; x += DRAM_BUS_WIDTH ) 
            {
                pix_addr = GetXY2AXIAddr(&mapCfg, 3, y+(cropRect.top/div_y), x, chroma_stride, fbSrc);
                vdi_read_memory(core_idx, pix_addr, (Uint8 *)(puc + ((y * chroma_stride)+x)), DRAM_BUS_WIDTH,  endian);
            }
        }

    }

#ifdef USE_CROP_INFO
#ifdef FILL_GRAY
    //cb

    puc = pDst + stride*height;
    osal_memset(puc, 0x80, (cropRect.top/div_y)*(stride/div_x));//top 
    puc += (cropRect.top/div_y)*(stride/div_x);
    for ( y=0 ; y<copy_height ; y+=1 ) 
    {
        osal_memset(puc, 0x80, (cropRect.left/div_x)); //left
        osal_memset(puc + (cropRect.right/div_x), 0x80, chroma_stride  - (cropRect.right/div_x)); //right
        puc += chroma_stride;
    }

    osal_memset(puc, 0x80, ((height-cropRect.bottom)/div_y)*chroma_stride); //bottom
    //cr
    puc = pDst + stride*height + chroma_stride*(height/div_y);
    osal_memset(puc, 0x80, (cropRect.top/div_y)*(stride/div_x));//top
    puc += (cropRect.top/div_y)*(stride/div_x);
    for ( y=0 ; y<copy_height ; y+=1 ) 
    {
        osal_memset(puc, 0x80, (cropRect.left/div_x)); //left
        osal_memset(puc + (cropRect.right/div_x), 0x80, chroma_stride  - (cropRect.right/div_x)); //right
        puc += chroma_stride;
    }
    osal_memset(puc, 0x80, ((height-cropRect.bottom)/div_y)*chroma_stride); //bottom
#endif
#endif
    return;
}




int ProcessEncodedBitstreamBurst(Uint32 core_idx, osal_file_t fp, int targetAddr,
	PhysicalAddress bsBufStartAddr, PhysicalAddress bsBufEndAddr,
	int size, int endian )
{
	Uint8 * buffer = 0;
	int room = 0;
	int file_wr_size = 0;

	buffer = (Uint8 *)osal_malloc(size);
	if( ( targetAddr + size ) > (int)bsBufEndAddr )
	{
		room = bsBufEndAddr - targetAddr;
		vdi_read_memory(core_idx, targetAddr, buffer, room,  endian);
		vdi_read_memory(core_idx, bsBufStartAddr, buffer+room, (size-room), endian);
	}
	else
	{
		vdi_read_memory(core_idx, targetAddr, buffer, size, endian); 
	}	

	if (fp) {
		file_wr_size = osal_fwrite(buffer, sizeof(Uint8), size, fp);
		osal_fflush(fp);
	}

	osal_free( buffer );

	return file_wr_size;
}


void read_encoded_bitstream_burst(Uint32 core_idx, unsigned char * buffer, int targetAddr,
	PhysicalAddress bsBufStartAddr, PhysicalAddress bsBufEndAddr,
	int size, int endian )
{
	int room = 0;

	if( ( targetAddr + size ) > (int)bsBufEndAddr )
	{
		room = bsBufEndAddr - targetAddr;
		vdi_read_memory(core_idx, targetAddr, buffer, room,  endian);
		vdi_read_memory(core_idx, bsBufStartAddr, buffer+room, (size-room), endian);
	}
	else
	{
		vdi_read_memory(core_idx, targetAddr, buffer, size, endian); 
	}
}


int FillSdramBurst(Uint32 core_idx, BufInfo *pBufInfo, Uint32 targetAddr,
	vpu_buffer_t *pVbStream,
	Uint32 size,  int checkeos,
	int *streameos, int endian )
{
	int room;
	//Uint8 *pBuf                    = (Uint8 *)pVbStream->virt_addr;
	PhysicalAddress bsBufStartAddr = pVbStream->phys_addr;
	PhysicalAddress bsBufEndAddr   = pVbStream->phys_addr+pVbStream->size;
	int             offset         = targetAddr - bsBufStartAddr;

	pBufInfo->count = 0;

	if (checkeos == 1 && (pBufInfo->point >= pBufInfo->size))
	{
//		printf("*************error,checkeos=%d,pBufInfo->point=%#x,pBufInfo->size=%#x\n",checkeos,pBufInfo->point,pBufInfo->size);
		*streameos = 1;
		return 0;
	}

	if (offset<0)				return -1;
	if (offset>pVbStream->size)	return -1;


	if ((pBufInfo->size - pBufInfo->point) < (int)size)
		pBufInfo->count = (pBufInfo->size - pBufInfo->point);
	else
		pBufInfo->count = size;



	if( (targetAddr+pBufInfo->count) > bsBufEndAddr) //
	{   
//		printf("111111111111\n");
		room   = bsBufEndAddr - targetAddr;
		//write to physical address
		vdi_write_memory(core_idx, targetAddr, pBufInfo->buf+pBufInfo->point, room, endian);
		vdi_write_memory(core_idx, bsBufStartAddr, pBufInfo->buf+pBufInfo->point+room, (pBufInfo->count-room), endian);		
	}
	else
	{
//		printf("222222222222222\n");
		//write to physical address
//		printf("FillSdramBurst addr=%#x, data=%p, len=%#x\n",targetAddr, pBufInfo->buf+pBufInfo->point, pBufInfo->count);
		vdi_write_memory(core_idx, targetAddr, pBufInfo->buf+pBufInfo->point, pBufInfo->count, endian);		
	}
	pBufInfo->point += pBufInfo->count;
	return pBufInfo->count;	

}

