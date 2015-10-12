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

#ifndef _VPU_TYPES_H_
#define _VPU_TYPES_H_

typedef unsigned char	Uint8;
typedef unsigned int	Uint32;
typedef unsigned short	Uint16;
typedef char	        Int8;
typedef int	            Int32;
typedef short	        Int16;
#if defined(_MSC_VER)
typedef unsigned _int64 Uint64;
typedef _int64 Int64;
#else
typedef unsigned long long Uint64;
typedef long long Int64;
#endif
#ifndef PhysicalAddress
typedef Uint32 PhysicalAddress;
#endif
#ifndef BYTE
typedef unsigned char   BYTE;
#endif
#ifndef BOOL
typedef int BOOL;
#endif
#ifndef NULL
#define NULL	0
#endif


#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(P) { (P) = (P); }
#endif



#endif	/* _VPU_TYPES_H_ */
