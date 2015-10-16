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


#ifndef __gc_hal_options_h_
#define __gc_hal_options_h_

/*
    gcdPRINT_VERSION

        Print HAL version.
*/
#ifndef gcdPRINT_VERSION
#   define gcdPRINT_VERSION                     0
#endif

/*
    USE_NEW_LINUX_SIGNAL

        This define enables the Linux kernel signaling between kernel and user.
*/
#ifndef USE_NEW_LINUX_SIGNAL
#   define USE_NEW_LINUX_SIGNAL                 0
#endif

/*
    VIVANTE_PROFILER

        This define enables the profiler.
*/
#ifndef VIVANTE_PROFILER
#   define VIVANTE_PROFILER                     1
#endif

#ifndef VIVANTE_PROFILER_PERDRAW
#   define  VIVANTE_PROFILER_PERDRAW    0
#endif

/*
    VIVANTE_PROFILER_CONTEXT

        This define enables the profiler according to each hw context.
*/
#ifndef VIVANTE_PROFILER_CONTEXT
#   define VIVANTE_PROFILER_CONTEXT             1
#endif

/*
    gcdUSE_VG

        Enable VG HAL layer (only for GC350).
*/
#ifndef gcdUSE_VG
#   define gcdUSE_VG                            0
#endif

/*
    USE_SW_FB

        Set to 1 if the frame buffer memory cannot be accessed by the GPU.
*/
#ifndef USE_SW_FB
#   define USE_SW_FB                            0
#endif

/*
    USE_SUPER_SAMPLING

        This define enables super-sampling support.
*/
#define USE_SUPER_SAMPLING                      0

/*
    PROFILE_HAL_COUNTERS

        This define enables HAL counter profiling support.  HW and SHADER
        counter profiling depends on this.
*/
#ifndef PROFILE_HAL_COUNTERS
#   define PROFILE_HAL_COUNTERS                 1
#endif

/*
    PROFILE_HW_COUNTERS

        This define enables HW counter profiling support.
*/
#ifndef PROFILE_HW_COUNTERS
#   define PROFILE_HW_COUNTERS                  1
#endif

/*
    PROFILE_SHADER_COUNTERS

        This define enables SHADER counter profiling support.
*/
#ifndef PROFILE_SHADER_COUNTERS
#   define PROFILE_SHADER_COUNTERS              1
#endif

/*
    COMMAND_PROCESSOR_VERSION

        The version of the command buffer and task manager.
*/
#define COMMAND_PROCESSOR_VERSION               1

/*
    gcdDUMP_KEY

        Set this to a string that appears in 'cat /proc/<pid>/cmdline'. E.g. 'camera'.
        HAL will create dumps for the processes matching this key.
*/
#ifndef gcdDUMP_KEY
#   define gcdDUMP_KEY                          "process"
#endif

/*
    gcdDUMP_PATH

        The dump file location. Some processes cannot write to the sdcard.
        Try apps' data dir, e.g. /data/data/com.android.launcher
*/
#ifndef gcdDUMP_PATH
#if defined(ANDROID)
#   define gcdDUMP_PATH                         "/mnt/sdcard/"
#else
#   define gcdDUMP_PATH                         "./"
#endif
#endif

/*
    gcdDUMP

        When set to 1, a dump of all states and memory uploads, as well as other
        hardware related execution will be printed to the debug console.  This
        data can be used for playing back applications.
*/
#ifndef gcdDUMP
#   define gcdDUMP                              0
#endif

/*
    gcdDUMP_API

        When set to 1, a high level dump of the EGL and GL/VG APs's are
        captured.
*/
#ifndef gcdDUMP_API
#   define gcdDUMP_API                          0
#endif

/*
    gcdDUMP_FRAMERATE
        When set to a value other than zero, averaqe frame rate will be dumped.
        The value set is the starting frame that the average will be calculated.
        This is needed because sometimes first few frames are too slow to be included
        in the average. Frame count starts from 1.
*/
#ifndef gcdDUMP_FRAMERATE
#   define gcdDUMP_FRAMERATE					0
#endif

/*
    gcdVIRTUAL_COMMAND_BUFFER
        When set to 1, user command buffer and context buffer will be allocated
        from gcvPOOL_VIRTUAL.
*/
#ifndef gcdVIRTUAL_COMMAND_BUFFER
#   define gcdVIRTUAL_COMMAND_BUFFER            1
#endif

/*
    gcdENABLE_FSCALE_VAL_ADJUST
        When non-zero, FSCALE_VAL when gcvPOWER_ON can be adjusted externally.
 */
#ifndef gcdENABLE_FSCALE_VAL_ADJUST
#   define gcdENABLE_FSCALE_VAL_ADJUST          1
#endif

/*
    gcdDUMP_IN_KERNEL

        When set to 1, all dumps will happen in the kernel.  This is handy if
        you want the kernel to dump its command buffers as well and the data
        needs to be in sync.
*/
#ifndef gcdDUMP_IN_KERNEL
#   define gcdDUMP_IN_KERNEL                    0
#endif

/*
    gcdDUMP_COMMAND

        When set to non-zero, the command queue will dump all incoming command
        and context buffers as well as all other modifications to the command
        queue.
*/
#ifndef gcdDUMP_COMMAND
#   define gcdDUMP_COMMAND                      0
#endif

/*
    gcdDUMP_FRAME_TGA

    When set to a value other than 0, a dump of the frame specified by the value,
    will be done into frame.tga. Frame count starts from 1.
 */
#ifndef gcdDUMP_FRAME_TGA
#define gcdDUMP_FRAME_TGA                       0
#endif
/*
    gcdNULL_DRIVER

    Set to 1 for infinite speed hardware.
    Set to 2 for bypassing the HAL.
    Set to 3 for bypassing the drivers.
*/
#ifndef gcdNULL_DRIVER
#   define gcdNULL_DRIVER                       0
#endif

/*
    gcdENABLE_TIMEOUT_DETECTION

        Enable timeout detection.
*/
#ifndef gcdENABLE_TIMEOUT_DETECTION
#   define gcdENABLE_TIMEOUT_DETECTION          0
#endif

/*
    gcdCMD_BUFFER_SIZE

        Number of bytes in a command buffer.
*/
#ifndef gcdCMD_BUFFER_SIZE
#   define gcdCMD_BUFFER_SIZE                   (128 << 10)
#endif

/*
    gcdCMD_BUFFERS

        Number of command buffers to use per client.
*/
#ifndef gcdCMD_BUFFERS
#   define gcdCMD_BUFFERS                       2
#endif

/*
    gcdMAX_CMD_BUFFERS

        Maximum number of command buffers to use per client.
*/
#ifndef gcdMAX_CMD_BUFFERS
#   define gcdMAX_CMD_BUFFERS                   8
#endif

/*
    gcdCOMMAND_QUEUES

        Number of command queues in the kernel.
*/
#ifndef gcdCOMMAND_QUEUES
#   define gcdCOMMAND_QUEUES                    2
#endif

/*
    gcdPOWER_CONTROL_DELAY

        The delay in milliseconds required to wait until the GPU has woke up
        from a suspend or power-down state.  This is system dependent because
        the bus clock also needs to stabalize.
*/
#ifndef gcdPOWER_CONTROL_DELAY
#   define gcdPOWER_CONTROL_DELAY               0
#endif

/*
    gcdMIRROR_PAGETABLE

        Enable it when GPUs with old MMU and new MMU exist at same SoC. It makes
        each GPU use same virtual address to access same physical memory.
*/
#ifndef gcdMIRROR_PAGETABLE
#   define gcdMIRROR_PAGETABLE                  0
#endif

/*
    gcdMMU_SIZE

        Size of the MMU page table in bytes.  Each 4 bytes can hold 4kB worth of
        virtual data.
*/
#ifndef gcdMMU_SIZE
#if gcdMIRROR_PAGETABLE
#   define gcdMMU_SIZE                          0x200000
#else
#   define gcdMMU_SIZE                          (1024 << 10)
#endif
#endif

/*
    gcdSECURE_USER

        Use logical addresses instead of physical addresses in user land.  In
        this case a hint table is created for both command buffers and context
        buffers, and that hint table will be used to patch up those buffers in
        the kernel when they are ready to submit.
*/
#ifndef gcdSECURE_USER
#   define gcdSECURE_USER                       0
#endif

/*
    gcdSECURE_CACHE_SLOTS

        Number of slots in the logical to DMA address cache table.  Each time a
        logical address needs to be translated into a DMA address for the GPU,
        this cache will be walked.  The replacement scheme is LRU.
*/
#ifndef gcdSECURE_CACHE_SLOTS
#   define gcdSECURE_CACHE_SLOTS                1024
#endif

/*
    gcdSECURE_CACHE_METHOD

        Replacement scheme used for Secure Cache.  The following options are
        available:

            gcdSECURE_CACHE_LRU
                A standard LRU cache.

            gcdSECURE_CACHE_LINEAR
                A linear walker with the idea that an application will always
                render the scene in a similar way, so the next entry in the
                cache should be a hit most of the time.

            gcdSECURE_CACHE_HASH
                A 256-entry hash table.

            gcdSECURE_CACHE_TABLE
                A simple cache but with potential of a lot of cache replacement.
*/
#ifndef gcdSECURE_CACHE_METHOD
#   define gcdSECURE_CACHE_METHOD               gcdSECURE_CACHE_HASH
#endif

/*
    gcdREGISTER_ACCESS_FROM_USER

        Set to 1 to allow IOCTL calls to get through from user land.  This
        should only be in debug or development drops.
*/
#ifndef gcdREGISTER_ACCESS_FROM_USER
#   define gcdREGISTER_ACCESS_FROM_USER         1
#endif

/*
    gcdUSER_HEAP_ALLOCATOR

        Set to 1 to enable user mode heap allocator for fast memory allocation
        and destroying. Otherwise, memory allocation/destroying in user mode
        will be directly managed by system. Only for linux for now.
*/
#ifndef gcdUSER_HEAP_ALLOCATOR
#   define gcdUSER_HEAP_ALLOCATOR               1
#endif

/*
    gcdHEAP_SIZE

        Set the allocation size for the internal heaps.  Each time a heap is
        full, a new heap will be allocated with this minmimum amount of bytes.
        The bigger this size, the fewer heaps there are to allocate, the better
        the performance.  However, heaps won't be freed until they are
        completely free, so there might be some more memory waste if the size is
        too big.
*/
#ifndef gcdHEAP_SIZE
#   define gcdHEAP_SIZE                         (64 << 10)
#endif

/*
    gcdPOWER_SUSNPEND_WHEN_IDLE

        Set to 1 to make GPU enter gcvPOWER_SUSPEND when idle detected,
        otherwise GPU will enter gcvPOWER_IDLE.
*/
#ifndef gcdPOWER_SUSNPEND_WHEN_IDLE
#   define gcdPOWER_SUSNPEND_WHEN_IDLE          1
#endif

/*
    gcdFPGA_BUILD

        This define enables work arounds for FPGA images.
*/
#ifndef gcdFPGA_BUILD
#   define gcdFPGA_BUILD                        0
#endif

/*
    gcdGPU_TIMEOUT

        This define specified the number of milliseconds the system will wait
        before it broadcasts the GPU is stuck.  In other words, it will define
        the timeout of any operation that needs to wait for the GPU.

        If the value is 0, no timeout will be checked for.
*/
#ifndef gcdGPU_TIMEOUT
#if gcdFPGA_BUILD
#       define gcdGPU_TIMEOUT                   0
#   else
#       define gcdGPU_TIMEOUT                   20000
#   endif
#endif

/*
    gcdGPU_ADVANCETIMER

        it is advance timer.
*/
#ifndef gcdGPU_ADVANCETIMER
#   define gcdGPU_ADVANCETIMER                  250
#endif

/*
    gcdSTATIC_LINK

        This define disalbes static linking;
*/
#ifndef gcdSTATIC_LINK
#   define gcdSTATIC_LINK                       0
#endif

/*
    gcdUSE_NEW_HEAP

        Setting this define to 1 enables new heap.
*/
#ifndef gcdUSE_NEW_HEAP
#   define gcdUSE_NEW_HEAP                      0
#endif

/*
    gcdCMD_NO_2D_CONTEXT

        This define enables no-context 2D command buffer.
*/
#ifndef gcdCMD_NO_2D_CONTEXT
#   define gcdCMD_NO_2D_CONTEXT                 1
#endif

/*
    gcdENABLE_BANK_ALIGNMENT

    When enabled, video memory is allocated bank aligned. The vendor can modify
    _GetSurfaceBankAlignment() and gcoSURF_GetBankOffsetBytes() to define how
    different types of allocations are bank and channel aligned.
    When disabled (default), no bank alignment is done.
*/
#ifndef gcdENABLE_BANK_ALIGNMENT
#   define gcdENABLE_BANK_ALIGNMENT             0
#endif

/*
    gcdBANK_BIT_START

    Specifies the start bit of the bank (inclusive).
*/
#ifndef gcdBANK_BIT_START
#   define gcdBANK_BIT_START                    12
#endif

/*
    gcdBANK_BIT_END

    Specifies the end bit of the bank (inclusive).
*/
#ifndef gcdBANK_BIT_END
#   define gcdBANK_BIT_END                      14
#endif

/*
    gcdBANK_CHANNEL_BIT

    When set, video memory when allocated bank aligned is allocated such that
    render and depth buffer addresses alternate on the channel bit specified.
    This option has an effect only when gcdENABLE_BANK_ALIGNMENT is enabled.
    When disabled (default), no alteration is done.
*/
#ifndef gcdBANK_CHANNEL_BIT
#   define gcdBANK_CHANNEL_BIT                  7
#endif

/*
    gcdDYNAMIC_SPEED

        When non-zero, it informs the kernel driver to use the speed throttling
        broadcasting functions to inform the system the GPU should be spet up or
        slowed down. It will send a broadcast for slowdown each "interval"
        specified by this define in milliseconds
        (gckOS_BroadcastCalibrateSpeed).
*/
#ifndef gcdDYNAMIC_SPEED
#    define gcdDYNAMIC_SPEED                    2000
#endif

/*
    gcdDYNAMIC_EVENT_THRESHOLD

        When non-zero, it specifies the maximum number of available events at
        which the kernel driver will issue a broadcast to speed up the GPU
        (gckOS_BroadcastHurry).
*/
#ifndef gcdDYNAMIC_EVENT_THRESHOLD
#    define gcdDYNAMIC_EVENT_THRESHOLD          5
#endif

/*
    gcdENABLE_PROFILING

        Enable profiling macros.
*/
#ifndef gcdENABLE_PROFILING
#   define gcdENABLE_PROFILING                  0
#endif

/*
    gcdENABLE_128B_MERGE

        Enable 128B merge for the BUS control.
*/
#ifndef gcdENABLE_128B_MERGE
#   define gcdENABLE_128B_MERGE                 0
#endif

/*
    gcdFRAME_DB

        When non-zero, it specified the number of frames inside the frame
        database. The frame DB will collect per-frame timestamps and hardware
        counters.
*/
#ifndef gcdFRAME_DB
#   define gcdFRAME_DB                          0
#   define gcdFRAME_DB_RESET                    0
#   define gcdFRAME_DB_NAME                     "/var/log/frameDB.log"
#endif

/*
    gcdENABLE_VG
            enable the 2D openVG
*/

#ifndef gcdENABLE_VG
#   define gcdENABLE_VG                         0
#endif

/*
    gcdDYNAMIC_MAP_RESERVED_MEMORY

        When gcvPOOL_SYSTEM is constructed from RESERVED memory,
        driver can map the whole reserved memory to kernel space
        at the beginning, or just map a piece of memory when need
        to access.

        Notice:
        -  It's only for the 2D openVG. For other cores, there is
           _NO_ need to map reserved memory to kernel.
        -  It's meaningless when memory is allocated by
           gckOS_AllocateContiguous, in that case, memory is always
           mapped by system when allocated.
*/
#ifndef gcdDYNAMIC_MAP_RESERVED_MEMORY
#   define gcdDYNAMIC_MAP_RESERVED_MEMORY      1
#endif

/*
   gcdPAGED_MEMORY_CACHEABLE

        When non-zero, paged memory will be cacheable.

        Normally, driver will detemines whether a video memory
        is cacheable or not. When cacheable is not neccessary,
        it will be writecombine.

        This option is only for those SOC which can't enable
        writecombine without enabling cacheable.
*/

#ifndef gcdPAGED_MEMORY_CACHEABLE
#   define gcdPAGED_MEMORY_CACHEABLE            0
#endif

/*
   gcdNONPAGED_MEMORY_CACHEABLE

        When non-zero, non paged memory will be cacheable.
*/

#ifndef gcdNONPAGED_MEMORY_CACHEABLE
#   define gcdNONPAGED_MEMORY_CACHEABLE         0
#endif

/*
   gcdNONPAGED_MEMORY_BUFFERABLE

        When non-zero, non paged memory will be bufferable.
        gcdNONPAGED_MEMORY_BUFFERABLE and gcdNONPAGED_MEMORY_CACHEABLE
        can't be set 1 at same time
*/

#ifndef gcdNONPAGED_MEMORY_BUFFERABLE
#   define gcdNONPAGED_MEMORY_BUFFERABLE        1
#endif

/*
    gcdENABLE_INFINITE_SPEED_HW
            enable the Infinte HW , this is for 2D openVG
*/

#ifndef gcdENABLE_INFINITE_SPEED_HW
#   define gcdENABLE_INFINITE_SPEED_HW          0
#endif

/*
    gcdENABLE_TS_DOUBLE_BUFFER
            enable the TS double buffer, this is for 2D openVG
*/

#ifndef gcdENABLE_TS_DOUBLE_BUFFER
#   define gcdENABLE_TS_DOUBLE_BUFFER           1
#endif

/*
    gcd6000_SUPPORT

    Temporary define to enable/disable 6000 support.
 */
#ifndef gcd6000_SUPPORT
#   define gcd6000_SUPPORT                      0
#endif

/*
    gcdPOWEROFF_TIMEOUT

        When non-zero, GPU will power off automatically from
        idle state, and gcdPOWEROFF_TIMEOUT is also the default
        timeout in milliseconds.
 */

#ifndef gcdPOWEROFF_TIMEOUT
#   define gcdPOWEROFF_TIMEOUT                  300
#endif

/*
    gcdUSE_VIDMEM_PER_PID
*/
#ifndef gcdUSE_VIDMEM_PER_PID
#   define gcdUSE_VIDMEM_PER_PID                0
#endif

/*
    QNX_SINGLE_THREADED_DEBUGGING
*/
#ifndef QNX_SINGLE_THREADED_DEBUGGING
#   define QNX_SINGLE_THREADED_DEBUGGING        0
#endif

/*
    gcdENABLE_RECOVERY

        This define enables the recovery code.
*/
#ifndef gcdENABLE_RECOVERY
#   define gcdENABLE_RECOVERY                   1
#endif

/*
    gcdRENDER_THREADS

        Number of render threads. Make it zero, and there will be no render
        threads.
*/
#ifndef gcdRENDER_THREADS
#   define gcdRENDER_THREADS                    0
#endif

/*
    gcdSMP

        This define enables SMP support.

        Currently, it only works on Linux/Android,
        Kbuild will config it according to whether
        CONFIG_SMP is set.

*/
#ifndef gcdSMP
#   define gcdSMP                               0
#endif

/*
    gcdSUPPORT_SWAP_RECTANGLE

        Support swap with a specific rectangle.

        Set the rectangle with eglSetSwapRectangleVIV api.
*/
#ifndef gcdSUPPORT_SWAP_RECTANGLE
#   define gcdSUPPORT_SWAP_RECTANGLE            0
#endif

/*
    gcdGPU_LINEAR_BUFFER_ENABLED

        Use linear buffer for GPU apps so HWC can do 2D composition.
*/
#ifndef gcdGPU_LINEAR_BUFFER_ENABLED
#   define gcdGPU_LINEAR_BUFFER_ENABLED         1
#endif

/*
    gcdENABLE_RENDER_INTO_WINDOW

        Enable Render-Into-Window (ie, No-Resolve) feature on android.
        NOTE that even if enabled, it still depends on hardware feature and
        android application behavior. When hardware feature or application
        behavior can not support render into window mode, it will fail back
        to normal mode.
        When Render-Into-Window is finally used, window back buffer of android
        applications will be allocated matching render target tiling format.
        Otherwise buffer tiling is decided by the above option
        'gcdGPU_LINEAR_BUFFER_ENABLED'.
*/
#ifndef gcdENABLE_RENDER_INTO_WINDOW
#   define gcdENABLE_RENDER_INTO_WINDOW         1
#endif

/*
    gcdSHARED_RESOLVE_BUFFER_ENABLED

        Use shared resolve buffer for all app buffers.
*/
#ifndef gcdSHARED_RESOLVE_BUFFER_ENABLED
#   define gcdSHARED_RESOLVE_BUFFER_ENABLED         0
#endif

/*
     gcdUSE_TRIANGLE_STRIP_PATCH
 */
#ifndef gcdUSE_TRIANGLE_STRIP_PATCH
#   define gcdUSE_TRIANGLE_STRIP_PATCH            1
#endif

/*
    gcdENABLE_OUTER_CACHE_PATCH

        Enable the outer cache patch.
*/
#ifndef gcdENABLE_OUTER_CACHE_PATCH
#   define gcdENABLE_OUTER_CACHE_PATCH          0
#endif

#ifndef gcdANDROID_UNALIGNED_LINEAR_COMPOSITION_ADJUST
#ifdef ANDROID
#      define  gcdANDROID_UNALIGNED_LINEAR_COMPOSITION_ADJUST    1
#   else
#      define  gcdANDROID_UNALIGNED_LINEAR_COMPOSITION_ADJUST    0
#   endif
#endif

#ifndef gcdENABLE_PE_DITHER_FIX
#   define gcdENABLE_PE_DITHER_FIX              1
#endif

#ifndef gcdSHARED_PAGETABLE
#   define gcdSHARED_PAGETABLE                  1
#endif
#ifndef gcdUSE_PVR
#   define gcdUSE_PVR			                1
#endif

/*
    gcdSMALL_BLOCK_SIZE

        When non-zero, a part of VIDMEM will be reserved for requests
        whose requesting size is less than gcdSMALL_BLOCK_SIZE.

        For Linux, it's the size of a page. If this requeset fallbacks
        to gcvPOOL_CONTIGUOUS or gcvPOOL_VIRTUAL, memory will be wasted
        because they allocate a page at least.
 */
#ifndef gcdSMALL_BLOCK_SIZE
#   define gcdSMALL_BLOCK_SIZE                  4096
#   define gcdRATIO_FOR_SMALL_MEMORY            32
#endif

/*
    gcdCONTIGUOUS_SIZE_LIMIT
        When non-zero, size of video node from gcvPOOL_CONTIGUOUS is
        limited by gcdCONTIGUOUS_SIZE_LIMIT.
 */
#ifndef gcdCONTIGUOUS_SIZE_LIMIT
#   define gcdCONTIGUOUS_SIZE_LIMIT             0
#endif

#ifndef gcdDISALBE_EARLY_EARLY_Z
#   define gcdDISALBE_EARLY_EARLY_Z             1
#endif

#ifndef gcdSHADER_SRC_BY_MACHINECODE
#   define gcdSHADER_SRC_BY_MACHINECODE         1
#endif

/*
    gcdLINK_QUEUE_SIZE

        When non-zero, driver maintains a queue to record information of
        latest lined context buffer and command buffer. Data in this queue
        is be used to debug.
*/
#ifndef gcdLINK_QUEUE_SIZE
#   define gcdLINK_QUEUE_SIZE                  0
#endif

/*  gcdALPHA_KILL_IN_SHADER
 *
 *  Enable alpha kill inside the shader. This will be set automatically by the
 *  HAL if certain states match a criteria.
 */
#ifndef gcdALPHA_KILL_IN_SHADER
#   define gcdALPHA_KILL_IN_SHADER              1
#endif

#ifndef gcdUSE_WCLIP_PATCH
#   define gcdUSE_WCLIP_PATCH                   1
#endif

#ifndef gcdHZ_L2_DISALBE
#   define gcdHZ_L2_DISALBE                     1
#endif

#ifndef gcdBUGFIX15_DISABLE
#   define gcdBUGFIX15_DISABLE                  1
#endif

#ifndef gcdDISABLE_HZ_FAST_CLEAR
#   define gcdDISABLE_HZ_FAST_CLEAR             1
#endif

#ifndef gcdUSE_NPOT_PATCH
#define gcdUSE_NPOT_PATCH                       1
#endif

#ifndef gcdSYNC
#   define gcdSYNC                              1
#endif

#ifndef gcdENABLE_SPECIAL_HINT3
#   define gcdENABLE_SPECIAL_HINT3               1
#endif

#if defined(ANDROID)
#ifndef gcdPRE_ROTATION
#   define gcdPRE_ROTATION                      1
#endif
#endif

/*
    gcdDVFS

        When non-zero, software will make use of dynamic voltage and
        frequency feature.
 */
#ifndef gcdDVFS
#   define gcdDVFS                               0
#   define gcdDVFS_ANAYLSE_WINDOW                4
#   define gcdDVFS_POLLING_TIME                  (gcdDVFS_ANAYLSE_WINDOW * 4)
#endif

#ifndef gcdPRINT_SWAP_TIME
#   define gcdPRINT_SWAP_TIME                    0
#endif

/*
    gcdANDROID_NATIVE_FENCE_SYNC

        Enable android native fence sync. It is introduced since jellybean-4.2.
        Depends on linux kernel option: CONFIG_SYNC.

        0: Disabled
        1: Build framework for native fence sync feature, and EGL extension
        2: Enable async swap buffers for client
           * Native fence sync for client 'queueBuffer' in EGL, which is
             'acquireFenceFd' for layer in compositor side.
        3. Enable async hwcomposer composition.
           * 'releaseFenceFd' for layer in compositor side, which is native
             fence sync when client 'dequeueBuffer'
           * Native fence sync for compositor 'queueBuffer' in EGL, which is
             'acquireFenceFd' for framebuffer target for DC
 */
#ifndef gcdANDROID_NATIVE_FENCE_SYNC
#   define gcdANDROID_NATIVE_FENCE_SYNC        0
#endif

#ifndef gcdFORCE_MIPMAP
#   define gcdFORCE_MIPMAP                     0
#endif

/*
    gcdFORCE_GAL_LOAD_TWICE

        When non-zero, each thread except the main one will load libGAL.so twice to avoid potential segmetantion fault when app using dlopen/dlclose.
        If threads exit arbitrarily, libGAL.so may not unload until the process quit.
 */
#ifndef gcdFORCE_GAL_LOAD_TWICE
#   define gcdFORCE_GAL_LOAD_TWICE             0
#endif

#endif /* __gc_hal_options_h_ */
