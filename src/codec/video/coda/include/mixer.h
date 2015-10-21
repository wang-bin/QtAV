//------------------------------------------------------------------------------
// File: Mixer.h
//
// Copyright (c) 2006, Chips & Media.  All rights reserved.
//------------------------------------------------------------------------------

#ifndef _MIXER_H_
#define _MIXER_H_

#include "vpuapi.h"

typedef struct {
        int  Format;
        int  Index;
        vpu_buffer_t vbAll;
        vpu_buffer_t vbY;
        vpu_buffer_t vbCb;
        vpu_buffer_t vbCr;
        int  strideY;
        int  strideC;
        vpu_buffer_t vbMvCol;

} FRAME_BUF;


#define HAVE_HW_MIXER

#if defined (__cplusplus)
extern "C" {
#endif 
	
	int		DisplayMixer(FrameBuffer *fp, int width, int height);
	void    InitMixerInt();
	void	WaitMixerInt();	
	int		MixerIsIdle();	
	int sw_mixer_open(int core_idx, int width, int height);
	int sw_mixer_draw(int core_idx, int x, int y, int width, int height, int format, unsigned char* pbImage, int interleave);
	void sw_mixer_close(int core_idx);

	void  dmac_single_pic_copy(Uint32 core_idx, FrameBuffer srcFrame, FrameBuffer dstFrame, int src_x, int src_y, int dst_x, int dst_y, int size_y, int size_x, int stride, int map_type, int interleave);
	void  dmac_mvc_pic_copy(Uint32 core_idx, FrameBuffer srcFrame, FrameBuffer dstFrame, int size_y, int size_x,int map_type, int interleave, int viewIdx);
#ifdef HAVE_HW_MIXER
        void hw_mixer_open(int width, int height);
        int hw_mixer_draw(int x, int y, int width, int height, int format,
                        unsigned long yaddr, unsigned long cbaddr, unsigned long craddr);
        void hw_mixer_close(void);
#endif

#if defined (__cplusplus)
}
#endif 

#endif //#ifndef _MIXER_H_
