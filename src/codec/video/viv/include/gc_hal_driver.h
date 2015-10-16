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


#ifndef __gc_hal_driver_h_
#define __gc_hal_driver_h_

#include "gc_hal_enum.h"
#include "gc_hal_types.h"

#if gcdENABLE_VG
#include "gc_hal_driver_vg.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************\
******************************* I/O Control Codes ******************************
\******************************************************************************/

#define gcvHAL_CLASS                    "galcore"
#define IOCTL_GCHAL_INTERFACE           30000
#define IOCTL_GCHAL_KERNEL_INTERFACE    30001
#define IOCTL_GCHAL_TERMINATE           30002

/******************************************************************************\
********************************* Command Codes ********************************
\******************************************************************************/

typedef enum _gceHAL_COMMAND_CODES
{
    /* Generic query. */
    gcvHAL_QUERY_VIDEO_MEMORY,
    gcvHAL_QUERY_CHIP_IDENTITY,

    /* Contiguous memory. */
    gcvHAL_ALLOCATE_NON_PAGED_MEMORY,
    gcvHAL_FREE_NON_PAGED_MEMORY,
    gcvHAL_ALLOCATE_CONTIGUOUS_MEMORY,
    gcvHAL_FREE_CONTIGUOUS_MEMORY,

    /* Video memory allocation. */
    gcvHAL_ALLOCATE_VIDEO_MEMORY,           /* Enforced alignment. */
    gcvHAL_ALLOCATE_LINEAR_VIDEO_MEMORY,    /* No alignment. */
    gcvHAL_FREE_VIDEO_MEMORY,

    /* Physical-to-logical mapping. */
    gcvHAL_MAP_MEMORY,
    gcvHAL_UNMAP_MEMORY,

    /* Logical-to-physical mapping. */
    gcvHAL_MAP_USER_MEMORY,
    gcvHAL_UNMAP_USER_MEMORY,

    /* Surface lock/unlock. */
    gcvHAL_LOCK_VIDEO_MEMORY,
    gcvHAL_UNLOCK_VIDEO_MEMORY,

    /* Event queue. */
    gcvHAL_EVENT_COMMIT,

    gcvHAL_USER_SIGNAL,
    gcvHAL_SIGNAL,
    gcvHAL_WRITE_DATA,

    gcvHAL_COMMIT,
    gcvHAL_STALL,

    gcvHAL_READ_REGISTER,
    gcvHAL_WRITE_REGISTER,

    gcvHAL_GET_PROFILE_SETTING,
    gcvHAL_SET_PROFILE_SETTING,

    gcvHAL_READ_ALL_PROFILE_REGISTERS,
    gcvHAL_PROFILE_REGISTERS_2D,
#if VIVANTE_PROFILER_PERDRAW
    gcvHAL_READ_PROFILER_REGISTER_SETTING,
#endif

    /* Power management. */
    gcvHAL_SET_POWER_MANAGEMENT_STATE,
    gcvHAL_QUERY_POWER_MANAGEMENT_STATE,

    gcvHAL_GET_BASE_ADDRESS,

    gcvHAL_SET_IDLE, /* reserved */

    /* Queries. */
    gcvHAL_QUERY_KERNEL_SETTINGS,

    /* Reset. */
    gcvHAL_RESET,

    /* Map physical address into handle. */
    gcvHAL_MAP_PHYSICAL,

    /* Debugger stuff. */
    gcvHAL_DEBUG,

    /* Cache stuff. */
    gcvHAL_CACHE,

    /* TimeStamp */
    gcvHAL_TIMESTAMP,

    /* Database. */
    gcvHAL_DATABASE,

    /* Version. */
    gcvHAL_VERSION,

    /* Chip info */
    gcvHAL_CHIP_INFO,

    /* Process attaching/detaching. */
    gcvHAL_ATTACH,
    gcvHAL_DETACH,

    /* Composition. */
    gcvHAL_COMPOSE,

    /* Set timeOut value */
    gcvHAL_SET_TIMEOUT,

    /* Frame database. */
    gcvHAL_GET_FRAME_INFO,

    /* Shared info for each process */
    gcvHAL_GET_SHARED_INFO,
    gcvHAL_SET_SHARED_INFO,
    gcvHAL_QUERY_COMMAND_BUFFER,

    gcvHAL_COMMIT_DONE,

    /* GPU and event dump */
    gcvHAL_DUMP_GPU_STATE,
    gcvHAL_DUMP_EVENT,

    /* Virtual command buffer. */
    gcvHAL_ALLOCATE_VIRTUAL_COMMAND_BUFFER,
    gcvHAL_FREE_VIRTUAL_COMMAND_BUFFER,

    /* FSCALE_VAL. */
    gcvHAL_SET_FSCALE_VALUE,
    gcvHAL_GET_FSCALE_VALUE,

    /* Reset time stamp. */
    gcvHAL_QUERY_RESET_TIME_STAMP,

    /* Sync point operations. */
    gcvHAL_SYNC_POINT,

    /* Create native fence and return its fd. */
    gcvHAL_CREATE_NATIVE_FENCE,
}
gceHAL_COMMAND_CODES;

/******************************************************************************\
****************************** Interface Structure *****************************
\******************************************************************************/

#define gcdMAX_PROFILE_FILE_NAME    128

/* Kernel settings. */
typedef struct _gcsKERNEL_SETTINGS
{
    /* Used RealTime signal between kernel and user. */
    gctINT signal;
}
gcsKERNEL_SETTINGS;


/* gcvHAL_QUERY_CHIP_IDENTITY */
typedef struct _gcsHAL_QUERY_CHIP_IDENTITY * gcsHAL_QUERY_CHIP_IDENTITY_PTR;
typedef struct _gcsHAL_QUERY_CHIP_IDENTITY
{

    /* Chip model. */
    gceCHIPMODEL                chipModel;

    /* Revision value.*/
    gctUINT32                   chipRevision;

    /* Supported feature fields. */
    gctUINT32                   chipFeatures;

    /* Supported minor feature fields. */
    gctUINT32                   chipMinorFeatures;

    /* Supported minor feature 1 fields. */
    gctUINT32                   chipMinorFeatures1;

    /* Supported minor feature 2 fields. */
    gctUINT32                   chipMinorFeatures2;

    /* Supported minor feature 3 fields. */
    gctUINT32                   chipMinorFeatures3;

    /* Supported minor feature 4 fields. */
    gctUINT32                   chipMinorFeatures4;

    /* Number of streams supported. */
    gctUINT32                   streamCount;

    /* Total number of temporary registers per thread. */
    gctUINT32                   registerMax;

    /* Maximum number of threads. */
    gctUINT32                   threadCount;

    /* Number of shader cores. */
    gctUINT32                   shaderCoreCount;

    /* Size of the vertex cache. */
    gctUINT32                   vertexCacheSize;

    /* Number of entries in the vertex output buffer. */
    gctUINT32                   vertexOutputBufferSize;

    /* Number of pixel pipes. */
    gctUINT32                   pixelPipes;

    /* Number of instructions. */
    gctUINT32                   instructionCount;

    /* Number of constants. */
    gctUINT32                   numConstants;

    /* Buffer size */
    gctUINT32                   bufferSize;

    /* Number of varyings */
    gctUINT32                   varyingsCount;

    /* Supertile layout style in hardware */
    gctUINT32                   superTileMode;
}
gcsHAL_QUERY_CHIP_IDENTITY;

/* gcvHAL_COMPOSE. */
typedef struct _gcsHAL_COMPOSE * gcsHAL_COMPOSE_PTR;
typedef struct _gcsHAL_COMPOSE
{
    /* Composition state buffer. */
    gctUINT64                   physical;
    gctUINT64                   logical;
    gctUINT                     offset;
    gctUINT                     size;

    /* Composition end signal. */
    gctUINT64                   process;
    gctUINT64                   signal;

    /* User signals. */
    gctUINT64                   userProcess;
    gctUINT64                   userSignal1;
    gctUINT64                   userSignal2;

#if defined(__QNXNTO__)
    /* Client pulse side-channel connection ID. */
    gctINT32                    coid;

    /* Set by server. */
    gctINT32                    rcvid;
#endif
}
gcsHAL_COMPOSE;


typedef struct _gcsHAL_INTERFACE
{
    /* Command code. */
    gceHAL_COMMAND_CODES        command;

    /* Hardware type. */
    gceHARDWARE_TYPE            hardwareType;

    /* Status value. */
    gceSTATUS                   status;

    /* Handle to this interface channel. */
    gctUINT64                   handle;

    /* Pid of the client. */
    gctUINT32                   pid;

    /* Union of command structures. */
    union _u
    {
        /* gcvHAL_GET_BASE_ADDRESS */
        struct _gcsHAL_GET_BASE_ADDRESS
        {
            /* Physical memory address of internal memory. */
            OUT gctUINT32               baseAddress;
        }
        GetBaseAddress;

        /* gcvHAL_QUERY_VIDEO_MEMORY */
        struct _gcsHAL_QUERY_VIDEO_MEMORY
        {
            /* Physical memory address of internal memory. Just a name. */
            OUT gctUINT32               internalPhysical;

            /* Size in bytes of internal memory. */
            OUT gctUINT64               internalSize;

            /* Physical memory address of external memory. Just a name. */
            OUT gctUINT32               externalPhysical;

            /* Size in bytes of external memory.*/
            OUT gctUINT64               externalSize;

            /* Physical memory address of contiguous memory. Just a name. */
            OUT gctUINT32               contiguousPhysical;

            /* Size in bytes of contiguous memory.*/
            OUT gctUINT64               contiguousSize;
        }
        QueryVideoMemory;

        /* gcvHAL_QUERY_CHIP_IDENTITY */
        gcsHAL_QUERY_CHIP_IDENTITY      QueryChipIdentity;

        /* gcvHAL_MAP_MEMORY */
        struct _gcsHAL_MAP_MEMORY
        {
            /* Physical memory address to map. Just a name on Linux/Qnx. */
            IN gctUINT32                physical;

            /* Number of bytes in physical memory to map. */
            IN gctUINT64                bytes;

            /* Address of mapped memory. */
            OUT gctUINT64               logical;
        }
        MapMemory;

        /* gcvHAL_UNMAP_MEMORY */
        struct _gcsHAL_UNMAP_MEMORY
        {
            /* Physical memory address to unmap. Just a name on Linux/Qnx. */
            IN gctUINT32                physical;

            /* Number of bytes in physical memory to unmap. */
            IN gctUINT64                bytes;

            /* Address of mapped memory to unmap. */
            IN gctUINT64                logical;
        }
        UnmapMemory;

        /* gcvHAL_ALLOCATE_LINEAR_VIDEO_MEMORY */
        struct _gcsHAL_ALLOCATE_LINEAR_VIDEO_MEMORY
        {
            /* Number of bytes to allocate. */
            IN OUT gctUINT              bytes;

            /* Buffer alignment. */
            IN gctUINT                  alignment;

            /* Type of allocation. */
            IN gceSURF_TYPE             type;

            /* Memory pool to allocate from. */
            IN OUT gcePOOL              pool;

            /* Allocated video memory in gcuVIDMEM_NODE. */
            OUT gctUINT64               node;
        }
        AllocateLinearVideoMemory;

        /* gcvHAL_ALLOCATE_VIDEO_MEMORY */
        struct _gcsHAL_ALLOCATE_VIDEO_MEMORY
        {
            /* Width of rectangle to allocate. */
            IN OUT gctUINT              width;

            /* Height of rectangle to allocate. */
            IN OUT gctUINT              height;

            /* Depth of rectangle to allocate. */
            IN gctUINT                  depth;

            /* Format rectangle to allocate in gceSURF_FORMAT. */
            IN gceSURF_FORMAT           format;

            /* Type of allocation. */
            IN gceSURF_TYPE             type;

            /* Memory pool to allocate from. */
            IN OUT gcePOOL              pool;

            /* Allocated video memory in gcuVIDMEM_NODE. */
            OUT gctUINT64               node;
        }
        AllocateVideoMemory;

        /* gcvHAL_FREE_VIDEO_MEMORY */
        struct _gcsHAL_FREE_VIDEO_MEMORY
        {
            /* Allocated video memory in gcuVIDMEM_NODE. */
            IN gctUINT64        node;

#ifdef __QNXNTO__
/* TODO: This is part of the unlock - why is it here? */
            /* Mapped logical address to unmap in user space. */
            OUT gctUINT64       memory;

            /* Number of bytes to allocated. */
            OUT gctUINT64       bytes;
#endif
        }
        FreeVideoMemory;

        /* gcvHAL_LOCK_VIDEO_MEMORY */
        struct _gcsHAL_LOCK_VIDEO_MEMORY
        {
            /* Allocated video memory gcuVIDMEM_NODE gcuVIDMEM_NODE. */
            IN gctUINT64            node;

            /* Cache configuration. */
            /* Only gcvPOOL_CONTIGUOUS and gcvPOOL_VIRUTAL
            ** can be configured */
            IN gctBOOL              cacheable;

            /* Hardware specific address. */
            OUT gctUINT32           address;

            /* Mapped logical address. */
            OUT gctUINT64           memory;
        }
        LockVideoMemory;

        /* gcvHAL_UNLOCK_VIDEO_MEMORY */
        struct _gcsHAL_UNLOCK_VIDEO_MEMORY
        {
            /* Allocated video memory in gcuVIDMEM_NODE. */
            IN gctUINT64            node;

            /* Type of surface. */
            IN gceSURF_TYPE         type;

            /* Flag to unlock surface asynchroneously. */
            IN OUT gctBOOL          asynchroneous;
        }
        UnlockVideoMemory;

        /* gcvHAL_ALLOCATE_NON_PAGED_MEMORY */
        struct _gcsHAL_ALLOCATE_NON_PAGED_MEMORY
        {
            /* Number of bytes to allocate. */
            IN OUT gctUINT64        bytes;

            /* Physical address of allocation. Just a name. */
            OUT gctUINT32           physical;

            /* Logical address of allocation. */
            OUT gctUINT64           logical;
        }
        AllocateNonPagedMemory;

        /* gcvHAL_FREE_NON_PAGED_MEMORY */
        struct _gcsHAL_FREE_NON_PAGED_MEMORY
        {
            /* Number of bytes allocated. */
            IN gctUINT64            bytes;

            /* Physical address of allocation. Just a name. */
            IN gctUINT32            physical;

            /* Logical address of allocation. */
            IN gctUINT64            logical;
        }
        FreeNonPagedMemory;

        /* gcvHAL_ALLOCATE_NON_PAGED_MEMORY */
        struct _gcsHAL_ALLOCATE_VIRTUAL_COMMAND_BUFFER
        {
            /* Number of bytes to allocate. */
            IN OUT gctUINT64        bytes;

            /* Physical address of allocation. Just a name. */
            OUT gctUINT32           physical;

            /* Logical address of allocation. */
            OUT gctUINT64           logical;
        }
        AllocateVirtualCommandBuffer;

        /* gcvHAL_FREE_NON_PAGED_MEMORY */
        struct _gcsHAL_FREE_VIRTUAL_COMMAND_BUFFER
        {
            /* Number of bytes allocated. */
            IN gctUINT64            bytes;

            /* Physical address of allocation. Just a name. */
            IN gctUINT32            physical;

            /* Logical address of allocation. */
            IN gctUINT64            logical;
        }
        FreeVirtualCommandBuffer;

        /* gcvHAL_EVENT_COMMIT. */
        struct _gcsHAL_EVENT_COMMIT
        {
            /* Event queue in gcsQUEUE. */
            IN gctUINT64             queue;
        }
        Event;

        /* gcvHAL_COMMIT */
        struct _gcsHAL_COMMIT
        {
            /* Context buffer object gckCONTEXT. */
            IN gctUINT64            context;

            /* Command buffer gcoCMDBUF. */
            IN gctUINT64            commandBuffer;

            /* State delta buffer in gcsSTATE_DELTA. */
            gctUINT64               delta;

            /* Event queue in gcsQUEUE. */
            IN gctUINT64            queue;
        }
        Commit;

        /* gcvHAL_MAP_USER_MEMORY */
        struct _gcsHAL_MAP_USER_MEMORY
        {
            /* Base address of user memory to map. */
            IN gctUINT64                memory;

            /* Physical address of user memory to map. */
            IN gctUINT32                physical;

            /* Size of user memory in bytes to map. */
            IN gctUINT64                size;

            /* Info record required by gcvHAL_UNMAP_USER_MEMORY. Just a name. */
            OUT gctUINT32               info;

            /* Physical address of mapped memory. */
            OUT gctUINT32               address;
        }
        MapUserMemory;

        /* gcvHAL_UNMAP_USER_MEMORY */
        struct _gcsHAL_UNMAP_USER_MEMORY
        {
            /* Base address of user memory to unmap. */
            IN gctUINT64                memory;

            /* Size of user memory in bytes to unmap. */
            IN gctUINT64                size;

            /* Info record returned by gcvHAL_MAP_USER_MEMORY. Just a name. */
            IN gctUINT32                info;

            /* Physical address of mapped memory as returned by
               gcvHAL_MAP_USER_MEMORY. */
            IN gctUINT32                address;
        }
        UnmapUserMemory;
#if !USE_NEW_LINUX_SIGNAL
        /* gcsHAL_USER_SIGNAL  */
        struct _gcsHAL_USER_SIGNAL
        {
            /* Command. */
            gceUSER_SIGNAL_COMMAND_CODES command;

            /* Signal ID. */
            IN OUT gctINT               id;

            /* Reset mode. */
            IN gctBOOL                  manualReset;

            /* Wait timedout. */
            IN gctUINT32                wait;

            /* State. */
            IN gctBOOL                  state;
        }
        UserSignal;
#endif

        /* gcvHAL_SIGNAL. */
        struct _gcsHAL_SIGNAL
        {
            /* Signal handle to signal gctSIGNAL. */
            IN gctUINT64                signal;

            /* Reserved gctSIGNAL. */
            IN gctUINT64                auxSignal;

            /* Process owning the signal gctHANDLE. */
            IN gctUINT64                process;

#if defined(__QNXNTO__)
            /* Client pulse side-channel connection ID. Set by client in gcoOS_CreateSignal. */
            IN gctINT32                 coid;

            /* Set by server. */
            IN gctINT32                 rcvid;
#endif
            /* Event generated from where of pipeline */
            IN gceKERNEL_WHERE          fromWhere;
        }
        Signal;

        /* gcvHAL_WRITE_DATA. */
        struct _gcsHAL_WRITE_DATA
        {
            /* Address to write data to. */
            IN gctUINT32                address;

            /* Data to write. */
            IN gctUINT32                data;
        }
        WriteData;

        /* gcvHAL_ALLOCATE_CONTIGUOUS_MEMORY */
        struct _gcsHAL_ALLOCATE_CONTIGUOUS_MEMORY
        {
            /* Number of bytes to allocate. */
            IN OUT gctUINT64            bytes;

            /* Hardware address of allocation. */
            OUT gctUINT32               address;

            /* Physical address of allocation. Just a name. */
            OUT gctUINT32               physical;

            /* Logical address of allocation. */
            OUT gctUINT64               logical;
        }
        AllocateContiguousMemory;

        /* gcvHAL_FREE_CONTIGUOUS_MEMORY */
        struct _gcsHAL_FREE_CONTIGUOUS_MEMORY
        {
            /* Number of bytes allocated. */
            IN gctUINT64                bytes;

            /* Physical address of allocation. Just a name. */
            IN gctUINT32                physical;

            /* Logical address of allocation. */
            IN gctUINT64                logical;
        }
        FreeContiguousMemory;

        /* gcvHAL_READ_REGISTER */
        struct _gcsHAL_READ_REGISTER
        {
            /* Logical address of memory to write data to. */
            IN gctUINT32            address;

            /* Data read. */
            OUT gctUINT32           data;
        }
        ReadRegisterData;

        /* gcvHAL_WRITE_REGISTER */
        struct _gcsHAL_WRITE_REGISTER
        {
            /* Logical address of memory to write data to. */
            IN gctUINT32            address;

            /* Data read. */
            IN gctUINT32            data;
        }
        WriteRegisterData;

#if VIVANTE_PROFILER
        /* gcvHAL_GET_PROFILE_SETTING */
        struct _gcsHAL_GET_PROFILE_SETTING
        {
            /* Enable profiling */
            OUT gctBOOL             enable;

            /* The profile file name */
            OUT gctCHAR             fileName[gcdMAX_PROFILE_FILE_NAME];
        }
        GetProfileSetting;

        /* gcvHAL_SET_PROFILE_SETTING */
        struct _gcsHAL_SET_PROFILE_SETTING
        {
            /* Enable profiling */
            IN gctBOOL              enable;

            /* The profile file name */
            IN gctCHAR              fileName[gcdMAX_PROFILE_FILE_NAME];
        }
        SetProfileSetting;

#if VIVANTE_PROFILER_PERDRAW
        /* gcvHAL_READ_PROFILER_REGISTER_SETTING */
        struct _gcsHAL_READ_PROFILER_REGISTER_SETTING
         {
            /*Should Clear Register*/
            IN gctBOOL               bclear;
         }
        SetProfilerRegisterClear;
#endif

        /* gcvHAL_READ_ALL_PROFILE_REGISTERS */
        struct _gcsHAL_READ_ALL_PROFILE_REGISTERS
        {
#if VIVANTE_PROFILER_CONTEXT
            /* Context buffer object gckCONTEXT. Just a name. */
            IN gctUINT32                context;
#endif
            /* Data read. */
            OUT gcsPROFILER_COUNTERS    counters;
        }
        RegisterProfileData;

        /* gcvHAL_PROFILE_REGISTERS_2D */
        struct _gcsHAL_PROFILE_REGISTERS_2D
        {
            /* Data read in gcs2D_PROFILE. */
            OUT gctUINT64       hwProfile2D;
        }
        RegisterProfileData2D;
#endif
        /* Power management. */
        /* gcvHAL_SET_POWER_MANAGEMENT_STATE */
        struct _gcsHAL_SET_POWER_MANAGEMENT
        {
            /* Data read. */
            IN gceCHIPPOWERSTATE        state;
        }
        SetPowerManagement;

        /* gcvHAL_QUERY_POWER_MANAGEMENT_STATE */
        struct _gcsHAL_QUERY_POWER_MANAGEMENT
        {
            /* Data read. */
            OUT gceCHIPPOWERSTATE       state;

            /* Idle query. */
            OUT gctBOOL                 isIdle;
        }
        QueryPowerManagement;

        /* gcvHAL_QUERY_KERNEL_SETTINGS */
        struct _gcsHAL_QUERY_KERNEL_SETTINGS
        {
            /* Settings.*/
            OUT gcsKERNEL_SETTINGS      settings;
        }
        QueryKernelSettings;

        /* gcvHAL_MAP_PHYSICAL */
        struct _gcsHAL_MAP_PHYSICAL
        {
            /* gcvTRUE to map, gcvFALSE to unmap. */
            IN gctBOOL                  map;

            /* Physical address. */
            IN OUT gctUINT64            physical;
        }
        MapPhysical;

        /* gcvHAL_DEBUG */
        struct _gcsHAL_DEBUG
        {
            /* If gcvTRUE, set the debug information. */
            IN gctBOOL                  set;
            IN gctUINT32                level;
            IN gctUINT32                zones;
            IN gctBOOL                  enable;

            IN gceDEBUG_MESSAGE_TYPE    type;
            IN gctUINT32                messageSize;

            /* Message to print if not empty. */
            IN gctCHAR                  message[80];
        }
        Debug;

        /* gcvHAL_CACHE */
        struct _gcsHAL_CACHE
        {
            IN gceCACHEOPERATION        operation;
            /* gctHANDLE */
            IN gctUINT64                process;
            IN gctUINT64                logical;
            IN gctUINT64                bytes;
            /* gcuVIDMEM_NODE_PTR */
            IN gctUINT64                node;
        }
        Cache;

        /* gcvHAL_TIMESTAMP */
        struct _gcsHAL_TIMESTAMP
        {
            /* Timer select. */
            IN gctUINT32                timer;

            /* Timer request type (0-stop, 1-start, 2-send delta). */
            IN gctUINT32                request;

            /* Result of delta time in microseconds. */
            OUT gctINT32                timeDelta;
        }
        TimeStamp;

        /* gcvHAL_DATABASE */
        struct _gcsHAL_DATABASE
        {
            /* Set to gcvTRUE if you want to query a particular process ID.
            ** Set to gcvFALSE to query the last detached process. */
            IN gctBOOL                  validProcessID;

            /* Process ID to query. */
            IN gctUINT32                processID;

            /* Information. */
            OUT gcuDATABASE_INFO        vidMem;
            OUT gcuDATABASE_INFO        nonPaged;
            OUT gcuDATABASE_INFO        contiguous;
            OUT gcuDATABASE_INFO        gpuIdle;
        }
        Database;

        /* gcvHAL_VERSION */
        struct _gcsHAL_VERSION
        {
            /* Major version: N.n.n. */
            OUT gctINT32                major;

            /* Minor version: n.N.n. */
            OUT gctINT32                minor;

            /* Patch version: n.n.N. */
            OUT gctINT32                patch;

            /* Build version. */
            OUT gctUINT32               build;
        }
        Version;

        /* gcvHAL_CHIP_INFO */
        struct _gcsHAL_CHIP_INFO
        {
            /* Chip count. */
            OUT gctINT32                count;

            /* Chip types. */
            OUT gceHARDWARE_TYPE        types[gcdCHIP_COUNT];
        }
        ChipInfo;

        /* gcvHAL_ATTACH */
        struct _gcsHAL_ATTACH
        {
            /* Context buffer object gckCONTEXT. Just a name. */
            OUT gctUINT32               context;

            /* Number of states in the buffer. */
            OUT gctUINT64               stateCount;
        }
        Attach;

        /* gcvHAL_DETACH */
        struct _gcsHAL_DETACH
        {
            /* Context buffer object gckCONTEXT. Just a name. */
            IN gctUINT32                context;
        }
        Detach;

        /* gcvHAL_COMPOSE. */
        gcsHAL_COMPOSE            Compose;

        /* gcvHAL_GET_FRAME_INFO. */
        struct _gcsHAL_GET_FRAME_INFO
        {
            /* gcsHAL_FRAME_INFO* */
            OUT gctUINT64     frameInfo;
        }
        GetFrameInfo;

        /* gcvHAL_SET_TIME_OUT. */
        struct _gcsHAL_SET_TIMEOUT
        {
            gctUINT32                   timeOut;
        }
        SetTimeOut;

#if gcdENABLE_VG
		/* gcvHAL_COMMIT */
		struct _gcsHAL_VGCOMMIT
		{
			/* Context buffer in gcsVGCONTEXT. */
			IN gctUINT64			context;

			/* Command queue in gcsVGCMDQUEUE. */
			IN gctUINT64			queue;

			/* Number of entries in the queue. */
			IN gctUINT			entryCount;

			/* Task table in gcsTASK_MASTER_TABLE. */
			IN gctUINT64	                taskTable;
		}
		VGCommit;

		/* gcvHAL_QUERY_COMMAND_BUFFER */
		struct _gcsHAL_QUERY_COMMAND_BUFFER
		{
			/* Command buffer attributes. */
			OUT gcsCOMMAND_BUFFER_INFO	information;
		}
		QueryCommandBuffer;

#endif

        struct _gcsHAL_GET_SHARED_INFO
        {
            /* Process id. */
            IN gctUINT32            pid;

            /* Data id. */
            IN gctUINT32            dataId;

            /* Data size. */
            IN gctSIZE_T            bytes;

            /* Pointer to save the shared data. */
            OUT gctPOINTER          data;
        }
        GetSharedInfo;

        struct _gcsHAL_SET_SHARED_INFO
        {
            /* Data id. */
            IN gctUINT32            dataId;

            /* Data to be shared. */
            IN gctPOINTER           data;

            /* Data size. */
            IN gctSIZE_T            bytes;
        }
        SetSharedInfo;

        struct _gcsHAL_SET_FSCALE_VALUE
        {
            IN gctUINT              value;
        }
        SetFscaleValue;

        struct _gcsHAL_GET_FSCALE_VALUE
        {
            OUT gctUINT             value;
            OUT gctUINT             minValue;
            OUT gctUINT             maxValue;
        }
        GetFscaleValue;

        struct _gcsHAL_QUERY_RESET_TIME_STAMP
        {
            OUT gctUINT64           timeStamp;
        }
        QueryResetTimeStamp;

        struct _gcsHAL_SYNC_POINT
        {
            /* Command. */
            gceSYNC_POINT_COMMAND_CODES command;

            /* Sync point. */
            IN OUT gctUINT64            syncPoint;

            /* From where. */
            IN gceKERNEL_WHERE          fromWhere;

            /* Signaled state. */
            OUT gctBOOL                 state;
        }
        SyncPoint;

        struct _gcsHAL_CREATE_NATIVE_FENCE
        {
            /* Signal id to dup. */
            IN gctUINT64                syncPoint;

            /* Native fence file descriptor. */
            OUT gctINT                  fenceFD;

        }
        CreateNativeFence;
    }
    u;
}
gcsHAL_INTERFACE;


#ifdef __cplusplus
}
#endif

#endif /* __gc_hal_driver_h_ */
