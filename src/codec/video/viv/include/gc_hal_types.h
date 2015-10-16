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


#ifndef __gc_hal_types_h_
#define __gc_hal_types_h_

#include "gc_hal_version.h"
#include "gc_hal_options.h"

#ifdef _WIN32
#pragma warning(disable:4127)   /* Conditional expression is constant (do { }
                                ** while(0)). */
#pragma warning(disable:4100)   /* Unreferenced formal parameter. */
#pragma warning(disable:4204)   /* Non-constant aggregate initializer (C99). */
#pragma warning(disable:4131)   /* Uses old-style declarator (for Bison and
                                ** Flex generated files). */
#pragma warning(disable:4206)   /* Translation unit is empty. */
#endif

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************\
**  Platform macros.
*/

#if defined(__GNUC__)
#   define gcdHAS_ELLIPSES      1       /* GCC always has it. */
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
#   define gcdHAS_ELLIPSES      1       /* C99 has it. */
#elif defined(_MSC_VER) && (_MSC_VER >= 1500)
#   define gcdHAS_ELLIPSES      1       /* MSVC 2007+ has it. */
#elif defined(UNDER_CE)
#if UNDER_CE >= 600
#       define gcdHAS_ELLIPSES  1
#   else
#       define gcdHAS_ELLIPSES  0
#   endif
#else
#   error "gcdHAS_ELLIPSES: Platform could not be determined"
#endif

/******************************************************************************\
************************************ Keyword ***********************************
\******************************************************************************/

#if (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L))
#   define gcmINLINE            inline      /* C99 keyword. */
#elif defined(__GNUC__)
#   define gcmINLINE            __inline__  /* GNU keyword. */
#elif defined(_MSC_VER) || defined(UNDER_CE)
#   define gcmINLINE            __inline    /* Internal keyword. */
#else
#   error "gcmINLINE: Platform could not be determined"
#endif

/* Possible debug flags. */
#define gcdDEBUG_NONE           0
#define gcdDEBUG_ALL            (1 << 0)
#define gcdDEBUG_FATAL          (1 << 1)
#define gcdDEBUG_TRACE          (1 << 2)
#define gcdDEBUG_BREAK          (1 << 3)
#define gcdDEBUG_ASSERT         (1 << 4)
#define gcdDEBUG_CODE           (1 << 5)
#define gcdDEBUG_STACK          (1 << 6)

#define gcmIS_DEBUG(flag)       ( gcdDEBUG & (flag | gcdDEBUG_ALL) )

#ifndef gcdDEBUG
#if (defined(DBG) && DBG) || defined(DEBUG) || defined(_DEBUG)
#       define gcdDEBUG         gcdDEBUG_ALL
#   else
#       define gcdDEBUG         gcdDEBUG_NONE
#   endif
#endif

#ifdef _USRDLL
#ifdef _MSC_VER
#ifdef HAL_EXPORTS
#           define HALAPI       __declspec(dllexport)
#       else
#           define HALAPI       __declspec(dllimport)
#       endif
#       define HALDECL          __cdecl
#   else
#ifdef HAL_EXPORTS
#           define HALAPI
#       else
#           define HALAPI       extern
#       endif
#   endif
#else
#   define HALAPI
#   define HALDECL
#endif

/******************************************************************************\
********************************** Common Types ********************************
\******************************************************************************/

#define gcvFALSE                0
#define gcvTRUE                 1

#define gcvINFINITE             ((gctUINT32) ~0U)

#define gcvINVALID_HANDLE       ((gctHANDLE) ~0U)

typedef int                     gctBOOL;
typedef gctBOOL *               gctBOOL_PTR;

typedef int                     gctINT;
typedef long                    gctLONG;
typedef signed char             gctINT8;
typedef signed short            gctINT16;
typedef signed int              gctINT32;
typedef signed long long        gctINT64;

typedef gctINT *                gctINT_PTR;
typedef gctINT8 *               gctINT8_PTR;
typedef gctINT16 *              gctINT16_PTR;
typedef gctINT32 *              gctINT32_PTR;
typedef gctINT64 *              gctINT64_PTR;

typedef unsigned int            gctUINT;
typedef unsigned char           gctUINT8;
typedef unsigned short          gctUINT16;
typedef unsigned int            gctUINT32;
typedef unsigned long long      gctUINT64;
typedef unsigned long           gctUINTPTR_T;

typedef gctUINT *               gctUINT_PTR;
typedef gctUINT8 *              gctUINT8_PTR;
typedef gctUINT16 *             gctUINT16_PTR;
typedef gctUINT32 *             gctUINT32_PTR;
typedef gctUINT64 *             gctUINT64_PTR;

typedef unsigned long           gctSIZE_T;
typedef gctSIZE_T *             gctSIZE_T_PTR;

#ifdef __cplusplus
#   define gcvNULL              0
#else
#   define gcvNULL              ((void *) 0)
#endif

typedef float                   gctFLOAT;
typedef signed int              gctFIXED_POINT;
typedef float *                 gctFLOAT_PTR;

typedef void *                  gctPHYS_ADDR;
typedef void *                  gctHANDLE;
typedef void *                  gctFILE;
typedef void *                  gctSIGNAL;
typedef void *                  gctWINDOW;
typedef void *                  gctIMAGE;
typedef void *                  gctSYNC_POINT;

typedef void *					gctSEMAPHORE;

typedef void *                  gctPOINTER;
typedef const void *            gctCONST_POINTER;

typedef char                    gctCHAR;
typedef char *                  gctSTRING;
typedef const char *            gctCONST_STRING;

typedef struct _gcsCOUNT_STRING
{
    gctSIZE_T                   Length;
    gctCONST_STRING             String;
}
gcsCOUNT_STRING;

typedef union _gcuFLOAT_UINT32
{
    gctFLOAT    f;
    gctUINT32   u;
}
gcuFLOAT_UINT32;

/* Fixed point constants. */
#define gcvZERO_X               ((gctFIXED_POINT) 0x00000000)
#define gcvHALF_X               ((gctFIXED_POINT) 0x00008000)
#define gcvONE_X                ((gctFIXED_POINT) 0x00010000)
#define gcvNEGONE_X             ((gctFIXED_POINT) 0xFFFF0000)
#define gcvTWO_X                ((gctFIXED_POINT) 0x00020000)

/* Stringizing macro. */
#define gcmSTRING(Value)        #Value

/******************************************************************************\
******************************* Fixed Point Math *******************************
\******************************************************************************/

#define gcmXMultiply(x1, x2)            gcoMATH_MultiplyFixed(x1, x2)
#define gcmXDivide(x1, x2)              gcoMATH_DivideFixed(x1, x2)
#define gcmXMultiplyDivide(x1, x2, x3)  gcoMATH_MultiplyDivideFixed(x1, x2, x3)

/* 2D Engine profile. */
typedef struct _gcs2D_PROFILE
{
    /* Cycle count.
       32bit counter incremented every 2D clock cycle.
       Wraps back to 0 when the counter overflows.
    */
    gctUINT32 cycleCount;

    /* Pixels rendered by the 2D engine.
       Resets to 0 every time it is read. */
    gctUINT32 pixelsRendered;
}
gcs2D_PROFILE;

/* Macro to combine four characters into a Charcater Code. */
#define gcmCC(c1, c2, c3, c4) \
( \
    (char) (c1) \
    | \
    ((char) (c2) <<  8) \
    | \
    ((char) (c3) << 16) \
    | \
    ((char) (c4) << 24) \
)

#define gcmPRINTABLE(c)         ((((c) >= ' ') && ((c) <= '}')) ? ((c) != '%' ?  (c) : ' ') : ' ')

#define gcmCC_PRINT(cc) \
    gcmPRINTABLE((char) ( (cc)        & 0xFF)), \
    gcmPRINTABLE((char) (((cc) >>  8) & 0xFF)), \
    gcmPRINTABLE((char) (((cc) >> 16) & 0xFF)), \
    gcmPRINTABLE((char) (((cc) >> 24) & 0xFF))

/******************************************************************************\
****************************** Function Parameters *****************************
\******************************************************************************/

#define IN
#define OUT
#define OPTIONAL

/******************************************************************************\
********************************* Status Codes *********************************
\******************************************************************************/

typedef enum _gceSTATUS
{
    gcvSTATUS_OK                    =   0,
    gcvSTATUS_FALSE                 =   0,
    gcvSTATUS_TRUE                  =   1,
    gcvSTATUS_NO_MORE_DATA          =   2,
    gcvSTATUS_CACHED                =   3,
    gcvSTATUS_MIPMAP_TOO_LARGE      =   4,
    gcvSTATUS_NAME_NOT_FOUND        =   5,
    gcvSTATUS_NOT_OUR_INTERRUPT     =   6,
    gcvSTATUS_MISMATCH              =   7,
    gcvSTATUS_MIPMAP_TOO_SMALL      =   8,
    gcvSTATUS_LARGER                =   9,
    gcvSTATUS_SMALLER               =   10,
    gcvSTATUS_CHIP_NOT_READY        =   11,
    gcvSTATUS_NEED_CONVERSION       =   12,
    gcvSTATUS_SKIP                  =   13,
    gcvSTATUS_DATA_TOO_LARGE        =   14,
    gcvSTATUS_INVALID_CONFIG        =   15,
    gcvSTATUS_CHANGED               =   16,
    gcvSTATUS_NOT_SUPPORT_DITHER    =   17,
	gcvSTATUS_EXECUTED				=	18,
    gcvSTATUS_TERMINATE             =   19,

    gcvSTATUS_CONVERT_TO_SINGLE_STREAM    =   20,

    gcvSTATUS_INVALID_ARGUMENT      =   -1,
    gcvSTATUS_INVALID_OBJECT        =   -2,
    gcvSTATUS_OUT_OF_MEMORY         =   -3,
    gcvSTATUS_MEMORY_LOCKED         =   -4,
    gcvSTATUS_MEMORY_UNLOCKED       =   -5,
    gcvSTATUS_HEAP_CORRUPTED        =   -6,
    gcvSTATUS_GENERIC_IO            =   -7,
    gcvSTATUS_INVALID_ADDRESS       =   -8,
    gcvSTATUS_CONTEXT_LOSSED        =   -9,
    gcvSTATUS_TOO_COMPLEX           =   -10,
    gcvSTATUS_BUFFER_TOO_SMALL      =   -11,
    gcvSTATUS_INTERFACE_ERROR       =   -12,
    gcvSTATUS_NOT_SUPPORTED         =   -13,
    gcvSTATUS_MORE_DATA             =   -14,
    gcvSTATUS_TIMEOUT               =   -15,
    gcvSTATUS_OUT_OF_RESOURCES      =   -16,
    gcvSTATUS_INVALID_DATA          =   -17,
    gcvSTATUS_INVALID_MIPMAP        =   -18,
    gcvSTATUS_NOT_FOUND             =   -19,
    gcvSTATUS_NOT_ALIGNED           =   -20,
    gcvSTATUS_INVALID_REQUEST       =   -21,
    gcvSTATUS_GPU_NOT_RESPONDING    =   -22,
    gcvSTATUS_TIMER_OVERFLOW        =   -23,
    gcvSTATUS_VERSION_MISMATCH      =   -24,
    gcvSTATUS_LOCKED                =   -25,
    gcvSTATUS_INTERRUPTED           =   -26,
    gcvSTATUS_DEVICE                =   -27,
    gcvSTATUS_NOT_MULTI_PIPE_ALIGNED =   -28,

    /* Linker errors. */
    gcvSTATUS_GLOBAL_TYPE_MISMATCH  =   -1000,
    gcvSTATUS_TOO_MANY_ATTRIBUTES   =   -1001,
    gcvSTATUS_TOO_MANY_UNIFORMS     =   -1002,
    gcvSTATUS_TOO_MANY_VARYINGS     =   -1003,
    gcvSTATUS_UNDECLARED_VARYING    =   -1004,
    gcvSTATUS_VARYING_TYPE_MISMATCH =   -1005,
    gcvSTATUS_MISSING_MAIN          =   -1006,
    gcvSTATUS_NAME_MISMATCH         =   -1007,
    gcvSTATUS_INVALID_INDEX         =   -1008,
    gcvSTATUS_UNIFORM_TYPE_MISMATCH =   -1009,

    /* Compiler errors. */
    gcvSTATUS_COMPILER_FE_PREPROCESSOR_ERROR = -2000,
    gcvSTATUS_COMPILER_FE_PARSER_ERROR = -2001,
}
gceSTATUS;

/******************************************************************************\
********************************* Status Macros ********************************
\******************************************************************************/

#define gcmIS_ERROR(status)         (status < 0)
#define gcmNO_ERROR(status)         (status >= 0)
#define gcmIS_SUCCESS(status)       (status == gcvSTATUS_OK)

/******************************************************************************\
********************************* Field Macros *********************************
\******************************************************************************/

#define __gcmSTART(reg_field) \
    (0 ? reg_field)

#define __gcmEND(reg_field) \
    (1 ? reg_field)

#define __gcmGETSIZE(reg_field) \
    (__gcmEND(reg_field) - __gcmSTART(reg_field) + 1)

#define __gcmALIGN(data, reg_field) \
    (((gctUINT32) (data)) << __gcmSTART(reg_field))

#define __gcmMASK(reg_field) \
    ((gctUINT32) ((__gcmGETSIZE(reg_field) == 32) \
        ?  ~0 \
        : (~(~0 << __gcmGETSIZE(reg_field)))))

/*******************************************************************************
**
**  gcmFIELDMASK
**
**      Get aligned field mask.
**
**  ARGUMENTS:
**
**      reg     Name of register.
**      field   Name of field within register.
*/
#define gcmFIELDMASK(reg, field) \
( \
    __gcmALIGN(__gcmMASK(reg##_##field), reg##_##field) \
)

/*******************************************************************************
**
**  gcmGETFIELD
**
**      Extract the value of a field from specified data.
**
**  ARGUMENTS:
**
**      data    Data value.
**      reg     Name of register.
**      field   Name of field within register.
*/
#define gcmGETFIELD(data, reg, field) \
( \
    ((((gctUINT32) (data)) >> __gcmSTART(reg##_##field)) \
        & __gcmMASK(reg##_##field)) \
)

/*******************************************************************************
**
**  gcmSETFIELD
**
**      Set the value of a field within specified data.
**
**  ARGUMENTS:
**
**      data    Data value.
**      reg     Name of register.
**      field   Name of field within register.
**      value   Value for field.
*/
#define gcmSETFIELD(data, reg, field, value) \
( \
    (((gctUINT32) (data)) \
        & ~__gcmALIGN(__gcmMASK(reg##_##field), reg##_##field)) \
        |  __gcmALIGN((gctUINT32) (value) \
            & __gcmMASK(reg##_##field), reg##_##field) \
)

/*******************************************************************************
**
**  gcmSETFIELDVALUE
**
**      Set the value of a field within specified data with a
**      predefined value.
**
**  ARGUMENTS:
**
**      data    Data value.
**      reg     Name of register.
**      field   Name of field within register.
**      value   Name of the value within the field.
*/
#define gcmSETFIELDVALUE(data, reg, field, value) \
( \
    (((gctUINT32) (data)) \
        & ~__gcmALIGN(__gcmMASK(reg##_##field), reg##_##field)) \
        |  __gcmALIGN(reg##_##field##_##value \
            & __gcmMASK(reg##_##field), reg##_##field) \
)

/*******************************************************************************
**
**  gcmGETMASKEDFIELDMASK
**
**      Determine field mask of a masked field.
**
**  ARGUMENTS:
**
**      reg     Name of register.
**      field   Name of field within register.
*/
#define gcmGETMASKEDFIELDMASK(reg, field) \
( \
    gcmSETFIELD(0, reg,          field, ~0) | \
    gcmSETFIELD(0, reg, MASK_ ## field, ~0)   \
)

/*******************************************************************************
**
**  gcmSETMASKEDFIELD
**
**      Set the value of a masked field with specified data.
**
**  ARGUMENTS:
**
**      reg     Name of register.
**      field   Name of field within register.
**      value   Value for field.
*/
#define gcmSETMASKEDFIELD(reg, field, value) \
( \
    gcmSETFIELD     (~0, reg,          field, value) & \
    gcmSETFIELDVALUE(~0, reg, MASK_ ## field, ENABLED) \
)

/*******************************************************************************
**
**  gcmSETMASKEDFIELDVALUE
**
**      Set the value of a masked field with specified data.
**
**  ARGUMENTS:
**
**      reg     Name of register.
**      field   Name of field within register.
**      value   Value for field.
*/
#define gcmSETMASKEDFIELDVALUE(reg, field, value) \
( \
    gcmSETFIELDVALUE(~0, reg,          field, value) & \
    gcmSETFIELDVALUE(~0, reg, MASK_ ## field, ENABLED) \
)

/*******************************************************************************
**
**  gcmVERIFYFIELDVALUE
**
**      Verify if the value of a field within specified data equals a
**      predefined value.
**
**  ARGUMENTS:
**
**      data    Data value.
**      reg     Name of register.
**      field   Name of field within register.
**      value   Name of the value within the field.
*/
#define gcmVERIFYFIELDVALUE(data, reg, field, value) \
( \
    (((gctUINT32) (data)) >> __gcmSTART(reg##_##field) & \
                             __gcmMASK(reg##_##field)) \
        == \
    (reg##_##field##_##value & __gcmMASK(reg##_##field)) \
)

/*******************************************************************************
**  Bit field macros.
*/

#define __gcmSTARTBIT(Field) \
    ( 1 ? Field )

#define __gcmBITSIZE(Field) \
    ( 0 ? Field )

#define __gcmBITMASK(Field) \
( \
    (1 << __gcmBITSIZE(Field)) - 1 \
)

#define gcmGETBITS(Value, Type, Field) \
( \
    ( ((Type) (Value)) >> __gcmSTARTBIT(Field) ) \
    & \
    __gcmBITMASK(Field) \
)

#define gcmSETBITS(Value, Type, Field, NewValue) \
( \
    ( ((Type) (Value)) \
    & ~(__gcmBITMASK(Field) << __gcmSTARTBIT(Field)) \
    ) \
    | \
    ( ( ((Type) (NewValue)) \
      & __gcmBITMASK(Field) \
      ) << __gcmSTARTBIT(Field) \
    ) \
)

/*******************************************************************************
**
**  gcmISINREGRANGE
**
**      Verify whether the specified address is in the register range.
**
**  ARGUMENTS:
**
**      Address Address to be verified.
**      Name    Name of a register.
*/

#define gcmISINREGRANGE(Address, Name) \
( \
    ((Address & (~0U << Name ## _LSB)) == (Name ## _Address >> 2)) \
)

/*******************************************************************************
**
**  A set of macros to aid state loading.
**
**  ARGUMENTS:
**
**      CommandBuffer   Pointer to a gcoCMDBUF object.
**      StateDelta      Pointer to a gcsSTATE_DELTA state delta structure.
**      Memory          Destination memory pointer of gctUINT32_PTR type.
**      PartOfContext   Whether or not the state is a part of the context.
**      FixedPoint      Whether or not the state is of the fixed point format.
**      Count           Number of consecutive states to be loaded.
**      Address         State address.
**      Data            Data to be set to the state.
*/

/*----------------------------------------------------------------------------*/

#if gcmIS_DEBUG(gcdDEBUG_CODE)

#   define gcmSTORELOADSTATE(CommandBuffer, Memory, Address, Count) \
        CommandBuffer->lastLoadStatePtr     = gcmPTR_TO_UINT64(Memory); \
        CommandBuffer->lastLoadStateAddress = Address; \
        CommandBuffer->lastLoadStateCount   = Count

#   define gcmVERIFYLOADSTATE(CommandBuffer, Memory, Address) \
        gcmASSERT( \
            (gctUINT) (Memory  - gcmUINT64_TO_TYPE(CommandBuffer->lastLoadStatePtr, gctUINT32_PTR) - 1) \
            == \
            (gctUINT) (Address - CommandBuffer->lastLoadStateAddress) \
            ); \
        \
        gcmASSERT(CommandBuffer->lastLoadStateCount > 0); \
        \
        CommandBuffer->lastLoadStateCount -= 1

#   define gcmVERIFYLOADSTATEDONE(CommandBuffer) \
        gcmASSERT(CommandBuffer->lastLoadStateCount == 0)

#else

#   define gcmSTORELOADSTATE(CommandBuffer, Memory, Address, Count)
#   define gcmVERIFYLOADSTATE(CommandBuffer, Memory, Address)
#   define gcmVERIFYLOADSTATEDONE(CommandBuffer)

#endif

#if gcdSECURE_USER

#   define gcmDEFINESECUREUSER() \
        gctUINT         __secure_user_offset__; \
        gctUINT32_PTR   __secure_user_hintArray__;

#   define gcmBEGINSECUREUSER() \
        __secure_user_offset__ = reserve->lastOffset; \
        \
        __secure_user_hintArray__ = gcmUINT64_TO_PTR(reserve->hintArrayTail)

#   define gcmENDSECUREUSER() \
        reserve->hintArrayTail = gcmPTR_TO_UINT64(__secure_user_hintArray__)

#   define gcmSKIPSECUREUSER() \
        __secure_user_offset__ += gcmSIZEOF(gctUINT32)

#   define gcmUPDATESECUREUSER() \
        *__secure_user_hintArray__ = __secure_user_offset__; \
        \
        __secure_user_offset__    += gcmSIZEOF(gctUINT32); \
        __secure_user_hintArray__ += 1

#else

#   define gcmDEFINESECUREUSER()
#   define gcmBEGINSECUREUSER()
#   define gcmENDSECUREUSER()
#   define gcmSKIPSECUREUSER()
#   define gcmUPDATESECUREUSER()

#endif

/*----------------------------------------------------------------------------*/

#if gcdDUMP
#   define gcmDUMPSTATEDATA(StateDelta, FixedPoint, Address, Data) \
        if (FixedPoint) \
        { \
            gcmDUMP(gcvNULL, "@[state.x 0x%04X 0x%08X]", \
                Address, Data \
                ); \
        } \
        else \
        { \
            gcmDUMP(gcvNULL, "@[state 0x%04X 0x%08X]", \
                Address, Data \
                ); \
        }
#else
#   define gcmDUMPSTATEDATA(StateDelta, FixedPoint, Address, Data)
#endif

/*----------------------------------------------------------------------------*/

#define gcmDEFINESTATEBUFFER(CommandBuffer, StateDelta, Memory, ReserveSize) \
    gcmDEFINESECUREUSER() \
    gctSIZE_T ReserveSize; \
    gcoCMDBUF CommandBuffer; \
    gctUINT32_PTR Memory; \
    gcsSTATE_DELTA_PTR StateDelta

#define gcmBEGINSTATEBUFFER(Hardware, CommandBuffer, StateDelta, Memory, ReserveSize) \
{ \
    gcmONERROR(gcoBUFFER_Reserve( \
        Hardware->buffer, ReserveSize, gcvTRUE, &CommandBuffer \
        )); \
    \
    Memory =  gcmUINT64_TO_PTR(CommandBuffer->lastReserve); \
    \
    StateDelta = Hardware->delta; \
    \
    gcmBEGINSECUREUSER(); \
}

#define gcmENDSTATEBUFFER(CommandBuffer, Memory, ReserveSize) \
{ \
    gcmENDSECUREUSER(); \
    \
    gcmASSERT( \
        gcmUINT64_TO_TYPE(CommandBuffer->lastReserve, gctUINT8_PTR) + ReserveSize \
        == \
         (gctUINT8_PTR) Memory \
        ); \
}

/*----------------------------------------------------------------------------*/

#define gcmBEGINSTATEBATCH(CommandBuffer, Memory, FixedPoint, Address, Count) \
{ \
    gcmASSERT(((Memory - gcmUINT64_TO_TYPE(CommandBuffer->lastReserve, gctUINT32_PTR)) & 1) == 0); \
    gcmASSERT((gctUINT32)Count <= 1024); \
    \
    gcmVERIFYLOADSTATEDONE(CommandBuffer); \
    \
    gcmSTORELOADSTATE(CommandBuffer, Memory, Address, Count); \
    \
    *Memory++ \
        = gcmSETFIELDVALUE(0, AQ_COMMAND_LOAD_STATE_COMMAND, OPCODE,  LOAD_STATE) \
        | gcmSETFIELD     (0, AQ_COMMAND_LOAD_STATE_COMMAND, FLOAT,   FixedPoint) \
        | gcmSETFIELD     (0, AQ_COMMAND_LOAD_STATE_COMMAND, COUNT,   Count) \
        | gcmSETFIELD     (0, AQ_COMMAND_LOAD_STATE_COMMAND, ADDRESS, Address); \
    \
    gcmSKIPSECUREUSER(); \
}

#define gcmENDSTATEBATCH(CommandBuffer, Memory) \
{ \
    gcmVERIFYLOADSTATEDONE(CommandBuffer); \
    \
    gcmASSERT(((Memory - gcmUINT64_TO_TYPE(CommandBuffer->lastReserve, gctUINT32_PTR)) & 1) == 0); \
}

/*----------------------------------------------------------------------------*/

#define gcmSETSTATEDATA(StateDelta, CommandBuffer, Memory, FixedPoint, \
                        Address, Data) \
{ \
    gctUINT32 __temp_data32__; \
    \
    gcmVERIFYLOADSTATE(CommandBuffer, Memory, Address); \
    \
    __temp_data32__ = Data; \
    \
    *Memory++ = __temp_data32__; \
    \
    gcoHARDWARE_UpdateDelta( \
        StateDelta, FixedPoint, Address, 0, __temp_data32__ \
        ); \
    \
    gcmDUMPSTATEDATA(StateDelta, FixedPoint, Address, __temp_data32__); \
    \
    gcmUPDATESECUREUSER(); \
}

#define gcmSETCTRLSTATE(StateDelta, CommandBuffer, Memory, Address, Data) \
{ \
    gctUINT32 __temp_data32__; \
    \
    gcmVERIFYLOADSTATE(CommandBuffer, Memory, Address); \
    \
    __temp_data32__ = Data; \
    \
    *Memory++ = __temp_data32__; \
    \
    gcmDUMPSTATEDATA(StateDelta, gcvFALSE, Address, __temp_data32__); \
    \
    gcmSKIPSECUREUSER(); \
}

#define gcmSETFILLER(CommandBuffer, Memory) \
{ \
    gcmVERIFYLOADSTATEDONE(CommandBuffer); \
    \
    Memory += 1; \
    \
    gcmSKIPSECUREUSER(); \
}

/*----------------------------------------------------------------------------*/

#define gcmSETSINGLESTATE(StateDelta, CommandBuffer, Memory, FixedPoint, \
                          Address, Data) \
{ \
    gcmBEGINSTATEBATCH(CommandBuffer, Memory, FixedPoint, Address, 1); \
    gcmSETSTATEDATA(StateDelta, CommandBuffer, Memory, FixedPoint, \
                    Address, Data); \
    gcmENDSTATEBATCH(CommandBuffer, Memory); \
}

#define gcmSETSINGLECTRLSTATE(StateDelta, CommandBuffer, Memory, FixedPoint, \
                              Address, Data) \
{ \
    gcmBEGINSTATEBATCH(CommandBuffer, Memory, FixedPoint, Address, 1); \
    gcmSETCTRLSTATE(StateDelta, CommandBuffer, Memory, Address, Data); \
    gcmENDSTATEBATCH(CommandBuffer, Memory); \
}


/*******************************************************************************
**
**  gcmSETSTARTDECOMMAND
**
**      Form a START_DE command.
**
**  ARGUMENTS:
**
**      Memory          Destination memory pointer of gctUINT32_PTR type.
**      Count           Number of the rectangles.
*/

#define gcmSETSTARTDECOMMAND(Memory, Count) \
{ \
    *Memory++ \
        = gcmSETFIELDVALUE(0, AQ_COMMAND_START_DE_COMMAND, OPCODE,     START_DE) \
        | gcmSETFIELD     (0, AQ_COMMAND_START_DE_COMMAND, COUNT,      Count) \
        | gcmSETFIELD     (0, AQ_COMMAND_START_DE_COMMAND, DATA_COUNT, 0); \
    \
    *Memory++ = 0xDEADDEED; \
}

/******************************************************************************\
******************************** Ceiling Macro ********************************
\******************************************************************************/
#define gcmCEIL(x) ((x - (gctUINT32)x) == 0 ? (gctUINT32)x : (gctUINT32)x + 1)

/******************************************************************************\
******************************** Min/Max Macros ********************************
\******************************************************************************/

#define gcmMIN(x, y)            (((x) <= (y)) ?  (x) :  (y))
#define gcmMAX(x, y)            (((x) >= (y)) ?  (x) :  (y))
#define gcmCLAMP(x, min, max)   (((x) < (min)) ? (min) : \
                                 ((x) > (max)) ? (max) : (x))
#define gcmABS(x)               (((x) < 0)    ? -(x) :  (x))
#define gcmNEG(x)               (((x) < 0)    ?  (x) : -(x))

/*******************************************************************************
**
**  gcmPTR2INT
**
**      Convert a pointer to an integer value.
**
**  ARGUMENTS:
**
**      p       Pointer value.
*/
#if defined(_WIN32) || (defined(__LP64__) && __LP64__)
#   define gcmPTR2INT(p) \
    ( \
        (gctUINT32) (gctUINT64) (p) \
    )
#else
#   define gcmPTR2INT(p) \
    ( \
        (gctUINT32) (p) \
    )
#endif

/*******************************************************************************
**
**  gcmINT2PTR
**
**      Convert an integer value into a pointer.
**
**  ARGUMENTS:
**
**      v       Integer value.
*/
#ifdef __LP64__
#   define gcmINT2PTR(i) \
    ( \
        (gctPOINTER) (gctINT64) (i) \
    )
#else
#   define gcmINT2PTR(i) \
    ( \
        (gctPOINTER) (i) \
    )
#endif

/*******************************************************************************
**
**  gcmOFFSETOF
**
**      Compute the byte offset of a field inside a structure.
**
**  ARGUMENTS:
**
**      s       Structure name.
**      field   Field name.
*/
#define gcmOFFSETOF(s, field) \
( \
    gcmPTR2INT(& (((struct s *) 0)->field)) \
)

#define gcmSWAB32(x) ((gctUINT32)( \
        (((gctUINT32)(x) & (gctUINT32)0x000000FFUL) << 24) | \
        (((gctUINT32)(x) & (gctUINT32)0x0000FF00UL) << 8)  | \
        (((gctUINT32)(x) & (gctUINT32)0x00FF0000UL) >> 8)  | \
        (((gctUINT32)(x) & (gctUINT32)0xFF000000UL) >> 24)))

/*******************************************************************************
***** Database ****************************************************************/

typedef struct _gcsDATABASE_COUNTERS
{
    /* Number of currently allocated bytes. */
    gctUINT64                   bytes;

    /* Maximum number of bytes allocated (memory footprint). */
    gctUINT64                   maxBytes;

    /* Total number of bytes allocated. */
    gctUINT64                   totalBytes;
}
gcsDATABASE_COUNTERS;

typedef struct _gcuDATABASE_INFO
{
    /* Counters. */
    gcsDATABASE_COUNTERS        counters;

    /* Time value. */
    gctUINT64                   time;
}
gcuDATABASE_INFO;

/*******************************************************************************
***** Frame database **********************************************************/

/* gcsHAL_FRAME_INFO */
typedef struct _gcsHAL_FRAME_INFO
{
    /* Current timer tick. */
    OUT gctUINT64               ticks;

    /* Bandwidth counters. */
    OUT gctUINT                 readBytes8[8];
    OUT gctUINT                 writeBytes8[8];

    /* Counters. */
    OUT gctUINT                 cycles[8];
    OUT gctUINT                 idleCycles[8];
    OUT gctUINT                 mcCycles[8];
    OUT gctUINT                 readRequests[8];
    OUT gctUINT                 writeRequests[8];

    /* FE counters. */
    OUT gctUINT                 drawCount;
    OUT gctUINT                 vertexOutCount;
    OUT gctUINT                 vertexMissCount;

    /* 3D counters. */
    OUT gctUINT                 vertexCount;
    OUT gctUINT                 primitiveCount;
    OUT gctUINT                 rejectedPrimitives;
    OUT gctUINT                 culledPrimitives;
    OUT gctUINT                 clippedPrimitives;
    OUT gctUINT                 droppedPrimitives;
    OUT gctUINT                 frustumClippedPrimitives;
    OUT gctUINT                 outPrimitives;
    OUT gctUINT                 inPrimitives;
    OUT gctUINT                 culledQuadCount;
    OUT gctUINT                 totalQuadCount;
    OUT gctUINT                 quadCount;
    OUT gctUINT                 totalPixelCount;

    /* PE counters. */
    OUT gctUINT                 colorKilled[8];
    OUT gctUINT                 colorDrawn[8];
    OUT gctUINT                 depthKilled[8];
    OUT gctUINT                 depthDrawn[8];

    /* Shader counters. */
    OUT gctUINT                 shaderCycles;
    OUT gctUINT                 vsInstructionCount;
    OUT gctUINT                 vsTextureCount;
    OUT gctUINT                 vsBranchCount;
    OUT gctUINT                 vsVertices;
    OUT gctUINT                 psInstructionCount;
    OUT gctUINT                 psTextureCount;
    OUT gctUINT                 psBranchCount;
    OUT gctUINT                 psPixels;

    /* Texture counters. */
    OUT gctUINT                 bilinearRequests;
    OUT gctUINT                 trilinearRequests;
    OUT gctUINT                 txBytes8[2];
    OUT gctUINT                 txHitCount;
    OUT gctUINT                 txMissCount;
}
gcsHAL_FRAME_INFO;

typedef enum _gcePATCH_ID
{
    gcePATCH_UNKNOWN = 0xFFFFFFFF,

    /* Benchmark list*/
    gcePATCH_GLB11 = 0x0,
    gcePATCH_GLB21,
    gcePATCH_GLB25,
    gcePATCH_GLB27,

    gcePATCH_BM21,
    gcePATCH_MM,
    gcePATCH_MM06,
    gcePATCH_MM07,
    gcePATCH_QUADRANT,
    gcePATCH_ANTUTU,
    gcePATCH_SMARTBENCH,
    gcePATCH_JPCT,
    gcePATCH_NENAMARK,
    gcePATCH_NENAMARK2,
    gcePATCH_NEOCORE,
    gcePATCH_GLB,
    gcePATCH_GB,
    gcePATCH_RTESTVA,
    gcePATCH_BMX,
    gcePATCH_BMGUI,

    /* Game list */
    gcePATCH_NBA2013,
    gcePATCH_BARDTALE,
    gcePATCH_BUSPARKING3D,
    gcePATCH_FISHBOODLE,
    gcePATCH_SUBWAYSURFER,
    gcePATCH_HIGHWAYDRIVER,
    gcePATCH_PREMIUM,
    gcePATCH_RACEILLEGAL,
    gcePATCH_BLABLA,
    gcePATCH_MEGARUN,
    gcePATCH_GALAXYONFIRE2,
    gcePATCH_GLOFTR3HM,
    gcePATCH_GLOFTSXHM,
    gcePATCH_GLOFTF3HM,
    gcePATCH_GLOFTGANG,
    gcePATCH_XRUNNER,
    gcePATCH_WP,
    gcePATCH_DEVIL,
    gcePATCH_HOLYARCH,
    gcePATCH_MUSE,
    gcePATCH_SG,
    gcePATCH_SIEGECRAFT,
    gcePATCH_CARCHALLENGE,
    gcePATCH_HEROESCALL,
    gcePATCH_MONOPOLY,
    gcePATCH_CTGL20,
    gcePATCH_FIREFOX,
    gcePATCH_CHORME,
    gcePATCH_DUOKANTV,
    gcePATCH_TESTAPP,

    /* Count enum*/
    gcePATCH_COUNT,
}
gcePATCH_ID;

#if gcdLINK_QUEUE_SIZE
typedef struct _gckLINKDATA * gckLINKDATA;
struct _gckLINKDATA
{
    gctUINT32                   start;
    gctUINT32                   end;
    gctINT                      pid;
};

typedef struct _gckLINKQUEUE * gckLINKQUEUE;
struct _gckLINKQUEUE
{
    struct _gckLINKDATA         data[gcdLINK_QUEUE_SIZE];
    gctUINT32                   rear;
    gctUINT32                   front;
    gctUINT32                   count;
};
#endif

#ifdef __cplusplus
}
#endif

#endif /* __gc_hal_types_h_ */
