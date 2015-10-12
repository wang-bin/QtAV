//------------------------------------------------------------------------------
// File: Mixer.h
//
// Copyright (c) 2006, Chips & Media.  All rights reserved.
//------------------------------------------------------------------------------

#ifndef _MIXER_H_
#define _MIXER_H_

#include "vpuapi.h"
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
	

#if defined (__cplusplus)
}
#endif 

#endif //#ifndef _MIXER_H_
