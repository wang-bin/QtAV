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


#ifndef __gc_hal_base_h_
#define __gc_hal_base_h_

#include "gc_hal_enum.h"
#include "gc_hal_types.h"

#include "gc_hal_dump.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LINUX 1

/******************************************************************************\
****************************** Object Declarations *****************************
\******************************************************************************/

typedef struct _gckOS *                 gckOS;
typedef struct _gcoHAL *                gcoHAL;
typedef struct _gcoOS *                 gcoOS;
typedef struct _gco2D *                 gco2D;

#ifndef VIVANTE_NO_3D
typedef struct _gco3D *                 gco3D;
#endif

typedef struct _gcoSURF *               gcoSURF;
typedef struct _gcsSURF_INFO *          gcsSURF_INFO_PTR;
typedef struct _gcsSURF_NODE *          gcsSURF_NODE_PTR;
typedef struct _gcsSURF_FORMAT_INFO *   gcsSURF_FORMAT_INFO_PTR;
typedef struct _gcsPOINT *              gcsPOINT_PTR;
typedef struct _gcsSIZE *               gcsSIZE_PTR;
typedef struct _gcsRECT *               gcsRECT_PTR;
typedef struct _gcsBOUNDARY *           gcsBOUNDARY_PTR;
typedef struct _gcoDUMP *               gcoDUMP;
typedef struct _gcoHARDWARE *           gcoHARDWARE;
typedef union  _gcuVIDMEM_NODE *        gcuVIDMEM_NODE_PTR;

typedef struct gcsATOM *                gcsATOM_PTR;

#if gcdENABLE_VG
typedef struct _gcoVG *                 gcoVG;
typedef struct _gcsCOMPLETION_SIGNAL *	gcsCOMPLETION_SIGNAL_PTR;
typedef struct _gcsCONTEXT_MAP *		gcsCONTEXT_MAP_PTR;
#else
typedef void *                          gcoVG;
#endif

#if gcdSYNC
typedef struct _gcoFENCE *              gcoFENCE;
typedef struct _gcsSYNC_CONTEXT  *      gcsSYNC_CONTEXT_PTR;
#endif

typedef struct _gcoOS_SymbolsList gcoOS_SymbolsList;

/******************************************************************************\
******************************* Process local storage *************************
\******************************************************************************/
typedef struct _gcsPLS * gcsPLS_PTR;

typedef void (* gctPLS_DESTRUCTOR) (
    gcsPLS_PTR
    );

typedef struct _gcsPLS
{
    /* Global objects. */
    gcoOS                       os;
    gcoHAL                      hal;

    /* Internal memory pool. */
    gctSIZE_T                   internalSize;
    gctPHYS_ADDR                internalPhysical;
    gctPOINTER                  internalLogical;

    /* External memory pool. */
    gctSIZE_T                   externalSize;
    gctPHYS_ADDR                externalPhysical;
    gctPOINTER                  externalLogical;

    /* Contiguous memory pool. */
    gctSIZE_T                   contiguousSize;
    gctPHYS_ADDR                contiguousPhysical;
    gctPOINTER                  contiguousLogical;

    /* EGL-specific process-wide objects. */
    gctPOINTER                  eglDisplayInfo;
    gctPOINTER                  eglSurfaceInfo;
    gceSURF_FORMAT              eglConfigFormat;

    /* PorcessID of the constrcutor process */
    gctUINT32                   processID;
#if gcdFORCE_GAL_LOAD_TWICE
    /* ThreadID of the constrcutor process. */
    gctSIZE_T                   threadID;
    /* Flag for calling module destructor. */
    gctBOOL                     exiting;
#endif

    /* Reference count for destructor. */
    gcsATOM_PTR                 reference;
    gctBOOL                     bKFS;
#if gcdUSE_NPOT_PATCH
    gctBOOL                     bNeedSupportNP2Texture;
#endif

    /* Destructor for eglDisplayInfo. */
    gctPLS_DESTRUCTOR           destructor;
}
gcsPLS;

extern gcsPLS gcPLS;

/******************************************************************************\
******************************* Thread local storage *************************
\******************************************************************************/

typedef struct _gcsTLS * gcsTLS_PTR;

typedef void (* gctTLS_DESTRUCTOR) (
    gcsTLS_PTR
    );

typedef struct _gcsTLS
{
    gceHARDWARE_TYPE            currentType;
    gcoHARDWARE                 hardware;
    /* Only for separated 3D and 2D */
    gcoHARDWARE                 hardware2D;
#if gcdENABLE_VG
    gcoVGHARDWARE               vg;
    gcoVG                       engineVG;
#endif /* gcdENABLE_VG */
    gctPOINTER                  context;
    gctTLS_DESTRUCTOR           destructor;
    gctBOOL                     ProcessExiting;

#ifndef VIVANTE_NO_3D
	gco3D						engine3D;
#endif
#if gcdSYNC
    gctBOOL                     fenceEnable;
#endif
	gco2D						engine2D;
    gctBOOL                     copied;

#if gcdFORCE_GAL_LOAD_TWICE
    /* libGAL.so handle */
    gctHANDLE                   handle;
#endif
}
gcsTLS;

/******************************************************************************\
********************************* Enumerations *********************************
\******************************************************************************/

typedef enum _gcePLS_VALUE
{
  gcePLS_VALUE_EGL_DISPLAY_INFO,
  gcePLS_VALUE_EGL_SURFACE_INFO,
  gcePLS_VALUE_EGL_CONFIG_FORMAT_INFO,
  gcePLS_VALUE_EGL_DESTRUCTOR_INFO,
}
gcePLS_VALUE;

/* Video memory pool type. */
typedef enum _gcePOOL
{
    gcvPOOL_UNKNOWN = 0,
    gcvPOOL_DEFAULT,
    gcvPOOL_LOCAL,
    gcvPOOL_LOCAL_INTERNAL,
    gcvPOOL_LOCAL_EXTERNAL,
    gcvPOOL_UNIFIED,
    gcvPOOL_SYSTEM,
    gcvPOOL_VIRTUAL,
    gcvPOOL_USER,
    gcvPOOL_CONTIGUOUS,
    gcvPOOL_DEFAULT_FORCE_CONTIGUOUS,
    gcvPOOL_DEFAULT_FORCE_CONTIGUOUS_CACHEABLE,

    gcvPOOL_NUMBER_OF_POOLS
}
gcePOOL;

#ifndef VIVANTE_NO_3D
/* Blending functions. */
typedef enum _gceBLEND_FUNCTION
{
    gcvBLEND_ZERO,
    gcvBLEND_ONE,
    gcvBLEND_SOURCE_COLOR,
    gcvBLEND_INV_SOURCE_COLOR,
    gcvBLEND_SOURCE_ALPHA,
    gcvBLEND_INV_SOURCE_ALPHA,
    gcvBLEND_TARGET_COLOR,
    gcvBLEND_INV_TARGET_COLOR,
    gcvBLEND_TARGET_ALPHA,
    gcvBLEND_INV_TARGET_ALPHA,
    gcvBLEND_SOURCE_ALPHA_SATURATE,
    gcvBLEND_CONST_COLOR,
    gcvBLEND_INV_CONST_COLOR,
    gcvBLEND_CONST_ALPHA,
    gcvBLEND_INV_CONST_ALPHA,
}
gceBLEND_FUNCTION;

/* Blending modes. */
typedef enum _gceBLEND_MODE
{
    gcvBLEND_ADD,
    gcvBLEND_SUBTRACT,
    gcvBLEND_REVERSE_SUBTRACT,
    gcvBLEND_MIN,
    gcvBLEND_MAX,
}
gceBLEND_MODE;

/* API flags. */
typedef enum _gceAPI
{
    gcvAPI_D3D                  = 0x1,
    gcvAPI_OPENGL               = 0x2,
    gcvAPI_OPENVG               = 0x3,
    gcvAPI_OPENCL               = 0x4,
}
gceAPI;

/* Depth modes. */
typedef enum _gceDEPTH_MODE
{
    gcvDEPTH_NONE,
    gcvDEPTH_Z,
    gcvDEPTH_W,
}
gceDEPTH_MODE;
#endif /* VIVANTE_NO_3D */

typedef enum _gceWHERE
{
    gcvWHERE_COMMAND,
    gcvWHERE_RASTER,
    gcvWHERE_PIXEL,
}
gceWHERE;

typedef enum _gceHOW
{
    gcvHOW_SEMAPHORE            = 0x1,
    gcvHOW_STALL                = 0x2,
    gcvHOW_SEMAPHORE_STALL      = 0x3,
}
gceHOW;

typedef enum _gceSignalHandlerType
{
    gcvHANDLE_SIGFPE_WHEN_SIGNAL_CODE_IS_0        = 0x1,
}
gceSignalHandlerType;


#if gcdENABLE_VG
/* gcsHAL_Limits*/
typedef struct _gcsHAL_LIMITS
{
    /* chip info */
    gceCHIPMODEL    chipModel;
    gctUINT32       chipRevision;
    gctUINT32       featureCount;
    gctUINT32       *chipFeatures;

    /* target caps */
	gctUINT32         maxWidth;
	gctUINT32         maxHeight;
	gctUINT32         multiTargetCount;
	gctUINT32         maxSamples;

}gcsHAL_LIMITS;
#endif

/******************************************************************************\
*********** Generic Memory Allocation Optimization Using Containers ************
\******************************************************************************/

/* Generic container definition. */
typedef struct _gcsCONTAINER_LINK * gcsCONTAINER_LINK_PTR;
typedef struct _gcsCONTAINER_LINK
{
    /* Points to the next container. */
    gcsCONTAINER_LINK_PTR           next;
}
gcsCONTAINER_LINK;

typedef struct _gcsCONTAINER_RECORD * gcsCONTAINER_RECORD_PTR;
typedef struct _gcsCONTAINER_RECORD
{
    gcsCONTAINER_RECORD_PTR         prev;
    gcsCONTAINER_RECORD_PTR         next;
}
gcsCONTAINER_RECORD;

typedef struct _gcsCONTAINER * gcsCONTAINER_PTR;
typedef struct _gcsCONTAINER
{
    gctUINT                         containerSize;
    gctUINT                         recordSize;
    gctUINT                         recordCount;
    gcsCONTAINER_LINK_PTR           containers;
    gcsCONTAINER_RECORD             freeList;
    gcsCONTAINER_RECORD             allocList;
}
gcsCONTAINER;

gceSTATUS
gcsCONTAINER_Construct(
    IN gcsCONTAINER_PTR Container,
    gctUINT RecordsPerContainer,
    gctUINT RecordSize
    );

gceSTATUS
gcsCONTAINER_Destroy(
    IN gcsCONTAINER_PTR Container
    );

gceSTATUS
gcsCONTAINER_AllocateRecord(
    IN gcsCONTAINER_PTR Container,
    OUT gctPOINTER * Record
    );

gceSTATUS
gcsCONTAINER_FreeRecord(
    IN gcsCONTAINER_PTR Container,
    IN gctPOINTER Record
    );

gceSTATUS
gcsCONTAINER_FreeAll(
    IN gcsCONTAINER_PTR Container
    );

/******************************************************************************\
********************************* gcoHAL Object *********************************
\******************************************************************************/

/* Construct a new gcoHAL object. */
gceSTATUS
gcoHAL_Construct(
    IN gctPOINTER Context,
    IN gcoOS Os,
    OUT gcoHAL * Hal
    );

/* Destroy an gcoHAL object. */
gceSTATUS
gcoHAL_Destroy(
    IN gcoHAL Hal
    );

/* Get pointer to gco2D object. */
gceSTATUS
gcoHAL_Get2DEngine(
    IN gcoHAL Hal,
    OUT gco2D * Engine
    );

gceSTATUS
gcoHAL_SetFscaleValue(
    IN gctUINT FscaleValue
    );

gceSTATUS
gcoHAL_GetFscaleValue(
    OUT gctUINT * FscaleValue,
    OUT gctUINT * MinFscaleValue,
    OUT gctUINT * MaxFscaleValue
    );

gceSTATUS
gcoHAL_SetBltNP2Texture(
    gctBOOL enable
    );

#ifndef VIVANTE_NO_3D
/* Get pointer to gco3D object. */
gceSTATUS
gcoHAL_Get3DEngine(
    IN gcoHAL Hal,
    OUT gco3D * Engine
    );

gceSTATUS
gcoHAL_Query3DEngine(
    IN gcoHAL Hal,
    OUT gco3D * Engine
    );

gceSTATUS
gcoHAL_Set3DEngine(
    IN gcoHAL Hal,
    IN gco3D Engine
    );

gceSTATUS
gcoHAL_Get3DHardware(
    IN gcoHAL Hal,
    OUT gcoHARDWARE * Hardware
    );

gceSTATUS
gcoHAL_Set3DHardware(
    IN gcoHAL Hal,
    IN gcoHARDWARE Hardware
    );


#endif /* VIVANTE_NO_3D */

/* Verify whether the specified feature is available in hardware. */
gceSTATUS
gcoHAL_IsFeatureAvailable(
    IN gcoHAL Hal,
    IN gceFEATURE Feature
    );

/* Query the identity of the hardware. */
gceSTATUS
gcoHAL_QueryChipIdentity(
    IN gcoHAL Hal,
    OUT gceCHIPMODEL* ChipModel,
    OUT gctUINT32* ChipRevision,
    OUT gctUINT32* ChipFeatures,
    OUT gctUINT32* ChipMinorFeatures
    );

/* Query the minor features of the hardware. */
gceSTATUS gcoHAL_QueryChipMinorFeatures(
    IN gcoHAL Hal,
    OUT gctUINT32* NumFeatures,
    OUT gctUINT32* ChipMinorFeatures
    );

/* Query the amount of video memory. */
gceSTATUS
gcoHAL_QueryVideoMemory(
    IN gcoHAL Hal,
    OUT gctPHYS_ADDR * InternalAddress,
    OUT gctSIZE_T * InternalSize,
    OUT gctPHYS_ADDR * ExternalAddress,
    OUT gctSIZE_T * ExternalSize,
    OUT gctPHYS_ADDR * ContiguousAddress,
    OUT gctSIZE_T * ContiguousSize
    );

/* Map video memory. */
gceSTATUS
gcoHAL_MapMemory(
    IN gcoHAL Hal,
    IN gctPHYS_ADDR Physical,
    IN gctSIZE_T NumberOfBytes,
    OUT gctPOINTER * Logical
    );

/* Unmap video memory. */
gceSTATUS
gcoHAL_UnmapMemory(
    IN gcoHAL Hal,
    IN gctPHYS_ADDR Physical,
    IN gctSIZE_T NumberOfBytes,
    IN gctPOINTER Logical
    );

/* Schedule an unmap of a buffer mapped through its physical address. */
gceSTATUS
gcoHAL_ScheduleUnmapMemory(
    IN gcoHAL Hal,
    IN gctPHYS_ADDR Physical,
    IN gctSIZE_T NumberOfBytes,
    IN gctPOINTER Logical
    );

/* Map user memory. */
gceSTATUS
gcoHAL_MapUserMemory(
    IN gctPOINTER Logical,
    IN gctUINT32 Physical,
    IN gctSIZE_T Size,
    OUT gctPOINTER * Info,
    OUT gctUINT32_PTR GPUAddress
    );

/* Unmap user memory. */
gceSTATUS
gcoHAL_UnmapUserMemory(
    IN gctPOINTER Logical,
    IN gctSIZE_T Size,
    IN gctPOINTER Info,
    IN gctUINT32 GPUAddress
    );

/* Schedule an unmap of a user buffer using event mechanism. */
gceSTATUS
gcoHAL_ScheduleUnmapUserMemory(
    IN gcoHAL Hal,
    IN gctPOINTER Info,
    IN gctSIZE_T Size,
    IN gctUINT32 Address,
    IN gctPOINTER Memory
    );

/* Commit the current command buffer. */
gceSTATUS
gcoHAL_Commit(
    IN gcoHAL Hal,
    IN gctBOOL Stall
    );

/* Query the tile capabilities. */
gceSTATUS
gcoHAL_QueryTiled(
    IN gcoHAL Hal,
    OUT gctINT32 * TileWidth2D,
    OUT gctINT32 * TileHeight2D,
    OUT gctINT32 * TileWidth3D,
    OUT gctINT32 * TileHeight3D
    );

gceSTATUS
gcoHAL_Compact(
    IN gcoHAL Hal
    );

#if VIVANTE_PROFILER
gceSTATUS
gcoHAL_ProfileStart(
    IN gcoHAL Hal
    );

gceSTATUS
gcoHAL_ProfileEnd(
    IN gcoHAL Hal,
    IN gctCONST_STRING Title
    );
#endif

/* Power Management */
gceSTATUS
gcoHAL_SetPowerManagementState(
    IN gcoHAL Hal,
    IN gceCHIPPOWERSTATE State
    );

gceSTATUS
gcoHAL_QueryPowerManagementState(
    IN gcoHAL Hal,
    OUT gceCHIPPOWERSTATE *State
    );

/* Set the filter type for filter blit. */
gceSTATUS
gcoHAL_SetFilterType(
    IN gcoHAL Hal,
    IN gceFILTER_TYPE FilterType
    );

gceSTATUS
gcoHAL_GetDump(
    IN gcoHAL Hal,
    OUT gcoDUMP * Dump
    );

/* Call the kernel HAL layer. */
gceSTATUS
gcoHAL_Call(
    IN gcoHAL Hal,
    IN OUT gcsHAL_INTERFACE_PTR Interface
    );

gceSTATUS
gcoHAL_GetPatchID(
    IN  gcoHAL Hal,
    OUT gcePATCH_ID * PatchID
    );

/* Schedule an event. */
gceSTATUS
gcoHAL_ScheduleEvent(
    IN gcoHAL Hal,
    IN OUT gcsHAL_INTERFACE_PTR Interface
    );

/* Destroy a surface. */
gceSTATUS
gcoHAL_DestroySurface(
    IN gcoHAL Hal,
    IN gcoSURF Surface
    );

/* Request a start/stop timestamp. */
gceSTATUS
gcoHAL_SetTimer(
    IN gcoHAL Hal,
    IN gctUINT32 Index,
    IN gctBOOL Start
    );

/* Get Time delta from a Timer in microseconds. */
gceSTATUS
gcoHAL_GetTimerTime(
    IN gcoHAL Hal,
    IN gctUINT32 Timer,
    OUT gctINT32_PTR TimeDelta
    );

/* set timeout value. */
gceSTATUS
gcoHAL_SetTimeOut(
    IN gcoHAL Hal,
    IN gctUINT32 timeOut
    );

gceSTATUS
gcoHAL_SetHardwareType(
    IN gcoHAL Hal,
    IN gceHARDWARE_TYPE HardwardType
    );

gceSTATUS
gcoHAL_GetHardwareType(
    IN gcoHAL Hal,
    OUT gceHARDWARE_TYPE * HardwardType
    );

gceSTATUS
gcoHAL_QueryChipCount(
    IN gcoHAL Hal,
    OUT gctINT32 * Count
    );

gceSTATUS
gcoHAL_QuerySeparated3D2D(
    IN gcoHAL Hal
    );

gceSTATUS
gcoHAL_QuerySpecialHint(
    IN gceSPECIAL_HINT Hint
    );

gceSTATUS
gcoHAL_SetSpecialHintData(
    IN gcoHARDWARE Hardware
    );

/* Get pointer to gcoVG object. */
gceSTATUS
gcoHAL_GetVGEngine(
    IN gcoHAL Hal,
    OUT gcoVG * Engine
    );

#if gcdENABLE_VG
gceSTATUS
gcoHAL_QueryChipLimits(
    IN gcoHAL           Hal,
    IN gctINT32         Chip,
    OUT gcsHAL_LIMITS   *Limits);

gceSTATUS
gcoHAL_QueryChipFeature(
    IN gcoHAL       Hal,
    IN gctINT32     Chip,
    IN gceFEATURE   Feature);

#endif
/******************************************************************************\
********************************** gcoOS Object *********************************
\******************************************************************************/

/* Get PLS value for given key */
gctPOINTER
gcoOS_GetPLSValue(
    IN gcePLS_VALUE key
    );

/* Set PLS value of a given key */
void
gcoOS_SetPLSValue(
    IN gcePLS_VALUE key,
    OUT gctPOINTER value
    );

/* Get access to the thread local storage. */
gceSTATUS
gcoOS_GetTLS(
    OUT gcsTLS_PTR * TLS
    );

    /* Copy the TLS from a source thread. */
    gceSTATUS gcoOS_CopyTLS(IN gcsTLS_PTR Source);

/* Destroy the objects associated with the current thread. */
void
gcoOS_FreeThreadData(
    IN gctBOOL ProcessExiting
    );

/* Construct a new gcoOS object. */
gceSTATUS
gcoOS_Construct(
    IN gctPOINTER Context,
    OUT gcoOS * Os
    );

/* Destroy an gcoOS object. */
gceSTATUS
gcoOS_Destroy(
    IN gcoOS Os
    );

/* Get the base address for the physical memory. */
gceSTATUS
gcoOS_GetBaseAddress(
    IN gcoOS Os,
    OUT gctUINT32_PTR BaseAddress
    );

/* Allocate memory from the heap. */
gceSTATUS
gcoOS_Allocate(
    IN gcoOS Os,
    IN gctSIZE_T Bytes,
    OUT gctPOINTER * Memory
    );

/* Get allocated memory size. */
gceSTATUS
gcoOS_GetMemorySize(
    IN gcoOS Os,
    IN gctPOINTER Memory,
    OUT gctSIZE_T_PTR MemorySize
    );

/* Free allocated memory. */
gceSTATUS
gcoOS_Free(
    IN gcoOS Os,
    IN gctPOINTER Memory
    );

/* Allocate memory. */
gceSTATUS
gcoOS_AllocateMemory(
    IN gcoOS Os,
    IN gctSIZE_T Bytes,
    OUT gctPOINTER * Memory
    );

/* Free memory. */
gceSTATUS
gcoOS_FreeMemory(
    IN gcoOS Os,
    IN gctPOINTER Memory
    );

/* Allocate contiguous memory. */
gceSTATUS
gcoOS_AllocateContiguous(
    IN gcoOS Os,
    IN gctBOOL InUserSpace,
    IN OUT gctSIZE_T * Bytes,
    OUT gctPHYS_ADDR * Physical,
    OUT gctPOINTER * Logical
    );

/* Free contiguous memory. */
gceSTATUS
gcoOS_FreeContiguous(
    IN gcoOS Os,
    IN gctPHYS_ADDR Physical,
    IN gctPOINTER Logical,
    IN gctSIZE_T Bytes
    );

gceSTATUS
gcoSURF_GetBankOffsetBytes(
    IN gcoSURF Surfce,
    IN gceSURF_TYPE Type,
    IN gctUINT32 Stride,
    IN gctUINT32_PTR Bytes
    );

/* Map user memory. */
gceSTATUS
gcoOS_MapUserMemory(
    IN gcoOS Os,
    IN gctPOINTER Memory,
    IN gctSIZE_T Size,
    OUT gctPOINTER * Info,
    OUT gctUINT32_PTR Address
    );

/* Map user memory. */
gceSTATUS
gcoOS_MapUserMemoryEx(
    IN gcoOS Os,
    IN gctPOINTER Memory,
    IN gctUINT32 Physical,
    IN gctSIZE_T Size,
    OUT gctPOINTER * Info,
    OUT gctUINT32_PTR Address
    );

/* Unmap user memory. */
gceSTATUS
gcoOS_UnmapUserMemory(
    IN gcoOS Os,
    IN gctPOINTER Memory,
    IN gctSIZE_T Size,
    IN gctPOINTER Info,
    IN gctUINT32 Address
    );

/* Device I/O Control call to the kernel HAL layer. */
gceSTATUS
gcoOS_DeviceControl(
    IN gcoOS Os,
    IN gctUINT32 IoControlCode,
    IN gctPOINTER InputBuffer,
    IN gctSIZE_T InputBufferSize,
    IN gctPOINTER OutputBuffer,
    IN gctSIZE_T OutputBufferSize
    );

/* Allocate non paged memory. */
gceSTATUS
gcoOS_AllocateNonPagedMemory(
    IN gcoOS Os,
    IN gctBOOL InUserSpace,
    IN OUT gctSIZE_T * Bytes,
    OUT gctPHYS_ADDR * Physical,
    OUT gctPOINTER * Logical
    );

/* Free non paged memory. */
gceSTATUS
gcoOS_FreeNonPagedMemory(
    IN gcoOS Os,
    IN gctSIZE_T Bytes,
    IN gctPHYS_ADDR Physical,
    IN gctPOINTER Logical
    );

#define gcmOS_SAFE_FREE(os, mem) \
	gcoOS_Free(os, mem); \
	mem = gcvNULL

#define gcmkOS_SAFE_FREE(os, mem) \
    gckOS_Free(os, mem); \
	mem = gcvNULL

typedef enum _gceFILE_MODE
{
    gcvFILE_CREATE          = 0,
    gcvFILE_APPEND,
    gcvFILE_READ,
    gcvFILE_CREATETEXT,
    gcvFILE_APPENDTEXT,
    gcvFILE_READTEXT,
}
gceFILE_MODE;

/* Open a file. */
gceSTATUS
gcoOS_Open(
    IN gcoOS Os,
    IN gctCONST_STRING FileName,
    IN gceFILE_MODE Mode,
    OUT gctFILE * File
    );

/* Close a file. */
gceSTATUS
gcoOS_Close(
    IN gcoOS Os,
    IN gctFILE File
    );

/* Read data from a file. */
gceSTATUS
gcoOS_Read(
    IN gcoOS Os,
    IN gctFILE File,
    IN gctSIZE_T ByteCount,
    IN gctPOINTER Data,
    OUT gctSIZE_T * ByteRead
    );

/* Write data to a file. */
gceSTATUS
gcoOS_Write(
    IN gcoOS Os,
    IN gctFILE File,
    IN gctSIZE_T ByteCount,
    IN gctCONST_POINTER Data
    );

/* Flush data to a file. */
gceSTATUS
gcoOS_Flush(
    IN gcoOS Os,
    IN gctFILE File
    );

/* Close a file descriptor. */
gceSTATUS
gcoOS_CloseFD(
    IN gcoOS Os,
    IN gctINT FD
    );

/* Dup file descriptor to another. */
gceSTATUS
gcoOS_DupFD(
    IN gcoOS Os,
    IN gctINT FD,
    OUT gctINT * FD2
    );

/* Create an endpoint for communication. */
gceSTATUS
gcoOS_Socket(
    IN gcoOS Os,
    IN gctINT Domain,
    IN gctINT Type,
    IN gctINT Protocol,
    OUT gctINT *SockFd
    );

/* Close a socket. */
gceSTATUS
gcoOS_CloseSocket(
    IN gcoOS Os,
    IN gctINT SockFd
    );

/* Initiate a connection on a socket. */
gceSTATUS
gcoOS_Connect(
    IN gcoOS Os,
    IN gctINT SockFd,
    IN gctCONST_POINTER HostName,
    IN gctUINT Port);

/* Shut down part of connection on a socket. */
gceSTATUS
gcoOS_Shutdown(
    IN gcoOS Os,
    IN gctINT SockFd,
    IN gctINT How
    );

/* Send a message on a socket. */
gceSTATUS
gcoOS_Send(
    IN gcoOS Os,
    IN gctINT SockFd,
    IN gctSIZE_T ByteCount,
    IN gctCONST_POINTER Data,
    IN gctINT Flags
    );

/* Initiate a connection on a socket. */
gceSTATUS
gcoOS_WaitForSend(
    IN gcoOS Os,
    IN gctINT SockFd,
    IN gctINT Seconds,
    IN gctINT MicroSeconds);

/* Get environment variable value. */
gceSTATUS
gcoOS_GetEnv(
    IN gcoOS Os,
    IN gctCONST_STRING VarName,
    OUT gctSTRING * Value
    );

/* Set environment variable value. */
gceSTATUS
gcoOS_SetEnv(
    IN gcoOS Os,
    IN gctCONST_STRING VarName,
    IN gctSTRING Value
    );

/* Get current working directory. */
gceSTATUS
gcoOS_GetCwd(
    IN gcoOS Os,
	IN gctINT SizeInBytes,
    OUT gctSTRING Buffer
    );

/* Get file status info. */
gceSTATUS
gcoOS_Stat(
    IN gcoOS Os,
    IN gctCONST_STRING FileName,
    OUT gctPOINTER Buffer
    );

typedef enum _gceFILE_WHENCE
{
    gcvFILE_SEEK_SET,
    gcvFILE_SEEK_CUR,
    gcvFILE_SEEK_END
}
gceFILE_WHENCE;

/* Set the current position of a file. */
gceSTATUS
gcoOS_Seek(
    IN gcoOS Os,
    IN gctFILE File,
    IN gctUINT32 Offset,
    IN gceFILE_WHENCE Whence
    );

/* Set the current position of a file. */
gceSTATUS
gcoOS_SetPos(
    IN gcoOS Os,
    IN gctFILE File,
    IN gctUINT32 Position
    );

/* Get the current position of a file. */
gceSTATUS
gcoOS_GetPos(
    IN gcoOS Os,
    IN gctFILE File,
    OUT gctUINT32 * Position
    );

/* Same as strstr. */
gceSTATUS
gcoOS_StrStr(
    IN gctCONST_STRING String,
    IN gctCONST_STRING SubString,
    OUT gctSTRING * Output
    );

/* Find the last occurance of a character inside a string. */
gceSTATUS
gcoOS_StrFindReverse(
    IN gctCONST_STRING String,
    IN gctINT8 Character,
    OUT gctSTRING * Output
    );

gceSTATUS
gcoOS_StrDup(
    IN gcoOS Os,
    IN gctCONST_STRING String,
    OUT gctSTRING * Target
    );

/* Copy a string. */
gceSTATUS
gcoOS_StrCopySafe(
    IN gctSTRING Destination,
    IN gctSIZE_T DestinationSize,
    IN gctCONST_STRING Source
    );

/* Append a string. */
gceSTATUS
gcoOS_StrCatSafe(
    IN gctSTRING Destination,
    IN gctSIZE_T DestinationSize,
    IN gctCONST_STRING Source
    );

/* Compare two strings. */
gceSTATUS
gcoOS_StrCmp(
    IN gctCONST_STRING String1,
    IN gctCONST_STRING String2
    );

/* Compare characters of two strings. */
gceSTATUS
gcoOS_StrNCmp(
    IN gctCONST_STRING String1,
    IN gctCONST_STRING String2,
    IN gctSIZE_T Count
    );

/* Convert string to float. */
gceSTATUS
gcoOS_StrToFloat(
    IN gctCONST_STRING String,
    OUT gctFLOAT * Float
    );

/* Convert hex string to integer. */
gceSTATUS
gcoOS_HexStrToInt(
	IN gctCONST_STRING String,
	OUT gctINT * Int
	);

/* Convert hex string to float. */
gceSTATUS
gcoOS_HexStrToFloat(
	IN gctCONST_STRING String,
	OUT gctFLOAT * Float
	);

/* Convert string to integer. */
gceSTATUS
gcoOS_StrToInt(
    IN gctCONST_STRING String,
    OUT gctINT * Int
    );

gceSTATUS
gcoOS_MemCmp(
    IN gctCONST_POINTER Memory1,
    IN gctCONST_POINTER Memory2,
    IN gctSIZE_T Bytes
    );

gceSTATUS
gcoOS_PrintStrSafe(
    OUT gctSTRING String,
    IN gctSIZE_T StringSize,
    IN OUT gctUINT * Offset,
    IN gctCONST_STRING Format,
    ...
    );

gceSTATUS
gcoOS_LoadLibrary(
    IN gcoOS Os,
    IN gctCONST_STRING Library,
    OUT gctHANDLE * Handle
    );

gceSTATUS
gcoOS_FreeLibrary(
    IN gcoOS Os,
    IN gctHANDLE Handle
    );

gceSTATUS
gcoOS_GetProcAddress(
    IN gcoOS Os,
    IN gctHANDLE Handle,
    IN gctCONST_STRING Name,
    OUT gctPOINTER * Function
    );

gceSTATUS
gcoOS_Compact(
    IN gcoOS Os
    );

gceSTATUS
gcoOS_AddSignalHandler (
    IN gceSignalHandlerType SignalHandlerType
    );

#if VIVANTE_PROFILER
gceSTATUS
gcoOS_ProfileStart(
    IN gcoOS Os
    );

gceSTATUS
gcoOS_ProfileEnd(
    IN gcoOS Os,
    IN gctCONST_STRING Title
    );

gceSTATUS
gcoOS_SetProfileSetting(
        IN gcoOS Os,
        IN gctBOOL Enable,
        IN gctCONST_STRING FileName
        );
#endif

gctBOOL
gcoOS_IsNeededSupportNP2Texture(
    IN gctCHAR* ProcName
    );

/* Query the video memory. */
gceSTATUS
gcoOS_QueryVideoMemory(
    IN gcoOS Os,
    OUT gctPHYS_ADDR * InternalAddress,
    OUT gctSIZE_T * InternalSize,
    OUT gctPHYS_ADDR * ExternalAddress,
    OUT gctSIZE_T * ExternalSize,
    OUT gctPHYS_ADDR * ContiguousAddress,
    OUT gctSIZE_T * ContiguousSize
    );

/* Detect if the process is the executable specified. */
gceSTATUS
gcoOS_DetectProcessByNamePid(
    IN gctCONST_STRING Name,
    IN gctHANDLE Pid
    );

/* Detect if the current process is the executable specified. */
gceSTATUS
gcoOS_DetectProcessByName(
    IN gctCONST_STRING Name
    );

gceSTATUS
gcoOS_DetectProcessByEncryptedName(
    IN gctCONST_STRING Name
    );

#if defined(ANDROID)
gceSTATUS
gcoOS_DetectProgrameByEncryptedSymbols(
    IN gcoOS_SymbolsList Symbols
    );
#endif

/*----------------------------------------------------------------------------*/
/*----- Atoms ----------------------------------------------------------------*/

/* Construct an atom. */
gceSTATUS
gcoOS_AtomConstruct(
    IN gcoOS Os,
    OUT gcsATOM_PTR * Atom
    );

/* Destroy an atom. */
gceSTATUS
gcoOS_AtomDestroy(
    IN gcoOS Os,
    IN gcsATOM_PTR Atom
    );

/* Increment an atom. */
gceSTATUS
gcoOS_AtomIncrement(
    IN gcoOS Os,
    IN gcsATOM_PTR Atom,
    OUT gctINT32_PTR OldValue
    );

/* Decrement an atom. */
gceSTATUS
gcoOS_AtomDecrement(
    IN gcoOS Os,
    IN gcsATOM_PTR Atom,
    OUT gctINT32_PTR OldValue
    );

gctHANDLE
gcoOS_GetCurrentProcessID(
    void
    );

gctHANDLE
gcoOS_GetCurrentThreadID(
    void
    );

/*----------------------------------------------------------------------------*/
/*----- Time -----------------------------------------------------------------*/

/* Get the number of milliseconds since the system started. */
gctUINT32
gcoOS_GetTicks(
    void
    );

/* Get time in microseconds. */
gceSTATUS
gcoOS_GetTime(
    gctUINT64_PTR Time
    );

/* Get CPU usage in microseconds. */
gceSTATUS
gcoOS_GetCPUTime(
    gctUINT64_PTR CPUTime
    );

/* Get memory usage. */
gceSTATUS
gcoOS_GetMemoryUsage(
    gctUINT32_PTR MaxRSS,
    gctUINT32_PTR IxRSS,
    gctUINT32_PTR IdRSS,
    gctUINT32_PTR IsRSS
    );

/* Delay a number of microseconds. */
gceSTATUS
gcoOS_Delay(
    IN gcoOS Os,
    IN gctUINT32 Delay
    );

/*----------------------------------------------------------------------------*/
/*----- Threads --------------------------------------------------------------*/

#ifdef _WIN32
/* Cannot include windows.h here becuase "near" and "far"
 * which are used in gcsDEPTH_INFO, are defined to nothing in WinDef.h.
 * So, use the real value of DWORD and WINAPI, instead.
 * DWORD is unsigned long, and WINAPI is __stdcall.
 * If these two are change in WinDef.h, the following two typdefs
 * need to be changed, too.
 */
typedef unsigned long gctTHREAD_RETURN;
typedef unsigned long (__stdcall * gcTHREAD_ROUTINE)(void * Argument);
#else
typedef void * gctTHREAD_RETURN;
typedef void * (* gcTHREAD_ROUTINE)(void *);
#endif

/* Create a new thread. */
gceSTATUS
gcoOS_CreateThread(
    IN gcoOS Os,
    IN gcTHREAD_ROUTINE Worker,
    IN gctPOINTER Argument,
    OUT gctPOINTER * Thread
    );

/* Close a thread. */
gceSTATUS
gcoOS_CloseThread(
    IN gcoOS Os,
    IN gctPOINTER Thread
    );

/*----------------------------------------------------------------------------*/
/*----- Mutexes --------------------------------------------------------------*/

/* Create a new mutex. */
gceSTATUS
gcoOS_CreateMutex(
    IN gcoOS Os,
    OUT gctPOINTER * Mutex
    );

/* Delete a mutex. */
gceSTATUS
gcoOS_DeleteMutex(
    IN gcoOS Os,
    IN gctPOINTER Mutex
    );

/* Acquire a mutex. */
gceSTATUS
gcoOS_AcquireMutex(
    IN gcoOS Os,
    IN gctPOINTER Mutex,
    IN gctUINT32 Timeout
    );

/* Release a mutex. */
gceSTATUS
gcoOS_ReleaseMutex(
    IN gcoOS Os,
    IN gctPOINTER Mutex
    );

/*----------------------------------------------------------------------------*/
/*----- Signals --------------------------------------------------------------*/

/* Create a signal. */
gceSTATUS
gcoOS_CreateSignal(
    IN gcoOS Os,
    IN gctBOOL ManualReset,
    OUT gctSIGNAL * Signal
    );

/* Destroy a signal. */
gceSTATUS
gcoOS_DestroySignal(
    IN gcoOS Os,
    IN gctSIGNAL Signal
    );

/* Signal a signal. */
gceSTATUS
gcoOS_Signal(
    IN gcoOS Os,
    IN gctSIGNAL Signal,
    IN gctBOOL State
    );

/* Wait for a signal. */
gceSTATUS
gcoOS_WaitSignal(
    IN gcoOS Os,
    IN gctSIGNAL Signal,
    IN gctUINT32 Wait
    );

/* Map a signal from another process */
gceSTATUS
gcoOS_MapSignal(
    IN gctSIGNAL  RemoteSignal,
    OUT gctSIGNAL * LocalSignal
    );

/* Unmap a signal mapped from another process */
gceSTATUS
gcoOS_UnmapSignal(
    IN gctSIGNAL Signal
    );

/*----------------------------------------------------------------------------*/
/*----- Android Native Fence -------------------------------------------------*/

/* Create sync point. */
gceSTATUS
gcoOS_CreateSyncPoint(
    IN gcoOS Os,
    OUT gctSYNC_POINT * SyncPoint
    );

/* Destroy sync point. */
gceSTATUS
gcoOS_DestroySyncPoint(
    IN gcoOS Os,
    IN gctSYNC_POINT SyncPoint
    );

/* Create native fence. */
gceSTATUS
gcoOS_CreateNativeFence(
    IN gcoOS Os,
    IN gctSYNC_POINT SyncPoint,
    OUT gctINT * FenceFD
    );

/* Wait on native fence. */
gceSTATUS
gcoOS_WaitNativeFence(
    IN gcoOS Os,
    IN gctINT FenceFD,
    IN gctUINT32 Timeout
    );

/*----------------------------------------------------------------------------*/
/*----- Memory Access and Cache ----------------------------------------------*/

/* Write a register. */
gceSTATUS
gcoOS_WriteRegister(
    IN gcoOS Os,
    IN gctUINT32 Address,
    IN gctUINT32 Data
    );

/* Read a register. */
gceSTATUS
gcoOS_ReadRegister(
    IN gcoOS Os,
    IN gctUINT32 Address,
    OUT gctUINT32 * Data
    );

gceSTATUS
gcoOS_CacheClean(
    IN gcoOS Os,
    IN gctUINT64 Node,
    IN gctPOINTER Logical,
    IN gctSIZE_T Bytes
    );

gceSTATUS
gcoOS_CacheFlush(
    IN gcoOS Os,
    IN gctUINT64 Node,
    IN gctPOINTER Logical,
    IN gctSIZE_T Bytes
    );

gceSTATUS
gcoOS_CacheInvalidate(
    IN gcoOS Os,
    IN gctUINT64 Node,
    IN gctPOINTER Logical,
    IN gctSIZE_T Bytes
    );

gceSTATUS
gcoOS_MemoryBarrier(
    IN gcoOS Os,
    IN gctPOINTER Logical
    );


/*----------------------------------------------------------------------------*/
/*----- Profile --------------------------------------------------------------*/

gceSTATUS
gckOS_GetProfileTick(
    OUT gctUINT64_PTR Tick
    );

gceSTATUS
gckOS_QueryProfileTickRate(
    OUT gctUINT64_PTR TickRate
    );

gctUINT32
gckOS_ProfileToMS(
    IN gctUINT64 Ticks
    );

gceSTATUS
gcoOS_GetProfileTick(
    OUT gctUINT64_PTR Tick
    );

gceSTATUS
gcoOS_QueryProfileTickRate(
    OUT gctUINT64_PTR TickRate
    );

#define _gcmPROFILE_INIT(prefix, freq, start) \
    do { \
        prefix ## OS_QueryProfileTickRate(&(freq)); \
        prefix ## OS_GetProfileTick(&(start)); \
    } while (gcvFALSE)

#define _gcmPROFILE_QUERY(prefix, start, ticks) \
    do { \
        prefix ## OS_GetProfileTick(&(ticks)); \
        (ticks) = ((ticks) > (start)) ? ((ticks) - (start)) \
                                      : (~0ull - (start) + (ticks) + 1); \
    } while (gcvFALSE)

#if gcdENABLE_PROFILING
#   define gcmkPROFILE_INIT(freq, start)    _gcmPROFILE_INIT(gck, freq, start)
#   define gcmkPROFILE_QUERY(start, ticks)  _gcmPROFILE_QUERY(gck, start, ticks)
#   define gcmPROFILE_INIT(freq, start)     _gcmPROFILE_INIT(gco, freq, start)
#   define gcmPROFILE_QUERY(start, ticks)   _gcmPROFILE_QUERY(gco, start, ticks)
#   define gcmPROFILE_ONLY(x)               x
#   define gcmPROFILE_ELSE(x)               do { } while (gcvFALSE)
#   define gcmPROFILE_DECLARE_ONLY(x)       x
#   define gcmPROFILE_DECLARE_ELSE(x)       typedef x
#else
#   define gcmkPROFILE_INIT(start, freq)    do { } while (gcvFALSE)
#   define gcmkPROFILE_QUERY(start, ticks)  do { } while (gcvFALSE)
#   define gcmPROFILE_INIT(start, freq)     do { } while (gcvFALSE)
#   define gcmPROFILE_QUERY(start, ticks)   do { } while (gcvFALSE)
#   define gcmPROFILE_ONLY(x)               do { } while (gcvFALSE)
#   define gcmPROFILE_ELSE(x)               x
#   define gcmPROFILE_DECLARE_ONLY(x)       typedef x
#   define gcmPROFILE_DECLARE_ELSE(x)       x
#endif

/*******************************************************************************
**  gcoMATH object
*/

#define gcdPI                   3.14159265358979323846f

/* Kernel. */
gctINT
gckMATH_ModuloInt(
    IN gctINT X,
    IN gctINT Y
    );

/* User. */
gctUINT32
gcoMATH_Log2in5dot5(
    IN gctINT X
    );


gctFLOAT
gcoMATH_UIntAsFloat(
    IN gctUINT32 X
    );

gctUINT32
gcoMATH_FloatAsUInt(
    IN gctFLOAT X
    );

gctBOOL
gcoMATH_CompareEqualF(
    IN gctFLOAT X,
    IN gctFLOAT Y
    );

gctUINT16
gcoMATH_UInt8AsFloat16(
    IN gctUINT8 X
    );

/******************************************************************************\
**************************** Coordinate Structures *****************************
\******************************************************************************/

typedef struct _gcsPOINT
{
    gctINT32                    x;
    gctINT32                    y;
}
gcsPOINT;

typedef struct _gcsSIZE
{
    gctINT32                    width;
    gctINT32                    height;
}
gcsSIZE;

typedef struct _gcsRECT
{
    gctINT32                    left;
    gctINT32                    top;
    gctINT32                    right;
    gctINT32                    bottom;
}
gcsRECT;

typedef union _gcsPIXEL
{
    struct
    {
        gctFLOAT r, g, b, a;
        gctFLOAT d, s;
    } pf;

    struct
    {
        gctINT32 r, g, b, a;
        gctINT32 d, s;
    } pi;

    struct
    {
        gctUINT32 r, g, b, a;
        gctUINT32 d, s;
    } pui;

} gcsPIXEL;


/******************************************************************************\
********************************* gcoSURF Object ********************************
\******************************************************************************/

/*----------------------------------------------------------------------------*/
/*------------------------------- gcoSURF Common ------------------------------*/

/* Color format classes. */
typedef enum _gceFORMAT_CLASS
{
    gcvFORMAT_CLASS_RGBA        = 4500,
    gcvFORMAT_CLASS_YUV,
    gcvFORMAT_CLASS_INDEX,
    gcvFORMAT_CLASS_LUMINANCE,
    gcvFORMAT_CLASS_BUMP,
    gcvFORMAT_CLASS_DEPTH,
}
gceFORMAT_CLASS;

/* Special enums for width field in gcsFORMAT_COMPONENT. */
typedef enum _gceCOMPONENT_CONTROL
{
    gcvCOMPONENT_NOTPRESENT     = 0x00,
    gcvCOMPONENT_DONTCARE       = 0x80,
    gcvCOMPONENT_WIDTHMASK      = 0x7F,
    gcvCOMPONENT_ODD            = 0x80
}
gceCOMPONENT_CONTROL;

/* Color format component parameters. */
typedef struct _gcsFORMAT_COMPONENT
{
    gctUINT8                    start;
    gctUINT8                    width;
}
gcsFORMAT_COMPONENT;

/* RGBA color format class. */
typedef struct _gcsFORMAT_CLASS_TYPE_RGBA
{
    gcsFORMAT_COMPONENT         alpha;
    gcsFORMAT_COMPONENT         red;
    gcsFORMAT_COMPONENT         green;
    gcsFORMAT_COMPONENT         blue;
}
gcsFORMAT_CLASS_TYPE_RGBA;

/* YUV color format class. */
typedef struct _gcsFORMAT_CLASS_TYPE_YUV
{
    gcsFORMAT_COMPONENT         y;
    gcsFORMAT_COMPONENT         u;
    gcsFORMAT_COMPONENT         v;
}
gcsFORMAT_CLASS_TYPE_YUV;

/* Index color format class. */
typedef struct _gcsFORMAT_CLASS_TYPE_INDEX
{
    gcsFORMAT_COMPONENT         value;
}
gcsFORMAT_CLASS_TYPE_INDEX;

/* Luminance color format class. */
typedef struct _gcsFORMAT_CLASS_TYPE_LUMINANCE
{
    gcsFORMAT_COMPONENT         alpha;
    gcsFORMAT_COMPONENT         value;
}
gcsFORMAT_CLASS_TYPE_LUMINANCE;

/* Bump map color format class. */
typedef struct _gcsFORMAT_CLASS_TYPE_BUMP
{
    gcsFORMAT_COMPONENT         alpha;
    gcsFORMAT_COMPONENT         l;
    gcsFORMAT_COMPONENT         v;
    gcsFORMAT_COMPONENT         u;
    gcsFORMAT_COMPONENT         q;
    gcsFORMAT_COMPONENT         w;
}
gcsFORMAT_CLASS_TYPE_BUMP;

/* Depth and stencil format class. */
typedef struct _gcsFORMAT_CLASS_TYPE_DEPTH
{
    gcsFORMAT_COMPONENT         depth;
    gcsFORMAT_COMPONENT         stencil;
}
gcsFORMAT_CLASS_TYPE_DEPTH;

/* Format parameters. */
typedef struct _gcsSURF_FORMAT_INFO
{
    /* Format code and class. */
    gceSURF_FORMAT              format;
    gceFORMAT_CLASS             fmtClass;

    /* The size of one pixel in bits. */
    gctUINT8                    bitsPerPixel;

    /* Component swizzle. */
    gceSURF_SWIZZLE             swizzle;

    /* Some formats have two neighbour pixels interleaved together. */
    /* To describe such format, set the flag to 1 and add another   */
    /* like this one describing the odd pixel format.               */
    gctUINT8                    interleaved;

    /* Format components. */
    union
    {
        gcsFORMAT_CLASS_TYPE_BUMP       bump;
        gcsFORMAT_CLASS_TYPE_RGBA       rgba;
        gcsFORMAT_CLASS_TYPE_YUV        yuv;
        gcsFORMAT_CLASS_TYPE_LUMINANCE  lum;
        gcsFORMAT_CLASS_TYPE_INDEX      index;
        gcsFORMAT_CLASS_TYPE_DEPTH      depth;
    } u;
}
gcsSURF_FORMAT_INFO;

/* Frame buffer information. */
typedef struct _gcsSURF_FRAMEBUFFER
{
    gctPOINTER                  logical;
    gctUINT                     width, height;
    gctINT                      stride;
    gceSURF_FORMAT              format;
}
gcsSURF_FRAMEBUFFER;

typedef struct _gcsVIDMEM_NODE_SHARED_INFO
{
    gctBOOL                     tileStatusDisabled;
    gcsPOINT                    SrcOrigin;
    gcsPOINT                    DestOrigin;
    gcsSIZE                     RectSize;
    gctUINT32                   clearValue;
}
gcsVIDMEM_NODE_SHARED_INFO;

/* Generic pixel component descriptors. */
extern gcsFORMAT_COMPONENT gcvPIXEL_COMP_XXX8;
extern gcsFORMAT_COMPONENT gcvPIXEL_COMP_XX8X;
extern gcsFORMAT_COMPONENT gcvPIXEL_COMP_X8XX;
extern gcsFORMAT_COMPONENT gcvPIXEL_COMP_8XXX;

typedef enum _gceORIENTATION
{
    gcvORIENTATION_TOP_BOTTOM,
    gcvORIENTATION_BOTTOM_TOP,
}
gceORIENTATION;


/* Construct a new gcoSURF object. */
gceSTATUS
gcoSURF_Construct(
    IN gcoHAL Hal,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT Depth,
    IN gceSURF_TYPE Type,
    IN gceSURF_FORMAT Format,
    IN gcePOOL Pool,
    OUT gcoSURF * Surface
    );

/* Destroy an gcoSURF object. */
gceSTATUS
gcoSURF_Destroy(
    IN gcoSURF Surface
    );

/* Map user-allocated surface. */
gceSTATUS
gcoSURF_MapUserSurface(
    IN gcoSURF Surface,
    IN gctUINT Alignment,
    IN gctPOINTER Logical,
    IN gctUINT32 Physical
    );

/* Query vid mem node info. */
gceSTATUS
gcoSURF_QueryVidMemNode(
    IN gcoSURF Surface,
    OUT gctUINT64 * Node,
    OUT gcePOOL * Pool,
    OUT gctUINT_PTR Bytes
    );

/* Set the color type of the surface. */
gceSTATUS
gcoSURF_SetColorType(
    IN gcoSURF Surface,
    IN gceSURF_COLOR_TYPE ColorType
    );

/* Get the color type of the surface. */
gceSTATUS
gcoSURF_GetColorType(
    IN gcoSURF Surface,
    OUT gceSURF_COLOR_TYPE *ColorType
    );

/* Set the surface ration angle. */
gceSTATUS
gcoSURF_SetRotation(
    IN gcoSURF Surface,
    IN gceSURF_ROTATION Rotation
    );

gceSTATUS
gcoSURF_SetPreRotation(
    IN gcoSURF Surface,
    IN gceSURF_ROTATION Rotation
    );

gceSTATUS
gcoSURF_GetPreRotation(
    IN gcoSURF Surface,
    IN gceSURF_ROTATION *Rotation
    );

gceSTATUS
gcoSURF_IsValid(
    IN gcoSURF Surface
    );

#ifndef VIVANTE_NO_3D
/* Verify and return the state of the tile status mechanism. */
gceSTATUS
gcoSURF_IsTileStatusSupported(
    IN gcoSURF Surface
    );

/* Process tile status for the specified surface. */
gceSTATUS
gcoSURF_SetTileStatus(
    IN gcoSURF Surface
    );

/* Enable tile status for the specified surface. */
gceSTATUS
gcoSURF_EnableTileStatus(
    IN gcoSURF Surface
    );

/* Disable tile status for the specified surface. */
gceSTATUS
gcoSURF_DisableTileStatus(
    IN gcoSURF Surface,
    IN gctBOOL Decompress
    );

gceSTATUS
gcoSURF_AlignResolveRect(
    IN gcoSURF Surf,
    IN gcsPOINT_PTR RectOrigin,
    IN gcsPOINT_PTR RectSize,
    OUT gcsPOINT_PTR AlignedOrigin,
    OUT gcsPOINT_PTR AlignedSize
    );
#endif /* VIVANTE_NO_3D */

/* Get surface size. */
gceSTATUS
gcoSURF_GetSize(
    IN gcoSURF Surface,
    OUT gctUINT * Width,
    OUT gctUINT * Height,
    OUT gctUINT * Depth
    );

/* Get surface aligned sizes. */
gceSTATUS
gcoSURF_GetAlignedSize(
    IN gcoSURF Surface,
    OUT gctUINT * Width,
    OUT gctUINT * Height,
    OUT gctINT * Stride
    );

/* Get alignments. */
gceSTATUS
gcoSURF_GetAlignment(
    IN gceSURF_TYPE Type,
    IN gceSURF_FORMAT Format,
    OUT gctUINT * AddressAlignment,
    OUT gctUINT * XAlignment,
    OUT gctUINT * YAlignment
    );

/* Get surface type and format. */
gceSTATUS
gcoSURF_GetFormat(
    IN gcoSURF Surface,
    OUT gceSURF_TYPE * Type,
    OUT gceSURF_FORMAT * Format
    );

/* Get surface tiling. */
gceSTATUS
gcoSURF_GetTiling(
    IN gcoSURF Surface,
    OUT gceTILING * Tiling
    );

/* Lock the surface. */
gceSTATUS
gcoSURF_Lock(
    IN gcoSURF Surface,
    IN OUT gctUINT32 * Address,
    IN OUT gctPOINTER * Memory
    );

/* Unlock the surface. */
gceSTATUS
gcoSURF_Unlock(
    IN gcoSURF Surface,
    IN gctPOINTER Memory
    );

/* Return pixel format parameters. */
gceSTATUS
gcoSURF_QueryFormat(
    IN gceSURF_FORMAT Format,
    OUT gcsSURF_FORMAT_INFO_PTR * Info
    );

/* Compute the color pixel mask. */
gceSTATUS
gcoSURF_ComputeColorMask(
    IN gcsSURF_FORMAT_INFO_PTR Format,
    OUT gctUINT32_PTR ColorMask
    );

/* Flush the surface. */
gceSTATUS
gcoSURF_Flush(
    IN gcoSURF Surface
    );

/* Fill surface from it's tile status buffer. */
gceSTATUS
gcoSURF_FillFromTile(
    IN gcoSURF Surface
    );

/* Check if surface needs a filler. */
gceSTATUS gcoSURF_NeedFiller(IN gcoSURF Surface);

/* Fill surface with a value. */
gceSTATUS
gcoSURF_Fill(
    IN gcoSURF Surface,
    IN gcsPOINT_PTR Origin,
    IN gcsSIZE_PTR Size,
    IN gctUINT32 Value,
    IN gctUINT32 Mask
    );

/* Alpha blend two surfaces together. */
gceSTATUS
gcoSURF_Blend(
    IN gcoSURF SrcSurface,
    IN gcoSURF DestSurface,
    IN gcsPOINT_PTR SrcOrig,
    IN gcsPOINT_PTR DestOrigin,
    IN gcsSIZE_PTR Size,
    IN gceSURF_BLEND_MODE Mode
    );

/* Create a new gcoSURF wrapper object. */
gceSTATUS
gcoSURF_ConstructWrapper(
    IN gcoHAL Hal,
    OUT gcoSURF * Surface
    );

/* Set the underlying buffer for the surface wrapper. */
gceSTATUS
gcoSURF_SetBuffer(
    IN gcoSURF Surface,
    IN gceSURF_TYPE Type,
    IN gceSURF_FORMAT Format,
    IN gctUINT Stride,
    IN gctPOINTER Logical,
    IN gctUINT32 Physical
    );

/* Set the size of the surface in pixels and map the underlying buffer. */
gceSTATUS
gcoSURF_SetWindow(
    IN gcoSURF Surface,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height
    );

/* Set width/height alignment of the surface directly and calculate stride/size. This is only for dri backend now. Please be careful before use. */
gceSTATUS
gcoSURF_SetAlignment(
    IN gcoSURF Surface,
    IN gctUINT Width,
    IN gctUINT Height
    );

/* Increase reference count of the surface. */
gceSTATUS
gcoSURF_ReferenceSurface(
    IN gcoSURF Surface
    );

/* Get surface reference count. */
gceSTATUS
gcoSURF_QueryReferenceCount(
    IN gcoSURF Surface,
    OUT gctINT32 * ReferenceCount
    );

/* Set surface orientation. */
gceSTATUS
gcoSURF_SetOrientation(
    IN gcoSURF Surface,
    IN gceORIENTATION Orientation
    );

/* Query surface orientation. */
gceSTATUS
gcoSURF_QueryOrientation(
    IN gcoSURF Surface,
    OUT gceORIENTATION * Orientation
    );

gceSTATUS
gcoSURF_SetOffset(
    IN gcoSURF Surface,
    IN gctUINT Offset
    );

gceSTATUS
gcoSURF_GetOffset(
    IN gcoSURF Surface,
    OUT gctUINT *Offset
    );

gceSTATUS
gcoSURF_NODE_Cache(
    IN gcsSURF_NODE_PTR Node,
    IN gctPOINTER Logical,
    IN gctSIZE_T Bytes,
    IN gceCACHEOPERATION Operation
    );

/* Perform CPU cache operation on surface */
gceSTATUS
gcoSURF_CPUCacheOperation(
    IN gcoSURF Surface,
    IN gceCACHEOPERATION Operation
    );


gceSTATUS
gcoSURF_SetLinearResolveAddress(
    IN gcoSURF Surface,
    IN gctUINT32 Address,
    IN gctPOINTER Memory
    );

    gceSTATUS
    gcoSURF_Swap(IN gcoSURF Surface1, IN gcoSURF Surface2);

/******************************************************************************\
********************************* gcoDUMP Object ********************************
\******************************************************************************/

/* Construct a new gcoDUMP object. */
gceSTATUS
gcoDUMP_Construct(
    IN gcoOS Os,
    IN gcoHAL Hal,
    OUT gcoDUMP * Dump
    );

/* Destroy a gcoDUMP object. */
gceSTATUS
gcoDUMP_Destroy(
    IN gcoDUMP Dump
    );

/* Enable/disable dumping. */
gceSTATUS
gcoDUMP_Control(
    IN gcoDUMP Dump,
    IN gctSTRING FileName
    );

gceSTATUS
gcoDUMP_IsEnabled(
    IN gcoDUMP Dump,
    OUT gctBOOL * Enabled
    );

/* Add surface. */
gceSTATUS
gcoDUMP_AddSurface(
    IN gcoDUMP Dump,
    IN gctINT32 Width,
    IN gctINT32 Height,
    IN gceSURF_FORMAT PixelFormat,
    IN gctUINT32 Address,
    IN gctSIZE_T ByteCount
    );

/* Mark the beginning of a frame. */
gceSTATUS
gcoDUMP_FrameBegin(
    IN gcoDUMP Dump
    );

/* Mark the end of a frame. */
gceSTATUS
gcoDUMP_FrameEnd(
    IN gcoDUMP Dump
    );

/* Dump data. */
gceSTATUS
gcoDUMP_DumpData(
    IN gcoDUMP Dump,
    IN gceDUMP_TAG Type,
    IN gctUINT32 Address,
    IN gctSIZE_T ByteCount,
    IN gctCONST_POINTER Data
    );

/* Delete an address. */
gceSTATUS
gcoDUMP_Delete(
    IN gcoDUMP Dump,
    IN gctUINT32 Address
    );

/* Enable dump or not. */
gceSTATUS
gcoDUMP_SetDumpFlag(
    IN gctBOOL DumpState
    );

/******************************************************************************\
******************************* gcsRECT Structure ******************************
\******************************************************************************/

/* Initialize rectangle structure. */
gceSTATUS
gcsRECT_Set(
    OUT gcsRECT_PTR Rect,
    IN gctINT32 Left,
    IN gctINT32 Top,
    IN gctINT32 Right,
    IN gctINT32 Bottom
    );

/* Return the width of the rectangle. */
gceSTATUS
gcsRECT_Width(
    IN gcsRECT_PTR Rect,
    OUT gctINT32 * Width
    );

/* Return the height of the rectangle. */
gceSTATUS
gcsRECT_Height(
    IN gcsRECT_PTR Rect,
    OUT gctINT32 * Height
    );

/* Ensure that top left corner is to the left and above the right bottom. */
gceSTATUS
gcsRECT_Normalize(
    IN OUT gcsRECT_PTR Rect
    );

/* Compare two rectangles. */
gceSTATUS
gcsRECT_IsEqual(
    IN gcsRECT_PTR Rect1,
    IN gcsRECT_PTR Rect2,
    OUT gctBOOL * Equal
    );

/* Compare the sizes of two rectangles. */
gceSTATUS
gcsRECT_IsOfEqualSize(
    IN gcsRECT_PTR Rect1,
    IN gcsRECT_PTR Rect2,
    OUT gctBOOL * EqualSize
    );

gceSTATUS
gcsRECT_RelativeRotation(
    IN gceSURF_ROTATION Orientation,
    IN OUT gceSURF_ROTATION *Relation);

gceSTATUS

gcsRECT_Rotate(

    IN OUT gcsRECT_PTR Rect,

    IN gceSURF_ROTATION Rotation,

    IN gceSURF_ROTATION toRotation,

    IN gctINT32 SurfaceWidth,

    IN gctINT32 SurfaceHeight

    );

/******************************************************************************\
**************************** gcsBOUNDARY Structure *****************************
\******************************************************************************/

typedef struct _gcsBOUNDARY
{
    gctINT                      x;
    gctINT                      y;
    gctINT                      width;
    gctINT                      height;
}
gcsBOUNDARY;

/******************************************************************************\
********************************* gcoHEAP Object ********************************
\******************************************************************************/

typedef struct _gcoHEAP *       gcoHEAP;

/* Construct a new gcoHEAP object. */
gceSTATUS
gcoHEAP_Construct(
    IN gcoOS Os,
    IN gctSIZE_T AllocationSize,
    OUT gcoHEAP * Heap
    );

/* Destroy an gcoHEAP object. */
gceSTATUS
gcoHEAP_Destroy(
    IN gcoHEAP Heap
    );

/* Allocate memory. */
gceSTATUS
gcoHEAP_Allocate(
    IN gcoHEAP Heap,
    IN gctSIZE_T Bytes,
    OUT gctPOINTER * Node
    );

gceSTATUS
gcoHEAP_GetMemorySize(
    IN gcoHEAP Heap,
    IN gctPOINTER Memory,
    OUT gctSIZE_T_PTR MemorySize
    );

/* Free memory. */
gceSTATUS
gcoHEAP_Free(
    IN gcoHEAP Heap,
    IN gctPOINTER Node
    );

#if (VIVANTE_PROFILER  || gcdDEBUG)
/* Profile the heap. */
gceSTATUS
gcoHEAP_ProfileStart(
    IN gcoHEAP Heap
    );

gceSTATUS
gcoHEAP_ProfileEnd(
    IN gcoHEAP Heap,
    IN gctCONST_STRING Title
    );
#endif


/******************************************************************************\
******************************* Debugging Macros *******************************
\******************************************************************************/

void
gcoOS_SetDebugLevel(
    IN gctUINT32 Level
    );

void
gcoOS_GetDebugLevel(
    OUT gctUINT32_PTR DebugLevel
    );

void
gcoOS_SetDebugZone(
    IN gctUINT32 Zone
    );

void
gcoOS_GetDebugZone(
    IN gctUINT32 Zone,
    OUT gctUINT32_PTR DebugZone
    );

void
gcoOS_SetDebugLevelZone(
    IN gctUINT32 Level,
    IN gctUINT32 Zone
    );

void
gcoOS_SetDebugZones(
    IN gctUINT32 Zones,
    IN gctBOOL Enable
    );

void
gcoOS_SetDebugFile(
    IN gctCONST_STRING FileName
    );

gctFILE
gcoOS_ReplaceDebugFile(
    IN gctFILE fp
	);

/*******************************************************************************
**
**  gcmFATAL
**
**      Print a message to the debugger and execute a break point.
**
**  ARGUMENTS:
**
**      message Message.
**      ...     Optional arguments.
*/

void
gckOS_DebugFatal(
    IN gctCONST_STRING Message,
    ...
    );

void
gcoOS_DebugFatal(
    IN gctCONST_STRING Message,
    ...
    );

#if gcmIS_DEBUG(gcdDEBUG_FATAL)
#   define gcmFATAL             gcoOS_DebugFatal
#   define gcmkFATAL            gckOS_DebugFatal
#elif gcdHAS_ELLIPSES
#   define gcmFATAL(...)
#   define gcmkFATAL(...)
#else
    gcmINLINE static void
    __dummy_fatal(
        IN gctCONST_STRING Message,
        ...
        )
    {
    }
#   define gcmFATAL             __dummy_fatal
#   define gcmkFATAL            __dummy_fatal
#endif

#define gcmENUM2TEXT(e)         case e: return #e

/*******************************************************************************
**
**  gcmTRACE
**
**      Print a message to the debugfer if the correct level has been set.  In
**      retail mode this macro does nothing.
**
**  ARGUMENTS:
**
**      level   Level of message.
**      message Message.
**      ...     Optional arguments.
*/
#define gcvLEVEL_NONE           -1
#define gcvLEVEL_ERROR          0
#define gcvLEVEL_WARNING        1
#define gcvLEVEL_INFO           2
#define gcvLEVEL_VERBOSE        3

void
gckOS_DebugTrace(
    IN gctUINT32 Level,
    IN gctCONST_STRING Message,
    ...
    );

void
gckOS_DebugTraceN(
    IN gctUINT32 Level,
    IN gctUINT ArgumentSize,
    IN gctCONST_STRING Message,
    ...
    );

void
gcoOS_DebugTrace(
    IN gctUINT32 Level,
    IN gctCONST_STRING Message,
    ...
    );

#if gcmIS_DEBUG(gcdDEBUG_TRACE)
#   define gcmTRACE             gcoOS_DebugTrace
#   define gcmkTRACE            gckOS_DebugTrace
#   define gcmkTRACE_N          gckOS_DebugTraceN
#elif gcdHAS_ELLIPSES
#   define gcmTRACE(...)
#   define gcmkTRACE(...)
#   define gcmkTRACE_N(...)
#else
    gcmINLINE static void
    __dummy_trace(
        IN gctUINT32 Level,
        IN gctCONST_STRING Message,
        ...
        )
    {
    }

    gcmINLINE static void
    __dummy_trace_n(
        IN gctUINT32 Level,
        IN gctUINT ArgumentSize,
        IN gctCONST_STRING Message,
        ...
        )
    {
    }

#   define gcmTRACE             __dummy_trace
#   define gcmkTRACE            __dummy_trace
#   define gcmkTRACE_N          __dummy_trace_n
#endif

/* Zones common for kernel and user. */
#define gcvZONE_OS              (1 << 0)
#define gcvZONE_HARDWARE        (1 << 1)
#define gcvZONE_HEAP            (1 << 2)
#define gcvZONE_SIGNAL          (1 << 27)

/* Kernel zones. */
#define gcvZONE_KERNEL          (1 << 3)
#define gcvZONE_VIDMEM          (1 << 4)
#define gcvZONE_COMMAND         (1 << 5)
#define gcvZONE_DRIVER          (1 << 6)
#define gcvZONE_CMODEL          (1 << 7)
#define gcvZONE_MMU             (1 << 8)
#define gcvZONE_EVENT           (1 << 9)
#define gcvZONE_DEVICE          (1 << 10)
#define gcvZONE_DATABASE        (1 << 11)
#define gcvZONE_INTERRUPT       (1 << 12)
#define gcvZONE_POWER           (1 << 13)

/* User zones. */
#define gcvZONE_HAL             (1 << 3)
#define gcvZONE_BUFFER          (1 << 4)
#define gcvZONE_CONTEXT         (1 << 5)
#define gcvZONE_SURFACE         (1 << 6)
#define gcvZONE_INDEX           (1 << 7)
#define gcvZONE_STREAM          (1 << 8)
#define gcvZONE_TEXTURE         (1 << 9)
#define gcvZONE_2D              (1 << 10)
#define gcvZONE_3D              (1 << 11)
#define gcvZONE_COMPILER        (1 << 12)
#define gcvZONE_MEMORY          (1 << 13)
#define gcvZONE_STATE           (1 << 14)
#define gcvZONE_AUX             (1 << 15)
#define gcvZONE_VERTEX          (1 << 16)
#define gcvZONE_CL              (1 << 17)
#define gcvZONE_COMPOSITION     (1 << 17)
#define gcvZONE_VG              (1 << 18)
#define gcvZONE_IMAGE           (1 << 19)
#define gcvZONE_UTILITY         (1 << 20)
#define gcvZONE_PARAMETERS      (1 << 21)

/* API definitions. */
#define gcvZONE_API_HAL         (1 << 28)
#define gcvZONE_API_EGL         (2 << 28)
#define gcvZONE_API_ES11        (3 << 28)
#define gcvZONE_API_ES20        (4 << 28)
#define gcvZONE_API_VG11        (5 << 28)
#define gcvZONE_API_GL          (6 << 28)
#define gcvZONE_API_DFB         (7 << 28)
#define gcvZONE_API_GDI         (8 << 28)
#define gcvZONE_API_D3D         (9 << 28)
#define gcvZONE_API_ES30        (10 << 28)


#define gcmZONE_GET_API(zone)   ((zone) >> 28)
/*Set gcdZONE_MASE like 0x0 | gcvZONE_API_EGL
will enable print EGL module debug info*/
#define gcdZONE_MASK            0x0FFFFFFF

/* Handy zones. */
#define gcvZONE_NONE            0
#define gcvZONE_ALL             0x0FFFFFFF

/*Dump API depth set 1 for API, 2 for API and API behavior*/
#define gcvDUMP_API_DEPTH       1

/*******************************************************************************
**
**  gcmTRACE_ZONE
**
**      Print a message to the debugger if the correct level and zone has been
**      set.  In retail mode this macro does nothing.
**
**  ARGUMENTS:
**
**      Level   Level of message.
**      Zone    Zone of message.
**      Message Message.
**      ...     Optional arguments.
*/

void
gckOS_DebugTraceZone(
    IN gctUINT32 Level,
    IN gctUINT32 Zone,
    IN gctCONST_STRING Message,
    ...
    );

void
gckOS_DebugTraceZoneN(
    IN gctUINT32 Level,
    IN gctUINT32 Zone,
    IN gctUINT ArgumentSize,
    IN gctCONST_STRING Message,
    ...
    );

void
gcoOS_DebugTraceZone(
    IN gctUINT32 Level,
    IN gctUINT32 Zone,
    IN gctCONST_STRING Message,
    ...
    );

#if gcmIS_DEBUG(gcdDEBUG_TRACE)
#   define gcmTRACE_ZONE            gcoOS_DebugTraceZone
#   define gcmkTRACE_ZONE           gckOS_DebugTraceZone
#   define gcmkTRACE_ZONE_N         gckOS_DebugTraceZoneN
#elif gcdHAS_ELLIPSES
#   define gcmTRACE_ZONE(...)
#   define gcmkTRACE_ZONE(...)
#   define gcmkTRACE_ZONE_N(...)
#else
    gcmINLINE static void
    __dummy_trace_zone(
        IN gctUINT32 Level,
        IN gctUINT32 Zone,
        IN gctCONST_STRING Message,
        ...
        )
    {
    }

    gcmINLINE static void
    __dummy_trace_zone_n(
        IN gctUINT32 Level,
        IN gctUINT32 Zone,
        IN gctUINT ArgumentSize,
        IN gctCONST_STRING Message,
        ...
        )
    {
    }

#   define gcmTRACE_ZONE            __dummy_trace_zone
#   define gcmkTRACE_ZONE           __dummy_trace_zone
#   define gcmkTRACE_ZONE_N         __dummy_trace_zone_n
#endif

/*******************************************************************************
**
**  gcmDEBUG_ONLY
**
**      Execute a statement or function only in DEBUG mode.
**
**  ARGUMENTS:
**
**      f       Statement or function to execute.
*/
#if gcmIS_DEBUG(gcdDEBUG_CODE)
#   define gcmDEBUG_ONLY(f)         f
#else
#   define gcmDEBUG_ONLY(f)
#endif

/*******************************************************************************
**
**  gcmSTACK_PUSH
**  gcmSTACK_POP
**  gcmSTACK_DUMP
**
**      Push or pop a function with entry arguments on the trace stack.
**
**  ARGUMENTS:
**
**      Function    Name of function.
**      Line        Line number.
**      Text        Optional text.
**      ...         Optional arguments for text.
*/
#if gcmIS_DEBUG(gcdDEBUG_STACK)
    void
    gcoOS_StackPush(
        IN gctCONST_STRING Function,
        IN gctINT Line,
        IN gctCONST_STRING Text,
        ...
        );
    void
    gcoOS_StackPop(
        IN gctCONST_STRING Function
        );
    void
    gcoOS_StackDump(
        void
        );
#   define gcmSTACK_PUSH            gcoOS_StackPush
#   define gcmSTACK_POP             gcoOS_StackPop
#   define gcmSTACK_DUMP            gcoOS_StackDump
#elif gcdHAS_ELLIPSES
#   define gcmSTACK_PUSH(...)       do { } while (0)
#   define gcmSTACK_POP(Function)   do { } while (0)
#   define gcmSTACK_DUMP()          do { } while (0)
#else
    gcmINLINE static void
    __dummy_stack_push(
        IN gctCONST_STRING Function,
        IN gctINT Line,
        IN gctCONST_STRING Text, ...
        )
    {
    }
#   define gcmSTACK_PUSH            __dummy_stack_push
#   define gcmSTACK_POP(Function)   do { } while (0)
#   define gcmSTACK_DUMP()          do { } while (0)
#endif

/******************************************************************************\
******************************** Logging Macros ********************************
\******************************************************************************/

#define gcdHEADER_LEVEL             gcvLEVEL_VERBOSE


#if gcdENABLE_PROFILING
void
gcoOS_ProfileDB(
    IN gctCONST_STRING Function,
    IN OUT gctBOOL_PTR Initialized
    );

#define gcmHEADER() \
    static gctBOOL __profile__initialized__ = gcvFALSE; \
    gcmSTACK_PUSH(__FUNCTION__, __LINE__, gcvNULL, gcvNULL); \
    gcoOS_ProfileDB(__FUNCTION__, &__profile__initialized__)
#define gcmHEADER_ARG(...) \
    static gctBOOL __profile__initialized__ = gcvFALSE; \
    gcmSTACK_PUSH(__FUNCTION__, __LINE__, Text, __VA_ARGS__); \
    gcoOS_ProfileDB(__FUNCTION__, &__profile__initialized__)
#define gcmFOOTER() \
    gcmSTACK_POP(__FUNCTION__); \
    gcoOS_ProfileDB(__FUNCTION__, gcvNULL)
#define gcmFOOTER_NO() \
    gcmSTACK_POP(__FUNCTION__); \
    gcoOS_ProfileDB(__FUNCTION__, gcvNULL)
#define gcmFOOTER_ARG(...) \
    gcmSTACK_POP(__FUNCTION__); \
    gcoOS_ProfileDB(__FUNCTION__, gcvNULL)
#define gcmFOOTER_KILL() \
    gcmSTACK_POP(__FUNCTION__); \
    gcoOS_ProfileDB(gcvNULL, gcvNULL)

#else /* gcdENABLE_PROFILING */

#if gcdHAS_ELLIPSES
#define gcmHEADER() \
    gctINT8 __user__ = 1; \
    gctINT8_PTR __user_ptr__ = &__user__; \
    gcmSTACK_PUSH(__FUNCTION__, __LINE__, gcvNULL, gcvNULL); \
    gcmTRACE_ZONE(gcdHEADER_LEVEL, _GC_OBJ_ZONE, \
                  "++%s(%d)", __FUNCTION__, __LINE__)
#else
    gcmINLINE static void
    __dummy_header(void)
    {
    }
#   define gcmHEADER                   __dummy_header
#endif

#if gcdHAS_ELLIPSES
#   define gcmHEADER_ARG(Text, ...) \
        gctINT8 __user__ = 1; \
        gctINT8_PTR __user_ptr__ = &__user__; \
        gcmSTACK_PUSH(__FUNCTION__, __LINE__, Text, __VA_ARGS__); \
        gcmTRACE_ZONE(gcdHEADER_LEVEL, _GC_OBJ_ZONE, \
                      "++%s(%d): " Text, __FUNCTION__, __LINE__, __VA_ARGS__)
#else
    gcmINLINE static void
    __dummy_header_arg(
        IN gctCONST_STRING Text,
        ...
        )
    {
    }
#   define gcmHEADER_ARG                __dummy_header_arg
#endif

#if gcdHAS_ELLIPSES
#   define gcmFOOTER() \
    gcmSTACK_POP(__FUNCTION__); \
    gcmPROFILE_ONLY(gcmTRACE_ZONE(gcdHEADER_LEVEL, _GC_OBJ_ZONE, \
                                  "--%s(%d) [%llu,%llu]: status=%d(%s)", \
                                  __FUNCTION__, __LINE__, \
                                  __ticks__, __total__, \
                                  status, gcoOS_DebugStatus2Name(status))); \
    gcmPROFILE_ELSE(gcmTRACE_ZONE(gcdHEADER_LEVEL, _GC_OBJ_ZONE, \
                                  "--%s(%d): status=%d(%s)", \
                                  __FUNCTION__, __LINE__, \
                                  status, gcoOS_DebugStatus2Name(status))); \
    *__user_ptr__ -= 1
#else
    gcmINLINE static void
    __dummy_footer(void)
    {
    }
#   define gcmFOOTER                    __dummy_footer
#endif

#if gcdHAS_ELLIPSES
#define gcmFOOTER_NO() \
    gcmSTACK_POP(__FUNCTION__); \
    gcmTRACE_ZONE(gcdHEADER_LEVEL, _GC_OBJ_ZONE, \
                  "--%s(%d)", __FUNCTION__, __LINE__); \
    *__user_ptr__ -= 1
#else
    gcmINLINE static void
    __dummy_footer_no(void)
    {
    }
#   define gcmFOOTER_NO                 __dummy_footer_no
#endif

#if gcdHAS_ELLIPSES
#define gcmFOOTER_KILL() \
    gcmSTACK_POP(__FUNCTION__); \
    gcmTRACE_ZONE(gcdHEADER_LEVEL, _GC_OBJ_ZONE, \
                  "--%s(%d)", __FUNCTION__, __LINE__); \
    *__user_ptr__ -= 1
#else
    gcmINLINE static void
    __dummy_footer_kill(void)
    {
    }
#   define gcmFOOTER_KILL               __dummy_footer_kill
#endif

#if gcdHAS_ELLIPSES
#   define gcmFOOTER_ARG(Text, ...) \
        gcmSTACK_POP(__FUNCTION__); \
        gcmTRACE_ZONE(gcdHEADER_LEVEL, _GC_OBJ_ZONE, \
                      "--%s(%d): " Text, __FUNCTION__, __LINE__, __VA_ARGS__); \
        *__user_ptr__ -= 1
#else
    gcmINLINE static void
    __dummy_footer_arg(
        IN gctCONST_STRING Text,
        ...
        )
    {
    }
#   define gcmFOOTER_ARG                __dummy_footer_arg
#endif

#endif /* gcdENABLE_PROFILING */

#if gcdHAS_ELLIPSES
#define gcmkHEADER() \
    gctINT8 __kernel__ = 1; \
    gctINT8_PTR __kernel_ptr__ = &__kernel__; \
    gcmkTRACE_ZONE(gcdHEADER_LEVEL, _GC_OBJ_ZONE, \
                   "++%s(%d)", __FUNCTION__, __LINE__)
#else
    gcmINLINE static void
    __dummy_kheader(void)
    {
    }
#   define gcmkHEADER                  __dummy_kheader
#endif

#if gcdHAS_ELLIPSES
#   define gcmkHEADER_ARG(Text, ...) \
        gctINT8 __kernel__ = 1; \
        gctINT8_PTR __kernel_ptr__ = &__kernel__; \
        gcmkTRACE_ZONE(gcdHEADER_LEVEL, _GC_OBJ_ZONE, \
                       "++%s(%d): " Text, __FUNCTION__, __LINE__, __VA_ARGS__)
#else
    gcmINLINE static void
    __dummy_kheader_arg(
        IN gctCONST_STRING Text,
        ...
        )
    {
    }
#   define gcmkHEADER_ARG               __dummy_kheader_arg
#endif

#if gcdHAS_ELLIPSES
#define gcmkFOOTER() \
    gcmkTRACE_ZONE(gcdHEADER_LEVEL, _GC_OBJ_ZONE, \
                   "--%s(%d): status=%d(%s)", \
                   __FUNCTION__, __LINE__, status, gckOS_DebugStatus2Name(status)); \
    *__kernel_ptr__ -= 1
#else
    gcmINLINE static void
    __dummy_kfooter(void)
    {
    }
#   define gcmkFOOTER                   __dummy_kfooter
#endif

#if gcdHAS_ELLIPSES
#define gcmkFOOTER_NO() \
    gcmkTRACE_ZONE(gcdHEADER_LEVEL, _GC_OBJ_ZONE, \
                   "--%s(%d)", __FUNCTION__, __LINE__); \
    *__kernel_ptr__ -= 1
#else
    gcmINLINE static void
    __dummy_kfooter_no(void)
    {
    }
#   define gcmkFOOTER_NO                __dummy_kfooter_no
#endif

#if gcdHAS_ELLIPSES
#   define gcmkFOOTER_ARG(Text, ...) \
        gcmkTRACE_ZONE(gcdHEADER_LEVEL, _GC_OBJ_ZONE, \
                       "--%s(%d): " Text, \
                       __FUNCTION__, __LINE__, __VA_ARGS__); \
        *__kernel_ptr__ -= 1
#else
    gcmINLINE static void
    __dummy_kfooter_arg(
        IN gctCONST_STRING Text,
        ...
        )
    {
    }
#   define gcmkFOOTER_ARG               __dummy_kfooter_arg
#endif

#define gcmOPT_VALUE(ptr)               (((ptr) == gcvNULL) ? 0 : *(ptr))
#define gcmOPT_VALUE_INDEX(ptr, index)  (((ptr) == gcvNULL) ? 0 : ptr[index])
#define gcmOPT_POINTER(ptr)             (((ptr) == gcvNULL) ? gcvNULL : *(ptr))
#define gcmOPT_STRING(ptr)              (((ptr) == gcvNULL) ? "(nil)" : (ptr))

void
gckOS_Print(
    IN gctCONST_STRING Message,
    ...
    );

void
gckOS_PrintN(
    IN gctUINT ArgumentSize,
    IN gctCONST_STRING Message,
    ...
    );

void
gckOS_CopyPrint(
    IN gctCONST_STRING Message,
    ...
    );

void
gcoOS_Print(
    IN gctCONST_STRING Message,
    ...
    );

#define gcmPRINT                gcoOS_Print
#define gcmkPRINT               gckOS_Print
#define gcmkPRINT_N             gckOS_PrintN

#if gcdPRINT_VERSION
#   define gcmPRINT_VERSION()       do { \
                                        _gcmPRINT_VERSION(gcm); \
                                        gcmSTACK_DUMP(); \
                                    } while (0)
#   define gcmkPRINT_VERSION()      _gcmPRINT_VERSION(gcmk)
#   define _gcmPRINT_VERSION(prefix) \
        prefix##TRACE(gcvLEVEL_ERROR, \
                      "Vivante HAL version %d.%d.%d build %d  %s  %s", \
                      gcvVERSION_MAJOR, gcvVERSION_MINOR, gcvVERSION_PATCH, \
                      gcvVERSION_BUILD, gcvVERSION_DATE, gcvVERSION_TIME )
#else
#   define gcmPRINT_VERSION()       do { gcmSTACK_DUMP(); } while (gcvFALSE)
#   define gcmkPRINT_VERSION()      do { } while (gcvFALSE)
#endif

typedef enum _gceDUMP_BUFFER
{
    gceDUMP_BUFFER_CONTEXT,
    gceDUMP_BUFFER_USER,
    gceDUMP_BUFFER_KERNEL,
    gceDUMP_BUFFER_LINK,
    gceDUMP_BUFFER_WAITLINK,
    gceDUMP_BUFFER_FROM_USER,
}
gceDUMP_BUFFER;

void
gckOS_DumpBuffer(
    IN gckOS Os,
    IN gctPOINTER Buffer,
    IN gctUINT Size,
    IN gceDUMP_BUFFER Type,
    IN gctBOOL CopyMessage
    );

#define gcmkDUMPBUFFER          gckOS_DumpBuffer

#if gcdDUMP_COMMAND
#   define gcmkDUMPCOMMAND(Os, Buffer, Size, Type, CopyMessage) \
        gcmkDUMPBUFFER(Os, Buffer, Size, Type, CopyMessage)
#else
#   define gcmkDUMPCOMMAND(Os, Buffer, Size, Type, CopyMessage)
#endif

#if gcmIS_DEBUG(gcdDEBUG_CODE)

void
gckOS_DebugFlush(
    gctCONST_STRING CallerName,
    gctUINT LineNumber,
    gctUINT32 DmaAddress
    );

#   define gcmkDEBUGFLUSH(DmaAddress) \
        gckOS_DebugFlush(__FUNCTION__, __LINE__, DmaAddress)
#else
#   define gcmkDEBUGFLUSH(DmaAddress)
#endif

/*******************************************************************************
**
**  gcmDUMP_FRAMERATE
**
**      Print average frame rate
**
*/
#if gcdDUMP_FRAMERATE
    gceSTATUS
    gcfDumpFrameRate(
        void
    );
#   define gcmDUMP_FRAMERATE        gcfDumpFrameRate
#elif gcdHAS_ELLIPSES
#   define gcmDUMP_FRAMERATE(...)
#else
    gcmINLINE static void
    __dummy_dump_frame_rate(
        void
        )
    {
    }
#   define gcmDUMP_FRAMERATE        __dummy_dump_frame_rate
#endif


/*******************************************************************************
**
**  gcmDUMP
**
**      Print a dump message.
**
**  ARGUMENTS:
**
**      gctSTRING   Message.
**
**      ...         Optional arguments.
*/
#if gcdDUMP
    gceSTATUS
    gcfDump(
        IN gcoOS Os,
        IN gctCONST_STRING String,
        ...
        );
#  define gcmDUMP               gcfDump
#elif gcdHAS_ELLIPSES
#  define gcmDUMP(...)
#else
    gcmINLINE static void
    __dummy_dump(
        IN gcoOS Os,
        IN gctCONST_STRING Message,
        ...
        )
    {
    }
#  define gcmDUMP               __dummy_dump
#endif

/*******************************************************************************
**
**  gcmDUMP_DATA
**
**      Add data to the dump.
**
**  ARGUMENTS:
**
**      gctSTRING Tag
**          Tag for dump.
**
**      gctPOINTER Logical
**          Logical address of buffer.
**
**      gctSIZE_T Bytes
**          Number of bytes.
*/

#if gcdDUMP || gcdDUMP_COMMAND
    gceSTATUS
    gcfDumpData(
        IN gcoOS Os,
        IN gctSTRING Tag,
        IN gctPOINTER Logical,
        IN gctSIZE_T Bytes
        );
#  define gcmDUMP_DATA          gcfDumpData
#elif gcdHAS_ELLIPSES
#  define gcmDUMP_DATA(...)
#else
    gcmINLINE static void
    __dummy_dump_data(
        IN gcoOS Os,
        IN gctSTRING Tag,
        IN gctPOINTER Logical,
        IN gctSIZE_T Bytes
        )
    {
    }
#  define gcmDUMP_DATA          __dummy_dump_data
#endif

/*******************************************************************************
**
**  gcmDUMP_BUFFER
**
**      Print a buffer to the dump.
**
**  ARGUMENTS:
**
**      gctSTRING Tag
**          Tag for dump.
**
**      gctUINT32 Physical
**          Physical address of buffer.
**
**      gctPOINTER Logical
**          Logical address of buffer.
**
**      gctUINT32 Offset
**          Offset into buffer.
**
**      gctSIZE_T Bytes
**          Number of bytes.
*/

#if gcdDUMP || gcdDUMP_COMMAND
gceSTATUS
gcfDumpBuffer(
    IN gcoOS Os,
    IN gctSTRING Tag,
    IN gctUINT32 Physical,
    IN gctPOINTER Logical,
    IN gctUINT32 Offset,
    IN gctSIZE_T Bytes
    );
#   define gcmDUMP_BUFFER       gcfDumpBuffer
#elif gcdHAS_ELLIPSES
#   define gcmDUMP_BUFFER(...)
#else
    gcmINLINE static void
    __dummy_dump_buffer(
        IN gcoOS Os,
        IN gctSTRING Tag,
        IN gctUINT32 Physical,
        IN gctPOINTER Logical,
        IN gctUINT32 Offset,
        IN gctSIZE_T Bytes
        )
    {
    }
#   define gcmDUMP_BUFFER       __dummy_dump_buffer
#endif

/*******************************************************************************
**
**  gcmDUMP_API
**
**      Print a dump message for a high level API prefixed by the function name.
**
**  ARGUMENTS:
**
**      gctSTRING   Message.
**
**      ...         Optional arguments.
*/
gceSTATUS gcfDumpApi(IN gctCONST_STRING String, ...);
#if gcdDUMP_API
#   define gcmDUMP_API           gcfDumpApi
#elif gcdHAS_ELLIPSES
#   define gcmDUMP_API(...)
#else
    gcmINLINE static void
    __dummy_dump_api(
        IN gctCONST_STRING Message,
        ...
        )
    {
    }
#  define gcmDUMP_API           __dummy_dump_api
#endif

/*******************************************************************************
**
**  gcmDUMP_API_ARRAY
**
**      Print an array of data.
**
**  ARGUMENTS:
**
**      gctUINT32_PTR   Pointer to array.
**      gctUINT32       Size.
*/
gceSTATUS gcfDumpArray(IN gctCONST_POINTER Data, IN gctUINT32 Size);
#if gcdDUMP_API
#   define gcmDUMP_API_ARRAY        gcfDumpArray
#elif gcdHAS_ELLIPSES
#   define gcmDUMP_API_ARRAY(...)
#else
    gcmINLINE static void
    __dummy_dump_api_array(
        IN gctCONST_POINTER Data,
        IN gctUINT32 Size
        )
    {
    }
#   define gcmDUMP_API_ARRAY        __dummy_dump_api_array
#endif

/*******************************************************************************
**
**  gcmDUMP_API_ARRAY_TOKEN
**
**      Print an array of data terminated by a token.
**
**  ARGUMENTS:
**
**      gctUINT32_PTR   Pointer to array.
**      gctUINT32       Termination.
*/
gceSTATUS gcfDumpArrayToken(IN gctCONST_POINTER Data, IN gctUINT32 Termination);
#if gcdDUMP_API
#   define gcmDUMP_API_ARRAY_TOKEN  gcfDumpArrayToken
#elif gcdHAS_ELLIPSES
#   define gcmDUMP_API_ARRAY_TOKEN(...)
#else
    gcmINLINE static void
    __dummy_dump_api_array_token(
        IN gctCONST_POINTER Data,
        IN gctUINT32 Termination
        )
    {
    }
#   define gcmDUMP_API_ARRAY_TOKEN  __dummy_dump_api_array_token
#endif

/*******************************************************************************
**
**  gcmDUMP_API_DATA
**
**      Print an array of bytes.
**
**  ARGUMENTS:
**
**      gctCONST_POINTER    Pointer to array.
**      gctSIZE_T           Size.
*/
gceSTATUS gcfDumpApiData(IN gctCONST_POINTER Data, IN gctSIZE_T Size);
#if gcdDUMP_API
#   define gcmDUMP_API_DATA         gcfDumpApiData
#elif gcdHAS_ELLIPSES
#   define gcmDUMP_API_DATA(...)
#else
    gcmINLINE static void
    __dummy_dump_api_data(
        IN gctCONST_POINTER Data,
        IN gctSIZE_T Size
        )
    {
    }
#   define gcmDUMP_API_DATA         __dummy_dump_api_data
#endif

/*******************************************************************************
**
**  gcmTRACE_RELEASE
**
**      Print a message to the shader debugger.
**
**  ARGUMENTS:
**
**      message Message.
**      ...     Optional arguments.
*/

#define gcmTRACE_RELEASE                gcoOS_DebugShaderTrace

void
gcoOS_DebugShaderTrace(
    IN gctCONST_STRING Message,
    ...
    );

void
gcoOS_SetDebugShaderFiles(
    IN gctCONST_STRING VSFileName,
    IN gctCONST_STRING FSFileName
    );

void
gcoOS_SetDebugShaderFileType(
    IN gctUINT32 ShaderType
    );

void
gcoOS_EnableDebugBuffer(
    IN gctBOOL Enable
    );

/*******************************************************************************
**
**  gcmBREAK
**
**      Break into the debugger.  In retail mode this macro does nothing.
**
**  ARGUMENTS:
**
**      None.
*/

void
gcoOS_DebugBreak(
    void
    );

void
gckOS_DebugBreak(
    void
    );

#if gcmIS_DEBUG(gcdDEBUG_BREAK)
#   define gcmBREAK             gcoOS_DebugBreak
#   define gcmkBREAK            gckOS_DebugBreak
#else
#   define gcmBREAK()
#   define gcmkBREAK()
#endif

/*******************************************************************************
**
**  gcmASSERT
**
**      Evaluate an expression and break into the debugger if the expression
**      evaluates to false.  In retail mode this macro does nothing.
**
**  ARGUMENTS:
**
**      exp     Expression to evaluate.
*/
#if gcmIS_DEBUG(gcdDEBUG_ASSERT)
#   define _gcmASSERT(prefix, exp) \
        do \
        { \
            if (!(exp)) \
            { \
                prefix##TRACE(gcvLEVEL_ERROR, \
                              #prefix "ASSERT at %s(%d)", \
                              __FUNCTION__, __LINE__); \
                prefix##TRACE(gcvLEVEL_ERROR, \
                              "(%s)", #exp); \
                prefix##BREAK(); \
            } \
        } \
        while (gcvFALSE)
#   define gcmASSERT(exp)           _gcmASSERT(gcm, exp)
#   define gcmkASSERT(exp)          _gcmASSERT(gcmk, exp)
#else
#   define gcmASSERT(exp)
#   define gcmkASSERT(exp)
#endif

/*******************************************************************************
**
**  gcmVERIFY
**
**      Verify if an expression returns true.  If the expression does not
**      evaluates to true, an assertion will happen in debug mode.
**
**  ARGUMENTS:
**
**      exp     Expression to evaluate.
*/
#if gcmIS_DEBUG(gcdDEBUG_ASSERT)
#   define gcmVERIFY(exp)           gcmASSERT(exp)
#   define gcmkVERIFY(exp)          gcmkASSERT(exp)
#else
#   define gcmVERIFY(exp)           exp
#   define gcmkVERIFY(exp)          exp
#endif

/*******************************************************************************
**
**  gcmVERIFY_OK
**
**      Verify a fucntion returns gcvSTATUS_OK.  If the function does not return
**      gcvSTATUS_OK, an assertion will happen in debug mode.
**
**  ARGUMENTS:
**
**      func    Function to evaluate.
*/

void
gcoOS_Verify(
    IN gceSTATUS Status1
    );

void
gckOS_Verify(
    IN gceSTATUS Status1
    );

#if gcmIS_DEBUG(gcdDEBUG_ASSERT)
#   define gcmVERIFY_OK(func) \
        do \
        { \
            gceSTATUS verifyStatus = func; \
            gcoOS_Verify(verifyStatus); \
            if (verifyStatus != gcvSTATUS_OK) \
            { \
                gcmTRACE( \
                    gcvLEVEL_ERROR, \
                    "gcmVERIFY_OK(%d): function returned %d", \
                    __LINE__, verifyStatus \
                    ); \
            } \
            gcmASSERT(verifyStatus == gcvSTATUS_OK); \
        } \
        while (gcvFALSE)
#   define gcmkVERIFY_OK(func) \
        do \
        { \
            gceSTATUS verifyStatus = func; \
            if (verifyStatus != gcvSTATUS_OK) \
            { \
                gcmkTRACE( \
                    gcvLEVEL_ERROR, \
                    "gcmkVERIFY_OK(%d): function returned %d", \
                    __LINE__, verifyStatus \
                    ); \
            } \
            gckOS_Verify(verifyStatus); \
            gcmkASSERT(verifyStatus == gcvSTATUS_OK); \
        } \
        while (gcvFALSE)
#else
#   define gcmVERIFY_OK(func)       func
#   define gcmkVERIFY_OK(func)      func
#endif

gctCONST_STRING
gcoOS_DebugStatus2Name(
    gceSTATUS status
    );

gctCONST_STRING
gckOS_DebugStatus2Name(
    gceSTATUS status
    );

/*******************************************************************************
**
**  gcmERR_BREAK
**
**      Executes a break statement on error.
**
**  ASSUMPTIONS:
**
**      'status' variable of gceSTATUS type must be defined.
**
**  ARGUMENTS:
**
**      func    Function to evaluate.
*/
#define _gcmERR_BREAK(prefix, func) \
    status = func; \
    if (gcmIS_ERROR(status)) \
    { \
        prefix##PRINT_VERSION(); \
        prefix##TRACE(gcvLEVEL_ERROR, \
            #prefix "ERR_BREAK: status=%d(%s) @ %s(%d)", \
            status, gcoOS_DebugStatus2Name(status), __FUNCTION__, __LINE__); \
        break; \
    } \
    do { } while (gcvFALSE)
#define _gcmkERR_BREAK(prefix, func) \
    status = func; \
    if (gcmIS_ERROR(status)) \
    { \
        prefix##PRINT_VERSION(); \
        prefix##TRACE(gcvLEVEL_ERROR, \
            #prefix "ERR_BREAK: status=%d(%s) @ %s(%d)", \
            status, gckOS_DebugStatus2Name(status), __FUNCTION__, __LINE__); \
        break; \
    } \
    do { } while (gcvFALSE)
#define gcmERR_BREAK(func)          _gcmERR_BREAK(gcm, func)
#define gcmkERR_BREAK(func)         _gcmkERR_BREAK(gcmk, func)

/*******************************************************************************
**
**  gcmERR_RETURN
**
**      Executes a return on error.
**
**  ASSUMPTIONS:
**
**      'status' variable of gceSTATUS type must be defined.
**
**  ARGUMENTS:
**
**      func    Function to evaluate.
*/
#define _gcmERR_RETURN(prefix, func) \
    status = func; \
    if (gcmIS_ERROR(status)) \
    { \
        prefix##PRINT_VERSION(); \
        prefix##TRACE(gcvLEVEL_ERROR, \
            #prefix "ERR_RETURN: status=%d(%s) @ %s(%d)", \
            status, gcoOS_DebugStatus2Name(status), __FUNCTION__, __LINE__); \
        prefix##FOOTER(); \
        return status; \
    } \
    do { } while (gcvFALSE)
#define _gcmkERR_RETURN(prefix, func) \
    status = func; \
    if (gcmIS_ERROR(status)) \
    { \
        prefix##PRINT_VERSION(); \
        prefix##TRACE(gcvLEVEL_ERROR, \
            #prefix "ERR_RETURN: status=%d(%s) @ %s(%d)", \
            status, gckOS_DebugStatus2Name(status), __FUNCTION__, __LINE__); \
        prefix##FOOTER(); \
        return status; \
    } \
    do { } while (gcvFALSE)
#define gcmERR_RETURN(func)         _gcmERR_RETURN(gcm, func)
#define gcmkERR_RETURN(func)        _gcmkERR_RETURN(gcmk, func)


/*******************************************************************************
**
**  gcmONERROR
**
**      Jump to the error handler in case there is an error.
**
**  ASSUMPTIONS:
**
**      'status' variable of gceSTATUS type must be defined.
**
**  ARGUMENTS:
**
**      func    Function to evaluate.
*/
#define _gcmONERROR(prefix, func) \
    do \
    { \
        status = func; \
        if (gcmIS_ERROR(status)) \
        { \
            prefix##PRINT_VERSION(); \
            prefix##TRACE(gcvLEVEL_ERROR, \
                #prefix "ONERROR: status=%d(%s) @ %s(%d)", \
                status, gcoOS_DebugStatus2Name(status), __FUNCTION__, __LINE__); \
            goto OnError; \
        } \
    } \
    while (gcvFALSE)
#define _gcmkONERROR(prefix, func) \
    do \
    { \
        status = func; \
        if (gcmIS_ERROR(status)) \
        { \
            prefix##PRINT_VERSION(); \
            prefix##TRACE(gcvLEVEL_ERROR, \
                #prefix "ONERROR: status=%d(%s) @ %s(%d)", \
                status, gckOS_DebugStatus2Name(status), __FUNCTION__, __LINE__); \
            goto OnError; \
        } \
    } \
    while (gcvFALSE)
#define gcmONERROR(func)            _gcmONERROR(gcm, func)
#define gcmkONERROR(func)           _gcmkONERROR(gcmk, func)

/*******************************************************************************
**
**  gcmVERIFY_LOCK
**
**      Verifies whether the surface is locked.
**
**  ARGUMENTS:
**
**      surfaceInfo Pointer to the surface iniformational structure.
*/
#define gcmVERIFY_LOCK(surfaceInfo) \
    if (!surfaceInfo->node.valid) \
    { \
        gcmONERROR(gcvSTATUS_MEMORY_UNLOCKED); \
    } \

/*******************************************************************************
**
**  gcmVERIFY_NODE_LOCK
**
**      Verifies whether the surface node is locked.
**
**  ARGUMENTS:
**
**      surfaceInfo Pointer to the surface iniformational structure.
*/
#define gcmVERIFY_NODE_LOCK(surfaceNode) \
    if (!(surfaceNode)->valid) \
    { \
        status = gcvSTATUS_MEMORY_UNLOCKED; \
        break; \
    } \
    do { } while (gcvFALSE)

/*******************************************************************************
**
**  gcmBADOBJECT_BREAK
**
**      Executes a break statement on bad object.
**
**  ARGUMENTS:
**
**      obj     Object to test.
**      t       Expected type of the object.
*/
#define gcmBADOBJECT_BREAK(obj, t) \
    if ((obj == gcvNULL) \
    ||  (((gcsOBJECT *)(obj))->type != t) \
    ) \
    { \
        status = gcvSTATUS_INVALID_OBJECT; \
        break; \
    } \
    do { } while (gcvFALSE)

/*******************************************************************************
**
**  gcmCHECK_STATUS
**
**      Executes a break statement on error.
**
**  ASSUMPTIONS:
**
**      'status' variable of gceSTATUS type must be defined.
**
**  ARGUMENTS:
**
**      func    Function to evaluate.
*/
#define _gcmCHECK_STATUS(prefix, func) \
    do \
    { \
        last = func; \
        if (gcmIS_ERROR(last)) \
        { \
            prefix##TRACE(gcvLEVEL_ERROR, \
                #prefix "CHECK_STATUS: status=%d(%s) @ %s(%d)", \
                last, gcoOS_DebugStatus2Name(last), __FUNCTION__, __LINE__); \
            status = last; \
        } \
    } \
    while (gcvFALSE)
#define _gcmkCHECK_STATUS(prefix, func) \
    do \
    { \
        last = func; \
        if (gcmIS_ERROR(last)) \
        { \
            prefix##TRACE(gcvLEVEL_ERROR, \
                #prefix "CHECK_STATUS: status=%d(%s) @ %s(%d)", \
                last, gckOS_DebugStatus2Name(last), __FUNCTION__, __LINE__); \
            status = last; \
        } \
    } \
    while (gcvFALSE)
#define gcmCHECK_STATUS(func)       _gcmCHECK_STATUS(gcm, func)
#define gcmkCHECK_STATUS(func)      _gcmkCHECK_STATUS(gcmk, func)

/*******************************************************************************
**
**  gcmVERIFY_ARGUMENT
**
**      Assert if an argument does not apply to the specified expression.  If
**      the argument evaluates to false, gcvSTATUS_INVALID_ARGUMENT will be
**      returned from the current function.  In retail mode this macro does
**      nothing.
**
**  ARGUMENTS:
**
**      arg     Argument to evaluate.
*/
#   define _gcmVERIFY_ARGUMENT(prefix, arg) \
       do \
       { \
           if (!(arg)) \
           { \
               prefix##TRACE(gcvLEVEL_ERROR, #prefix "VERIFY_ARGUMENT failed:"); \
               prefix##ASSERT(arg); \
               prefix##FOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT); \
               return gcvSTATUS_INVALID_ARGUMENT; \
           } \
       } \
       while (gcvFALSE)
#   define gcmVERIFY_ARGUMENT(arg)     _gcmVERIFY_ARGUMENT(gcm, arg)
#   define gcmkVERIFY_ARGUMENT(arg)    _gcmVERIFY_ARGUMENT(gcmk, arg)

/*******************************************************************************
**
**  gcmDEBUG_VERIFY_ARGUMENT
**
**      Works just like gcmVERIFY_ARGUMENT, but is only valid in debug mode.
**      Use this to verify arguments inside non-public API functions.
*/
#if gcdDEBUG
#   define gcmDEBUG_VERIFY_ARGUMENT(arg)    _gcmVERIFY_ARGUMENT(gcm, arg)
#   define gcmkDEBUG_VERIFY_ARGUMENT(arg)   _gcmkVERIFY_ARGUMENT(gcm, arg)
#else
#   define gcmDEBUG_VERIFY_ARGUMENT(arg)
#   define gcmkDEBUG_VERIFY_ARGUMENT(arg)
#endif

/*******************************************************************************
**
**  gcmVERIFY_ARGUMENT_RETURN
**
**      Assert if an argument does not apply to the specified expression.  If
**      the argument evaluates to false, gcvSTATUS_INVALID_ARGUMENT will be
**      returned from the current function.  In retail mode this macro does
**      nothing.
**
**  ARGUMENTS:
**
**      arg     Argument to evaluate.
*/
#   define _gcmVERIFY_ARGUMENT_RETURN(prefix, arg, value) \
       do \
       { \
           if (!(arg)) \
           { \
               prefix##TRACE(gcvLEVEL_ERROR, \
                             #prefix "gcmVERIFY_ARGUMENT_RETURN failed:"); \
               prefix##ASSERT(arg); \
               prefix##FOOTER_ARG("value=%d", value); \
               return value; \
           } \
       } \
       while (gcvFALSE)
#   define gcmVERIFY_ARGUMENT_RETURN(arg, value) \
                _gcmVERIFY_ARGUMENT_RETURN(gcm, arg, value)
#   define gcmkVERIFY_ARGUMENT_RETURN(arg, value) \
                _gcmVERIFY_ARGUMENT_RETURN(gcmk, arg, value)

#define MAX_LOOP_COUNT 0x7FFFFFFF

/******************************************************************************\
****************************** User Debug Option ******************************
\******************************************************************************/

/* User option. */
typedef enum _gceDEBUG_MSG
{
    gcvDEBUG_MSG_NONE,
    gcvDEBUG_MSG_ERROR,
    gcvDEBUG_MSG_WARNING
}
gceDEBUG_MSG;

typedef struct _gcsUSER_DEBUG_OPTION
{
    gceDEBUG_MSG        debugMsg;
}
gcsUSER_DEBUG_OPTION;

gcsUSER_DEBUG_OPTION *
gcGetUserDebugOption(
    void
    );

struct _gcoOS_SymbolsList
{
    gcePATCH_ID patchId;
    const char * symList[10];
};

#if gcdHAS_ELLIPSES
#define gcmUSER_DEBUG_MSG(level, ...) \
    do \
    { \
        if (level <= gcGetUserDebugOption()->debugMsg) \
        { \
            gcoOS_Print(__VA_ARGS__); \
        } \
    } while (gcvFALSE)

#define gcmUSER_DEBUG_ERROR_MSG(...)   gcmUSER_DEBUG_MSG(gcvDEBUG_MSG_ERROR, "Error: " __VA_ARGS__)
#define gcmUSER_DEBUG_WARNING_MSG(...) gcmUSER_DEBUG_MSG(gcvDEBUG_MSG_WARNING, "Warring: " __VA_ARGS__)
#else
#define gcmUSER_DEBUG_MSG
#define gcmUSER_DEBUG_ERROR_MSG
#define gcmUSER_DEBUG_WARNING_MSG
#endif

#ifdef __cplusplus
}
#endif

#endif /* __gc_hal_base_h_ */
