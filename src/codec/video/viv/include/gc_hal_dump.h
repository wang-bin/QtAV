/****************************************************************************
*
*    Copyright (c) 2005 - 2013 by Vivante Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of Vivante Corporation. This is proprietary information owned by
*    Vivante Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of Vivante Corporation.
*
*****************************************************************************/


#ifndef __gc_hal_dump_h_
#define __gc_hal_dump_h_

#ifdef __cplusplus
extern "C" {
#endif

/*
**	FILE LAYOUT:
**
**		gcsDUMP_FILE structure
**
**		gcsDUMP_DATA frame
**			gcsDUMP_DATA or gcDUMP_DATA_SIZE records rendingring the frame
**			gctUINT8 data[length]
*/

#define gcvDUMP_FILE_SIGNATURE		gcmCC('g','c','D','B')

typedef struct _gcsDUMP_FILE
{
	gctUINT32   		signature;	/* File signature */
	gctSIZE_T 			length;		/* Length of file */
	gctUINT32 			frames;		/* Number of frames in file */
}
gcsDUMP_FILE;

typedef enum _gceDUMP_TAG
{
	gcvTAG_SURFACE					= gcmCC('s','u','r','f'),
	gcvTAG_FRAME					= gcmCC('f','r','m',' '),
	gcvTAG_COMMAND					= gcmCC('c','m','d',' '),
	gcvTAG_INDEX					= gcmCC('i','n','d','x'),
	gcvTAG_STREAM					= gcmCC('s','t','r','m'),
	gcvTAG_TEXTURE					= gcmCC('t','e','x','t'),
	gcvTAG_RENDER_TARGET			= gcmCC('r','n','d','r'),
	gcvTAG_DEPTH					= gcmCC('z','b','u','f'),
	gcvTAG_RESOLVE					= gcmCC('r','s','l','v'),
	gcvTAG_DELETE					= gcmCC('d','e','l',' '),
}
gceDUMP_TAG;

typedef struct _gcsDUMP_SURFACE
{
	gceDUMP_TAG			type;		/* Type of record. */
	gctUINT32     		address;	/* Address of the surface. */
	gctINT16      		width;		/* Width of surface. */
	gctINT16	   		height;		/* Height of surface. */
	gceSURF_FORMAT		format;		/* Surface pixel format. */
	gctSIZE_T			length;		/* Number of bytes inside the surface. */
}
gcsDUMP_SURFACE;

typedef struct _gcsDUMP_DATA
{
	gceDUMP_TAG		 	type;		/* Type of record. */
	gctSIZE_T     		length;		/* Number of bytes of data. */
	gctUINT32     		address;	/* Address for the data. */
}
gcsDUMP_DATA;

#ifdef __cplusplus
}
#endif

#endif /* __gc_hal_dump_h_ */

