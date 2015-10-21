/****************************************************************************
*
*    Copyright (c) 2005 - 2012 by Vivante Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of Vivante Corporation. This is proprietary information owned by
*    Vivante Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of Vivante Corporation.
*
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#include <stdarg.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include "../vdi/vdi.h"
#include <linux/fb.h>
#include <galUtil.h>
#include "mixer.h"

typedef struct Test2D {
//      GalTest     base;
        GalRuntime  runtime;

        // destination surface
        gcoSURF                 dstSurf;
        gceSURF_FORMAT          dstFormat;
        gctUINT                 dstWidth;
        gctUINT                 dstHeight;
        gctINT                  dstStride;
        gctUINT32               dstPhyAddr;
        gctPOINTER              dstLgcAddr;

        //source surface
        gcoSURF                 srcSurf;
        gceSURF_FORMAT          srcFormat;
        gctUINT                 srcWidth;
        gctUINT                 srcHeight;
        gctINT                  srcStride;
        gctUINT32               srcPhyAddr;
        gctPOINTER              srcLgcAddr;

} Test2D;

/*
 * YUV2RGB initial info
 */
struct Test2D *test2D;

/* Base objects */
gcoOS       g_os                = gcvNULL;
gcoHAL      g_hal               = gcvNULL;
gco2D       g_2d                = gcvNULL;

static gctPHYS_ADDR g_ContiguousPhysical;
static gctSIZE_T    g_ContiguousSize;
static gctPOINTER   g_Contiguous;

/* target surface. */
gcoSURF          g_TargetSurf    = gcvNULL;
gctUINT          g_TargetWidth   = 0;
gctUINT          g_TargetHeight  = 0;
gceSURF_FORMAT   g_TargetFormat  = gcvSURF_A8R8G8B8;
gcePOOL          g_TargetPool    = gcvPOOL_DEFAULT;

unsigned int screen_width;
unsigned int screen_height;
unsigned int screen_size;
unsigned int shift;
/*
 * End YUV2RGB info
 */

static int fb_fd = 0;
unsigned int fb_phyaddr;
#define FBDEV_FILENAME  	"/dev/fb0"
#define SWICHFB_GET_PHYS        0xBAD

//#define FB_MULTI_BUFFER
#ifdef FB_MULTI_BUFFER
#define BUFFER_NUM 2
int backbuffer_num = 0;
#endif

gctBOOL hw_mixer_init()
{
        GalRuntime runtime;
        gceSTATUS status;

        /* Construct the gcoOS object. */
        status = gcoOS_Construct(gcvNULL, &g_os);
        if (status < 0)
        {
                printf("*ERROR* Failed to construct OS object (status = %d)\n", status);
                return gcvFALSE;
        }

        /* Construct the gcoHAL object. */
        status = gcoHAL_Construct(gcvNULL, g_os, &g_hal);
        if (status < 0)
        {
                printf("*ERROR* Failed to construct GAL object (status = %d)\n", status);
                return gcvFALSE;
        }

        status = gcoHAL_QueryVideoMemory(g_hal,
                                                                         NULL, NULL,
                                                                         NULL, NULL,
                                                                         &g_ContiguousPhysical, &g_ContiguousSize);
        if (gcmIS_ERROR(status))
        {
                printf("gcoHAL_QueryVideoMemory failed %d.", status);
                return gcvFALSE;
        }

        /* Map the contiguous memory. */
        if (g_ContiguousSize > 0)
        {
                status = gcoHAL_MapMemory(g_hal,
                                                                  g_ContiguousPhysical, g_ContiguousSize,
                                                                  &g_Contiguous);
                if (gcmIS_ERROR(status))
                {
                        printf("gcoHAL_MapMemory failed %d.", status);
                        return gcvFALSE;
                }
        }

        status = gcoHAL_Get2DEngine(g_hal, &g_2d);
        if (status < 0)
        {
                printf("*ERROR* Failed to get 2D engine object (status = %d)\n", status);
                return gcvFALSE;
        }

        status = gcoSURF_Construct(g_hal,
                        g_TargetWidth,
                        g_TargetHeight,
                        1,
                        gcvSURF_BITMAP,
                        g_TargetFormat,
                        g_TargetPool,
                        &g_TargetSurf);

        if (status < 0)
        {
                printf("*ERROR* Failed to construct SURFACE object (status = %d)\n", status);
                return gcvFALSE;
        }

        runtime.os              = g_os;
        runtime.hal             = g_hal;
        runtime.engine2d        = g_2d;
        runtime.target          = g_TargetSurf;
        runtime.width           = g_TargetWidth;
        runtime.height          = g_TargetHeight;
        runtime.format          = g_TargetFormat;
        runtime.pe20            = gcoHAL_IsFeatureAvailable(g_hal, gcvFEATURE_2DPE20);
        runtime.fullDFB         = gcoHAL_IsFeatureAvailable(g_hal, gcvFEATURE_FULL_DIRECTFB);

        test2D = (Test2D *)malloc(sizeof(Test2D));
        test2D->runtime = runtime;

        test2D->dstSurf    = runtime.target;
        test2D->dstFormat = runtime.format;
        test2D->dstWidth = 0;
        test2D->dstHeight = 0;
        test2D->dstStride = 0;
        test2D->dstPhyAddr = 0;
        test2D->dstLgcAddr = 0;

        test2D->srcSurf    = gcvNULL;
        test2D->srcWidth = 0;
        test2D->srcHeight = 0;
        test2D->srcStride = 0;
        test2D->srcPhyAddr = 0;
        test2D->srcLgcAddr = 0;
        test2D->srcFormat = gcvSURF_UNKNOWN;

        gcmONERROR(gcoSURF_GetAlignedSize(test2D->dstSurf,
                                                                                &test2D->dstWidth,
                                                                                &test2D->dstHeight,
                                                                                &test2D->dstStride));

        gcmONERROR(gcoSURF_Lock(test2D->dstSurf, &test2D->dstPhyAddr, &test2D->dstLgcAddr));


        if (!gcoHAL_IsFeatureAvailable(test2D->runtime.hal, gcvFEATURE_YUV420_SCALER))
        {
                printf("YUV420 scaler is not supported.\n");
                free(test2D);
                return gcvFALSE;
        }

        // output framework cfg info

        printf( "===================== Test Config =====================\n"
                "\tscreen window width : %d\n"
                "\tscreen window heitht: %d\n"
                "\tsurface format: %d\n"
                "=======================================================\n",
                g_TargetWidth,
                g_TargetHeight,
                g_TargetFormat);


    return gcvTRUE;

OnError:

    return gcvFALSE;
}

static void Destroy(Test2D *t2d)
{
        gceSTATUS status = gcvSTATUS_OK;
        if ((t2d->dstSurf != gcvNULL) && (t2d->dstLgcAddr != gcvNULL))
        {
                gcoSURF_Unlock(t2d->dstSurf, t2d->dstLgcAddr);
                t2d->dstLgcAddr = gcvNULL;
        }

        printf("destroy source surface!\n");
        // destroy source surface
        if (t2d->srcSurf != gcvNULL)
        {
                if (t2d->srcLgcAddr)
                {
                        gcmONERROR(gcoSURF_Unlock(t2d->srcSurf, t2d->srcLgcAddr));
                        t2d->srcLgcAddr = 0;
                }

                if (gcmIS_ERROR(gcoSURF_Destroy(t2d->srcSurf)))
                {
                        printf("Destroy Surf failed:%#x\n", status);
                }
                t2d->srcSurf = NULL;
        }

        free(t2d);

        return;
OnError:
        free(t2d);
        return;
}

/*
 *  Name:       Finalize()
 *  Returns:    None.
 *  Parameters: None.
 *  Description:Free all resource that the framework has used. These may include the memory resource it used, and the egl 
 *  system resource and so on. Here it includes "finalize the test object(case)", "free egl resource", "free library 
 *  resource", "finalize output file resource" and "destroy win32 resource".
*/
void hw_mixer_exit()
{
        if (test2D != gcvNULL)
        {
                Destroy(test2D);
        }

        printf("finalize the test object(case) !\n");

        if (g_hal != gcvNULL)
        {
                gcoHAL_Commit(g_hal, gcvTRUE);
        }

        if (test2D->srcSurf != gcvNULL)
        {
                gcoSURF_Destroy(test2D->srcSurf);
		test2D->srcSurf = NULL;
        }

        if (g_TargetSurf != gcvNULL)
        {
                gcoSURF_Destroy(g_TargetSurf);
                g_TargetSurf = NULL;
        }

        if (g_Contiguous != gcvNULL)
        {
                /* Unmap the contiguous memory. */
                gcmVERIFY_OK(gcoHAL_UnmapMemory(g_hal,
                                                                                g_ContiguousPhysical, g_ContiguousSize,
                                                                                g_Contiguous));
        }

        if (g_hal != gcvNULL)
        {
                gcoHAL_Commit(g_hal, gcvTRUE);
                gcoHAL_Destroy(g_hal);
                g_hal = NULL;
        }

        if (g_os != gcvNULL)
        {
                gcoOS_Destroy(g_os);
                g_os = NULL;
        }
}

int creat_srcsurf(int width,int height,int format)
{
	gceSTATUS status;
        gctUINT alignedWidth, alignedHeight;
        gctINT alignedStride;
	gcoSURF srcsurf = NULL;
	gctUINT32 address[3]; 
	gctPOINTER memory[3];

	status = gcoSURF_Construct(test2D->runtime.hal, width, height, 1, gcvSURF_BITMAP,format, gcvPOOL_DEFAULT, &srcsurf);
	if(status != gcvSTATUS_OK)
	{
		printf("surface construct error!\n");
		gcoSURF_Destroy(srcsurf);
		srcsurf = gcvNULL;
		return status;
	}

        gcmVERIFY_OK(gcoSURF_GetAlignedSize(srcsurf, &alignedWidth, &alignedHeight, &alignedStride));

	if(width != alignedWidth || height != alignedHeight)
        {
                printf("gcoSURF width and height is not aligned !\n");
                gcoSURF_Destroy(srcsurf);
                srcsurf = gcvNULL;
                return -1;
        }
	
	test2D->srcSurf = srcsurf;

	gcmONERROR(gcoSURF_GetAlignedSize(test2D->srcSurf,
                                                                                gcvNULL,
                                                                                gcvNULL,
                                                                                &test2D->srcStride));
        gcmONERROR(gcoSURF_GetSize(test2D->srcSurf,
                                                                &test2D->srcWidth,
                                                                &test2D->srcHeight,
                                                                gcvNULL));
        gcmONERROR(gcoSURF_GetFormat(test2D->srcSurf, gcvNULL, &test2D->srcFormat));

        test2D->srcPhyAddr = address[0];
        test2D->srcLgcAddr = memory[0];

	return 0;
OnError:

    return -1;

}

static gctBOOL render(Test2D *t2d,int width,int height,int format, unsigned long yaddr, unsigned long cbaddr,unsigned long craddr)
{
	gctUINT8 horKernel = 1, verKernel = 1;
	gcsRECT srcRect;
	gco2D egn2D = test2D->runtime.engine2d;
	gceSTATUS status;
//	int count;
	gcsRECT dstRect = {0, 0, t2d->dstWidth, t2d->dstHeight};
	
	gctUINT32 address[3];  //解码后的yuv文件的物理地址
        gctPOINTER memory[3]; 	//解码后的yuv文件的逻辑地址
//	gcoSURF surf = NULL;

	srcRect.left = 0;
	srcRect.top = 0;
	srcRect.right = width;
	srcRect.bottom = height;
#if 0
	if(height%32 != 0 )
		height = (height/32 + 1)*32; //拷贝原始文件数据大小时，需要对界，因为原始的的宏块需要对界。
	//根据源文件的格式而言来计算不同的大小
	if (format == gcvSURF_I420){
		count = width * height;
		count = count + (count>>1);
	}
#endif

	gcoSURF_Lock(t2d->srcSurf, address, memory);

	dma_copy_in_vmem(address[0],(gctUINT32)yaddr,width*height);
	dma_copy_in_vmem(address[1],(gctUINT32)cbaddr,width*height/4);
	dma_copy_in_vmem(address[2],(gctUINT32)craddr,width*height/4);
	
	// set clippint rect
	gcmONERROR(gco2D_SetClipping(egn2D, &dstRect));


	gcmONERROR(gcoSURF_SetDither(test2D->dstSurf, gcvTRUE));


	// set kernel size
	status = gco2D_SetKernelSize(egn2D, horKernel, verKernel);
	if (status != gcvSTATUS_OK)
	{
		printf("2D set kernel size failed:%#x\n", status);
		return gcvFALSE;
	}

	status = gco2D_EnableDither(egn2D, gcvTRUE);
	if (status != gcvSTATUS_OK)
	{
		printf("enable gco2D_EnableDither failed:%#x\n", status);
		return gcvFALSE;
	}


	status = gcoSURF_FilterBlit(test2D->srcSurf, test2D->dstSurf,
		&srcRect, &dstRect, &dstRect);
	if (status != gcvSTATUS_OK)
	{
		printf("2D FilterBlit failed:%#x\n", status);
		return gcvFALSE;
	}

	status = gco2D_EnableDither(egn2D, gcvFALSE);
	if (status != gcvSTATUS_OK)
	{
		printf("disable gco2D_EnableDither failed:%#x\n", status);
		return gcvFALSE;
	}

	gcmONERROR(gco2D_Flush(egn2D));

	gcmONERROR(gcoHAL_Commit(test2D->runtime.hal, gcvTRUE));
	
    	return gcvTRUE;

OnError:

    return gcvFALSE;
}


/*
 *  Name:       Run()
 *  Returns:    None.
 *  Parameters: None.
*/

static gctBOOL run(int width,int height,int format, unsigned long yaddr, unsigned long cbaddr, unsigned long craddr)
{
	int data_size = 0;
	struct fb_var_screeninfo var_info;

	if (g_TargetSurf == gcvNULL)
	{
		return -1;
	}
#if 0
	/******************************************************************************/
	****** we don't need to clear screen to black,for SWICH,modified by caizp******/
	*******************************************************************************/

	/* clear target surface to black. */
	if(gcmIS_ERROR(Gal2DCleanSurface(g_hal, g_TargetSurf, COLOR_ARGB8(0x00, 0x00, 0x00, 0x00))))
	{
		return -1;
	}
#endif
	if (render(test2D,width,height,format, yaddr, cbaddr, craddr) == gcvTRUE)
	{
		printf("Rendering convert frame ... succeed\n");
	}
	else
	{
		printf("Rendering convert frame ... failed\n");
		return -1;
	}
#if 1
//	gcoSURF_Lock(g_TargetSurf, &resolveAddress,&bits);
//	gcoSURF_GetAlignedSize(g_TargetSurf, &alignedWidth, &alignedHeight, &bitsStride);
//	printf("the resolveAddress=%#x,fb_phyaddr=%#x,alignedWidth=%d,alignedHeight=%d\n",resolveAddress,fb_phyaddr,alignedWidth,alignedHeight);
	
	if (g_TargetFormat == gcvSURF_A8R8G8B8)
		data_size = g_TargetWidth * g_TargetHeight * 4;
	else if (g_TargetFormat == gcvSURF_R5G6B5)
		data_size = g_TargetWidth * g_TargetHeight * 2;
		
//	data_size = alignedWidth * alignedHeight *4;
#if 0
	for(i=0;i<data_size;i+=4)
		*(u32 *)vmem_base++ = *(u32 *)bits++;	
#else

#ifdef FB_MULTI_BUFFER
	dma_copy_in_vmem(fb_phyaddr + screen_size * backbuffer_num + shift,test2D->dstPhyAddr,data_size);
	var_info.xoffset  = 0;
	var_info.yoffset  = screen_height * backbuffer_num;
	var_info.activate = FB_ACTIVATE_VBL;
	if(ioctl(fb_fd, FBIOPAN_DISPLAY, &var_info) != 0)
	{
		printf("Error during ioctl to pan display!\n");
		return gcvFALSE;
	}
	backbuffer_num = (backbuffer_num + 1) % BUFFER_NUM;
#else
	dma_copy_in_vmem(fb_phyaddr + shift, test2D->dstPhyAddr, data_size);
#endif

#endif
//	gcoSURF_Unlock(g_TargetSurf,bits);
#endif				
	printf("\n");

    /* stop rendering. */
    return gcvTRUE;
}

void hw_mixer_open(int width, int height)
{
	struct fb_var_screeninfo s_vscr_info;
	fb_fd = open(FBDEV_FILENAME, O_RDWR);
	if (fb_fd< 0)
	{
		printf("Unable to open framebuffer %s!  open returned: %i\n", FBDEV_FILENAME, fb_fd);
		return;
	}

        /** frame buffer display configuration get */
        if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &s_vscr_info) != 0)
        {
                printf("Error during ioctl to get framebuffer parameters!\n");
                return;
        }

	screen_width = s_vscr_info.xres;
	screen_height = s_vscr_info.yres;
	g_TargetWidth = s_vscr_info.xres;	
	g_TargetHeight = (screen_width * ((float)height / width)) < screen_height ? 
		(screen_width * ((float)height / width)) : screen_height; 

	shift = ((screen_height - g_TargetHeight) / 2) * screen_width * s_vscr_info.bits_per_pixel / 8;
	screen_size = screen_width * screen_height * s_vscr_info.bits_per_pixel / 8;

	if (s_vscr_info.bits_per_pixel == 32) {
		g_TargetFormat = gcvSURF_A8R8G8B8;
	} else if (s_vscr_info.bits_per_pixel == 16) {
		g_TargetFormat = gcvSURF_R5G6B5;
	}

	if (ioctl(fb_fd, SWICHFB_GET_PHYS, &fb_phyaddr) < 0)
	{
		printf("Error to get framebuffer real physical address!\n");
		close (fb_fd);
		return;
	}
#ifdef FB_MULTI_BUFFER
	s_vscr_info.yres_virtual = s_vscr_info.yres_virtual * BUFFER_NUM;

	if (ioctl(fb_fd, FBIOPUT_VSCREENINFO, &s_vscr_info) != 0)
	{
		printf("Error during ioctl to set framebuffer parameters!\n");
		return;
	}
#endif


	hw_mixer_init();
	if(creat_srcsurf(width, height, gcvSURF_I420)) {
		printf("creat_srcsurf error!\n");
		while(1);
	}
}

void hw_mixer_close()
{
	hw_mixer_exit();
	close(fb_fd);
}

/*******************************************************************************
**
**  hw_mixer_draw()
**
**  YUV2RGB Convert entry point.
**
**  INPUT:
**
**      position(x,y),yuv source file info(width,height,format,address).
**
**  OUTPUT:
**
**      Nothing.
**
**  RETURN:
**
**      int
**		 result.
*/
int hw_mixer_draw(int x, int y ,int width,int height,int format,unsigned long yaddr,unsigned long cbaddr,unsigned long craddr)
{

	/* Assume failure. */
	int result = -1;

	format = gcvSURF_I420;
	result = run(width,height,format,yaddr,cbaddr,craddr);

	return result;
}

