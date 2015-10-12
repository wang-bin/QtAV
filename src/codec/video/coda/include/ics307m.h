//------------------------------------------------------------------------------
// File: ICS307M.h
//
// Copyright (c) 2009, Chips & Media.  All rights reserved.
//------------------------------------------------------------------------------

#include "../config.h"
#ifdef PLATFORM_FPGA

#ifndef _CLOCK_H_
#define _CLOCK_H_

#define	DEVICE0_ADDR_COMMAND		0x75
#define DEVICE0_ADDR_PARAM0			0x76
#define	DEVICE0_ADDR_PARAM1			0x77
#define	DEVICE1_ADDR_COMMAND		0x78
#define DEVICE1_ADDR_PARAM0			0x79
#define	DEVICE1_ADDR_PARAM1			0x7a
#define DEVICE_ADDR_SW_RESET		0x7b


#define ACLK_MAX					30
#define ACLK_MIN					16
#define CCLK_MAX					30
#define CCLK_MIN					16


#if defined (__cplusplus)
extern "C"{
#endif 

int SetFreqICS307M(int Device, int OutFreqMHz, int InFreqMHz);
int SetHPITimingOpt();


#if defined (__cplusplus)
}
#endif 


#endif //#ifndef _CLOCK_H_

#endif 	
