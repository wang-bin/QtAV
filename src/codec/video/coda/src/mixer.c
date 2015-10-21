	//------------------------------------------------------------------------------
// File: Mixer.c
//
// Copyright (c) 2006, Chips & Media.  All rights reserved.
//------------------------------------------------------------------------------


#include "vpuapi.h"
#include "regdefine.h"
#include "mixer.h"
#if defined(PLATFORM_LINUX) || defined(PLATFORM_WIN32)
#include "stdio.h"
#endif
//------------------------------------------------------------------------------
// MIXER REGISTER ADDRESS
//------------------------------------------------------------------------------
#define MIX_BASE                0x1000000
#define DISP_MIX                0x2000000

#define MIX_INT                 (MIX_BASE + 0x044)

#define MIX_STRIDE_Y		    (MIX_BASE + 0x144)
#define MIX_STRIDE_CB	    	(MIX_BASE + 0x148)
#define MIX_STRIDE_CR   		(MIX_BASE + 0x14c)

#define MIX_ADDR_Y              (MIX_BASE + 0x138)
#define MIX_ADDR_CB             (MIX_BASE + 0x13C)
#define MIX_ADDR_CR             (MIX_BASE + 0x140)

#define MIX_RUN                 (MIX_BASE + 0x120)

#define DISP_TOTAL_SAMPLE       (DISP_MIX + 0x00C)
#define DISP_ACTIVE_SAMPLE      (DISP_MIX + 0x010)
#define DISP_HSYNC_START_END    (DISP_MIX + 0x014)
#define DISP_VSYNC_TOP_START    (DISP_MIX + 0x018)
#define DISP_VSYNC_TOP_END      (DISP_MIX + 0x01C)
#define DISP_VSYNC_BOT_START    (DISP_MIX + 0x020)
#define DISP_VSYNC_BOT_END      (DISP_MIX + 0x024)
#define DISP_ACTIVE_REGION_TOP  (DISP_MIX + 0x02C)
#define DISP_ACTIVE_REGION_BOT  (DISP_MIX + 0x030)


#define MIX_MIX_INTRPT			(MIX_BASE + 0x0000)
#define MIX_SYNC_STATE			(MIX_BASE + 0x0004)
#define MIX_SYNC_CTRL			(MIX_BASE + 0x0008)
#define MIX_TOTAL_SAMPLE		(MIX_BASE + 0x000c)
#define MIX_ACTIVE_SAMPLE		(MIX_BASE + 0x0010)
#define MIX_HSYNC_START_END	    (MIX_BASE + 0x0014)
#define MIX_VSYNC_TOP_START	    (MIX_BASE + 0x0018)
#define MIX_VSYNC_TOP_END		(MIX_BASE + 0x001c)
#define MIX_VSYNC_BOT_START	    (MIX_BASE + 0x0020)
#define MIX_VSYNC_BOT_END		(MIX_BASE + 0x0024)
#define MIX_ACT_REGION_SAMPLE	(MIX_BASE + 0x0028)
#define MIX_ACT_REGION_TOP		(MIX_BASE + 0x002c)
#define MIX_ACT_REGION_BOT		(MIX_BASE + 0x0030)
#define MIX_TOP_START			(MIX_BASE + 0x0034)
#define MIX_BOT_START			(MIX_BASE + 0x0038)
#define MIX_LINE_INC			(MIX_BASE + 0x003c)
#define MIX_LATCH_PARAM_CTRL	(MIX_BASE + 0x0040)
#define MIX_INTERRUPT			(MIX_BASE + 0x0044)

#define MIX_LAYER_CTRL			(MIX_BASE + 0x0100)
#define MIX_LAYER_ORDER		    (MIX_BASE + 0x0104)
#define MIX_BIG_ENDIAN			(MIX_BASE + 0x0108)
#define MIX_L0_BG_COLOR		    (MIX_BASE + 0x0110)
#define MIX_L1_CTRL			    (MIX_BASE + 0x0120)
#define MIX_L1_LSIZE			(MIX_BASE + 0x0124)
#define MIX_L1_SSIZE			(MIX_BASE + 0x0128)
#define MIX_L1_LPOS			    (MIX_BASE + 0x012c)
#define MIX_L1_SPOS			    (MIX_BASE + 0x0130)
#define MIX_L1_BG_COLOR		    (MIX_BASE + 0x0134)
#define MIX_L1_Y_SADDR			(MIX_BASE + 0x0138)
#define MIX_L1_CB_SADDR		    (MIX_BASE + 0x013c)
#define MIX_L1_CR_SADDR		    (MIX_BASE + 0x0140)
#define MIX_L1_Y_STRIDE		    (MIX_BASE + 0x0144)
#define MIX_L1_CB_STRIDE		(MIX_BASE + 0x0148)
#define MIX_L1_CR_STRIDE		(MIX_BASE + 0x014c)

#define MAX_DISP_PIC_WIDTH	1920
#define MAX_DISP_PIC_HEIGHT 1088



int  DisplayMixer(FrameBuffer *fp, int width, int height)
{
#ifdef CNM_FPGA_PLATFORM
#ifdef FPGA_LX_330
	int stride;
    int  staX, staY;
	
	width = ((width+15)&~15);
	height = ((height+15)&~15);
	stride = fp->stride;

    staX = (MAX_DISP_PIC_WIDTH - width) / 2;
    if (height > MAX_DISP_PIC_HEIGHT)
        staY = 0;
    else
        staY = (MAX_DISP_PIC_HEIGHT - height) / 2;
    if (staX % 16)
        staX = (staX + 15) / 16 * 16;

	if (staX < 0)
		staX = 0;

	if (staY < 0)
		staY = 0;

	if (width > MAX_DISP_PIC_WIDTH)
	{
		stride = width;
		width = MAX_DISP_PIC_WIDTH;
	}

	if (height > MAX_DISP_PIC_HEIGHT)
		height = MAX_DISP_PIC_HEIGHT;
	
    VpuWriteReg(core_idx, MIX_L0_BG_COLOR,  (0<<16) | (0x80 << 8) | 0x80);
	
    VpuWriteReg(core_idx, MIX_L1_LSIZE, (height<<12) | width);
    VpuWriteReg(core_idx, MIX_L1_SSIZE, (height<<12) | width);
    VpuWriteReg(core_idx, MIX_L1_LPOS,  (staY<<12) | staX);
	
    VpuWriteReg(core_idx, MIX_STRIDE_Y,  stride);
    VpuWriteReg(core_idx, MIX_STRIDE_CB, stride/2);
    VpuWriteReg(core_idx, MIX_STRIDE_CR, stride/2);
	

    VpuWriteReg(core_idx, MIX_ADDR_Y,  fp->bufY);
    VpuWriteReg(core_idx, MIX_ADDR_CB, fp->bufCb);
    VpuWriteReg(core_idx, MIX_ADDR_CR, fp->bufCr);
	
    VpuWriteReg(core_idx, DISP_HSYNC_START_END,  ((0x7d7-40)<<12)|(0x82f-40)); // horizontal center
    VpuWriteReg(core_idx, DISP_ACTIVE_REGION_TOP,((0x014-2 )<<12)|(0x230-2 ));
    VpuWriteReg(core_idx, DISP_ACTIVE_REGION_BOT,((0x247-2 )<<12)|(0x463-2 ));
	
    VpuWriteReg(core_idx, MIX_LAYER_CTRL,0x3);                       // backgroup on
    VpuWriteReg(core_idx, MIX_RUN, 0x92);                            // on, vdec, from sdram

#endif
#endif
	
	return 1;
}
void InitMixerInt()
{
#ifdef CNM_FPGA_PLATFORM
#ifdef FPGA_LX_330
	VpuWriteReg(core_idx, MIX_MIX_INTRPT, 0x05000000);
	VpuWriteReg(core_idx, MIX_INT, 0);
#endif
#endif
}

int MixerIsIdle()
{
#ifdef CNM_FPGA_PLATFORM
#ifdef FPGA_LX_330
	int  data;

	data = VpuReadReg(core_idx, MIX_INT);
	
	if ((data&1))
		 VpuWriteReg(core_idx, MIX_INT, 0);

	return (int)(data&1);
#endif
#endif
	return 0;
}


void WaitMixerInt()
{
#ifdef CNM_FPGA_PLATFORM
#ifdef FPGA_LX_330
    int  data;

    if (VpuReadReg(0, MIX_INT) == 1)
        VpuWriteReg(0, MIX_INT, 0);

    while (1) {
        data = VpuReadReg(0, MIX_INT);
        if (data & 1)
            break;
    }
    VpuWriteReg(0, MIX_INT, 0);
#endif
#endif
}



typedef enum { YUV444, YUV422, YUV420, NV12, NV21, YUV400, YUYV, YVYU, UYVY, VYUY, YYY, RGB_PLANAR, RGB32, RGB24, RGB16, YUV2RGB_COLOR_FORMAT_MAX } yuv2rgb_color_format;
static void vpu_yuv2rgb(int width, int height, yuv2rgb_color_format format, unsigned char *src, unsigned char *rgba, int chroma_reverse);
yuv2rgb_color_format convert_vpuapi_format_to_yuv2rgb_color_format(int yuv_format, int interleave);

#if defined(PLATFORM_WIN32) 
#include <windows.h>
#include <stdio.h>

#pragma comment(lib,"User32.lib")
#pragma comment(lib,"gdi32.lib")

#define DRAW_IN_WINDOW
typedef struct
{
	BITMAPINFOHEADER bmih;
	RGBQUAD rgb[256];
} BITMAPINFO2;


LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

typedef struct 
{
	HBITMAP		s_dib_section;
	void		*s_dib_buffer;
	HDC			s_hdc_memory;
	HGDIOBJ		s_old_hobject;
	HWND	s_hWnd;
	int wndShow;	
} sw_mixer_context_t;

static sw_mixer_context_t s_mixer[MAX_VPU_CORE_NUM*MAX_NUM_INSTANCE];
#endif	//defined(PLATFORM_WIN32) 


#ifdef PLATFORM_LINUX

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/time.h>
#include <linux/fb.h>

typedef struct 
{
	int s_fd;
	unsigned char *s_scr_ptr;
	unsigned char *s_rgb_ptr;
	unsigned long s_product;
	int s_fb_stride;
	int s_fb_height;
	int s_fb_width;
	int s_fb_bpp;	
} sw_mixer_context_t;

static struct fb_var_screeninfo vscr_info;
static struct fb_fix_screeninfo fscr_info;


static sw_mixer_context_t s_mixer[MAX_VPU_CORE_NUM*MAX_NUM_INSTANCE];
#endif //PLATFORM_LINUX


int sw_mixer_open(int core_idx, int width, int height)
{	
#ifdef PLATFORM_LINUX	
	
	sw_mixer_context_t *mixer = &s_mixer[core_idx];
	char fb_name[256];
	

	if (mixer->s_fd)
		return 1;
#ifdef ANDROID
	sprintf(fb_name, "/dev/graphics/fb%d", core_idx);
#else
	sprintf(fb_name, "/dev/fb%d", core_idx);
#endif

	mixer->s_fd = open(fb_name, O_RDWR);
	if (mixer->s_fd< 0)
	{
		VLOG(ERR, "Unable to open framebuffer %s\n", fb_name);
		return 0;
	}
	/** frame buffer display configuration get */
	if (ioctl(mixer->s_fd, FBIOGET_VSCREENINFO, &vscr_info) != 0 || ioctl(mixer->s_fd, FBIOGET_FSCREENINFO, &fscr_info) != 0)
	{
		VLOG(ERR, "Error during ioctl to get framebuffer parameters!\n");
		return 0;
	}
	mixer->s_fb_bpp = vscr_info.bits_per_pixel/8;
	mixer->s_fb_width = vscr_info.xres;
	mixer->s_fb_stride = fscr_info.line_length;
	mixer->s_fb_height = vscr_info.yres;
	mixer->s_product= mixer->s_fb_stride * mixer->s_fb_height;
	/** memory map frame buf memory */
	mixer->s_scr_ptr = (unsigned char*) mmap(0, mixer->s_product, PROT_READ | PROT_WRITE, MAP_SHARED, mixer->s_fd, 0);
	if (mixer->s_scr_ptr == NULL)
	{
		VLOG(ERR, "in %s Failed to mmap framebuffer memory!\n", __func__);
		close (mixer->s_fd);
		return 0;
	}
	
	mixer->s_rgb_ptr = osal_malloc(width*height*mixer->s_fb_bpp);
	if (mixer->s_rgb_ptr == NULL)
	{
		VLOG(ERR, "in %s Failed to allocate rgb memory!\n", __func__);
		close (mixer->s_fd);
		return 0;
	}
	VLOG(TRACE, "Successfully opened %s for display.\n", fb_name);
	VLOG(TRACE, "mmap framebuffer memory =%p product=%d\n", mixer->s_scr_ptr, (unsigned int)mixer->s_product, (int)width);
	VLOG(TRACE, "Display Size: width=%d, height=%d, stride=%d, Bit Count: %d \n", (int)mixer->s_fb_width, (int)mixer->s_fb_height, (int)mixer->s_fb_stride, (int)mixer->s_fb_bpp);
		
	
#endif	//#ifdef PLATFORM_LINUX

#if defined(PLATFORM_WIN32) 
	sw_mixer_context_t *mixer = &s_mixer[core_idx];

	HDC hdc;
	BITMAPINFO2	bmi2;
	if (mixer->s_dib_section)
		return 0;

	memset(&bmi2, 0x00, sizeof(bmi2));
	bmi2.bmih.biSize = sizeof(BITMAPINFOHEADER);
	bmi2.bmih.biWidth = width;
	bmi2.bmih.biHeight = -(height);
	bmi2.bmih.biPlanes = 1;
	bmi2.bmih.biBitCount = 32;
	bmi2.bmih.biCompression = BI_RGB;
	if( bmi2.bmih.biBitCount == 16 )
	{
		bmi2.bmih.biCompression = BI_BITFIELDS;
		*(DWORD *)(&bmi2.rgb[0]) = 0xF800;
		*(DWORD *)(&bmi2.rgb[1]) = 0x07e0;
		*(DWORD *)(&bmi2.rgb[2]) = 0x001F;
	}
	else
		bmi2.bmih.biCompression = BI_RGB;
	
	mixer->s_dib_section = CreateDIBSection(
		NULL,
		(PBITMAPINFO)&bmi2,
		DIB_RGB_COLORS,
		&mixer->s_dib_buffer,
		NULL,
		0
		);
	

	hdc = GetDC( mixer->s_hWnd );
	mixer->s_hdc_memory = CreateCompatibleDC( hdc );
	if (!mixer->s_hdc_memory)
		return 0;
	ReleaseDC(mixer->s_hWnd, hdc);
	
	mixer->s_old_hobject = SelectObject(mixer->s_hdc_memory, mixer->s_dib_section);
	if (!mixer->s_old_hobject)
		return 0;

#ifdef DRAW_IN_WINDOW
	{
		WNDCLASSEX wcex;
		
		wcex.cbSize = sizeof(WNDCLASSEX); 
		wcex.style   = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = WndProc;
		wcex.cbClsExtra  = 0;
		wcex.cbWndExtra  = 0;
		wcex.hInstance  = GetModuleHandle(NULL);
		wcex.hIcon   = LoadIcon(NULL, IDI_APPLICATION);
		wcex.hCursor  = LoadCursor(NULL, IDC_ARROW);
		wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
		wcex.lpszMenuName = NULL;
		wcex.lpszClassName = "CNMMIXER";
		wcex.hIconSm  = LoadIcon(NULL, IDI_APPLICATION);

		RegisterClassEx(&wcex);
		
		mixer->s_hWnd = CreateWindow("CNMMIXER", "C&M YuvViewer", WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT/*width*/, CW_USEDEFAULT/*height*/, NULL, NULL, GetModuleHandle(NULL), NULL);
		MoveWindow(mixer->s_hWnd, core_idx*width, 0, width+16, height+38, TRUE);
		mixer->wndShow = 0;
	}
#endif
#endif //#if defined(PLATFORM_WIN32) 
	return 1;
	
}


int sw_mixer_draw(int core_idx, int x, int y, int width, int height, int format, unsigned char* pbImage, int interleave)
{
#ifdef PLATFORM_LINUX
	sw_mixer_context_t *mixer = &s_mixer[core_idx];
	unsigned char* src_ptr = pbImage;
	unsigned char* dest_ptr = (unsigned char *)mixer->s_scr_ptr;
	unsigned char* src_cpy_ptr;
	unsigned char* dest_cpy_ptr ;
	int i;
	yuv2rgb_color_format color_format;
	if (mixer->s_fd < 0)
		return 0;
	
	color_format  = convert_vpuapi_format_to_yuv2rgb_color_format(format, interleave);
	
	if (color_format == YUV2RGB_COLOR_FORMAT_MAX)
	{
		printf("[ERROR]not supported image format\n");
		return 0;
	}

	if (mixer->s_fb_bpp == 4)
		vpu_yuv2rgb(width, height, color_format, src_ptr,  mixer->s_rgb_ptr, 1);
	else
	{
		printf("[ERROR]not supported bit count\n");
		return 0;
	}
	
	src_cpy_ptr = mixer->s_rgb_ptr;
	dest_cpy_ptr = dest_ptr;

	for (i = 0; i < height; ++i)
	{
		if (height > mixer->s_fb_height)
			break;

		if (mixer->s_fb_stride > (width*mixer->s_fb_bpp))
			osal_memcpy(dest_cpy_ptr, src_cpy_ptr, (width*mixer->s_fb_bpp));
		else
			osal_memcpy(dest_cpy_ptr, src_cpy_ptr, mixer->s_fb_stride);
		src_cpy_ptr += (width*mixer->s_fb_bpp);
		dest_cpy_ptr += (mixer->s_fb_stride);
	}
#endif	//#ifdef PLATFORM_LINUX

#if defined(PLATFORM_WIN32) 
	sw_mixer_context_t *mixer = &s_mixer[core_idx];
	HDC hdc_screen;
	RECT rc;
	int dispWidth;
	int dispHeight;
	yuv2rgb_color_format color_format;
	
	
	GetClientRect(mixer->s_hWnd, &rc);
	dispWidth = rc.right - rc.left;
	dispHeight = rc.bottom - rc.top;

	if (!mixer->wndShow)
	{
		ShowWindow(mixer->s_hWnd, SW_SHOW);
		UpdateWindow(mixer->s_hWnd);
		//SetForegroundWindow(mixer->s_hWnd);
		mixer->wndShow = 1;
	}
	else
	{
		MSG msg={0};
		while(1)
		{
			if (PeekMessage(&msg, mixer->s_hWnd, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
				break;
		}		
	}
	

#ifdef DRAW_IN_WINDOW
	hdc_screen = GetDC( mixer->s_hWnd );
#else
	hdc_screen = GetDC( NULL );
#endif

	color_format  = convert_vpuapi_format_to_yuv2rgb_color_format(format, interleave);

	if (color_format == YUV2RGB_COLOR_FORMAT_MAX)
	{
		printf("[ERROR]not supported image format\n");
		return 0;
	}

	vpu_yuv2rgb(width, height, color_format, pbImage, (unsigned char *)mixer->s_dib_buffer, 1);

	if( hdc_screen )
	{
		if (width != dispWidth || height != dispHeight)
		{
			StretchBlt(hdc_screen, 0, 0, dispWidth, dispHeight, mixer->s_hdc_memory, x, y, width, height, SRCCOPY); 
		}
		else
		{
			BitBlt( hdc_screen, x, y, width, height, mixer->s_hdc_memory, 0,0, SRCCOPY );	
		}


		ReleaseDC(mixer->s_hWnd, hdc_screen);
		hdc_screen = NULL;
	}

	//SetForegroundWindow(s_hWnd);
#endif	//#if defined(PLATFORM_WIN32) 
	return 1;

}

void sw_mixer_close(int core_idx)
{
#ifdef PLATFORM_LINUX
	sw_mixer_context_t *mixer = &s_mixer[core_idx];
	if (mixer->s_scr_ptr)
	{
		munmap(mixer->s_scr_ptr, mixer->s_product);
		mixer->s_scr_ptr = NULL;
	}
	if (mixer->s_rgb_ptr)
	{
		osal_free(mixer->s_rgb_ptr);
		mixer->s_rgb_ptr = NULL;
	}

	if (mixer->s_fd)
	{
		close(mixer->s_fd);
		mixer->s_fd = 0;
	}
#endif

#if defined(PLATFORM_WIN32) 
	sw_mixer_context_t *mixer = &s_mixer[core_idx];
	if (mixer->s_old_hobject)
	{
		SelectObject(mixer->s_hdc_memory,mixer->s_old_hobject);
		mixer->s_old_hobject = NULL;
	}
	if (mixer->s_hdc_memory)
	{
		DeleteDC(mixer->s_hdc_memory);
		mixer->s_hdc_memory = NULL;
	}
	if (mixer->s_dib_section)
	{
		DeleteObject(mixer->s_dib_section);
		mixer->s_dib_section = NULL;
	}

	if (mixer->s_hWnd)
	{
		DestroyWindow(mixer->s_hWnd);
		mixer->s_hWnd = NULL;
	}
#endif	//#if defined(PLATFORM_WIN32) 
}

#if defined(PLATFORM_WIN32) 
LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    switch (message)
    {
    case WM_CREATE:
        return (0);
		
    case WM_PAINT:
        hdc = BeginPaint (hwnd, &ps);
        
        EndPaint (hwnd, &ps);
        return (0);
		
	case WM_CLOSE:
		return (0);

    case WM_DESTROY:
        PostQuitMessage (0);
        return (0);
    }
    return DefWindowProc (hwnd, message, wParam, lParam);
}

#endif //defined(PLATFORM_WIN32) 

// inteleave : 0 (chroma separate mode), 1 (cbcr interleave mode), 2 (crcb interleave mode)
yuv2rgb_color_format convert_vpuapi_format_to_yuv2rgb_color_format(int yuv_format, int interleave) 
{
	//typedef enum { YUV444, YUV422, YUV420, NV12, NV21,  YUV400, YUYV, YVYU, UYVY, VYUY, YYY, RGB_PLANAR, RGB32, RGB24, RGB16 } yuv2rgb_color_format;
	yuv2rgb_color_format format;

	switch(yuv_format)
	{
	case FORMAT_400: format = YUV400; break;
	case FORMAT_444: format = YUV444; break;
	case FORMAT_224:
	case FORMAT_422: format = YUV422; break;
	case FORMAT_420: 
		if (interleave == 0)
			format = YUV420; 
		else if (interleave == 1)
			format = NV12;				
		else
			format = NV21; 
		break;
	default:
		format = YUV2RGB_COLOR_FORMAT_MAX; 
	}

	return format;
}


void vpu_yuv2rgb(int width, int height, yuv2rgb_color_format format, unsigned char *src, unsigned char *rgba, int cbcr_reverse)
{
#define vpu_clip(var) ((var>=255)?255:(var<=0)?0:var)
	int j, i;
	int c, d, e;

	unsigned char* line = rgba;
	unsigned char* cur;
	unsigned char* y = NULL;
	unsigned char* u = NULL;
	unsigned char* v = NULL;
	unsigned char* misc = NULL;

	int frame_size_y;
	int frame_size_uv;
	int frame_size;
	int t_width;

	frame_size_y = width*height;

	if( format == YUV444 || format == RGB_PLANAR)
		frame_size_uv = width*height;
	else if( format == YUV422 )
		frame_size_uv = (width*height)>>1;
	else if( format == YUV420 || format == NV12 || format == NV21 )
		frame_size_uv = (width*height)>>2;
	else 
		frame_size_uv = 0;

	if( format == YUYV || format == YVYU  || format == UYVY  || format == VYUY )
		frame_size = frame_size_y*2;
	else if( format == RGB32 )
		frame_size = frame_size_y*4;
	else if( format == RGB24 )
		frame_size = frame_size_y*3;
	else if( format == RGB16 )
		frame_size = frame_size_y*2;
	else
		frame_size = frame_size_y + frame_size_uv*2; 

	t_width = width;


	if( format == YUYV || format == YVYU  || format == UYVY  || format == VYUY ) {
		misc = src;
	}
	else if( format == NV12 || format == NV21) {	
		y = src;
		misc = src + frame_size_y;
	}
	else if( format == RGB32 || format == RGB24 || format == RGB16 ) {
		misc = src;
	}
	else {
		y = src;
		u = src + frame_size_y;
		v = src + frame_size_y + frame_size_uv;		
	}

	if( format == YUV444 ){

		for( j = 0 ; j < height ; j++ ){
			cur = line;
			for( i = 0 ; i < width ; i++ ){
				c = y[j*width+i] - 16;
				d = u[j*width+i] - 128;
				e = v[j*width+i] - 128;

				if (!cbcr_reverse) {
					d = u[j*width+i] - 128;
					e = v[j*width+i] - 128;
				} else {
					e = u[j*width+i] - 128;
					e = v[j*width+i] - 128;
				}
				(*cur) = vpu_clip(( 298 * c           + 409 * e + 128) >> 8);cur++;
				(*cur) = vpu_clip(( 298 * c - 100 * d - 208 * e + 128) >> 8);cur++;
				(*cur) = vpu_clip(( 298 * c + 516 * d           + 128) >> 8);cur++;
				(*cur) = 0; cur++;
			}
			line += t_width<<2;
		}
	}
	else if( format == YUV422){
		for( j = 0 ; j < height ; j++ ){
			cur = line;
			for( i = 0 ; i < width ; i++ ){
				c = y[j*width+i] - 16;
				d = u[j*(width>>1)+(i>>1)] - 128;
				e = v[j*(width>>1)+(i>>1)] - 128;

				if (!cbcr_reverse) {
					d = u[j*(width>>1)+(i>>1)] - 128;
					e = v[j*(width>>1)+(i>>1)] - 128;
				} else {
					e = u[j*(width>>1)+(i>>1)] - 128;
					d = v[j*(width>>1)+(i>>1)] - 128;
				}

				(*cur) = vpu_clip(( 298 * c           + 409 * e + 128) >> 8);cur++;
				(*cur) = vpu_clip(( 298 * c - 100 * d - 208 * e + 128) >> 8);cur++;
				(*cur) = vpu_clip(( 298 * c + 516 * d           + 128) >> 8);cur++;
				(*cur) = 0; cur++;
			}
			line += t_width<<2;
		}
	}
	else if( format == YUYV || format == YVYU  || format == UYVY  || format == VYUY )
	{
		unsigned char* t = misc;
		for( j = 0 ; j < height ; j++ ){
			cur = line;
			for( i = 0 ; i < width ; i+=2 ){
				switch( format) {
				case YUYV:
					c = *(t  ) - 16;
					if (!cbcr_reverse) {
						d = *(t+1) - 128;
						e = *(t+3) - 128;
					} else {
						e = *(t+1) - 128;
						d = *(t+3) - 128;
					}
					break;
				case YVYU:
					c = *(t  ) - 16;
					if (!cbcr_reverse) {
						d = *(t+3) - 128;
						e = *(t+1) - 128;
					} else {
						e = *(t+3) - 128;
						d = *(t+1) - 128;
					}
					break;
				case UYVY:
					c = *(t+1) - 16;
					if (!cbcr_reverse) {
						d = *(t  ) - 128;
						e = *(t+2) - 128;
					} else {
						e = *(t  ) - 128;
						d = *(t+2) - 128;
					}
					break;
				case VYUY:
					c = *(t+1) - 16;
					if (!cbcr_reverse) {
						d = *(t+2) - 128;
						e = *(t  ) - 128;
					} else {
						e = *(t+2) - 128;
						d = *(t  ) - 128;
					}
					break;
				default: // like YUYV
					c = *(t  ) - 16;
					if (!cbcr_reverse) {
						d = *(t+1) - 128;
						e = *(t+3) - 128;
					} else {
						e = *(t+1) - 128;
						d = *(t+3) - 128;
					}
					break;
				}

				(*cur) = vpu_clip(( 298 * c           + 409 * e + 128) >> 8);cur++;
				(*cur) = vpu_clip(( 298 * c - 100 * d - 208 * e + 128) >> 8);cur++;
				(*cur) = vpu_clip(( 298 * c + 516 * d           + 128) >> 8);cur++;
				(*cur) = 0;cur++;

				switch( format) {
				case YUYV:
				case YVYU:
					c = *(t+2) - 16;
					break;

				case VYUY:
				case UYVY:
					c = *(t+3) - 16;
					break;
				default: // like YUYV
					c = *(t+2) - 16;
					break;
				}

				(*cur) = vpu_clip(( 298 * c           + 409 * e + 128) >> 8);cur++;
				(*cur) = vpu_clip(( 298 * c - 100 * d - 208 * e + 128) >> 8);cur++;
				(*cur) = vpu_clip(( 298 * c + 516 * d           + 128) >> 8);cur++;
				(*cur) = 0; cur++;

				t += 4;
			}
			line += t_width<<2;
		}
	}
	else if( format == YUV420 || format == NV12 || format == NV21){
		for( j = 0 ; j < height ; j++ ){
			cur = line;
			for( i = 0 ; i < width ; i++ ){
				c = y[j*width+i] - 16;
				if (format == YUV420) {
					if (!cbcr_reverse) {
						d = u[(j>>1)*(width>>1)+(i>>1)] - 128;
						e = v[(j>>1)*(width>>1)+(i>>1)] - 128;					
					} else {
						e = u[(j>>1)*(width>>1)+(i>>1)] - 128;
						d = v[(j>>1)*(width>>1)+(i>>1)] - 128;	
					}
				}
				else if (format == NV12) {
					if (!cbcr_reverse) {
						d = misc[(j>>1)*width+(i>>1<<1)  ] - 128;
						e = misc[(j>>1)*width+(i>>1<<1)+1] - 128;					
					} else {
						e = misc[(j>>1)*width+(i>>1<<1)  ] - 128;
						d = misc[(j>>1)*width+(i>>1<<1)+1] - 128;	
					}
				}
				else { // if (m_color == NV21)
					if (!cbcr_reverse) {
						d = misc[(j>>1)*width+(i>>1<<1)+1] - 128;
						e = misc[(j>>1)*width+(i>>1<<1)  ] - 128;					
					} else {
						e = misc[(j>>1)*width+(i>>1<<1)+1] - 128;
						d = misc[(j>>1)*width+(i>>1<<1)  ] - 128;		
					}
				}
				(*cur) = vpu_clip(( 298 * c           + 409 * e + 128) >> 8);cur++;
				(*cur) = vpu_clip(( 298 * c - 100 * d - 208 * e + 128) >> 8);cur++;
				(*cur) = vpu_clip(( 298 * c + 516 * d           + 128) >> 8);cur++;
				(*cur) = 0; cur++;
			}
			line += t_width<<2;
		}
	}
	else if( format == RGB_PLANAR ){
		for( j = 0 ; j < height ; j++ ){
			cur = line;
			for( i = 0 ; i < width ; i++ ){
				(*cur) = y[j*width+i];cur++;
				(*cur) = u[j*width+i];cur++;
				(*cur) = v[j*width+i];cur++;
				(*cur) = 0; cur++;
			}
			line += t_width<<2;
		}
	}
	else if( format == RGB32 ){
		for( j = 0 ; j < height ; j++ ){
			cur = line;
			for( i = 0 ; i < width ; i++ ){
				(*cur) = misc[j*width*4+i];cur++;	// R
				(*cur) = misc[j*width*4+i+1];cur++;	// G
				(*cur) = misc[j*width*4+i+2];cur++;	// B
				(*cur) = misc[j*width*4+i+3];cur++;	// A
			}
			line += t_width<<2;
		}
	}
	else if( format == RGB24 ){
		for( j = 0 ; j < height ; j++ ){
			cur = line;
			for( i = 0 ; i < width ; i++ ){
				(*cur) = misc[j*width*3+i];cur++;	// R
				(*cur) = misc[j*width*3+i+1];cur++;	// G
				(*cur) = misc[j*width*3+i+2];cur++;	// B
				(*cur) = 0; cur++;
			}
			line += t_width<<2;
		}
	}
	else if( format == RGB16 ){
		for( j = 0 ; j < height ; j++ ){
			cur = line;
			for( i = 0 ; i < width ; i++ ){
				int tmp = misc[j*width*2+i]<<8 | misc[j*width*2+i+1];
				(*cur) = ((tmp>>11)&0x1F<<3);cur++; // R(5bit)
				(*cur) = ((tmp>>5 )&0x3F<<2);cur++; // G(6bit)
				(*cur) = ((tmp    )&0x1F<<3);cur++; // B(5bit)
				(*cur) = 0; cur++;
			}
			line += t_width<<2;
		}
	}
	else { // YYY
		for( j = 0 ; j < height ; j++ ){
			cur = line;
			for( i = 0 ; i < width ; i++ ){
				(*cur) = y[j*width+i]; cur++;
				(*cur) = y[j*width+i]; cur++;
				(*cur) = y[j*width+i]; cur++;
				(*cur) = 0; cur++;
			}
			line += t_width<<2;
		}	
	}
}


static void gdi_set_pic_info(Uint32 core_idx, int index, int cbcrIntlv, int picX, int picY, int stride, FrameBuffer rotatorOutput, int MapType, int format);
static void set_descriptor(Uint32 core_idx, int desc_num, int last_flag, int op_mode, int fill_2byte, int color1, int color0, int interval, int unit_size_y, int unit_size_x, int next_desc_addr, int src_dimension, int src_field_mode, int src_ycbcr, int src_index, int src_pos_y, int src_pos_x, int blk_height, int blk_width, int dst_dimension, int dst_field_mode, int dst_ycbcr, int dst_index, int dst_pos_y, int dst_pos_x) ;
static void write_descriptor(Uint32 core_idx, int desc_num, int dmac_mode, int desc_addr);
static void run_dmac(Uint32 core_idx, int dmac_mode, int desc_addr);

void  dmac_mvc_pic_copy(Uint32 core_idx, FrameBuffer srcFrame, FrameBuffer dstFrame, int size_y, int size_x,int map_type, int interleave, int viewIdx)
{
	int unit_size_y;
	int unit_size_x;
	int src_idx;
	int dst_idx;
	int dst_y;

	src_idx = srcFrame.myIndex;
	dst_idx = dstFrame.myIndex;

	unit_size_y = 16;
	unit_size_x = 16;

	if ((unit_size_y*unit_size_x/8) > 64) 
	{
		printf("[ERROR] unit size (y=%d, x=%d) is over 64 word",unit_size_y,unit_size_x);
	}

	dst_y = 0;
	if (viewIdx)
		dst_y = size_y/2;

	gdi_set_pic_info(core_idx, dst_idx, interleave, size_x, size_y, size_x, dstFrame, map_type, 0);

	//---------------------------------------
	// make descriptors for picture copy
	//---------------------------------------
	// set descriptor - Y
	set_descriptor (
		core_idx, 
		0				, // descriptor number
		0					, // last descriptor
		0					, // 0-copy, 1-color fill
		0                , // fill with 2 byte
		0                , // color1
		0                , // color0
		0x10              , // interval
		unit_size_y         , // unit vertical size
		unit_size_x         , // unit horizonal size
		0               , // next descriptor address
		1                , // 1-2D, 0-1D
		2                , // 0-frame, 2-top, 3-bottom
		0                , // 0-Y, 2-Cb, 3-Cr
		src_idx             , // source frame index
		0               , // source y position
		0               , // source x position
		size_y/2              , // block height
		size_x              , // block width
		1                , // 1-2D, 0-1D
		0                , // 0-frame, 2-top, 3-bottom
		0                , // 0-Y, 2-Cb, 3-Cr
		dst_idx             , // destination frame index
		dst_y               , // destination y position
		0                 // destination x position
		);

	if (interleave)
	{
		// set descriptor - Cb/Cr
		set_descriptor (
			core_idx, 
			1                   , // descriptor number
			1                , // last descriptor
			0                , // 0-copy, 1-color fill
			0                , // fill with 2 byte
			0                , // color1
			0                , // color0
			0x10              , // interval
			unit_size_y         , // unit vertical size
			unit_size_x         , // unit horizonal size
			0               , // next descriptor address
			1                , // 1-2D, 0-1D
			0                , // 0-frame, 2-top, 3-bottom
			2                , // 0-Y, 2-Cb, 3-Cr
			src_idx             , // source frame index
			0               , // source y position
			0               , // source x position
			size_y/2            , // block height
			size_x              , // block width
			1                , // 1-2D, 0-1D
			0                , // 0-frame, 2-top, 3-bottom
			2                , // 0-Y, 2-Cb, 3-Cr
			dst_idx             , // destination frame index
			dst_y               , // destination y position
			0                 // destination x position
			);
	}
	else 
	{
		// set descriptor - Cb
		set_descriptor (
			core_idx, 
			1                   , // descriptor number
			0                , // last descriptor
			0                , // 0-copy, 1-color fill
			0                , // fill with 2 byte
			0                , // color1
			0                , // color0
			0x10              , // interval
			unit_size_y         , // unit vertical size
			unit_size_x         , // unit horizonal size
			0               , // next descriptor address
			1                , // 1-2D, 0-1D
			2                , // 0-frame, 2-top, 3-bottom
			2                , // 0-Y, 2-Cb, 3-Cr
			src_idx             , // source frame index
			0               , // source y position
			0               , // source x position
			size_y/2/2            , // block height
			size_x/2            , // block width
			1                , // 1-2D, 0-1D
			0                , // 0-frame, 2-top, 3-bottom
			2                , // 0-Y, 2-Cb, 3-Cr
			dst_idx             , // destination frame index
			dst_y/2               , // destination y position
			0                 // destination x position
			);

		// set descriptor - Cr
		set_descriptor (
			core_idx, 
			2                   , // descriptor number
			1                , // last descriptor
			0                , // 0-copy, 1-color fill
			0                , // fill with 2 byte
			0                , // color1
			0                , // color0
			0x10              , // interval
			unit_size_y         , // unit vertical size
			unit_size_x         , // unit horizonal size
			0               , // next descriptor address
			1                , // 1-2D, 0-1D
			2                , // 0-frame, 2-top, 3-bottom
			3               , // 0-Y, 2-Cb, 3-Cr
			src_idx             , // source frame index
			0               , // source y position
			0               , // source x position
			size_y/2/2            , // block height
			size_x/2            , // block width
			1                , // 1-2D, 0-1D
			0                , // 0-frame, 2-top, 3-bottom
			3                , // 0-Y, 2-Cb, 3-Cr
			dst_idx             , // destination frame index
			dst_y/2               , // destination y position
			0                 // destination x position
			);
	}


	//-----------------------
	// Y
	//-----------------------
	// write_descriptor to dmac
	write_descriptor (
		core_idx, 
		0     ,  // desc_num  - descriptor number
		1  ,  // dmac_mode - 1-single, 0-consecutive
		0    // desc_addr - descriptor address to load
		);

	// run_dmac
	run_dmac (
		core_idx, 
		1  ,  // dmac_mode - 1-single, 0-consecutive
		0    // desc_addr - descriptor address to load
		);

	//-----------------------
	// Cb, Cr
	//-----------------------
	if (interleave) // interleaved
	{
		// write_descriptor to dmac
		write_descriptor (
			core_idx, 
			1     ,  // desc_num  - descriptor number
			1  ,  // dmac_mode - 1-single, 0-consecutive
			0    // desc_addr - descriptor address to load
			);

		// run_dmac
		run_dmac (
			core_idx, 
			1  ,  // dmac_mode - 1-single, 0-consecutive
			0    // desc_addr - descriptor address to load
			);
	}
	else
	{
		// separate
		// write_descriptor to dmac

		write_descriptor (
			core_idx, 
			1     ,  // desc_num  - descriptor number
			1  ,  // dmac_mode - 1-single, 0-consecutive
			0    // desc_addr - descriptor address to load
			);

		// run_dmac
		run_dmac (
			core_idx, 
			1  ,  // dmac_mode - 1-single, 0-consecutive
			0    // desc_addr - descriptor address to load
			);


		// write_descriptor to dmac
		write_descriptor (
			core_idx, 
			2     ,  // desc_num  - descriptor number
			1  ,  // dmac_mode - 1-single, 0-consecutive
			0    // desc_addr - descriptor address to load
			);

		// run_dmac
		run_dmac (
			core_idx, 
			1  ,  // dmac_mode - 1-single, 0-consecutive
			0    // desc_addr - descriptor address to load
			);
	}
}



void  dmac_single_pic_copy(Uint32 core_idx, FrameBuffer srcFrame, FrameBuffer dstFrame, int src_x, int src_y, int dst_x, int dst_y, int size_y, int size_x, int stride, int map_type, int interleave)
{
	int unit_size_y;
	int unit_size_x;
	int src_idx;
	int dst_idx;

	src_idx = srcFrame.myIndex;
	dst_idx = dstFrame.myIndex;

	unit_size_y = 16;
	unit_size_x = 16;

	if ((unit_size_y*unit_size_x/8) > 64) 
	{
		printf("[ERROR] unit size (y=%d, x=%d) is over 64 word",unit_size_y,unit_size_x);
	}

	gdi_set_pic_info(core_idx, dst_idx, interleave, size_x, size_y, stride, dstFrame, map_type, 0);

	//---------------------------------------
	// make descriptors for picture copy
	//---------------------------------------
	// set descriptor - Y
	set_descriptor (
		core_idx			,
		0				, // descriptor number
		0					, // last descriptor
		0					, // 0-copy, 1-color fill
		0                , // fill with 2 byte
		0                , // color1
		0                , // color0
		0x10              , // interval
		unit_size_y         , // unit vertical size
		unit_size_x         , // unit horizontal size
		0               , // next descriptor address
		1                , // 1-2D, 0-1D
		0                , // 0-frame, 2-top, 3-bottom
		0                , // 0-Y, 2-Cb, 3-Cr
		src_idx             , // source frame index
		src_y               , // source y position
		src_x               , // source x position
		size_y              , // block height
		size_x             , // block width
		1                , // 1-2D, 0-1D
		0                , // 0-frame, 2-top, 3-bottom
		0                , // 0-Y, 2-Cb, 3-Cr
		dst_idx             , // destination frame index
		dst_y               , // destination y position
		dst_x                 // destination x position
		);

	if (interleave)
	{
		// set descriptor - Cb/Cr
		set_descriptor (
			core_idx,
			1                   , // descriptor number
			1                , // last descriptor
			0                , // 0-copy, 1-color fill
			0                , // fill with 2 byte
			0                , // color1
			0                , // color0
			0x10              , // interval
			unit_size_y         , // unit vertical size
			unit_size_x         , // unit horizontal size
			0               , // next descriptor address
			1                , // 1-2D, 0-1D
			0                , // 0-frame, 2-top, 3-bottom
			2                , // 0-Y, 2-Cb, 3-Cr
			src_idx             , // source frame index
			src_y               , // source y position
			src_x               , // source x position
			size_y/2            , // block height
			size_x              , // block width
			1                , // 1-2D, 0-1D
			0                , // 0-frame, 2-top, 3-bottom
			2                , // 0-Y, 2-Cb, 3-Cr
			dst_idx             , // destination frame index
			dst_y/2               , // destination y position
			dst_x/2                 // destination x position
			);
	}
	else 
	{
		// set descriptor - Cb
		set_descriptor (
			core_idx,
			1                   , // descriptor number
			0                , // last descriptor
			0                , // 0-copy, 1-color fill
			0                , // fill with 2 byte
			0                , // color1
			0                , // color0
			0x10              , // interval
			unit_size_y         , // unit vertical size
			unit_size_x         , // unit horizonal size
			0               , // next descriptor address
			1                , // 1-2D, 0-1D
			0                , // 0-frame, 2-top, 3-bottom
			2                , // 0-Y, 2-Cb, 3-Cr
			src_idx             , // source frame index
			src_y               , // source y position
			src_x               , // source x position
			size_y/2            , // block height
			size_x/2            , // block width
			1                , // 1-2D, 0-1D
			0                , // 0-frame, 2-top, 3-bottom
			2                , // 0-Y, 2-Cb, 3-Cr
			dst_idx             , // destination frame index
			dst_y/2               , // destination y position
			dst_x/2                 // destination x position
			);

		// set descriptor - Cr
		set_descriptor (
			core_idx,
			2                   , // descriptor number
			1                , // last descriptor
			0                , // 0-copy, 1-color fill
			0                , // fill with 2 byte
			0                , // color1
			0                , // color0
			0x10              , // interval
			unit_size_y         , // unit vertical size
			unit_size_x         , // unit horizonal size
			0               , // next descriptor address
			1                , // 1-2D, 0-1D
			0                , // 0-frame, 2-top, 3-bottom
			3               , // 0-Y, 2-Cb, 3-Cr
			src_idx             , // source frame index
			src_y               , // source y position
			src_x               , // source x position
			size_y/2            , // block height
			size_x/2            , // block width
			1                , // 1-2D, 0-1D
			0                , // 0-frame, 2-top, 3-bottom
			3                , // 0-Y, 2-Cb, 3-Cr
			dst_idx             , // destination frame index
			dst_y/2               , // destination y position
			dst_x/2                 // destination x position
			);
	}



	//-----------------------
	// Y
	//-----------------------
	// write_descriptor to dmac

	write_descriptor (
		core_idx,
		0     ,  // desc_num  - descriptor number
		1  ,  // dmac_mode - 1-single, 0-consecutive
		0    // desc_addr - descriptor address to load
		);

	// run_dmac
	run_dmac (
		core_idx,
		1  ,  // dmac_mode - 1-single, 0-consecutive
		0    // desc_addr - descriptor address to load
		);

	//-----------------------
	// Cb, Cr
	//-----------------------
	if (interleave) // interleaved
	{
		// write_descriptor to dmac
		write_descriptor (
			core_idx,
			1     ,  // desc_num  - descriptor number
			1  ,  // dmac_mode - 1-single, 0-consecutive
			0    // desc_addr - descriptor address to load
			);

		// run_dmac
		run_dmac (
			core_idx,
			1  ,  // dmac_mode - 1-single, 0-consecutive
			0    // desc_addr - descriptor address to load
			);
	}
	else
	{
		// separate
		// write_descriptor to dmac
		write_descriptor (
			core_idx,
			1     ,  // desc_num  - descriptor number
			1  ,  // dmac_mode - 1-single, 0-consecutive
			0    // desc_addr - descriptor address to load
			);

		// run_dmac
		run_dmac (
			core_idx,
			1  ,  // dmac_mode - 1-single, 0-consecutive
			0    // desc_addr - descriptor address to load
			);


		// write_descriptor to dmac
		write_descriptor (
			core_idx,
			2     ,  // desc_num  - descriptor number
			1  ,  // dmac_mode - 1-single, 0-consecutive
			0    // desc_addr - descriptor address to load
			);

		// run_dmac
		run_dmac (
			core_idx,
			1  ,  // dmac_mode - 1-single, 0-consecutive
			0    // desc_addr - descriptor address to load
			);

	}


}
void gdi_set_pic_info(Uint32 core_idx, int index, int cbcrIntlv, int picX, int picY, int stride, FrameBuffer rotatorOutput, int MapType, int format)
{
	int gdi_status;

	gdi_status = 0;

	VpuWriteReg(core_idx, GDI_CONTROL, 1);

	while(!gdi_status) 
		gdi_status = VpuReadReg(core_idx, GDI_STATUS);

	if (MapType)
		VpuWriteReg(core_idx, GDI_INFO_CONTROL + (index*0x14), (2<<20) | ((format & 0x07) << 17) | (cbcrIntlv << 16) | stride);
	else
		VpuWriteReg(core_idx, GDI_INFO_CONTROL + (index*0x14), ((format & 0x07) << 17) | (cbcrIntlv << 16) | stride);


	VpuWriteReg(core_idx, GDI_INFO_PIC_SIZE + (index*0x14), (picX << 16) | picY);
	VpuWriteReg(core_idx, GDI_INFO_BASE_Y + (index*0x14), rotatorOutput.bufY);
	VpuWriteReg(core_idx, GDI_INFO_BASE_CB + (index*0x14), rotatorOutput.bufCb);
	VpuWriteReg(core_idx, GDI_INFO_BASE_CR + (index*0x14), rotatorOutput.bufCr);
	//VpuWriteReg(core_idx, GDI_CONTROL, 0);
}
//------------------------------------------------------------------------------
// set_descriptor
// int desc_num, // descriptor number
// int last_flag; // last descriptor
// int op_mode, // 0-copy, 1-color fill
// int fill_2byte, // fill with 2 byte
// int color1, // color1
// int color0, // color0
// int interval, // interval
// int unit_size_y, // unit vertical size
// int unit_size_x, // unit horizonal size
// int next_desc_addr, // next descriptor address
// int src_dimension, // 1-2D, 0-1D
// int src_field_mode, // 0-frame, 2-top, 3-bottom
// int src_ycbcr,// 0-Y, 2-Cb, 3-Cr
// int src_index,// source frame index
// int src_pos_y,// source y position
// int src_pos_x,// source x position
// int blk_height,// block height
// int blk_width, // block width
// int dst_dimension, // 1-2D, 0-1D
// int dst_field_mode,// 0-frame, 2-top, 3-bottom
// int dst_ycbcr, // 0-Y, 2-Cb, 3-Cr
// int dst_index, // destination frame index
// int dst_pos_y, // destination y position
// int dst_pos_x) // destination x position

//------------------------------------------------------------------------------
static int ar_last_flag[16];
static int ar_op_mode[16];
static int ar_fill_2byte[16];
static int ar_color1[16];
static int ar_color0[16];
static int ar_interval[16];
static int ar_unit_size_y[16];
static int ar_unit_size_x[16];   
static int ar_next_desc_addr[16]; 
static int ar_src_dimension[16];  
static int ar_src_field_mode[16]; 
static int ar_src_ycbcr[16];      
static int ar_src_index[16];      
static int ar_src_pos_y[16];      
static int ar_src_pos_x[16];      
static int ar_blk_height[16];     
static int ar_blk_width[16];      
static int ar_dst_dimension[16];  
static int ar_dst_field_mode[16]; 
static int ar_dst_ycbcr[16];      
static int ar_dst_index[16];      
static int ar_dst_pos_y[16];      
static int ar_dst_pos_x[16];      
static int ar_desc0[16];
static int ar_desc1[16];
static int ar_desc2[16];
static int ar_desc3[16];
static int ar_desc4[16];
static int ar_desc5[16];
static int ar_desc6[16];
static int ar_desc7[16];


void set_descriptor(Uint32 core_idx, int desc_num, int last_flag, int op_mode, int fill_2byte, int color1, int color0, int interval, int unit_size_y, int unit_size_x, int next_desc_addr, int src_dimension, int src_field_mode, int src_ycbcr, int src_index, int src_pos_y, int src_pos_x, int blk_height, int blk_width, int dst_dimension, int dst_field_mode, int dst_ycbcr, int dst_index, int dst_pos_y, int dst_pos_x) 
{
	int     var0;
	int     var1;
	int     var2;
	int     var3;
	int     var4;
	int     var5;
	int		var6;
	int     var7;

	if (desc_num > 16) 
	{
		printf("ERROR: desc_num[%1d] is over MAX_DESC_NUM",desc_num);
		return ;
	}
#if 0
	printf("[set_descriptor] last_flag       : %1d\n",last_flag       );
	printf("[set_descriptor] op_mode         : %1d\n",op_mode         );
	printf("[set_descriptor] fill_2byte      : %1d\n",fill_2byte      );
	printf("[set_descriptor] color1          : %h\n",color1          );
	printf("[set_descriptor] color0          : %h\n",color0          );
	printf("[set_descriptor] interval        : %1d\n",interval        );
	if (src_dimension) 
	{
		printf("[set_descriptor] unit_size_y     : %1d\n",unit_size_y     );
		printf("[set_descriptor] unit_size_x     : %1d\n",unit_size_x     );
		printf("[set_descriptor] next_desc_addr  : %h\n" ,next_desc_addr  );
		printf("[set_descriptor] src_dimension   : %1d\n",src_dimension   );
		printf("[set_descriptor] src_field_mode  : %1d\n",src_field_mode  );
		printf("[set_descriptor] src_ycbcr       : %1d\n",src_ycbcr       );
		printf("[set_descriptor] src_index       : %1d\n",src_index       );
		printf("[set_descriptor] src_pos_y       : %1d\n",src_pos_y       );
		printf("[set_descriptor] src_pos_x       : %1d\n",src_pos_x       );
		printf("[set_descriptor] blk_height      : %1d\n",blk_height      );
		printf("[set_descriptor] blk_width       : %1d\n",blk_width       );
		printf("[set_descriptor] dst_dimension   : %1d\n",dst_dimension   );
		printf("[set_descriptor] dst_field_mode  : %1d\n",dst_field_mode  );
		printf("[set_descriptor] dst_ycbcr       : %1d\n",dst_ycbcr       );
		printf("[set_descriptor] dst_index       : %1d\n",dst_index       );
		printf("[set_descriptor] dst_pos_y       : %1d\n",dst_pos_y       );
		printf("[set_descriptor] dst_pos_x       : %1d\n",dst_pos_x       );
	}
	else 
	{
		printf("[set_descriptor] unit_size_1d    : 0x%h\n",unit_size_y<<8|unit_size_x);
		printf("[set_descriptor] next_desc_addr  : 0x%h\n",next_desc_addr  );
		printf("[set_descriptor] src_addr_1d     : 0x%h\n",src_pos_y<<16|src_pos_x);
		printf("[set_descriptor] blk_addr_1d     : 0x%h\n",blk_height<<16|blk_width);
		printf("[set_descriptor] dst_addr_1d     : 0x%h\n",dst_pos_y<<16|dst_pos_x);
	}
#endif
	ar_last_flag       [desc_num] = last_flag;
	ar_op_mode         [desc_num] = op_mode;
	ar_fill_2byte      [desc_num] = fill_2byte;
	ar_color1          [desc_num] = color1;
	ar_color0          [desc_num] = color0;
	ar_interval        [desc_num] = interval;
	ar_unit_size_y     [desc_num] = unit_size_y;
	ar_unit_size_x     [desc_num] = unit_size_x;
	ar_next_desc_addr  [desc_num] = next_desc_addr;
	ar_src_dimension   [desc_num] = src_dimension;
	ar_src_field_mode  [desc_num] = src_field_mode;
	ar_src_ycbcr       [desc_num] = src_ycbcr;
	ar_src_index       [desc_num] = src_index;
	ar_src_pos_y       [desc_num] = src_pos_y;
	ar_src_pos_x       [desc_num] = src_pos_x;
	ar_blk_height      [desc_num] = blk_height;
	ar_blk_width       [desc_num] = blk_width;
	ar_dst_dimension   [desc_num] = dst_dimension;
	ar_dst_field_mode  [desc_num] = dst_field_mode;
	ar_dst_ycbcr       [desc_num] = dst_ycbcr;
	ar_dst_index       [desc_num] = dst_index;
	ar_dst_pos_y       [desc_num] = dst_pos_y;
	ar_dst_pos_x       [desc_num] = dst_pos_x;

	var0 = 0;
	var1 = 0;
	var2 = 0;
	var3 = 0;
	var4 = 0;
	var5 = 0;
	var6 = 0;
	var7 = 0;

	var0 = color0<<0;			//var0[7: 0] = color0
	var0 |= color1<<8;			//var0[15: 8] = color1
	var0 |= fill_2byte<<16;		//var0[16]    = fill_2byte    ;
	var0 |= op_mode<<17;		//var0[17]    = op_mode       ;
	var0 |= last_flag<<18;		 //var0[18]    = last_flag     ;
	
	
	var1 = 	unit_size_x<<0;		//var1[ 7: 0] = unit_size_x   ;
	var1 |= unit_size_y<<8;		//var1[15: 8] = unit_size_y   ;
	var1 |= interval<<16;		//var1[31:16] = interval      ;
		
	var2 = 	next_desc_addr<<0;		//var2        = next_desc_addr;

	var3 = src_index<<0;			//var3[ 7: 0] = src_index     ;
	var3 |= src_ycbcr<<8;			//var3[ 9: 8] = src_ycbcr     ;
	var3 |= src_field_mode<<10;		//var3[11:10] = src_field_mode;
	var3 |= src_dimension<<12;		//var3[12]    = src_dimension ;
		
	var4 = src_pos_x<<0;			//var4[15: 0] = src_pos_x     ;
	var4 |= src_pos_y<<16;			//var4[31:16] = src_pos_y     ;

	var5 = blk_width<<0;			//var5[15: 0] = blk_width     ;
	var5 |= blk_height<<16;			//var5[31:16] = blk_height    ;

	var6 = dst_index<<0;			//var6[ 7: 0] = dst_index     ;
	var6 |= dst_ycbcr<<8;			//var6[ 9: 8] = dst_ycbcr     ;
	var6 |= dst_field_mode<<10;		//var6[11:10] = dst_field_mode;
	var6 |= dst_dimension<<12;		//var6[12]    = dst_dimension ;
	
	
	var7 = dst_pos_x<<0;			//var7[15: 0] = dst_pos_x     ;
	var7 |= dst_pos_y<<16;			//var7[31:16] = dst_pos_y     ;

	
	ar_desc0[desc_num] = var0;
	ar_desc1[desc_num] = var1;
	ar_desc2[desc_num] = var2;
	ar_desc3[desc_num] = var3;
	ar_desc4[desc_num] = var4;
	ar_desc5[desc_num] = var5;
	ar_desc6[desc_num] = var6;
	ar_desc7[desc_num] = var7;

}



//------------------------------------------------------------------------------
// write_descriptor
// input   integer desc_num        ; // descriptor number
// input           dmac_mode       ; // 1-single, 0-consecutive
// input   [31:0]  desc_addr       ; // descriptor address to load

//------------------------------------------------------------------------------
void write_descriptor(Uint32 core_idx, int desc_num, int dmac_mode, int desc_addr)
{
	int last_flag       ; // last descriptor
	//int next_desc_addr  ; // next descriptor address
	int desc_num_tmp    ;
	int desc_addr_tmp   ;


	if (dmac_mode) 
	{
		// single mode, load descriptor to register
		VpuWriteReg(core_idx, DMAC_DESC0, ar_desc0[desc_num]);
		VpuWriteReg(core_idx, DMAC_DESC1, ar_desc1[desc_num]);
		VpuWriteReg(core_idx, DMAC_DESC2, ar_desc2[desc_num]);
		VpuWriteReg(core_idx, DMAC_DESC3, ar_desc3[desc_num]);
		VpuWriteReg(core_idx, DMAC_DESC4, ar_desc4[desc_num]);
		VpuWriteReg(core_idx, DMAC_DESC5, ar_desc5[desc_num]);
		VpuWriteReg(core_idx, DMAC_DESC6, ar_desc6[desc_num]);
		VpuWriteReg(core_idx, DMAC_DESC7, ar_desc7[desc_num]);
	}
	else 
	{
		// consecutive mode, load descriptor to desc_addr, write in big endian
		//--------------------------------------------
		// 2'b00: 64 bit little endian - byte order {7,6,5,4,3,2,1,0}
		// 2'b01: 64 bit big endian    - byte order {0,1,2,3,4,5,6,7}
		// 2'b10: 32 bit little endian - byte order {3,2,1,0,7,6,5,4}
		// 2'b11: 32 bit big endian    - byte order {4,5,6,7,0,1,2,3}
		//--------------------------------------------
		last_flag = 0;
		desc_num_tmp  = desc_num;
		desc_addr_tmp = desc_addr;

		while (last_flag)
		{
			VpuWriteReg(core_idx, desc_addr_tmp + 0x0, ar_desc0[desc_num_tmp]);
			VpuWriteReg(core_idx, desc_addr_tmp + 0x4, ar_desc1[desc_num_tmp]);
			VpuWriteReg(core_idx, desc_addr_tmp + 0x8, ar_desc2[desc_num_tmp]);
			VpuWriteReg(core_idx, desc_addr_tmp + 0xc, ar_desc3[desc_num_tmp]);
			VpuWriteReg(core_idx, desc_addr_tmp + 0x10, ar_desc4[desc_num_tmp]);
			VpuWriteReg(core_idx, desc_addr_tmp + 0x14, ar_desc5[desc_num_tmp]);
			VpuWriteReg(core_idx, desc_addr_tmp + 0x18, ar_desc6[desc_num_tmp]);
			VpuWriteReg(core_idx, desc_addr_tmp + 0x1c, ar_desc7[desc_num_tmp]);

			last_flag = ar_last_flag[desc_num_tmp];
			desc_addr_tmp = ar_next_desc_addr[desc_num_tmp];
			desc_num_tmp = desc_num_tmp + 1;
		}
	}
}
//------------------------------------------------------------------------------
// run_dmac
// input           dmac_mode       ; // 1-single, 0-consecutive
// input   [31:0]  desc_addr       ; // descriptor address to load
//------------------------------------------------------------------------------
void run_dmac(Uint32 core_idx, int dmac_mode, int desc_addr)
{
	int desc_endian     ;
	int running; // last descriptor


	//--------------------------------------------
	// desc_endian
	//--------------------------------------------
	// 2'b00: 64 bit little endian - byte order {7,6,5,4,3,2,1,0}
	// 2'b01: 64 bit big endian    - byte order {0,1,2,3,4,5,6,7}
	// 2'b10: 32 bit little endian - byte order {3,2,1,0,7,6,5,4}
	// 2'b11: 32 bit big endian    - byte order {4,5,6,7,0,1,2,3}
	//--------------------------------------------
	desc_endian = 1;

	VpuWriteReg(core_idx, DMAC_DMAC_MODE, (desc_endian<<1)|dmac_mode);
	VpuWriteReg(core_idx, DMAC_DESC_ADDR, desc_addr);
	VpuWriteReg(core_idx, DMAC_DMAC_RUN, 1);
#if 0
	printf("[run_dmac] dmac started...\n");
#endif

	running = 1;
	while (running)
	{
		running = VpuReadReg(core_idx, DMAC_DMAC_RUN);
	}
}
