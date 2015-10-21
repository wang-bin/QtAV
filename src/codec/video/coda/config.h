//------------------------------------------------------------------------------
// File: config.h
//
// Copyright (c) 2006, Chips & Media.  All rights reserved.
// This file should be modified by some developers of C&M according to product version.
//------------------------------------------------------------------------------


#ifndef __CONFIG_H__
#define __CONFIG_H__


#if defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(WIN32) || defined(__MINGW32__)
#       define PLATFORM_WIN32
#elif defined(linux) || defined(__linux) || defined(ANDROID)
#       define PLATFORM_LINUX
#else
#       define PLATFORM_NON_OS
#endif


#if defined(_MSC_VER)
#       include <windows.h>
#       define inline _inline
#elif defined(__GNUC__)
#elif defined(__ARMCC__)
#else
#  error "Unknown compiler."
#endif



#define API_VERSION 330



#if 0
#if defined(PLATFORM_NON_OS) || defined (ANDROID) || defined(MFHMFT_EXPORTS) || defined(OMXIL_COMPONENT) || defined(DXVA_UMOD_DRIVER)
//#define SUPPORT_FFMPEG_DEMUX
#else
#define SUPPORT_FFMPEG_DEMUX
#endif
#endif





//#define REPORT_PERFORMANCE


//#define SUPPORT_MEM_PROTECT



//#define BIT_CODE_FILE_PATH "/root/vpu-coda851/firmware/Firmware-v1327/Magellan.h"
#define BIT_CODE_FILE_PATH "../Magellan.h"
//#define BIT_CODE_FILE_PATH "/root/vpu-coda851/firmware/Firmware-v1.3.45/Magellan.h"
#define PROJECT_ROOT    "../"



#define CODA851


// #define SUPPORT_INFINITE_INSTANCE
 #define SUPPORT_INFINITE_INSTANCE_IN_CODA7L

 #define SUPPORT_DEC_JPEG_GDI_HOST
 #define SUPPORT_GDI
 #define SUPPORT_ENC_COARSE_ME
 #define SUPPORT_DEC_RESOLUTION_CHANGE
 #define SUPPORT_DEC_SLICE_BUFFER
 #define SUPPORT_SW_RESET


#endif  /* __CONFIG_H__ */

