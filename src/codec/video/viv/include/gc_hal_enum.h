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


#ifndef __gc_hal_enum_h_
#define __gc_hal_enum_h_

#ifdef __cplusplus
extern "C" {
#endif

/* Chip models. */
typedef enum _gceCHIPMODEL
{
    gcv300  = 0x0300,
    gcv320  = 0x0320,
    gcv350  = 0x0350,
    gcv355  = 0x0355,
    gcv400  = 0x0400,
    gcv410  = 0x0410,
    gcv420  = 0x0420,
    gcv450  = 0x0450,
    gcv500  = 0x0500,
    gcv530  = 0x0530,
    gcv600  = 0x0600,
    gcv700  = 0x0700,
    gcv800  = 0x0800,
    gcv860  = 0x0860,
    gcv880  = 0x0880,
    gcv1000 = 0x1000,
    gcv2000 = 0x2000,
    gcv2100 = 0x2100,
    gcv4000 = 0x4000,
}
gceCHIPMODEL;

/* Chip features. */
typedef enum _gceFEATURE
{
    gcvFEATURE_PIPE_2D = 0,
    gcvFEATURE_PIPE_3D,
    gcvFEATURE_PIPE_VG,
    gcvFEATURE_DC,
    gcvFEATURE_HIGH_DYNAMIC_RANGE,
    gcvFEATURE_MODULE_CG,
    gcvFEATURE_MIN_AREA,
    gcvFEATURE_BUFFER_INTERLEAVING,
    gcvFEATURE_BYTE_WRITE_2D,
    gcvFEATURE_ENDIANNESS_CONFIG,
    gcvFEATURE_DUAL_RETURN_BUS,
    gcvFEATURE_DEBUG_MODE,
    gcvFEATURE_YUY2_RENDER_TARGET,
    gcvFEATURE_FRAGMENT_PROCESSOR,
    gcvFEATURE_2DPE20,
    gcvFEATURE_FAST_CLEAR,
    gcvFEATURE_YUV420_TILER,
    gcvFEATURE_YUY2_AVERAGING,
    gcvFEATURE_FLIP_Y,
    gcvFEATURE_EARLY_Z,
    gcvFEATURE_Z_COMPRESSION,
    gcvFEATURE_MSAA,
    gcvFEATURE_SPECIAL_ANTI_ALIASING,
    gcvFEATURE_SPECIAL_MSAA_LOD,
    gcvFEATURE_422_TEXTURE_COMPRESSION,
    gcvFEATURE_DXT_TEXTURE_COMPRESSION,
    gcvFEATURE_ETC1_TEXTURE_COMPRESSION,
    gcvFEATURE_CORRECT_TEXTURE_CONVERTER,
    gcvFEATURE_TEXTURE_8K,
    gcvFEATURE_SCALER,
    gcvFEATURE_YUV420_SCALER,
    gcvFEATURE_SHADER_HAS_W,
    gcvFEATURE_SHADER_HAS_SIGN,
    gcvFEATURE_SHADER_HAS_FLOOR,
    gcvFEATURE_SHADER_HAS_CEIL,
    gcvFEATURE_SHADER_HAS_SQRT,
    gcvFEATURE_SHADER_HAS_TRIG,
    gcvFEATURE_VAA,
    gcvFEATURE_HZ,
    gcvFEATURE_CORRECT_STENCIL,
    gcvFEATURE_VG20,
    gcvFEATURE_VG_FILTER,
    gcvFEATURE_VG21,
    gcvFEATURE_VG_DOUBLE_BUFFER,
    gcvFEATURE_MC20,
    gcvFEATURE_SUPER_TILED,
    gcvFEATURE_2D_FILTERBLIT_PLUS_ALPHABLEND,
    gcvFEATURE_2D_DITHER,
    gcvFEATURE_2D_A8_TARGET,
    gcvFEATURE_2D_FILTERBLIT_FULLROTATION,
    gcvFEATURE_2D_BITBLIT_FULLROTATION,
    gcvFEATURE_WIDE_LINE,
    gcvFEATURE_FC_FLUSH_STALL,
    gcvFEATURE_FULL_DIRECTFB,
    gcvFEATURE_HALF_FLOAT_PIPE,
    gcvFEATURE_LINE_LOOP,
    gcvFEATURE_2D_YUV_BLIT,
    gcvFEATURE_2D_TILING,
    gcvFEATURE_NON_POWER_OF_TWO,
    gcvFEATURE_3D_TEXTURE,
    gcvFEATURE_TEXTURE_ARRAY,
    gcvFEATURE_TILE_FILLER,
    gcvFEATURE_LOGIC_OP,
    gcvFEATURE_COMPOSITION,
    gcvFEATURE_MIXED_STREAMS,
    gcvFEATURE_2D_MULTI_SOURCE_BLT,
    gcvFEATURE_END_EVENT,
    gcvFEATURE_VERTEX_10_10_10_2,
    gcvFEATURE_TEXTURE_10_10_10_2,
    gcvFEATURE_TEXTURE_ANISOTROPIC_FILTERING,
    gcvFEATURE_TEXTURE_FLOAT_HALF_FLOAT,
	gcvFEATURE_2D_ROTATION_STALL_FIX,
    gcvFEATURE_2D_MULTI_SOURCE_BLT_EX,
	gcvFEATURE_BUG_FIXES10,
    gcvFEATURE_2D_MINOR_TILING,
    /* Supertiled compressed textures are supported. */
    gcvFEATURE_TEX_COMPRRESSION_SUPERTILED,
    gcvFEATURE_FAST_MSAA,
    gcvFEATURE_BUG_FIXED_INDEXED_TRIANGLE_STRIP,
    gcvFEATURE_TEXTURE_TILED_READ,
    gcvFEATURE_DEPTH_BIAS_FIX,
    gcvFEATURE_RECT_PRIMITIVE,
	gcvFEATURE_BUG_FIXES11,
	gcvFEATURE_SUPERTILED_TEXTURE,
    gcvFEATURE_2D_NO_COLORBRUSH_INDEX8,
    gcvFEATURE_RS_YUV_TARGET,
    gcvFEATURE_2D_FC_SOURCE,
	gcvFEATURE_PE_DITHER_FIX,
    gcvFEATURE_2D_YUV_SEPARATE_STRIDE,
    gcvFEATURE_FRUSTUM_CLIP_FIX,
    gcvFEATURE_TEXTURE_LINEAR,
    gcvFEATURE_TEXTURE_YUV_ASSEMBLER,
    gcvFEATURE_SHADER_HAS_INSTRUCTION_CACHE,
    gcvFEATURE_DYNAMIC_FREQUENCY_SCALING,
    gcvFEATURE_BUGFIX15,
    gcvFEATURE_2D_GAMMA,
    gcvFEATURE_2D_COLOR_SPACE_CONVERSION,
    gcvFEATURE_2D_SUPER_TILE_VERSION,
    gcvFEATURE_2D_MIRROR_EXTENSION,
    gcvFEATURE_2D_SUPER_TILE_V1,
    gcvFEATURE_2D_SUPER_TILE_V2,
    gcvFEATURE_2D_SUPER_TILE_V3,
    gcvFEATURE_2D_MULTI_SOURCE_BLT_EX2,
    gcvFEATURE_ELEMENT_INDEX_UINT,
    gcvFEATURE_2D_COMPRESSION,
    gcvFEATURE_2D_OPF_YUV_OUTPUT,
    gcvFEATURE_2D_MULTI_SRC_BLT_TO_UNIFIED_DST_RECT,
    gcvFEATURE_2D_YUV_MODE,
    gcvFEATURE_DECOMPRESS_Z16,
	gcvFEATURE_LINEAR_RENDER_TARGET,
    gcvFEATURE_BUG_FIXES8,
    gcvFEATURE_HALTI2,
}
gceFEATURE;

/* Chip Power Status. */
typedef enum _gceCHIPPOWERSTATE
{
    gcvPOWER_ON = 0,
    gcvPOWER_OFF,
    gcvPOWER_IDLE,
    gcvPOWER_SUSPEND,
    gcvPOWER_SUSPEND_ATPOWERON,
    gcvPOWER_OFF_ATPOWERON,
    gcvPOWER_IDLE_BROADCAST,
    gcvPOWER_SUSPEND_BROADCAST,
    gcvPOWER_OFF_BROADCAST,
    gcvPOWER_OFF_RECOVERY,
    gcvPOWER_OFF_TIMEOUT,
    gcvPOWER_ON_AUTO
}
gceCHIPPOWERSTATE;

/* CPU cache operations */
typedef enum _gceCACHEOPERATION
{
    gcvCACHE_CLEAN      = 0x01,
    gcvCACHE_INVALIDATE = 0x02,
    gcvCACHE_FLUSH      = gcvCACHE_CLEAN  | gcvCACHE_INVALIDATE,
    gcvCACHE_MEMORY_BARRIER = 0x04
}
gceCACHEOPERATION;

/* Surface types. */
typedef enum _gceSURF_TYPE
{
    gcvSURF_TYPE_UNKNOWN = 0,
    gcvSURF_INDEX,
    gcvSURF_VERTEX,
    gcvSURF_TEXTURE,
    gcvSURF_RENDER_TARGET,
    gcvSURF_DEPTH,
    gcvSURF_BITMAP,
    gcvSURF_TILE_STATUS,
	gcvSURF_IMAGE,
    gcvSURF_MASK,
    gcvSURF_SCISSOR,
    gcvSURF_HIERARCHICAL_DEPTH,
    gcvSURF_NUM_TYPES, /* Make sure this is the last one! */

    /* Combinations. */
    gcvSURF_NO_TILE_STATUS = 0x100,
    gcvSURF_NO_VIDMEM      = 0x200, /* Used to allocate surfaces with no underlying vidmem node.
                                       In Android, vidmem node is allocated by another process. */
    gcvSURF_CACHEABLE      = 0x400, /* Used to allocate a cacheable surface */
    gcvSURF_FLIP           = 0x800, /* The Resolve Target the will been flip resolve from RT */
    gcvSURF_TILE_STATUS_DIRTY  = 0x1000, /* Init tile status to all dirty */

    gcvSURF_LINEAR             = 0x2000,

    gcvSURF_TEXTURE_LINEAR               = gcvSURF_TEXTURE
                                         | gcvSURF_LINEAR,

    gcvSURF_RENDER_TARGET_NO_TILE_STATUS = gcvSURF_RENDER_TARGET
                                         | gcvSURF_NO_TILE_STATUS,

    gcvSURF_RENDER_TARGET_TS_DIRTY = gcvSURF_RENDER_TARGET
                                         | gcvSURF_TILE_STATUS_DIRTY,

    gcvSURF_DEPTH_NO_TILE_STATUS         = gcvSURF_DEPTH
                                         | gcvSURF_NO_TILE_STATUS,

    gcvSURF_DEPTH_TS_DIRTY               = gcvSURF_DEPTH
                                         | gcvSURF_TILE_STATUS_DIRTY,

    /* Supported surface types with no vidmem node. */
    gcvSURF_BITMAP_NO_VIDMEM             = gcvSURF_BITMAP
                                         | gcvSURF_NO_VIDMEM,

    gcvSURF_TEXTURE_NO_VIDMEM            = gcvSURF_TEXTURE
                                         | gcvSURF_NO_VIDMEM,

    /* Cacheable surface types with no vidmem node. */
    gcvSURF_CACHEABLE_BITMAP_NO_VIDMEM   = gcvSURF_BITMAP_NO_VIDMEM
                                         | gcvSURF_CACHEABLE,

    gcvSURF_CACHEABLE_BITMAP             = gcvSURF_BITMAP
                                         | gcvSURF_CACHEABLE,

    gcvSURF_FLIP_BITMAP                  = gcvSURF_BITMAP
                                         | gcvSURF_FLIP,
}
gceSURF_TYPE;

typedef enum _gceSURF_USAGE
{
    gcvSURF_USAGE_UNKNOWN,
    gcvSURF_USAGE_RESOLVE_AFTER_CPU,
    gcvSURF_USAGE_RESOLVE_AFTER_3D
}
gceSURF_USAGE;

typedef enum _gceSURF_COLOR_TYPE
{
    gcvSURF_COLOR_UNKNOWN = 0,
    gcvSURF_COLOR_LINEAR        = 0x01,
    gcvSURF_COLOR_ALPHA_PRE     = 0x02,
}
gceSURF_COLOR_TYPE;

/* Rotation. */
typedef enum _gceSURF_ROTATION
{
    gcvSURF_0_DEGREE = 0,
    gcvSURF_90_DEGREE,
    gcvSURF_180_DEGREE,
    gcvSURF_270_DEGREE,
    gcvSURF_FLIP_X,
    gcvSURF_FLIP_Y,

	gcvSURF_POST_FLIP_X = 0x40000000,
    gcvSURF_POST_FLIP_Y = 0x80000000,
}
gceSURF_ROTATION;

typedef enum _gceMIPMAP_IMAGE_FORMAT
{
    gcvUNKNOWN_MIPMAP_IMAGE_FORMAT  = -2
}
gceMIPMAP_IMAGE_FORMAT;


/* Surface formats. */
typedef enum _gceSURF_FORMAT
{
    /* Unknown format. */
    gcvSURF_UNKNOWN             = 0,

    /* Palettized formats. */
    gcvSURF_INDEX1              = 100,
    gcvSURF_INDEX4,
    gcvSURF_INDEX8,

    /* RGB formats. */
    gcvSURF_A2R2G2B2            = 200,
    gcvSURF_R3G3B2,
    gcvSURF_A8R3G3B2,
    gcvSURF_X4R4G4B4,
    gcvSURF_A4R4G4B4,
    gcvSURF_R4G4B4A4,
    gcvSURF_X1R5G5B5,
    gcvSURF_A1R5G5B5,
    gcvSURF_R5G5B5A1,
    gcvSURF_R5G6B5,
    gcvSURF_R8G8B8,
    gcvSURF_X8R8G8B8,
    gcvSURF_A8R8G8B8,
    gcvSURF_R8G8B8A8,
    gcvSURF_G8R8G8B8,
    gcvSURF_R8G8B8G8,
    gcvSURF_X2R10G10B10,
    gcvSURF_A2R10G10B10,
    gcvSURF_X12R12G12B12,
    gcvSURF_A12R12G12B12,
    gcvSURF_X16R16G16B16,
    gcvSURF_A16R16G16B16,
    gcvSURF_A32R32G32B32,
    gcvSURF_R8G8B8X8,
    gcvSURF_R5G5B5X1,
    gcvSURF_R4G4B4X4,

    /* BGR formats. */
    gcvSURF_A4B4G4R4            = 300,
    gcvSURF_A1B5G5R5,
    gcvSURF_B5G6R5,
    gcvSURF_B8G8R8,
    gcvSURF_B16G16R16,
    gcvSURF_X8B8G8R8,
    gcvSURF_A8B8G8R8,
    gcvSURF_A2B10G10R10,
    gcvSURF_X16B16G16R16,
    gcvSURF_A16B16G16R16,
    gcvSURF_B32G32R32,
    gcvSURF_X32B32G32R32,
    gcvSURF_A32B32G32R32,
    gcvSURF_B4G4R4A4,
    gcvSURF_B5G5R5A1,
    gcvSURF_B8G8R8X8,
    gcvSURF_B8G8R8A8,
    gcvSURF_X4B4G4R4,
    gcvSURF_X1B5G5R5,
    gcvSURF_B4G4R4X4,
    gcvSURF_B5G5R5X1,
    gcvSURF_X2B10G10R10,

    /* Compressed formats. */
    gcvSURF_DXT1                = 400,
    gcvSURF_DXT2,
    gcvSURF_DXT3,
    gcvSURF_DXT4,
    gcvSURF_DXT5,
    gcvSURF_CXV8U8,
    gcvSURF_ETC1,
    gcvSURF_R11_EAC,
    gcvSURF_SIGNED_R11_EAC,
    gcvSURF_RG11_EAC,
    gcvSURF_SIGNED_RG11_EAC,
    gcvSURF_RGB8_ETC2,
    gcvSURF_SRGB8_ETC2,
    gcvSURF_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,
    gcvSURF_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2,
    gcvSURF_RGBA8_ETC2_EAC,
    gcvSURF_SRGB8_ALPHA8_ETC2_EAC,

    /* YUV formats. */
    gcvSURF_YUY2                = 500,
    gcvSURF_UYVY,
    gcvSURF_YV12,
    gcvSURF_I420,
    gcvSURF_NV12,
    gcvSURF_NV21,
    gcvSURF_NV16,
    gcvSURF_NV61,
    gcvSURF_YVYU,
    gcvSURF_VYUY,

    /* Depth formats. */
    gcvSURF_D16                 = 600,
    gcvSURF_D24S8,
    gcvSURF_D32,
    gcvSURF_D24X8,

    /* Alpha formats. */
    gcvSURF_A4                  = 700,
    gcvSURF_A8,
    gcvSURF_A12,
    gcvSURF_A16,
    gcvSURF_A32,
    gcvSURF_A1,

    /* Luminance formats. */
    gcvSURF_L4                  = 800,
    gcvSURF_L8,
    gcvSURF_L12,
    gcvSURF_L16,
    gcvSURF_L32,
    gcvSURF_L1,

    /* Alpha/Luminance formats. */
    gcvSURF_A4L4                = 900,
    gcvSURF_A2L6,
    gcvSURF_A8L8,
    gcvSURF_A4L12,
    gcvSURF_A12L12,
    gcvSURF_A16L16,

    /* Bump formats. */
    gcvSURF_L6V5U5              = 1000,
    gcvSURF_V8U8,
    gcvSURF_X8L8V8U8,
    gcvSURF_Q8W8V8U8,
    gcvSURF_A2W10V10U10,
    gcvSURF_V16U16,
    gcvSURF_Q16W16V16U16,

    /* R/RG/RA formats. */
    gcvSURF_R8                  = 1100,
    gcvSURF_X8R8,
    gcvSURF_G8R8,
    gcvSURF_X8G8R8,
    gcvSURF_A8R8,
    gcvSURF_R16,
    gcvSURF_X16R16,
    gcvSURF_G16R16,
    gcvSURF_X16G16R16,
    gcvSURF_A16R16,
    gcvSURF_R32,
    gcvSURF_X32R32,
    gcvSURF_G32R32,
    gcvSURF_X32G32R32,
    gcvSURF_A32R32,
    gcvSURF_RG16,

    /* Floating point formats. */
    gcvSURF_R16F                = 1200,
    gcvSURF_X16R16F,
    gcvSURF_G16R16F,
    gcvSURF_X16G16R16F,
    gcvSURF_B16G16R16F,
    gcvSURF_X16B16G16R16F,
    gcvSURF_A16B16G16R16F,
    gcvSURF_R32F,
    gcvSURF_X32R32F,
    gcvSURF_G32R32F,
    gcvSURF_X32G32R32F,
    gcvSURF_B32G32R32F,
    gcvSURF_X32B32G32R32F,
    gcvSURF_A32B32G32R32F,
    gcvSURF_A16F,
    gcvSURF_L16F,
    gcvSURF_A16L16F,
    gcvSURF_A16R16F,
    gcvSURF_A32F,
    gcvSURF_L32F,
    gcvSURF_A32L32F,
    gcvSURF_A32R32F,

}
gceSURF_FORMAT;

/* Pixel swizzle modes. */
typedef enum _gceSURF_SWIZZLE
{
    gcvSURF_NOSWIZZLE = 0,
    gcvSURF_ARGB,
    gcvSURF_ABGR,
    gcvSURF_RGBA,
    gcvSURF_BGRA
}
gceSURF_SWIZZLE;

/* Transparency modes. */
typedef enum _gceSURF_TRANSPARENCY
{
    /* Valid only for PE 1.0 */
    gcvSURF_OPAQUE = 0,
    gcvSURF_SOURCE_MATCH,
    gcvSURF_SOURCE_MASK,
    gcvSURF_PATTERN_MASK,
}
gceSURF_TRANSPARENCY;

/* Surface Alignment. */
typedef enum _gceSURF_ALIGNMENT
{
    gcvSURF_FOUR = 0,
    gcvSURF_SIXTEEN,
    gcvSURF_SUPER_TILED,
    gcvSURF_SPLIT_TILED,
    gcvSURF_SPLIT_SUPER_TILED,
}
gceSURF_ALIGNMENT;


/* Surface Addressing. */
typedef enum _gceSURF_ADDRESSING
{
    gcvSURF_NO_STRIDE_TILED = 0,
    gcvSURF_NO_STRIDE_LINEAR,
    gcvSURF_STRIDE_TILED,
    gcvSURF_STRIDE_LINEAR
}
gceSURF_ADDRESSING;

/* Transparency modes. */
typedef enum _gce2D_TRANSPARENCY
{
    /* Valid only for PE 2.0 */
    gcv2D_OPAQUE = 0,
    gcv2D_KEYED,
    gcv2D_MASKED
}
gce2D_TRANSPARENCY;

/* Mono packing modes. */
typedef enum _gceSURF_MONOPACK
{
    gcvSURF_PACKED8 = 0,
    gcvSURF_PACKED16,
    gcvSURF_PACKED32,
    gcvSURF_UNPACKED,
}
gceSURF_MONOPACK;

/* Blending modes. */
typedef enum _gceSURF_BLEND_MODE
{
    /* Porter-Duff blending modes.                   */
    /*                         Fsrc      Fdst        */
    gcvBLEND_CLEAR = 0,     /* 0         0           */
    gcvBLEND_SRC,           /* 1         0           */
    gcvBLEND_DST,           /* 0         1           */
    gcvBLEND_SRC_OVER_DST,  /* 1         1 - Asrc    */
    gcvBLEND_DST_OVER_SRC,  /* 1 - Adst  1           */
    gcvBLEND_SRC_IN_DST,    /* Adst      0           */
    gcvBLEND_DST_IN_SRC,    /* 0         Asrc        */
    gcvBLEND_SRC_OUT_DST,   /* 1 - Adst  0           */
    gcvBLEND_DST_OUT_SRC,   /* 0         1 - Asrc    */
    gcvBLEND_SRC_ATOP_DST,  /* Adst      1 - Asrc    */
    gcvBLEND_DST_ATOP_SRC,  /* 1 - Adst  Asrc        */
    gcvBLEND_SRC_XOR_DST,   /* 1 - Adst  1 - Asrc    */

    /* Special blending modes.                       */
    gcvBLEND_SET,           /* DST = 1               */
    gcvBLEND_SUB            /* DST = DST * (1 - SRC) */
}
gceSURF_BLEND_MODE;

/* Per-pixel alpha modes. */
typedef enum _gceSURF_PIXEL_ALPHA_MODE
{
    gcvSURF_PIXEL_ALPHA_STRAIGHT = 0,
    gcvSURF_PIXEL_ALPHA_INVERSED
}
gceSURF_PIXEL_ALPHA_MODE;

/* Global alpha modes. */
typedef enum _gceSURF_GLOBAL_ALPHA_MODE
{
    gcvSURF_GLOBAL_ALPHA_OFF = 0,
    gcvSURF_GLOBAL_ALPHA_ON,
    gcvSURF_GLOBAL_ALPHA_SCALE
}
gceSURF_GLOBAL_ALPHA_MODE;

/* Color component modes for alpha blending. */
typedef enum _gceSURF_PIXEL_COLOR_MODE
{
    gcvSURF_COLOR_STRAIGHT = 0,
    gcvSURF_COLOR_MULTIPLY
}
gceSURF_PIXEL_COLOR_MODE;

/* Color component modes for alpha blending. */
typedef enum _gce2D_PIXEL_COLOR_MULTIPLY_MODE
{
    gcv2D_COLOR_MULTIPLY_DISABLE = 0,
    gcv2D_COLOR_MULTIPLY_ENABLE
}
gce2D_PIXEL_COLOR_MULTIPLY_MODE;

/* Color component modes for alpha blending. */
typedef enum _gce2D_GLOBAL_COLOR_MULTIPLY_MODE
{
    gcv2D_GLOBAL_COLOR_MULTIPLY_DISABLE = 0,
    gcv2D_GLOBAL_COLOR_MULTIPLY_ALPHA,
    gcv2D_GLOBAL_COLOR_MULTIPLY_COLOR
}
gce2D_GLOBAL_COLOR_MULTIPLY_MODE;

/* Alpha blending factor modes. */
typedef enum _gceSURF_BLEND_FACTOR_MODE
{
    gcvSURF_BLEND_ZERO = 0,
    gcvSURF_BLEND_ONE,
    gcvSURF_BLEND_STRAIGHT,
    gcvSURF_BLEND_INVERSED,
    gcvSURF_BLEND_COLOR,
    gcvSURF_BLEND_COLOR_INVERSED,
    gcvSURF_BLEND_SRC_ALPHA_SATURATED,
    gcvSURF_BLEND_STRAIGHT_NO_CROSS,
    gcvSURF_BLEND_INVERSED_NO_CROSS,
    gcvSURF_BLEND_COLOR_NO_CROSS,
    gcvSURF_BLEND_COLOR_INVERSED_NO_CROSS,
    gcvSURF_BLEND_SRC_ALPHA_SATURATED_CROSS
}
gceSURF_BLEND_FACTOR_MODE;

/* Alpha blending porter duff rules. */
typedef enum _gce2D_PORTER_DUFF_RULE
{
    gcvPD_CLEAR = 0,
    gcvPD_SRC,
    gcvPD_SRC_OVER,
    gcvPD_DST_OVER,
    gcvPD_SRC_IN,
    gcvPD_DST_IN,
    gcvPD_SRC_OUT,
    gcvPD_DST_OUT,
    gcvPD_SRC_ATOP,
    gcvPD_DST_ATOP,
    gcvPD_ADD,
    gcvPD_XOR,
    gcvPD_DST
}
gce2D_PORTER_DUFF_RULE;

/* Alpha blending factor modes. */
typedef enum _gce2D_YUV_COLOR_MODE
{
    gcv2D_YUV_601= 0,
    gcv2D_YUV_709,
    gcv2D_YUV_USER_DEFINED,
    gcv2D_YUV_USER_DEFINED_CLAMP,

    /* Default setting is for src. gcv2D_YUV_DST
        can be ORed to set dst.
    */
    gcv2D_YUV_DST = 0x80000000,
}
gce2D_YUV_COLOR_MODE;

typedef enum _gce2D_COMMAND
{
    gcv2D_CLEAR = 0,
    gcv2D_LINE,
    gcv2D_BLT,
    gcv2D_STRETCH,
    gcv2D_HOR_FILTER,
    gcv2D_VER_FILTER,
    gcv2D_MULTI_SOURCE_BLT,
}
gce2D_COMMAND;

typedef enum _gce2D_TILE_STATUS_CONFIG
{
    gcv2D_TSC_DISABLE       = 0,
    gcv2D_TSC_ENABLE        = 0x00000001,
    gcv2D_TSC_COMPRESSED    = 0x00000002,
    gcv2D_TSC_DOWN_SAMPLER  = 0x00000004,
    gcv2D_TSC_2D_COMPRESSED = 0x00000008,
}
gce2D_TILE_STATUS_CONFIG;

typedef enum _gce2D_QUERY
{
    gcv2D_QUERY_RGB_ADDRESS_MIN_ALIGN       = 0,
    gcv2D_QUERY_RGB_STRIDE_MIN_ALIGN,
    gcv2D_QUERY_YUV_ADDRESS_MIN_ALIGN,
    gcv2D_QUERY_YUV_STRIDE_MIN_ALIGN,
}
gce2D_QUERY;

typedef enum _gce2D_SUPER_TILE_VERSION
{
    gcv2D_SUPER_TILE_VERSION_V1       = 1,
    gcv2D_SUPER_TILE_VERSION_V2       = 2,
    gcv2D_SUPER_TILE_VERSION_V3       = 3,
}
gce2D_SUPER_TILE_VERSION;

typedef enum _gce2D_STATE
{
    gcv2D_STATE_SPECIAL_FILTER_MIRROR_MODE       = 1,
    gcv2D_STATE_SUPER_TILE_VERSION,
    gcv2D_STATE_EN_GAMMA,
    gcv2D_STATE_DE_GAMMA,
    gcv2D_STATE_MULTI_SRC_BLIT_UNIFIED_DST_RECT,

    gcv2D_STATE_ARRAY_EN_GAMMA                   = 0x10001,
    gcv2D_STATE_ARRAY_DE_GAMMA,
    gcv2D_STATE_ARRAY_CSC_YUV_TO_RGB,
    gcv2D_STATE_ARRAY_CSC_RGB_TO_YUV,
}
gce2D_STATE;

#ifndef VIVANTE_NO_3D
/* Texture functions. */
typedef enum _gceTEXTURE_FUNCTION
{
    gcvTEXTURE_DUMMY = 0,
    gcvTEXTURE_REPLACE = 0,
    gcvTEXTURE_MODULATE,
    gcvTEXTURE_ADD,
    gcvTEXTURE_ADD_SIGNED,
    gcvTEXTURE_INTERPOLATE,
    gcvTEXTURE_SUBTRACT,
    gcvTEXTURE_DOT3
}
gceTEXTURE_FUNCTION;

/* Texture sources. */
typedef enum _gceTEXTURE_SOURCE
{
    gcvCOLOR_FROM_TEXTURE = 0,
    gcvCOLOR_FROM_CONSTANT_COLOR,
    gcvCOLOR_FROM_PRIMARY_COLOR,
    gcvCOLOR_FROM_PREVIOUS_COLOR
}
gceTEXTURE_SOURCE;

/* Texture source channels. */
typedef enum _gceTEXTURE_CHANNEL
{
    gcvFROM_COLOR = 0,
    gcvFROM_ONE_MINUS_COLOR,
    gcvFROM_ALPHA,
    gcvFROM_ONE_MINUS_ALPHA
}
gceTEXTURE_CHANNEL;
#endif /* VIVANTE_NO_3D */

/* Filter types. */
typedef enum _gceFILTER_TYPE
{
    gcvFILTER_SYNC = 0,
    gcvFILTER_BLUR,
    gcvFILTER_USER
}
gceFILTER_TYPE;

/* Filter pass types. */
typedef enum _gceFILTER_PASS_TYPE
{
    gcvFILTER_HOR_PASS = 0,
    gcvFILTER_VER_PASS
}
gceFILTER_PASS_TYPE;

/* Endian hints. */
typedef enum _gceENDIAN_HINT
{
    gcvENDIAN_NO_SWAP = 0,
    gcvENDIAN_SWAP_WORD,
    gcvENDIAN_SWAP_DWORD
}
gceENDIAN_HINT;

/* Tiling modes. */
typedef enum _gceTILING
{
    gcvLINEAR = 0,
    gcvTILED,
    gcvSUPERTILED,
    gcvMULTI_TILED,
    gcvMULTI_SUPERTILED,
    gcvMINORTILED,
}
gceTILING;

/* 2D pattern type. */
typedef enum _gce2D_PATTERN
{
    gcv2D_PATTERN_SOLID = 0,
    gcv2D_PATTERN_MONO,
    gcv2D_PATTERN_COLOR,
    gcv2D_PATTERN_INVALID
}
gce2D_PATTERN;

/* 2D source type. */
typedef enum _gce2D_SOURCE
{
    gcv2D_SOURCE_MASKED = 0,
    gcv2D_SOURCE_MONO,
    gcv2D_SOURCE_COLOR,
    gcv2D_SOURCE_INVALID
}
gce2D_SOURCE;

/* Pipes. */
typedef enum _gcePIPE_SELECT
{
    gcvPIPE_INVALID = ~0,
    gcvPIPE_3D      =  0,
    gcvPIPE_2D
}
gcePIPE_SELECT;

/* Hardware type. */
typedef enum _gceHARDWARE_TYPE
{
    gcvHARDWARE_INVALID = 0x00,
    gcvHARDWARE_3D      = 0x01,
    gcvHARDWARE_2D      = 0x02,
    gcvHARDWARE_VG      = 0x04,

    gcvHARDWARE_3D2D    = gcvHARDWARE_3D | gcvHARDWARE_2D
}
gceHARDWARE_TYPE;

#define gcdCHIP_COUNT               3

typedef enum _gceMMU_MODE
{
    gcvMMU_MODE_1K,
    gcvMMU_MODE_4K,
} gceMMU_MODE;

/* User signal command codes. */
typedef enum _gceUSER_SIGNAL_COMMAND_CODES
{
    gcvUSER_SIGNAL_CREATE,
    gcvUSER_SIGNAL_DESTROY,
    gcvUSER_SIGNAL_SIGNAL,
    gcvUSER_SIGNAL_WAIT,
    gcvUSER_SIGNAL_MAP,
    gcvUSER_SIGNAL_UNMAP,
}
gceUSER_SIGNAL_COMMAND_CODES;

/* Sync point command codes. */
typedef enum _gceSYNC_POINT_COMMAND_CODES
{
    gcvSYNC_POINT_CREATE,
    gcvSYNC_POINT_DESTROY,
    gcvSYNC_POINT_SIGNAL,
}
gceSYNC_POINT_COMMAND_CODES;

/* Event locations. */
typedef enum _gceKERNEL_WHERE
{
    gcvKERNEL_COMMAND,
    gcvKERNEL_VERTEX,
    gcvKERNEL_TRIANGLE,
    gcvKERNEL_TEXTURE,
    gcvKERNEL_PIXEL,
}
gceKERNEL_WHERE;

#if gcdENABLE_VG
/* Hardware blocks. */
typedef enum _gceBLOCK
{
	gcvBLOCK_COMMAND,
	gcvBLOCK_TESSELLATOR,
	gcvBLOCK_TESSELLATOR2,
	gcvBLOCK_TESSELLATOR3,
	gcvBLOCK_RASTER,
	gcvBLOCK_VG,
	gcvBLOCK_VG2,
	gcvBLOCK_VG3,
	gcvBLOCK_PIXEL,

	/* Number of defined blocks. */
	gcvBLOCK_COUNT
}
gceBLOCK;
#endif

/* gcdDUMP message type. */
typedef enum _gceDEBUG_MESSAGE_TYPE
{
    gcvMESSAGE_TEXT,
    gcvMESSAGE_DUMP
}
gceDEBUG_MESSAGE_TYPE;

typedef enum _gceSPECIAL_HINT
{
    gceSPECIAL_HINT0,
    gceSPECIAL_HINT1,
    gceSPECIAL_HINT2,
    gceSPECIAL_HINT3,
    /* For disable dynamic stream/index */
    gceSPECIAL_HINT4
}
gceSPECIAL_HINT;

typedef enum _gceMACHINECODE
{
    gcvMACHINECODE_HOVERJET0       = 0x0,
    gcvMACHINECODE_HOVERJET1      ,

    gcvMACHINECODE_TAIJI0         ,
    gcvMACHINECODE_TAIJI1         ,
    gcvMACHINECODE_TAIJI2         ,

    gcvMACHINECODE_ANTUTU0        ,

    gcvMACHINECODE_GLB27_RELEASE_0,
    gcvMACHINECODE_GLB27_RELEASE_1,

    gcvMACHINECODE_WAVESCAPE0     ,
    gcvMACHINECODE_WAVESCAPE1     ,

    gcvMACHINECODE_NENAMARKV2_4_0 ,
    gcvMACHINECODE_NENAMARKV2_4_1 ,

    gcvMACHINECODE_GLB25_RELEASE_0,
    gcvMACHINECODE_GLB25_RELEASE_1,
    gcvMACHINECODE_GLB25_RELEASE_2,
}
gceMACHINECODE;


/******************************************************************************\
****************************** Object Declarations *****************************
\******************************************************************************/

typedef struct _gckCONTEXT          * gckCONTEXT;
typedef struct _gcoCMDBUF           * gcoCMDBUF;
typedef struct _gcsSTATE_DELTA      * gcsSTATE_DELTA_PTR;
typedef struct _gcsQUEUE            * gcsQUEUE_PTR;
typedef struct _gcoQUEUE            * gcoQUEUE;
typedef struct _gcsHAL_INTERFACE    * gcsHAL_INTERFACE_PTR;
typedef struct _gcs2D_PROFILE       * gcs2D_PROFILE_PTR;

#if gcdENABLE_VG
typedef struct _gcoVGHARDWARE *			gcoVGHARDWARE;
typedef struct _gcoVGBUFFER *           gcoVGBUFFER;
typedef struct _gckVGHARDWARE *         gckVGHARDWARE;
typedef struct _gcsVGCONTEXT *			gcsVGCONTEXT_PTR;
typedef struct _gcsVGCONTEXT_MAP *		gcsVGCONTEXT_MAP_PTR;
typedef struct _gcsVGCMDQUEUE *			gcsVGCMDQUEUE_PTR;
typedef struct _gcsTASK_MASTER_TABLE *	gcsTASK_MASTER_TABLE_PTR;
typedef struct _gckVGKERNEL *			gckVGKERNEL;
typedef void *					        gctTHREAD;
#endif

#ifdef __cplusplus
}
#endif

#endif /* __gc_hal_enum_h_ */
