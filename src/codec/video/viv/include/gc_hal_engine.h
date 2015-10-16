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


#ifndef __gc_hal_engine_h_
#define __gc_hal_engine_h_

#ifndef VIVANTE_NO_3D
#include "gc_hal_types.h"
#include "gc_hal_enum.h"

#if gcdENABLE_VG
#include "gc_hal_engine_vg.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************\
****************************** Object Declarations *****************************
\******************************************************************************/

typedef struct _gcoSTREAM *             gcoSTREAM;
typedef struct _gcoVERTEX *             gcoVERTEX;
typedef struct _gcoTEXTURE *            gcoTEXTURE;
typedef struct _gcoINDEX *              gcoINDEX;
typedef struct _gcsVERTEX_ATTRIBUTES *  gcsVERTEX_ATTRIBUTES_PTR;
typedef struct _gcoVERTEXARRAY *        gcoVERTEXARRAY;

#define gcdATTRIBUTE_COUNT              16

/******************************************************************************\
********************************* Enumerations *********************************
\******************************************************************************/

/* Shading format. */
typedef enum _gceSHADING
{
    gcvSHADING_SMOOTH,
    gcvSHADING_FLAT_D3D,
    gcvSHADING_FLAT_OPENGL,
}
gceSHADING;

/* Culling modes. */
typedef enum _gceCULL
{
    gcvCULL_NONE,
    gcvCULL_CCW,
    gcvCULL_CW,
}
gceCULL;

/* Fill modes. */
typedef enum _gceFILL
{
    gcvFILL_POINT,
    gcvFILL_WIRE_FRAME,
    gcvFILL_SOLID,
}
gceFILL;

/* Compare modes. */
typedef enum _gceCOMPARE
{
    gcvCOMPARE_NEVER,
    gcvCOMPARE_NOT_EQUAL,
    gcvCOMPARE_LESS,
    gcvCOMPARE_LESS_OR_EQUAL,
    gcvCOMPARE_EQUAL,
    gcvCOMPARE_GREATER,
    gcvCOMPARE_GREATER_OR_EQUAL,
    gcvCOMPARE_ALWAYS,
    gcvCOMPARE_INVALID = -1
}
gceCOMPARE;

/* Stencil modes. */
typedef enum _gceSTENCIL_MODE
{
    gcvSTENCIL_NONE,
    gcvSTENCIL_SINGLE_SIDED,
    gcvSTENCIL_DOUBLE_SIDED,
}
gceSTENCIL_MODE;

/* Stencil operations. */
typedef enum _gceSTENCIL_OPERATION
{
    gcvSTENCIL_KEEP,
    gcvSTENCIL_REPLACE,
    gcvSTENCIL_ZERO,
    gcvSTENCIL_INVERT,
    gcvSTENCIL_INCREMENT,
    gcvSTENCIL_DECREMENT,
    gcvSTENCIL_INCREMENT_SATURATE,
    gcvSTENCIL_DECREMENT_SATURATE,
    gcvSTENCIL_OPERATION_INVALID = -1
}
gceSTENCIL_OPERATION;

/* Stencil selection. */
typedef enum _gceSTENCIL_WHERE
{
    gcvSTENCIL_FRONT,
    gcvSTENCIL_BACK,
}
gceSTENCIL_WHERE;

/* Texture addressing selection. */
typedef enum _gceTEXTURE_WHICH
{
    gcvTEXTURE_S,
    gcvTEXTURE_T,
    gcvTEXTURE_R,
}
gceTEXTURE_WHICH;

/* Texture addressing modes. */
typedef enum _gceTEXTURE_ADDRESSING
{
    gcvTEXTURE_WRAP,
    gcvTEXTURE_CLAMP,
    gcvTEXTURE_BORDER,
    gcvTEXTURE_MIRROR,
    gcvTEXTURE_MIRROR_ONCE,
}
gceTEXTURE_ADDRESSING;

/* Texture filters. */
typedef enum _gceTEXTURE_FILTER
{
    gcvTEXTURE_NONE,
    gcvTEXTURE_POINT,
    gcvTEXTURE_LINEAR,
    gcvTEXTURE_ANISOTROPIC,
}
gceTEXTURE_FILTER;

/* Primitive types. */
typedef enum _gcePRIMITIVE
{
    gcvPRIMITIVE_POINT_LIST,
    gcvPRIMITIVE_LINE_LIST,
    gcvPRIMITIVE_LINE_STRIP,
    gcvPRIMITIVE_LINE_LOOP,
    gcvPRIMITIVE_TRIANGLE_LIST,
    gcvPRIMITIVE_TRIANGLE_STRIP,
    gcvPRIMITIVE_TRIANGLE_FAN,
    gcvPRIMITIVE_RECTANGLE,
}
gcePRIMITIVE;

/* Index types. */
typedef enum _gceINDEX_TYPE
{
    gcvINDEX_8,
    gcvINDEX_16,
    gcvINDEX_32,
}
gceINDEX_TYPE;

/******************************************************************************\
********************************* gcoHAL Object *********************************
\******************************************************************************/

/* Query the target capabilities. */
gceSTATUS
gcoHAL_QueryTargetCaps(
    IN gcoHAL Hal,
    OUT gctUINT * MaxWidth,
    OUT gctUINT * MaxHeight,
    OUT gctUINT * MultiTargetCount,
    OUT gctUINT * MaxSamples
    );

gceSTATUS
gcoHAL_SetDepthOnly(
    IN gcoHAL Hal,
    IN gctBOOL Enable
    );

gceSTATUS
gcoHAL_QueryShaderCaps(
    IN gcoHAL Hal,
    OUT gctUINT * VertexUniforms,
    OUT gctUINT * FragmentUniforms,
    OUT gctUINT * Varyings
    );

gceSTATUS
gcoHAL_QueryTextureCaps(
    IN gcoHAL Hal,
    OUT gctUINT * MaxWidth,
    OUT gctUINT * MaxHeight,
    OUT gctUINT * MaxDepth,
    OUT gctBOOL * Cubic,
    OUT gctBOOL * NonPowerOfTwo,
    OUT gctUINT * VertexSamplers,
    OUT gctUINT * PixelSamplers
    );

gceSTATUS
gcoHAL_QueryTextureMaxAniso(
    IN gcoHAL Hal,
    OUT gctUINT * MaxAnisoValue
    );

gceSTATUS
gcoHAL_QueryStreamCaps(
    IN gcoHAL Hal,
    OUT gctUINT32 * MaxAttributes,
    OUT gctUINT32 * MaxStreamSize,
    OUT gctUINT32 * NumberOfStreams,
    OUT gctUINT32 * Alignment
    );

/******************************************************************************\
********************************* gcoSURF Object ********************************
\******************************************************************************/

/*----------------------------------------------------------------------------*/
/*--------------------------------- gcoSURF 3D --------------------------------*/

/* Copy surface. */
gceSTATUS
gcoSURF_Copy(
    IN gcoSURF Surface,
    IN gcoSURF Source
    );

/* Clear surface. */
gceSTATUS
gcoSURF_Clear(
    IN gcoSURF Surface,
    IN gctUINT Flags
    );

/* Set number of samples for a gcoSURF object. */
gceSTATUS
gcoSURF_SetSamples(
    IN gcoSURF Surface,
    IN gctUINT Samples
    );

/* Get the number of samples per pixel. */
gceSTATUS
gcoSURF_GetSamples(
    IN gcoSURF Surface,
    OUT gctUINT_PTR Samples
    );

/* Clear rectangular surface. */
gceSTATUS
gcoSURF_ClearRect(
    IN gcoSURF Surface,
    IN gctINT Left,
    IN gctINT Top,
    IN gctINT Right,
    IN gctINT Bottom,
    IN gctUINT Flags
    );

/* TO BE REMOVED */
    gceSTATUS
    depr_gcoSURF_Resolve(
        IN gcoSURF SrcSurface,
        IN gcoSURF DestSurface,
        IN gctUINT32 DestAddress,
        IN gctPOINTER DestBits,
        IN gctINT DestStride,
        IN gceSURF_TYPE DestType,
        IN gceSURF_FORMAT DestFormat,
        IN gctUINT DestWidth,
        IN gctUINT DestHeight
        );

    gceSTATUS
    depr_gcoSURF_ResolveRect(
        IN gcoSURF SrcSurface,
        IN gcoSURF DestSurface,
        IN gctUINT32 DestAddress,
        IN gctPOINTER DestBits,
        IN gctINT DestStride,
        IN gceSURF_TYPE DestType,
        IN gceSURF_FORMAT DestFormat,
        IN gctUINT DestWidth,
        IN gctUINT DestHeight,
        IN gcsPOINT_PTR SrcOrigin,
        IN gcsPOINT_PTR DestOrigin,
        IN gcsPOINT_PTR RectSize
        );

/* Resample surface. */
gceSTATUS
gcoSURF_Resample(
    IN gcoSURF SrcSurface,
    IN gcoSURF DestSurface
    );

/* Resolve surface. */
gceSTATUS
gcoSURF_Resolve(
    IN gcoSURF SrcSurface,
    IN gcoSURF DestSurface
    );

gceSTATUS
gcoSURF_IsHWResolveable(
    IN gcoSURF SrcSurface,
    IN gcoSURF DestSurface,
    IN gcsPOINT_PTR SrcOrigin,
    IN gcsPOINT_PTR DestOrigin,
    IN gcsPOINT_PTR RectSize
    );

/* Resolve rectangular area of a surface. */
gceSTATUS
gcoSURF_ResolveRect(
    IN gcoSURF SrcSurface,
    IN gcoSURF DestSurface,
    IN gcsPOINT_PTR SrcOrigin,
    IN gcsPOINT_PTR DestOrigin,
    IN gcsPOINT_PTR RectSize
    );

/* Set surface resolvability. */
gceSTATUS
gcoSURF_SetResolvability(
    IN gcoSURF Surface,
    IN gctBOOL Resolvable
    );

gceSTATUS
gcoSURF_IsRenderable(
    IN gcoSURF Surface
    );

gceSTATUS
gcoSURF_IsFormatRenderableAsRT(
    IN gcoSURF Surface
    );

#if gcdSYNC
gceSTATUS
gcoSURF_GetFence(
    IN gcoSURF Surface
    );
gceSTATUS
gcoSURF_WaitFence(
    IN gcoSURF Surface
    );

gceSTATUS
gcoSTREAM_GetFence(
    IN gcoSTREAM stream
    );

gceSTATUS
gcoSTREAM_WaitFence(
    IN gcoSTREAM stream
    );

gceSTATUS
gcoINDEX_GetFence(
    IN gcoINDEX index
    );

gceSTATUS
gcoINDEX_WaitFence(
    IN gcoINDEX index
    );
#endif

/******************************************************************************\
******************************** gcoINDEX Object *******************************
\******************************************************************************/

/* Construct a new gcoINDEX object. */
gceSTATUS
gcoINDEX_Construct(
    IN gcoHAL Hal,
    OUT gcoINDEX * Index
    );

/* Destroy a gcoINDEX object. */
gceSTATUS
gcoINDEX_Destroy(
    IN gcoINDEX Index
    );

/* Lock index in memory. */
gceSTATUS
gcoINDEX_Lock(
    IN gcoINDEX Index,
    OUT gctUINT32 * Address,
    OUT gctPOINTER * Memory
    );

/* Unlock index that was previously locked with gcoINDEX_Lock. */
gceSTATUS
gcoINDEX_Unlock(
    IN gcoINDEX Index
    );

/* Upload index data into the memory. */
gceSTATUS
gcoINDEX_Load(
    IN gcoINDEX Index,
    IN gceINDEX_TYPE IndexType,
    IN gctUINT32 IndexCount,
    IN gctPOINTER IndexBuffer
    );

/* Bind an index object to the hardware. */
gceSTATUS
gcoINDEX_Bind(
    IN gcoINDEX Index,
    IN gceINDEX_TYPE Type
    );

/* Bind an index object to the hardware. */
gceSTATUS
gcoINDEX_BindOffset(
    IN gcoINDEX Index,
    IN gceINDEX_TYPE Type,
    IN gctUINT32 Offset
    );

/* Free existing index buffer. */
gceSTATUS
gcoINDEX_Free(
    IN gcoINDEX Index
    );

/* Upload data into an index buffer. */
gceSTATUS
gcoINDEX_Upload(
    IN gcoINDEX Index,
    IN gctCONST_POINTER Buffer,
    IN gctSIZE_T Bytes
    );

/* Upload data into an index buffer starting at an offset. */
gceSTATUS
gcoINDEX_UploadOffset(
    IN gcoINDEX Index,
    IN gctUINT32 Offset,
    IN gctCONST_POINTER Buffer,
    IN gctSIZE_T Bytes
    );

/*Merge index2 to index1 from 0, index2 must subset of inex1*/
gceSTATUS
gcoINDEX_Merge(
    IN gcoINDEX Index1,
    IN gcoINDEX Index2
    );

/*check if index buffer is enough for this draw*/
gctBOOL
gcoINDEX_CheckRange(
    IN gcoINDEX Index,
    IN gceINDEX_TYPE Type,
    IN gctINT Count,
    IN gctUINT32  Indices
    );

/* Query the index capabilities. */
gceSTATUS
gcoINDEX_QueryCaps(
    OUT gctBOOL * Index8,
    OUT gctBOOL * Index16,
    OUT gctBOOL * Index32,
    OUT gctUINT * MaxIndex
    );

/* Determine the index range in the current index buffer. */
gceSTATUS
gcoINDEX_GetIndexRange(
    IN gcoINDEX Index,
    IN gceINDEX_TYPE Type,
    IN gctUINT32 Offset,
    IN gctUINT32 Count,
    OUT gctUINT32 * MinimumIndex,
    OUT gctUINT32 * MaximumIndex
    );

/* Dynamic buffer management. */
gceSTATUS
gcoINDEX_SetDynamic(
    IN gcoINDEX Index,
    IN gctSIZE_T Bytes,
    IN gctUINT Buffers
    );

gceSTATUS
gcoINDEX_UploadDynamic(
    IN gcoINDEX Index,
    IN gctCONST_POINTER Data,
    IN gctSIZE_T Bytes
    );

/******************************************************************************\
********************************** gco3D Object *********************************
\******************************************************************************/

/* Clear flags. */
typedef enum _gceCLEAR
{
    gcvCLEAR_COLOR              = 0x1,
    gcvCLEAR_DEPTH              = 0x2,
    gcvCLEAR_STENCIL            = 0x4,
    gcvCLEAR_HZ                 = 0x8,
    gcvCLEAR_HAS_VAA            = 0x10,
}
gceCLEAR;

/* Blending targets. */
typedef enum _gceBLEND_UNIT
{
    gcvBLEND_SOURCE,
    gcvBLEND_TARGET,
}
gceBLEND_UNIT;

/* Construct a new gco3D object. */
gceSTATUS
gco3D_Construct(
    IN gcoHAL Hal,
    OUT gco3D * Engine
    );

/* Destroy an gco3D object. */
gceSTATUS
gco3D_Destroy(
    IN gco3D Engine
    );

/* Set 3D API type. */
gceSTATUS
gco3D_SetAPI(
    IN gco3D Engine,
    IN gceAPI ApiType
    );

/* Set render target. */
gceSTATUS
gco3D_SetTarget(
    IN gco3D Engine,
    IN gcoSURF Surface
    );

/* Unset render target. */
gceSTATUS
gco3D_UnsetTarget(
    IN gco3D Engine,
    IN gcoSURF Surface
    );

/* Set depth buffer. */
gceSTATUS
gco3D_SetDepth(
    IN gco3D Engine,
    IN gcoSURF Surface
    );

/* Unset depth buffer. */
gceSTATUS
gco3D_UnsetDepth(
    IN gco3D Engine,
    IN gcoSURF Surface
    );

/* Set viewport. */
gceSTATUS
gco3D_SetViewport(
    IN gco3D Engine,
    IN gctINT32 Left,
    IN gctINT32 Top,
    IN gctINT32 Right,
    IN gctINT32 Bottom
    );

/* Set scissors. */
gceSTATUS
gco3D_SetScissors(
    IN gco3D Engine,
    IN gctINT32 Left,
    IN gctINT32 Top,
    IN gctINT32 Right,
    IN gctINT32 Bottom
    );

/* Set clear color. */
gceSTATUS
gco3D_SetClearColor(
    IN gco3D Engine,
    IN gctUINT8 Red,
    IN gctUINT8 Green,
    IN gctUINT8 Blue,
    IN gctUINT8 Alpha
    );

/* Set fixed point clear color. */
gceSTATUS
gco3D_SetClearColorX(
    IN gco3D Engine,
    IN gctFIXED_POINT Red,
    IN gctFIXED_POINT Green,
    IN gctFIXED_POINT Blue,
    IN gctFIXED_POINT Alpha
    );

/* Set floating point clear color. */
gceSTATUS
gco3D_SetClearColorF(
    IN gco3D Engine,
    IN gctFLOAT Red,
    IN gctFLOAT Green,
    IN gctFLOAT Blue,
    IN gctFLOAT Alpha
    );

/* Set fixed point clear depth. */
gceSTATUS
gco3D_SetClearDepthX(
    IN gco3D Engine,
    IN gctFIXED_POINT Depth
    );

/* Set floating point clear depth. */
gceSTATUS
gco3D_SetClearDepthF(
    IN gco3D Engine,
    IN gctFLOAT Depth
    );

/* Set clear stencil. */
gceSTATUS
gco3D_SetClearStencil(
    IN gco3D Engine,
    IN gctUINT32 Stencil
    );

/* Clear a Rect sub-surface. */
gceSTATUS
gco3D_ClearRect(
    IN gco3D Engine,
    IN gctUINT32 Address,
    IN gctPOINTER Memory,
    IN gctUINT32 Stride,
    IN gceSURF_FORMAT Format,
    IN gctINT32 Left,
    IN gctINT32 Top,
    IN gctINT32 Right,
    IN gctINT32 Bottom,
    IN gctUINT32 Width,
    IN gctUINT32 Height,
    IN gctUINT32 Flags
    );

/* Clear surface. */
gceSTATUS
gco3D_Clear(
    IN gco3D Engine,
    IN gctUINT32 Address,
    IN gctUINT32 Stride,
    IN gceSURF_FORMAT Format,
    IN gctUINT32 Width,
    IN gctUINT32 Height,
    IN gctUINT32 Flags
    );


/* Clear tile status. */
gceSTATUS
gco3D_ClearTileStatus(
    IN gco3D Engine,
    IN gcsSURF_INFO_PTR Surface,
    IN gctUINT32 TileStatusAddress,
    IN gctUINT32 Flags
    );

/* Set shading mode. */
gceSTATUS
gco3D_SetShading(
    IN gco3D Engine,
    IN gceSHADING Shading
    );

/* Set blending mode. */
gceSTATUS
gco3D_EnableBlending(
    IN gco3D Engine,
    IN gctBOOL Enable
    );

/* Set blending function. */
gceSTATUS
gco3D_SetBlendFunction(
    IN gco3D Engine,
    IN gceBLEND_UNIT Unit,
    IN gceBLEND_FUNCTION FunctionRGB,
    IN gceBLEND_FUNCTION FunctionAlpha
    );

/* Set blending mode. */
gceSTATUS
gco3D_SetBlendMode(
    IN gco3D Engine,
    IN gceBLEND_MODE ModeRGB,
    IN gceBLEND_MODE ModeAlpha
    );

/* Set blending color. */
gceSTATUS
gco3D_SetBlendColor(
    IN gco3D Engine,
    IN gctUINT Red,
    IN gctUINT Green,
    IN gctUINT Blue,
    IN gctUINT Alpha
    );

/* Set fixed point blending color. */
gceSTATUS
gco3D_SetBlendColorX(
    IN gco3D Engine,
    IN gctFIXED_POINT Red,
    IN gctFIXED_POINT Green,
    IN gctFIXED_POINT Blue,
    IN gctFIXED_POINT Alpha
    );

/* Set floating point blending color. */
gceSTATUS
gco3D_SetBlendColorF(
    IN gco3D Engine,
    IN gctFLOAT Red,
    IN gctFLOAT Green,
    IN gctFLOAT Blue,
    IN gctFLOAT Alpha
    );

/* Set culling mode. */
gceSTATUS
gco3D_SetCulling(
    IN gco3D Engine,
    IN gceCULL Mode
    );

/* Enable point size */
gceSTATUS
gco3D_SetPointSizeEnable(
    IN gco3D Engine,
    IN gctBOOL Enable
    );

/* Set point sprite */
gceSTATUS
gco3D_SetPointSprite(
    IN gco3D Engine,
    IN gctBOOL Enable
    );

/* Set fill mode. */
gceSTATUS
gco3D_SetFill(
    IN gco3D Engine,
    IN gceFILL Mode
    );

/* Set depth compare mode. */
gceSTATUS
gco3D_SetDepthCompare(
    IN gco3D Engine,
    IN gceCOMPARE Compare
    );

/* Enable depth writing. */
gceSTATUS
gco3D_EnableDepthWrite(
    IN gco3D Engine,
    IN gctBOOL Enable
    );

/* Set depth mode. */
gceSTATUS
gco3D_SetDepthMode(
    IN gco3D Engine,
    IN gceDEPTH_MODE Mode
    );

/* Set depth range. */
gceSTATUS
gco3D_SetDepthRangeX(
    IN gco3D Engine,
    IN gceDEPTH_MODE Mode,
    IN gctFIXED_POINT Near,
    IN gctFIXED_POINT Far
    );

/* Set depth range. */
gceSTATUS
gco3D_SetDepthRangeF(
    IN gco3D Engine,
    IN gceDEPTH_MODE Mode,
    IN gctFLOAT Near,
    IN gctFLOAT Far
    );

/* Set last pixel enable */
gceSTATUS
gco3D_SetLastPixelEnable(
    IN gco3D Engine,
    IN gctBOOL Enable
    );

/* Set depth Bias and Scale */
gceSTATUS
gco3D_SetDepthScaleBiasX(
    IN gco3D Engine,
    IN gctFIXED_POINT DepthScale,
    IN gctFIXED_POINT DepthBias
    );

gceSTATUS
gco3D_SetDepthScaleBiasF(
    IN gco3D Engine,
    IN gctFLOAT DepthScale,
    IN gctFLOAT DepthBias
    );

/* Set depth near and far clipping plane. */
gceSTATUS
gco3D_SetDepthPlaneF(
    IN gco3D Engine,
    IN gctFLOAT Near,
    IN gctFLOAT Far
    );

/* Enable or disable dithering. */
gceSTATUS
gco3D_EnableDither(
    IN gco3D Engine,
    IN gctBOOL Enable
    );

/* Set color write enable bits. */
gceSTATUS
gco3D_SetColorWrite(
    IN gco3D Engine,
    IN gctUINT8 Enable
    );

/* Enable or disable early depth. */
gceSTATUS
gco3D_SetEarlyDepth(
    IN gco3D Engine,
    IN gctBOOL Enable
    );

/* Enable or disable all early depth operations. */
gceSTATUS
gco3D_SetAllEarlyDepthModes(
    IN gco3D Engine,
    IN gctBOOL Disable
    );

/* Switch dynamic early mode */
gceSTATUS
gco3D_SwitchDynamicEarlyDepthMode(
    IN gco3D Engine
    );

/* Set dynamic early mode */
gceSTATUS
gco3D_DisableDynamicEarlyDepthMode(
    IN gco3D Engine,
    IN gctBOOL Disable
    );

/* Enable or disable depth-only mode. */
gceSTATUS
gco3D_SetDepthOnly(
    IN gco3D Engine,
    IN gctBOOL Enable
    );

typedef struct _gcsSTENCIL_INFO * gcsSTENCIL_INFO_PTR;
typedef struct _gcsSTENCIL_INFO
{
    gceSTENCIL_MODE         mode;

    gctUINT8                maskFront;
	gctUINT8                maskBack;
    gctUINT8                writeMaskFront;
    gctUINT8                writeMaskBack;

    gctUINT8                referenceFront;

    gceCOMPARE              compareFront;
    gceSTENCIL_OPERATION    passFront;
    gceSTENCIL_OPERATION    failFront;
    gceSTENCIL_OPERATION    depthFailFront;

    gctUINT8                referenceBack;
    gceCOMPARE              compareBack;
    gceSTENCIL_OPERATION    passBack;
    gceSTENCIL_OPERATION    failBack;
    gceSTENCIL_OPERATION    depthFailBack;
}
gcsSTENCIL_INFO;

/* Set stencil mode. */
gceSTATUS
gco3D_SetStencilMode(
    IN gco3D Engine,
    IN gceSTENCIL_MODE Mode
    );

/* Set stencil mask. */
gceSTATUS
gco3D_SetStencilMask(
    IN gco3D Engine,
    IN gctUINT8 Mask
    );

/* Set stencil back mask. */
gceSTATUS
gco3D_SetStencilMaskBack(
    IN gco3D Engine,
    IN gctUINT8 Mask
    );

/* Set stencil write mask. */
gceSTATUS
gco3D_SetStencilWriteMask(
    IN gco3D Engine,
    IN gctUINT8 Mask
    );

/* Set stencil back write mask. */
gceSTATUS
gco3D_SetStencilWriteMaskBack(
    IN gco3D Engine,
    IN gctUINT8 Mask
    );

/* Set stencil reference. */
gceSTATUS
gco3D_SetStencilReference(
    IN gco3D Engine,
    IN gctUINT8 Reference,
    IN gctBOOL Front
    );

/* Set stencil compare. */
gceSTATUS
gco3D_SetStencilCompare(
    IN gco3D Engine,
    IN gceSTENCIL_WHERE Where,
    IN gceCOMPARE Compare
    );

/* Set stencil operation on pass. */
gceSTATUS
gco3D_SetStencilPass(
    IN gco3D Engine,
    IN gceSTENCIL_WHERE Where,
    IN gceSTENCIL_OPERATION Operation
    );

/* Set stencil operation on fail. */
gceSTATUS
gco3D_SetStencilFail(
    IN gco3D Engine,
    IN gceSTENCIL_WHERE Where,
    IN gceSTENCIL_OPERATION Operation
    );

/* Set stencil operation on depth fail. */
gceSTATUS
gco3D_SetStencilDepthFail(
    IN gco3D Engine,
    IN gceSTENCIL_WHERE Where,
    IN gceSTENCIL_OPERATION Operation
    );

/* Set all stencil states in one blow. */
gceSTATUS
gco3D_SetStencilAll(
    IN gco3D Engine,
    IN gcsSTENCIL_INFO_PTR Info
    );

typedef struct _gcsALPHA_INFO * gcsALPHA_INFO_PTR;
typedef struct _gcsALPHA_INFO
{
    /* Alpha test states. */
    gctBOOL                 test;
    gceCOMPARE              compare;
    gctUINT8                reference;
    gctFLOAT                floatReference;

    /* Alpha blending states. */
    gctBOOL                 blend;

    gceBLEND_FUNCTION       srcFuncColor;
    gceBLEND_FUNCTION       srcFuncAlpha;
    gceBLEND_FUNCTION       trgFuncColor;
    gceBLEND_FUNCTION       trgFuncAlpha;

    gceBLEND_MODE           modeColor;
    gceBLEND_MODE           modeAlpha;

    gctUINT32               color;
}
gcsALPHA_INFO;

/* Enable or disable alpha test. */
gceSTATUS
gco3D_SetAlphaTest(
    IN gco3D Engine,
    IN gctBOOL Enable
    );

/* Set alpha test compare. */
gceSTATUS
gco3D_SetAlphaCompare(
    IN gco3D Engine,
    IN gceCOMPARE Compare
    );

/* Set alpha test reference in unsigned integer. */
gceSTATUS
gco3D_SetAlphaReference(
    IN gco3D Engine,
    IN gctUINT8 Reference,
    IN gctFLOAT FloatReference
    );

/* Set alpha test reference in fixed point. */
gceSTATUS
gco3D_SetAlphaReferenceX(
    IN gco3D Engine,
    IN gctFIXED_POINT Reference
    );

/* Set alpha test reference in floating point. */
gceSTATUS
gco3D_SetAlphaReferenceF(
    IN gco3D Engine,
    IN gctFLOAT Reference
    );

/* Enable/Disable anti-alias line. */
gceSTATUS
gco3D_SetAntiAliasLine(
    IN gco3D Engine,
    IN gctBOOL Enable
    );

/* Set texture slot for anti-alias line. */
gceSTATUS
gco3D_SetAALineTexSlot(
    IN gco3D Engine,
    IN gctUINT TexSlot
    );

/* Set anti-alias line width scale. */
gceSTATUS
gco3D_SetAALineWidth(
    IN gco3D Engine,
    IN gctFLOAT Width
    );

/* Draw a number of primitives. */
gceSTATUS
gco3D_DrawPrimitives(
    IN gco3D Engine,
    IN gcePRIMITIVE Type,
    IN gctINT StartVertex,
    IN gctSIZE_T PrimitiveCount
    );

gceSTATUS
gco3D_DrawPrimitivesCount(
    IN gco3D Engine,
    IN gcePRIMITIVE Type,
    IN gctINT* StartVertex,
    IN gctSIZE_T* VertexCount,
    IN gctSIZE_T PrimitiveCount
    );


/* Draw a number of primitives using offsets. */
gceSTATUS
gco3D_DrawPrimitivesOffset(
    IN gco3D Engine,
    IN gcePRIMITIVE Type,
    IN gctINT32 StartOffset,
    IN gctSIZE_T PrimitiveCount
    );

/* Draw a number of indexed primitives. */
gceSTATUS
gco3D_DrawIndexedPrimitives(
    IN gco3D Engine,
    IN gcePRIMITIVE Type,
    IN gctINT BaseVertex,
    IN gctINT StartIndex,
    IN gctSIZE_T PrimitiveCount
    );

/* Draw a number of indexed primitives using offsets. */
gceSTATUS
gco3D_DrawIndexedPrimitivesOffset(
    IN gco3D Engine,
    IN gcePRIMITIVE Type,
    IN gctINT32 BaseOffset,
    IN gctINT32 StartOffset,
    IN gctSIZE_T PrimitiveCount
    );

/* Enable or disable anti-aliasing. */
gceSTATUS
gco3D_SetAntiAlias(
    IN gco3D Engine,
    IN gctBOOL Enable
    );

/* Write data into the command buffer. */
gceSTATUS
gco3D_WriteBuffer(
    IN gco3D Engine,
    IN gctCONST_POINTER Data,
    IN gctSIZE_T Bytes,
    IN gctBOOL Aligned
    );

/* Send sempahore and stall until sempahore is signalled. */
gceSTATUS
gco3D_Semaphore(
    IN gco3D Engine,
    IN gceWHERE From,
    IN gceWHERE To,
    IN gceHOW How);

/* Set the subpixels center. */
gceSTATUS
gco3D_SetCentroids(
    IN gco3D Engine,
    IN gctUINT32 Index,
    IN gctPOINTER Centroids
    );

gceSTATUS
gco3D_SetLogicOp(
    IN gco3D Engine,
    IN gctUINT8 Rop
    );

/* OCL thread walker information. */
typedef struct _gcsTHREAD_WALKER_INFO * gcsTHREAD_WALKER_INFO_PTR;
typedef struct _gcsTHREAD_WALKER_INFO
{
    gctUINT32   dimensions;
    gctUINT32   traverseOrder;
    gctUINT32   enableSwathX;
    gctUINT32   enableSwathY;
    gctUINT32   enableSwathZ;
    gctUINT32   swathSizeX;
    gctUINT32   swathSizeY;
    gctUINT32   swathSizeZ;
    gctUINT32   valueOrder;

    gctUINT32   globalSizeX;
    gctUINT32   globalOffsetX;
    gctUINT32   globalSizeY;
    gctUINT32   globalOffsetY;
    gctUINT32   globalSizeZ;
    gctUINT32   globalOffsetZ;

    gctUINT32   workGroupSizeX;
    gctUINT32   workGroupCountX;
    gctUINT32   workGroupSizeY;
    gctUINT32   workGroupCountY;
    gctUINT32   workGroupSizeZ;
    gctUINT32   workGroupCountZ;

    gctUINT32   threadAllocation;
}
gcsTHREAD_WALKER_INFO;

/* Start OCL thread walker. */
gceSTATUS
gco3D_InvokeThreadWalker(
    IN gco3D Engine,
    IN gcsTHREAD_WALKER_INFO_PTR Info
    );

/* Set w clip and w plane limit value. */
gceSTATUS
gco3D_SetWClipEnable(
	IN gco3D Engine,
	IN gctBOOL Enable
    );

gceSTATUS
gco3D_GetWClipEnable(
    IN gco3D Engine,
    OUT gctBOOL * Enable
    );

gceSTATUS
gco3D_SetWPlaneLimitF(
	IN gco3D Engine,
	IN gctFLOAT Value
    );

gceSTATUS
gco3D_SetWPlaneLimitX(
	IN gco3D Engine,
	IN gctFIXED_POINT Value
    );


gceSTATUS
gco3D_SetWPlaneLimit(
        IN gco3D Engine,
        IN gctFLOAT Value
        );

/*----------------------------------------------------------------------------*/
/*-------------------------- gco3D Fragment Processor ------------------------*/

/* Set the fragment processor configuration. */
gceSTATUS
gco3D_SetFragmentConfiguration(
    IN gco3D Engine,
    IN gctBOOL ColorFromStream,
    IN gctBOOL EnableFog,
    IN gctBOOL EnableSmoothPoint,
    IN gctUINT32 ClipPlanes
    );

/* Enable/disable texture stage operation. */
gceSTATUS
gco3D_EnableTextureStage(
    IN gco3D Engine,
    IN gctINT Stage,
    IN gctBOOL Enable
    );

/* Program the channel enable masks for the color texture function. */
gceSTATUS
gco3D_SetTextureColorMask(
    IN gco3D Engine,
    IN gctINT Stage,
    IN gctBOOL ColorEnabled,
    IN gctBOOL AlphaEnabled
    );

/* Program the channel enable masks for the alpha texture function. */
gceSTATUS
gco3D_SetTextureAlphaMask(
    IN gco3D Engine,
    IN gctINT Stage,
    IN gctBOOL ColorEnabled,
    IN gctBOOL AlphaEnabled
    );

/* Program the constant fragment color. */
gceSTATUS
gco3D_SetFragmentColorX(
    IN gco3D Engine,
    IN gctFIXED_POINT Red,
    IN gctFIXED_POINT Green,
    IN gctFIXED_POINT Blue,
    IN gctFIXED_POINT Alpha
    );

gceSTATUS
gco3D_SetFragmentColorF(
    IN gco3D Engine,
    IN gctFLOAT Red,
    IN gctFLOAT Green,
    IN gctFLOAT Blue,
    IN gctFLOAT Alpha
    );

/* Program the constant fog color. */
gceSTATUS
gco3D_SetFogColorX(
    IN gco3D Engine,
    IN gctFIXED_POINT Red,
    IN gctFIXED_POINT Green,
    IN gctFIXED_POINT Blue,
    IN gctFIXED_POINT Alpha
    );

gceSTATUS
gco3D_SetFogColorF(
    IN gco3D Engine,
    IN gctFLOAT Red,
    IN gctFLOAT Green,
    IN gctFLOAT Blue,
    IN gctFLOAT Alpha
    );

/* Program the constant texture color. */
gceSTATUS
gco3D_SetTetxureColorX(
    IN gco3D Engine,
    IN gctINT Stage,
    IN gctFIXED_POINT Red,
    IN gctFIXED_POINT Green,
    IN gctFIXED_POINT Blue,
    IN gctFIXED_POINT Alpha
    );

gceSTATUS
gco3D_SetTetxureColorF(
    IN gco3D Engine,
    IN gctINT Stage,
    IN gctFLOAT Red,
    IN gctFLOAT Green,
    IN gctFLOAT Blue,
    IN gctFLOAT Alpha
    );

/* Configure color texture function. */
gceSTATUS
gco3D_SetColorTextureFunction(
    IN gco3D Engine,
    IN gctINT Stage,
    IN gceTEXTURE_FUNCTION Function,
    IN gceTEXTURE_SOURCE Source0,
    IN gceTEXTURE_CHANNEL Channel0,
    IN gceTEXTURE_SOURCE Source1,
    IN gceTEXTURE_CHANNEL Channel1,
    IN gceTEXTURE_SOURCE Source2,
    IN gceTEXTURE_CHANNEL Channel2,
    IN gctINT Scale
    );

/* Configure alpha texture function. */
gceSTATUS
gco3D_SetAlphaTextureFunction(
    IN gco3D Engine,
    IN gctINT Stage,
    IN gceTEXTURE_FUNCTION Function,
    IN gceTEXTURE_SOURCE Source0,
    IN gceTEXTURE_CHANNEL Channel0,
    IN gceTEXTURE_SOURCE Source1,
    IN gceTEXTURE_CHANNEL Channel1,
    IN gceTEXTURE_SOURCE Source2,
    IN gceTEXTURE_CHANNEL Channel2,
    IN gctINT Scale
    );

/* Invoke OCL thread walker. */
gceSTATUS
gcoHARDWARE_InvokeThreadWalker(
    IN gcsTHREAD_WALKER_INFO_PTR Info
    );

/******************************************************************************\
******************************* gcoTEXTURE Object *******************************
\******************************************************************************/

/* Cube faces. */
typedef enum _gceTEXTURE_FACE
{
    gcvFACE_NONE,
    gcvFACE_POSITIVE_X,
    gcvFACE_NEGATIVE_X,
    gcvFACE_POSITIVE_Y,
    gcvFACE_NEGATIVE_Y,
    gcvFACE_POSITIVE_Z,
    gcvFACE_NEGATIVE_Z,
}
gceTEXTURE_FACE;

#if gcdFORCE_MIPMAP
typedef enum
{
    gcvForceMipDisabled  = 0,
    gcvForceMipEnable    = 1,
    gcvForceMipGenerated = 2,
    gcvForceMipNever     = 3,
}gceFORCE_MIPMAP;
#endif

typedef struct _gcsTEXTURE
{
    /* Addressing modes. */
    gceTEXTURE_ADDRESSING       s;
    gceTEXTURE_ADDRESSING       t;
    gceTEXTURE_ADDRESSING       r;

    /* Border color. */
    gctUINT8                    border[4];

    /* Filters. */
    gceTEXTURE_FILTER           minFilter;
    gceTEXTURE_FILTER           magFilter;
    gceTEXTURE_FILTER           mipFilter;
    gctUINT                     anisoFilter;
    gctBOOL                     forceTopLevel;
    gctBOOL                     autoMipmap;
#if gcdFORCE_MIPMAP
    gceFORCE_MIPMAP             forceMipmap;
#endif
    /* Level of detail. */
    gctFIXED_POINT              lodBias;
    gctFIXED_POINT              lodMin;
    gctFIXED_POINT              lodMax;
}
gcsTEXTURE, * gcsTEXTURE_PTR;

/* Construct a new gcoTEXTURE object. */
gceSTATUS
gcoTEXTURE_Construct(
    IN gcoHAL Hal,
    OUT gcoTEXTURE * Texture
    );

/* Construct a new sized gcoTEXTURE object. */
gceSTATUS
gcoTEXTURE_ConstructSized(
    IN gcoHAL Hal,
    IN gceSURF_FORMAT Format,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT Depth,
    IN gctUINT Faces,
    IN gctUINT MipMapCount,
    IN gcePOOL Pool,
    OUT gcoTEXTURE * Texture
    );

/* Destroy an gcoTEXTURE object. */
gceSTATUS
gcoTEXTURE_Destroy(
    IN gcoTEXTURE Texture
    );
#if gcdFORCE_MIPMAP
gceSTATUS
gcoTEXTURE_DestroyForceMipmap(
    IN gcoTEXTURE Texture
    );

gceSTATUS
gcoTEXTURE_GetMipLevels(
    IN gcoTEXTURE Texture,
    OUT gctINT * levels
    );
#endif
/* Replace a mipmap in gcoTEXTURE object. */
gceSTATUS
gcoTEXTURE_ReplaceMipMap(
    IN gcoTEXTURE Texture,
    IN gctUINT Level,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctINT imageFormat,
    IN gceSURF_FORMAT Format,
    IN gctUINT Depth,
    IN gctUINT Faces,
    IN gcePOOL Pool
    );

/* Upload data to an gcoTEXTURE object. */
gceSTATUS
gcoTEXTURE_Upload(
    IN gcoTEXTURE Texture,
    IN gceTEXTURE_FACE Face,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT Slice,
    IN gctCONST_POINTER Memory,
    IN gctINT Stride,
    IN gceSURF_FORMAT Format
    );

/* Upload data to an gcoTEXTURE object. */
gceSTATUS
gcoTEXTURE_UploadSub(
    IN gcoTEXTURE Texture,
    IN gctUINT MipMap,
    IN gceTEXTURE_FACE Face,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT Slice,
    IN gctCONST_POINTER Memory,
    IN gctINT Stride,
    IN gceSURF_FORMAT Format
    );

/* Upload YUV data to an gcoTEXTURE object. */
gceSTATUS
gcoTEXTURE_UploadYUV(
    IN gcoTEXTURE Texture,
    IN gceTEXTURE_FACE Face,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT Slice,
    IN gctPOINTER Memory[3],
    IN gctINT Stride[3],
    IN gceSURF_FORMAT Format
    );

/* Upload compressed data to an gcoTEXTURE object. */
gceSTATUS
gcoTEXTURE_UploadCompressed(
    IN gcoTEXTURE Texture,
    IN gceTEXTURE_FACE Face,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT Slice,
    IN gctCONST_POINTER Memory,
    IN gctSIZE_T Bytes
    );

/* Upload compressed sub data to an gcoTEXTURE object. */
gceSTATUS
gcoTEXTURE_UploadCompressedSub(
    IN gcoTEXTURE Texture,
    IN gctUINT MipMap,
    IN gceTEXTURE_FACE Face,
    IN gctUINT XOffset,
    IN gctUINT YOffset,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT Slice,
    IN gctCONST_POINTER Memory,
    IN gctSIZE_T Size
    );

/* GetImageFormat of texture. */
gceSTATUS
gcoTEXTURE_GetImageFormat(
    IN gcoTEXTURE Texture,
    IN gctUINT MipMap,
    OUT gctINT *  ImageFormat
    );

/* Get gcoSURF object for a mipmap level. */
gceSTATUS
gcoTEXTURE_GetMipMap(
    IN gcoTEXTURE Texture,
    IN gctUINT MipMap,
    OUT gcoSURF * Surface
    );

/* Get gcoSURF object for a mipmap level and face offset. */
gceSTATUS
gcoTEXTURE_GetMipMapFace(
    IN gcoTEXTURE Texture,
    IN gctUINT MipMap,
    IN gceTEXTURE_FACE Face,
    OUT gcoSURF * Surface,
    OUT gctUINT32_PTR Offset
    );

gceSTATUS
gcoTEXTURE_AddMipMap(
    IN gcoTEXTURE Texture,
    IN gctINT Level,
    IN gctINT imageFormat,
    IN gceSURF_FORMAT Format,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT Depth,
    IN gctUINT Faces,
    IN gcePOOL Pool,
    OUT gcoSURF * Surface
    );

gceSTATUS
gcoTEXTURE_AddMipMapFromClient(
    IN gcoTEXTURE Texture,
    IN gctINT     Level,
    IN gcoSURF    Surface
    );

gceSTATUS
gcoTEXTURE_AddMipMapFromSurface(
    IN gcoTEXTURE Texture,
    IN gctINT     Level,
    IN gcoSURF    Surface
    );

gceSTATUS
gcoTEXTURE_SetMaxLevel(
    IN gcoTEXTURE Texture,
    IN gctUINT Levels
    );

gceSTATUS
gcoTEXTURE_SetEndianHint(
    IN gcoTEXTURE Texture,
    IN gceENDIAN_HINT EndianHint
    );

gceSTATUS
gcoTEXTURE_Disable(
    IN gcoHAL Hal,
    IN gctINT Sampler
    );

gceSTATUS
gcoTEXTURE_Flush(
    IN gcoTEXTURE Texture
    );

gceSTATUS
gcoTEXTURE_QueryCaps(
    IN  gcoHAL    Hal,
    OUT gctUINT * MaxWidth,
    OUT gctUINT * MaxHeight,
    OUT gctUINT * MaxDepth,
    OUT gctBOOL * Cubic,
    OUT gctBOOL * NonPowerOfTwo,
    OUT gctUINT * VertexSamplers,
    OUT gctUINT * PixelSamplers
    );

gceSTATUS
gcoTEXTURE_GetTiling(
    IN gcoTEXTURE Texture,
    IN gctINT preferLevel,
	OUT gceTILING * Tiling
    );

gceSTATUS
gcoTEXTURE_GetClosestFormat(
    IN gcoHAL Hal,
    IN gceSURF_FORMAT InFormat,
    OUT gceSURF_FORMAT* OutFormat
    );

gceSTATUS
gcoTEXTURE_RenderIntoMipMap(
    IN gcoTEXTURE Texture,
    IN gctINT Level
    );

gceSTATUS
gcoTEXTURE_IsRenderable(
    IN gcoTEXTURE Texture,
    IN gctUINT Level
    );

gceSTATUS
gcoTEXTURE_IsRenderableEx(
    IN gcoTEXTURE Texture,
    IN gctUINT Level
    );

gceSTATUS
gcoTEXTURE_IsComplete(
    IN gcoTEXTURE Texture,
    IN gctINT MaxLevel
    );

gceSTATUS
gcoTEXTURE_BindTexture(
    IN gcoTEXTURE Texture,
    IN gctINT Target,
    IN gctINT Sampler,
    IN gcsTEXTURE_PTR Info
    );

/******************************************************************************\
******************************* gcoSTREAM Object ******************************
\******************************************************************************/

typedef enum _gceVERTEX_FORMAT
{
    gcvVERTEX_BYTE,
    gcvVERTEX_UNSIGNED_BYTE,
    gcvVERTEX_SHORT,
    gcvVERTEX_UNSIGNED_SHORT,
    gcvVERTEX_INT,
    gcvVERTEX_UNSIGNED_INT,
    gcvVERTEX_FIXED,
    gcvVERTEX_HALF,
    gcvVERTEX_FLOAT,
    gcvVERTEX_UNSIGNED_INT_10_10_10_2,
    gcvVERTEX_INT_10_10_10_2,
}
gceVERTEX_FORMAT;

gceSTATUS
gcoSTREAM_Construct(
    IN gcoHAL Hal,
    OUT gcoSTREAM * Stream
    );

gceSTATUS
gcoSTREAM_Destroy(
    IN gcoSTREAM Stream
    );

gceSTATUS
gcoSTREAM_Upload(
    IN gcoSTREAM Stream,
    IN gctCONST_POINTER Buffer,
    IN gctUINT32 Offset,
    IN gctSIZE_T Bytes,
    IN gctBOOL Dynamic
    );

gceSTATUS
gcoSTREAM_SetStride(
    IN gcoSTREAM Stream,
    IN gctUINT32 Stride
    );

gceSTATUS
gcoSTREAM_Lock(
    IN gcoSTREAM Stream,
    OUT gctPOINTER * Logical,
    OUT gctUINT32 * Physical
    );

gceSTATUS
gcoSTREAM_Unlock(
    IN gcoSTREAM Stream
    );

gceSTATUS
gcoSTREAM_Reserve(
    IN gcoSTREAM Stream,
    IN gctSIZE_T Bytes
    );

gceSTATUS
gcoSTREAM_Flush(
    IN gcoSTREAM Stream
    );

/* Dynamic buffer API. */
gceSTATUS
gcoSTREAM_SetDynamic(
    IN gcoSTREAM Stream,
    IN gctSIZE_T Bytes,
    IN gctUINT Buffers
    );

typedef struct _gcsSTREAM_INFO
{
    gctUINT             index;
    gceVERTEX_FORMAT    format;
    gctBOOL             normalized;
    gctUINT             components;
    gctSIZE_T           size;
    gctCONST_POINTER    data;
    gctUINT             stride;
}
gcsSTREAM_INFO, * gcsSTREAM_INFO_PTR;

gceSTATUS
gcoSTREAM_UploadDynamic(
    IN gcoSTREAM Stream,
    IN gctUINT VertexCount,
    IN gctUINT InfoCount,
    IN gcsSTREAM_INFO_PTR Info,
    IN gcoVERTEX Vertex
    );

gceSTATUS
gcoSTREAM_CPUCacheOperation(
    IN gcoSTREAM Stream,
    IN gceCACHEOPERATION Operation
    );

/******************************************************************************\
******************************** gcoVERTEX Object ******************************
\******************************************************************************/

typedef struct _gcsVERTEX_ATTRIBUTES
{
    gceVERTEX_FORMAT            format;
    gctBOOL                     normalized;
    gctUINT32                   components;
    gctSIZE_T                   size;
    gctUINT32                   stream;
    gctUINT32                   offset;
    gctUINT32                   stride;
}
gcsVERTEX_ATTRIBUTES;

gceSTATUS
gcoVERTEX_Construct(
    IN gcoHAL Hal,
    OUT gcoVERTEX * Vertex
    );

gceSTATUS
gcoVERTEX_Destroy(
    IN gcoVERTEX Vertex
    );

gceSTATUS
gcoVERTEX_Reset(
    IN gcoVERTEX Vertex
    );

gceSTATUS
gcoVERTEX_EnableAttribute(
    IN gcoVERTEX Vertex,
    IN gctUINT32 Index,
    IN gceVERTEX_FORMAT Format,
    IN gctBOOL Normalized,
    IN gctUINT32 Components,
    IN gcoSTREAM Stream,
    IN gctUINT32 Offset,
    IN gctUINT32 Stride
    );

gceSTATUS
gcoVERTEX_DisableAttribute(
    IN gcoVERTEX Vertex,
    IN gctUINT32 Index
    );

gceSTATUS
gcoVERTEX_Bind(
    IN gcoVERTEX Vertex
    );

/*******************************************************************************
***** gcoVERTEXARRAY Object ***************************************************/

typedef struct _gcsVERTEXARRAY
{
    /* Enabled. */
    gctBOOL             enable;

    /* Number of components. */
    gctINT              size;

    /* Attribute format. */
    gceVERTEX_FORMAT    format;

    /* Flag whether the attribute is normalized or not. */
    gctBOOL             normalized;

    /* Stride of the component. */
    gctUINT             stride;

    /* Pointer to the attribute data. */
    gctCONST_POINTER    pointer;

    /* Stream object owning the attribute data. */
    gcoSTREAM           stream;

    /* Generic values for attribute. */
    gctFLOAT            genericValue[4];

    /* Generic size for attribute. */
    gctINT              genericSize;

    /* Vertex shader linkage. */
    gctUINT             linkage;

#if gcdUSE_WCLIP_PATCH
    gctBOOL             isPosition;
#endif
}
gcsVERTEXARRAY,
* gcsVERTEXARRAY_PTR;

gceSTATUS
gcoVERTEXARRAY_Construct(
    IN gcoHAL Hal,
    OUT gcoVERTEXARRAY * Vertex
    );

gceSTATUS
gcoVERTEXARRAY_Destroy(
    IN gcoVERTEXARRAY Vertex
    );

gceSTATUS
gcoVERTEXARRAY_Bind(
    IN gcoVERTEXARRAY Vertex,
    IN gctUINT32 EnableBits,
    IN gcsVERTEXARRAY_PTR VertexArray,
    IN gctUINT First,
    IN gctSIZE_T Count,
    IN gceINDEX_TYPE IndexType,
    IN gcoINDEX IndexObject,
    IN gctPOINTER IndexMemory,
    IN OUT gcePRIMITIVE * PrimitiveType,
#if gcdUSE_WCLIP_PATCH
    IN OUT gctUINT * PrimitiveCount,
    IN OUT gctFLOAT * wLimitRms,
    IN OUT gctBOOL * wLimitDirty
#else
    IN OUT gctUINT * PrimitiveCount
#endif
    );

gctUINT
gcoVERTEXARRAY_GetMaxStream(
    IN gcoVERTEXARRAY Vertex
);

gceSTATUS
gcoVERTEXARRAY_SetMaxStream(
    IN gcoVERTEXARRAY Vertex,
    gctUINT maxStreams
);
/*******************************************************************************
***** Composition *************************************************************/

typedef enum _gceCOMPOSITION
{
    gcvCOMPOSE_CLEAR = 1,
    gcvCOMPOSE_BLUR,
    gcvCOMPOSE_DIM,
    gcvCOMPOSE_LAYER
}
gceCOMPOSITION;

typedef struct _gcsCOMPOSITION * gcsCOMPOSITION_PTR;
typedef struct _gcsCOMPOSITION
{
    /* Structure size. */
    gctUINT                         structSize;

    /* Composition operation. */
    gceCOMPOSITION                  operation;

    /* Layer to be composed. */
    gcoSURF                         layer;

    /* Source and target coordinates. */
    gcsRECT                         srcRect;
    gcsRECT                         trgRect;

    /* Target rectangle */
    gcsPOINT                        v0;
    gcsPOINT                        v1;
    gcsPOINT                        v2;

    /* Blending parameters. */
    gctBOOL                         enableBlending;
    gctBOOL                         premultiplied;
    gctUINT8                        alphaValue;

    /* Clear color. */
    gctFLOAT                        r;
    gctFLOAT                        g;
    gctFLOAT                        b;
    gctFLOAT                        a;
}
gcsCOMPOSITION;

gceSTATUS
gco3D_ProbeComposition(
    gctBOOL ResetIfEmpty
    );

gceSTATUS
gco3D_CompositionBegin(
    void
    );

gceSTATUS
gco3D_ComposeLayer(
    IN gcsCOMPOSITION_PTR Layer
    );

gceSTATUS
gco3D_CompositionSignals(
    IN gctHANDLE Process,
    IN gctSIGNAL Signal1,
    IN gctSIGNAL Signal2
    );

gceSTATUS
gco3D_CompositionEnd(
    IN gcoSURF Target,
    IN gctBOOL Synchronous
    );

/* Frame Database */
gceSTATUS
gcoHAL_AddFrameDB(
    void
    );

gceSTATUS
gcoHAL_DumpFrameDB(
    gctCONST_STRING Filename OPTIONAL
    );

gceSTATUS
gcoHAL_GetSharedInfo(
    IN gctUINT32 Pid,
    IN gctUINT32 DataId,
    IN gctSIZE_T Bytes,
    OUT gctPOINTER Data
    );

gceSTATUS
gcoHAL_SetSharedInfo(
    IN gctUINT32 DataId,
    IN gctPOINTER Data,
    IN gctSIZE_T Bytes
    );

#if VIVANTE_PROFILER_CONTEXT
gceSTATUS
gcoHARDWARE_GetContext(
    IN gcoHARDWARE Hardware,
    OUT gctUINT32 * Context
    );
#endif

#ifdef __cplusplus
}
#endif

#endif /* VIVANTE_NO_3D */
#endif /* __gc_hal_engine_h_ */
