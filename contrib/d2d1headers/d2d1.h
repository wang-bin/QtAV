/*

	d2d1.h - Header file for the Direct2D API

	No original Microsoft headers were used in the creation of this
	file.
  
  2013.02.19 --
  Interface vtables have been organized by reverse-engineering
  the original order in Microsoft's d2d1.h using the following
  process:
  - Read Microsoft's "d2d1.h" on a Windows 8 computer with the
    Visual Studio Windows SDK installed, and manually write down
    the order of vtable functions in a reference text file.
  - Open the reference text file on a separate computer without
    the Windows SDK, and reorder the functions in this
    MinGW-compatible "d2d1.h" file according to the reference
    file.
  See also:
  http://blog.2of1.org/2010/09/20/use-of-microsoft-windows-sdk-headers-as-foss-reference-ok/
*/

#ifndef _D2D1_H
#define _D2D1_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#include <objbase.h>

#include <d2dbasetypes.h>
#include <d2derr.h>

#ifndef _COM_interface
#define _COM_interface struct
#endif

typedef UINT64 D2D1_TAG;

#define D2D1_DEFAULT_FLATTENING_TOLERANCE 0.25f
#define D2D1_INVALID_TAG ULONGLONG_MAX

/* enumerations */

/* todo - remove when d3d10 headers become available */
typedef enum D3D10_FEATURE_LEVEL1 {
  D3D10_FEATURE_LEVEL_10_0   = 0xa000,
  D3D10_FEATURE_LEVEL_10_1   = 0xa100,
  D3D10_FEATURE_LEVEL_9_1    = 0x9100,
  D3D10_FEATURE_LEVEL_9_2    = 0x9200,
  D3D10_FEATURE_LEVEL_9_3    = 0x9300 
} D3D10_FEATURE_LEVEL1;
/* */

/* todo - remove when dxgi.h becomes available */
typedef enum DXGI_FORMAT {
  DXGI_FORMAT_UNKNOWN                      = 0,
  DXGI_FORMAT_R32G32B32A32_TYPELESS        = 1,
  DXGI_FORMAT_R32G32B32A32_FLOAT           = 2,
  DXGI_FORMAT_R32G32B32A32_UINT            = 3,
  DXGI_FORMAT_R32G32B32A32_SINT            = 4,
  DXGI_FORMAT_R32G32B32_TYPELESS           = 5,
  DXGI_FORMAT_R32G32B32_FLOAT              = 6,
  DXGI_FORMAT_R32G32B32_UINT               = 7,
  DXGI_FORMAT_R32G32B32_SINT               = 8,
  DXGI_FORMAT_R16G16B16A16_TYPELESS        = 9,
  DXGI_FORMAT_R16G16B16A16_FLOAT           = 10,
  DXGI_FORMAT_R16G16B16A16_UNORM           = 11,
  DXGI_FORMAT_R16G16B16A16_UINT            = 12,
  DXGI_FORMAT_R16G16B16A16_SNORM           = 13,
  DXGI_FORMAT_R16G16B16A16_SINT            = 14,
  DXGI_FORMAT_R32G32_TYPELESS              = 15,
  DXGI_FORMAT_R32G32_FLOAT                 = 16,
  DXGI_FORMAT_R32G32_UINT                  = 17,
  DXGI_FORMAT_R32G32_SINT                  = 18,
  DXGI_FORMAT_R32G8X24_TYPELESS            = 19,
  DXGI_FORMAT_D32_FLOAT_S8X24_UINT         = 20,
  DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS     = 21,
  DXGI_FORMAT_X32_TYPELESS_G8X24_UINT      = 22,
  DXGI_FORMAT_R10G10B10A2_TYPELESS         = 23,
  DXGI_FORMAT_R10G10B10A2_UNORM            = 24,
  DXGI_FORMAT_R10G10B10A2_UINT             = 25,
  DXGI_FORMAT_R11G11B10_FLOAT              = 26,
  DXGI_FORMAT_R8G8B8A8_TYPELESS            = 27,
  DXGI_FORMAT_R8G8B8A8_UNORM               = 28,
  DXGI_FORMAT_R8G8B8A8_UNORM_SRGB          = 29,
  DXGI_FORMAT_R8G8B8A8_UINT                = 30,
  DXGI_FORMAT_R8G8B8A8_SNORM               = 31,
  DXGI_FORMAT_R8G8B8A8_SINT                = 32,
  DXGI_FORMAT_R16G16_TYPELESS              = 33,
  DXGI_FORMAT_R16G16_FLOAT                 = 34,
  DXGI_FORMAT_R16G16_UNORM                 = 35,
  DXGI_FORMAT_R16G16_UINT                  = 36,
  DXGI_FORMAT_R16G16_SNORM                 = 37,
  DXGI_FORMAT_R16G16_SINT                  = 38,
  DXGI_FORMAT_R32_TYPELESS                 = 39,
  DXGI_FORMAT_D32_FLOAT                    = 40,
  DXGI_FORMAT_R32_FLOAT                    = 41,
  DXGI_FORMAT_R32_UINT                     = 42,
  DXGI_FORMAT_R32_SINT                     = 43,
  DXGI_FORMAT_R24G8_TYPELESS               = 44,
  DXGI_FORMAT_D24_UNORM_S8_UINT            = 45,
  DXGI_FORMAT_R24_UNORM_X8_TYPELESS        = 46,
  DXGI_FORMAT_X24_TYPELESS_G8_UINT         = 47,
  DXGI_FORMAT_R8G8_TYPELESS                = 48,
  DXGI_FORMAT_R8G8_UNORM                   = 49,
  DXGI_FORMAT_R8G8_UINT                    = 50,
  DXGI_FORMAT_R8G8_SNORM                   = 51,
  DXGI_FORMAT_R8G8_SINT                    = 52,
  DXGI_FORMAT_R16_TYPELESS                 = 53,
  DXGI_FORMAT_R16_FLOAT                    = 54,
  DXGI_FORMAT_D16_UNORM                    = 55,
  DXGI_FORMAT_R16_UNORM                    = 56,
  DXGI_FORMAT_R16_UINT                     = 57,
  DXGI_FORMAT_R16_SNORM                    = 58,
  DXGI_FORMAT_R16_SINT                     = 59,
  DXGI_FORMAT_R8_TYPELESS                  = 60,
  DXGI_FORMAT_R8_UNORM                     = 61,
  DXGI_FORMAT_R8_UINT                      = 62,
  DXGI_FORMAT_R8_SNORM                     = 63,
  DXGI_FORMAT_R8_SINT                      = 64,
  DXGI_FORMAT_A8_UNORM                     = 65,
  DXGI_FORMAT_R1_UNORM                     = 66,
  DXGI_FORMAT_R9G9B9E5_SHAREDEXP           = 67,
  DXGI_FORMAT_R8G8_B8G8_UNORM              = 68,
  DXGI_FORMAT_G8R8_G8B8_UNORM              = 69,
  DXGI_FORMAT_BC1_TYPELESS                 = 70,
  DXGI_FORMAT_BC1_UNORM                    = 71,
  DXGI_FORMAT_BC1_UNORM_SRGB               = 72,
  DXGI_FORMAT_BC2_TYPELESS                 = 73,
  DXGI_FORMAT_BC2_UNORM                    = 74,
  DXGI_FORMAT_BC2_UNORM_SRGB               = 75,
  DXGI_FORMAT_BC3_TYPELESS                 = 76,
  DXGI_FORMAT_BC3_UNORM                    = 77,
  DXGI_FORMAT_BC3_UNORM_SRGB               = 78,
  DXGI_FORMAT_BC4_TYPELESS                 = 79,
  DXGI_FORMAT_BC4_UNORM                    = 80,
  DXGI_FORMAT_BC4_SNORM                    = 81,
  DXGI_FORMAT_BC5_TYPELESS                 = 82,
  DXGI_FORMAT_BC5_UNORM                    = 83,
  DXGI_FORMAT_BC5_SNORM                    = 84,
  DXGI_FORMAT_B5G6R5_UNORM                 = 85,
  DXGI_FORMAT_B5G5R5A1_UNORM               = 86,
  DXGI_FORMAT_B8G8R8A8_UNORM               = 87,
  DXGI_FORMAT_B8G8R8X8_UNORM               = 88,
  DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM   = 89,
  DXGI_FORMAT_B8G8R8A8_TYPELESS            = 90,
  DXGI_FORMAT_B8G8R8A8_UNORM_SRGB          = 91,
  DXGI_FORMAT_B8G8R8X8_TYPELESS            = 92,
  DXGI_FORMAT_B8G8R8X8_UNORM_SRGB          = 93,
  DXGI_FORMAT_BC6H_TYPELESS                = 94,
  DXGI_FORMAT_BC6H_UF16                    = 95,
  DXGI_FORMAT_BC6H_SF16                    = 96,
  DXGI_FORMAT_BC7_TYPELESS                 = 97,
  DXGI_FORMAT_BC7_UNORM                    = 98,
  DXGI_FORMAT_BC7_UNORM_SRGB               = 99,
  DXGI_FORMAT_FORCE_UINT                   = 0xffffffffUL 
} DXGI_FORMAT, *LPDXGI_FORMAT;
/* */

/* todo - remove other temporary defines for unavailable interfaces */
#define IWICBitmapSource void
#define IDWriteRenderingParams void
#define IDXGISurface void
#define IWICBitmap void
#define IDWriteTextFormat void
#define IDWriteTextLayout void
#define IDWriteFontFace void
/* */

/* todo - remove when dwrite headers becomes available */
typedef enum DWRITE_MEASURING_MODE {
  DWRITE_MEASURING_MODE_NATURAL,
  DWRITE_MEASURING_MODE_GDI_CLASSIC,
  DWRITE_MEASURING_MODE_GDI_NATURAL 
} DWRITE_MEASURING_MODE;

typedef struct DWRITE_GLYPH_OFFSET {
  FLOAT advanceOffset;
  FLOAT ascenderOffset;
} DWRITE_GLYPH_OFFSET;

typedef struct DWRITE_GLYPH_RUN {
  IDWriteFontFace           *fontFace;
  FLOAT                     fontEmSize;
  UINT32                    glyphCount;
  const short               *glyphIndices;
  const FLOAT               *glyphAdvances;
  const DWRITE_GLYPH_OFFSET *glyphOffsets;
  BOOL                      isSideways;
  UINT32                    bidiLevel;
} DWRITE_GLYPH_RUN;
/* */

typedef enum  {
  D2D1_ALPHA_MODE_UNKNOWN         = 0,
  D2D1_ALPHA_MODE_PREMULTIPLIED   = 1,
  D2D1_ALPHA_MODE_STRAIGHT        = 2,
  D2D1_ALPHA_MODE_IGNORE          = 3 
} D2D1_ALPHA_MODE;

typedef enum  {
  D2D1_ANTIALIAS_MODE_PER_PRIMITIVE   = 0,
  D2D1_ANTIALIAS_MODE_ALIASED         = 1 
} D2D1_ANTIALIAS_MODE;

typedef enum  {
  D2D1_ARC_SIZE_SMALL   = 0,
  D2D1_ARC_SIZE_LARGE   = 1 
} D2D1_ARC_SIZE;

typedef enum  {
  D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR   = 0,
  D2D1_BITMAP_INTERPOLATION_MODE_LINEAR             = 1 
} D2D1_BITMAP_INTERPOLATION_MODE;

typedef enum  {
  D2D1_CAP_STYLE_FLAT       = 0,
  D2D1_CAP_STYLE_SQUARE     = 1,
  D2D1_CAP_STYLE_ROUND      = 2,
  D2D1_CAP_STYLE_TRIANGLE   = 3 
} D2D1_CAP_STYLE;

typedef enum  {
  D2D1_COMBINE_MODE_UNION       = 0,
  D2D1_COMBINE_MODE_INTERSECT   = 1,
  D2D1_COMBINE_MODE_XOR         = 2,
  D2D1_COMBINE_MODE_EXCLUDE     = 3 
} D2D1_COMBINE_MODE;

typedef enum  {
  D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE             = 0x00000000,
  D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_GDI_COMPATIBLE   = 0x00000001 
} D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS;

typedef enum  {
  D2D1_DASH_STYLE_SOLID          = 0,
  D2D1_DASH_STYLE_DASH           = 1,
  D2D1_DASH_STYLE_DOT            = 2,
  D2D1_DASH_STYLE_DASH_DOT       = 3,
  D2D1_DASH_STYLE_DASH_DOT_DOT   = 4,
  D2D1_DASH_STYLE_CUSTOM         = 5 
} D2D1_DASH_STYLE;

typedef enum  {
  D2D1_DC_INITIALIZE_MODE_COPY    = 0,
  D2D1_DC_INITIALIZE_MODE_CLEAR   = 1 
} D2D1_DC_INITIALIZE_MODE;

typedef enum  {
  D2D1_DEBUG_LEVEL_NONE          = 0,
  D2D1_DEBUG_LEVEL_ERROR         = 1,
  D2D1_DEBUG_LEVEL_WARNING       = 2,
  D2D1_DEBUG_LEVEL_INFORMATION   = 3 
} D2D1_DEBUG_LEVEL;

typedef enum  {
  D2D1_DRAW_TEXT_OPTIONS_NO_SNAP   = 0x00000001,
  D2D1_DRAW_TEXT_OPTIONS_CLIP      = 0x00000002,
  D2D1_DRAW_TEXT_OPTIONS_NONE      = 0x00000000 
} D2D1_DRAW_TEXT_OPTIONS;

typedef enum  {
  D2D1_EXTEND_MODE_CLAMP    = 0,
  D2D1_EXTEND_MODE_WRAP     = 1,
  D2D1_EXTEND_MODE_MIRROR   = 2 
} D2D1_EXTEND_MODE;

typedef enum  {
  D2D1_FACTORY_TYPE_SINGLE_THREADED   = 0,
  D2D1_FACTORY_TYPE_MULTI_THREADED    = 1 
} D2D1_FACTORY_TYPE;

typedef enum  {
  D2D1_FEATURE_LEVEL_DEFAULT   = 0,
  D2D1_FEATURE_LEVEL_9         = D3D10_FEATURE_LEVEL_9_1,
  D2D1_FEATURE_LEVEL_10        = D3D10_FEATURE_LEVEL_10_0 
} D2D1_FEATURE_LEVEL;

typedef enum  {
  D2D1_FIGURE_BEGIN_FILLED   = 0,
  D2D1_FIGURE_BEGIN_HOLLOW   = 1 
} D2D1_FIGURE_BEGIN;

typedef enum  {
  D2D1_FIGURE_END_OPEN     = 0,
  D2D1_FIGURE_END_CLOSED   = 1 
} D2D1_FIGURE_END;

typedef enum  {
  D2D1_FILL_MODE_ALTERNATE   = 0,
  D2D1_FILL_MODE_WINDING     = 1 
} D2D1_FILL_MODE;

typedef enum  {
  D2D1_GAMMA_2_2   = 0,
  D2D1_GAMMA_1_0   = 1 
} D2D1_GAMMA;

typedef enum  {
  D2D1_GEOMETRY_RELATION_UNKNOWN        = 0,
  D2D1_GEOMETRY_RELATION_DISJOINT       = 1,
  D2D1_GEOMETRY_RELATION_IS_CONTAINED   = 2,
  D2D1_GEOMETRY_RELATION_CONTAINS       = 3,
  D2D1_GEOMETRY_RELATION_OVERLAP        = 4 
} D2D1_GEOMETRY_RELATION;

typedef enum  {
  D2D1_GEOMETRY_SIMPLIFICATION_OPTION_CUBICS_AND_LINES   = 0,
  D2D1_GEOMETRY_SIMPLIFICATION_OPTION_LINES              = 1 
} D2D1_GEOMETRY_SIMPLIFICATION_OPTION;

typedef enum  {
  D2D1_LAYER_OPTIONS_NONE                       = 0x00000000,
  D2D1_LAYER_OPTIONS_INITIALIZE_FOR_CLEARTYPE   = 0x00000001 
} D2D1_LAYER_OPTIONS;

typedef enum  {
  D2D1_LINE_JOIN_MITER            = 0,
  D2D1_LINE_JOIN_BEVEL            = 1,
  D2D1_LINE_JOIN_ROUND            = 2,
  D2D1_LINE_JOIN_MITER_OR_BEVEL   = 3 
} D2D1_LINE_JOIN;

typedef enum  {
  D2D1_OPACITY_MASK_CONTENT_GRAPHICS              = 0,
  D2D1_OPACITY_MASK_CONTENT_TEXT_NATURAL          = 1,
  D2D1_OPACITY_MASK_CONTENT_TEXT_GDI_COMPATIBLE   = 2 
} D2D1_OPACITY_MASK_CONTENT;

typedef enum  {
  D2D1_PATH_SEGMENT_NONE                    = 0x00000000,
  D2D1_PATH_SEGMENT_FORCE_UNSTROKED         = 0x00000001,
  D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN   = 0x00000002 
} D2D1_PATH_SEGMENT;

typedef enum  {
  D2D1_PRESENT_OPTIONS_NONE              = 0x00000000,
  D2D1_PRESENT_OPTIONS_RETAIN_CONTENTS   = 0x00000001,
  D2D1_PRESENT_OPTIONS_IMMEDIATELY       = 0x00000002 
} D2D1_PRESENT_OPTIONS;

typedef enum  {
  D2D1_RENDER_TARGET_TYPE_DEFAULT     = 0,
  D2D1_RENDER_TARGET_TYPE_SOFTWARE    = 1,
  D2D1_RENDER_TARGET_TYPE_HARDWARE    = 2 
} D2D1_RENDER_TARGET_TYPE;

typedef enum  {
  D2D1_RENDER_TARGET_USAGE_NONE                    = 0x00000000,
  D2D1_RENDER_TARGET_USAGE_FORCE_BITMAP_REMOTING   = 0x00000001,
  D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE          = 0x00000002 
} D2D1_RENDER_TARGET_USAGE;

typedef enum  {
  D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE   = 0,
  D2D1_SWEEP_DIRECTION_CLOCKWISE           = 1 
} D2D1_SWEEP_DIRECTION;

typedef enum  {
  D2D1_TEXT_ANTIALIAS_MODE_DEFAULT     = 0,
  D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE   = 1,
  D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE   = 2,
  D2D1_TEXT_ANTIALIAS_MODE_ALIASED     = 3 
} D2D1_TEXT_ANTIALIAS_MODE;

typedef enum  {
  D2D1_WINDOW_STATE_NONE       = 0x0000000,
  D2D1_WINDOW_STATE_OCCLUDED   = 0x0000001 
} D2D1_WINDOW_STATE;

/* interface forward declares */

typedef _COM_interface ID2D1Bitmap ID2D1Bitmap;
typedef _COM_interface ID2D1BitmapBrush ID2D1BitmapBrush;
typedef _COM_interface ID2D1BitmapRenderTarget ID2D1BitmapRenderTarget;
typedef _COM_interface ID2D1Brush ID2D1Brush;
typedef _COM_interface ID2D1DCRenderTarget ID2D1DCRenderTarget;
typedef _COM_interface ID2D1DrawingStateBlock ID2D1DrawingStateBlock;
typedef _COM_interface ID2D1EllipseGeometry ID2D1EllipseGeometry;
typedef _COM_interface ID2D1Factory ID2D1Factory;
typedef _COM_interface ID2D1GdiInteropRenderTarget ID2D1GdiInteropRenderTarget;
typedef _COM_interface ID2D1Geometry ID2D1Geometry;
typedef _COM_interface ID2D1GeometryGroup ID2D1GeometryGroup;
typedef _COM_interface ID2D1GeometrySink ID2D1GeometrySink;
typedef _COM_interface ID2D1GradientStopCollection ID2D1GradientStopCollection;
typedef _COM_interface ID2D1HwndRenderTarget ID2D1HwndRenderTarget;
typedef _COM_interface ID2D1Layer ID2D1Layer;
typedef _COM_interface ID2D1LinearGradientBrush ID2D1LinearGradientBrush;
typedef _COM_interface ID2D1Mesh ID2D1Mesh;
typedef _COM_interface ID2D1PathGeometry ID2D1PathGeometry;
typedef _COM_interface ID2D1RadialGradientBrush ID2D1RadialGradientBrush;
typedef _COM_interface ID2D1RectangleGeometry ID2D1RectangleGeometry;
typedef _COM_interface ID2D1RenderTarget ID2D1RenderTarget;
typedef _COM_interface ID2D1Resource ID2D1Resource;
typedef _COM_interface ID2D1RoundedRectangleGeometry ID2D1RoundedRectangleGeometry;
typedef _COM_interface ID2D1SimplifiedGeometrySink ID2D1SimplifiedGeometrySink;
typedef _COM_interface ID2D1SolidColorBrush ID2D1SolidColorBrush;
typedef _COM_interface ID2D1StrokeStyle ID2D1StrokeStyle;
typedef _COM_interface ID2D1TessellationSink ID2D1TessellationSink;
typedef _COM_interface ID2D1TransformedGeometry ID2D1TransformedGeometry;
typedef _COM_interface IUnknown IUnknown;

/* structures */

typedef struct D2D_MATRIX_3X2_F D2D1_MATRIX_3X2_F;

typedef struct D2D1_ARC_SEGMENT D2D1_ARC_SEGMENT;
typedef struct D2D1_BEZIER_SEGMENT D2D1_BEZIER_SEGMENT;
typedef struct D2D1_BITMAP_BRUSH_PROPERTIES D2D1_BITMAP_BRUSH_PROPERTIES;
typedef struct D2D1_BITMAP_PROPERTIES D2D1_BITMAP_PROPERTIES;
typedef struct D2D1_BRUSH_PROPERTIES D2D1_BRUSH_PROPERTIES;
typedef struct D2D1_DRAWING_STATE_DESCRIPTION D2D1_DRAWING_STATE_DESCRIPTION;
typedef struct D2D1_ELLIPSE D2D1_ELLIPSE;
typedef struct D2D1_FACTORY_OPTIONS D2D1_FACTORY_OPTIONS;
typedef struct D2D1_GRADIENT_STOP D2D1_GRADIENT_STOP;
typedef struct D2D1_HWND_RENDER_TARGET_PROPERTIES D2D1_HWND_RENDER_TARGET_PROPERTIES;
typedef struct D2D1_LAYER_PARAMETERS D2D1_LAYER_PARAMETERS;
typedef struct D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES;
typedef struct D2D1_PIXEL_FORMAT D2D1_PIXEL_FORMAT;
typedef struct D2D1_QUADRATIC_BEZIER_SEGMENT D2D1_QUADRATIC_BEZIER_SEGMENT;
typedef struct D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES;
typedef struct D2D1_RENDER_TARGET_PROPERTIES D2D1_RENDER_TARGET_PROPERTIES;
typedef struct D2D1_ROUNDED_RECT D2D1_ROUNDED_RECT;
typedef struct D2D1_STROKE_STYLE_PROPERTIES D2D1_STROKE_STYLE_PROPERTIES;
typedef struct D2D1_TRIANGLE D2D1_TRIANGLE;

struct D2D1_ARC_SEGMENT {
  D2D1_POINT_2F        point;
  D2D1_SIZE_F          size;
  FLOAT                rotationAngle;
  D2D1_SWEEP_DIRECTION sweepDirection;
  D2D1_ARC_SIZE        arcSize;
};

struct D2D1_BEZIER_SEGMENT {
  D2D1_POINT_2F point1;
  D2D1_POINT_2F point2;
  D2D1_POINT_2F point3;
};

struct D2D1_BITMAP_BRUSH_PROPERTIES {
  D2D1_EXTEND_MODE               extendModeX;
  D2D1_EXTEND_MODE               extendModeY;
  D2D1_BITMAP_INTERPOLATION_MODE interpolationMode;
};

struct D2D1_PIXEL_FORMAT {
  DXGI_FORMAT     format;
  D2D1_ALPHA_MODE alphaMode;
};

struct D2D1_BITMAP_PROPERTIES {
  D2D1_PIXEL_FORMAT pixelFormat;
  FLOAT             dpiX;
  FLOAT             dpiY;
};

struct D2D1_BRUSH_PROPERTIES {
  FLOAT             opacity;
  D2D1_MATRIX_3X2_F transform;
};

struct D2D1_DRAWING_STATE_DESCRIPTION {
  D2D1_ANTIALIAS_MODE      antialiasMode;
  D2D1_TEXT_ANTIALIAS_MODE textAntialiasMode;
  D2D1_TAG                 tag1;
  D2D1_TAG                 tag2;
  D2D1_MATRIX_3X2_F        transform;
};

struct D2D1_ELLIPSE {
  D2D1_POINT_2F point;
  FLOAT         radiusX;
  FLOAT         radiusY;
};

struct D2D1_FACTORY_OPTIONS {
  D2D1_DEBUG_LEVEL debugLevel;
};

struct D2D1_GRADIENT_STOP {
  FLOAT        position;
  D2D1_COLOR_F color;
};

struct D2D1_HWND_RENDER_TARGET_PROPERTIES {
  HWND                 hwnd;
  D2D1_SIZE_U          pixelSize;
  D2D1_PRESENT_OPTIONS presentOptions;
};

struct D2D1_LAYER_PARAMETERS {
  D2D1_RECT_F         contentBounds;
  ID2D1Geometry       *geometricMask;
  D2D1_ANTIALIAS_MODE maskAntialiasMode;
  D2D1_MATRIX_3X2_F   maskTransform;
  FLOAT               opacity;
  ID2D1Brush          *opacityBrush;
  D2D1_LAYER_OPTIONS  layerOptions;
};

struct D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES {
  D2D1_POINT_2F startPoint;
  D2D1_POINT_2F endPoint;
};

struct D2D1_QUADRATIC_BEZIER_SEGMENT {
  D2D1_POINT_2F point1;
  D2D1_POINT_2F point2;
};

struct D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES {
  D2D1_POINT_2F center;
  D2D1_POINT_2F gradientOriginOffset;
  FLOAT         radiusX;
  FLOAT         radiusY;
};

struct D2D1_RENDER_TARGET_PROPERTIES {
  D2D1_RENDER_TARGET_TYPE  type;
  D2D1_PIXEL_FORMAT        pixelFormat;
  FLOAT                    dpiX;
  FLOAT                    dpiY;
  D2D1_RENDER_TARGET_USAGE usage;
  D2D1_FEATURE_LEVEL       minLevel;
};

struct D2D1_ROUNDED_RECT {
  D2D1_RECT_F rect;
  FLOAT       radiusX;
  FLOAT       radiusY;
};

struct D2D1_STROKE_STYLE_PROPERTIES {
  D2D1_CAP_STYLE  startCap;
  D2D1_CAP_STYLE  endCap;
  D2D1_CAP_STYLE  dashCap;
  D2D1_LINE_JOIN  lineJoin;
  FLOAT           miterLimit;
  D2D1_DASH_STYLE dashStyle;
  FLOAT           dashOffset;
};

struct D2D1_TRIANGLE {
  D2D1_POINT_2F point1;
  D2D1_POINT_2F point2;
  D2D1_POINT_2F point3;
};

/* interfaces */

/**
 * Header generated from msdn for the purposes of allowing 
 * 3rd party compiler compatibility with the Microsoft API 
 */

#define INTERFACE ID2D1Resource
DECLARE_INTERFACE_(ID2D1Resource, IUnknown)
{
  BEGIN_INTERFACE

  /* IUnknown methods */
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;

  /* ID2D1Resource methods */
  STDMETHOD_(void, GetFactory)(THIS_ ID2D1Factory **factory) PURE;

  END_INTERFACE
};
#undef INTERFACE

#define ID2D1Resource_QueryInterface(this,A,B) (this)->lpVtbl->QueryInterface(this,A,B)
#define ID2D1Resource_AddRef(this) (this)->lpVtbl->AddRef(this)
#define ID2D1Resource_Release(this) (this)->lpVtbl->Release(this)
#define ID2D1Resource_GetFactory(this,A) (this)->lpVtbl->GetFactory(this,A)

#define INTERFACE ID2D1Bitmap
DECLARE_INTERFACE_(ID2D1Bitmap, ID2D1Resource)
{
  BEGIN_INTERFACE

  /* IUnknown methods */
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;

  /* ID2D1Resource methods */
  STDMETHOD_(void, GetFactory)(THIS_ ID2D1Factory **factory) PURE;

  /* ID2D1Bitmap methods */
  STDMETHOD_(D2D1_SIZE_F, GetSize)(THIS) PURE;
  STDMETHOD_(D2D1_SIZE_U, GetPixelSize)(THIS) PURE;
  STDMETHOD_(D2D1_PIXEL_FORMAT, GetPixelFormat)(THIS) PURE;
  STDMETHOD_(void, GetDpi)(THIS_ FLOAT *dpiX, FLOAT *dpiY) PURE;
  STDMETHOD(CopyFromBitmap)(THIS_ D2D1_POINT_2U *destPoint, ID2D1Bitmap *bitmap, D2D1_RECT_U *srcRect) PURE;
  STDMETHOD(CopyFromRenderTarget)(THIS_ D2D1_POINT_2U *destPoint, ID2D1RenderTarget *renderTarget, D2D1_RECT_U *srcRect) PURE;
  STDMETHOD(CopyFromMemory)(THIS_ D2D1_RECT_U *dstRect, const void *srcData, UINT32 pitch) PURE;

  END_INTERFACE
};
#undef INTERFACE

#define ID2D1Bitmap_QueryInterface(this,A,B) (this)->lpVtbl->QueryInterface(this,A,B)
#define ID2D1Bitmap_AddRef(this) (this)->lpVtbl->AddRef(this)
#define ID2D1Bitmap_Release(this) (this)->lpVtbl->Release(this)
#define ID2D1Bitmap_GetFactory(this,A) (this)->lpVtbl->GetFactory(this,A)
#define ID2D1Bitmap_CopyFromBitmap(this,A,B,C) (this)->lpVtbl->CopyFromBitmap(this,A,B,C)
#define ID2D1Bitmap_CopyFromRenderTarget(this,A,B,C) (this)->lpVtbl->CopyFromRenderTarget(this,A,B,C)
#define ID2D1Bitmap_CopyFromMemory(this,A,B,C) (this)->lpVtbl->CopyFromMemory(this,A,B,C)
#define ID2D1Bitmap_GetDpi(this,A,B) (this)->lpVtbl->GetDpi(this,A,B)
#define ID2D1Bitmap_GetPixelFormat(this) (this)->lpVtbl->GetPixelFormat(this)
#define ID2D1Bitmap_GetPixelSize(this) (this)->lpVtbl->GetPixelSize(this)
#define ID2D1Bitmap_GetSize(this) (this)->lpVtbl->GetSize(this)

#define INTERFACE ID2D1Brush
DECLARE_INTERFACE_(ID2D1Brush, ID2D1Resource)
{
  BEGIN_INTERFACE

  /* IUnknown methods */
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;

  /* ID2D1Resource methods */
  STDMETHOD_(void, GetFactory)(THIS_ ID2D1Factory **factory) PURE;

  /* ID2D1Brush methods */
  STDMETHOD_(void, SetOpacity)(THIS_ FLOAT opacity) PURE;
  STDMETHOD_(void, SetTransform)(THIS_ D2D1_MATRIX_3X2_F *transform) PURE;
  STDMETHOD_(FLOAT, GetOpacity)(THIS) PURE;
  STDMETHOD_(void, GetTransform)(THIS_ D2D1_MATRIX_3X2_F *transform) PURE;

  END_INTERFACE
};
#undef INTERFACE

#define ID2D1Brush_QueryInterface(this,A,B) (this)->lpVtbl->QueryInterface(this,A,B)
#define ID2D1Brush_AddRef(this) (this)->lpVtbl->AddRef(this)
#define ID2D1Brush_Release(this) (this)->lpVtbl->Release(this)
#define ID2D1Brush_GetFactory(this,A) (this)->lpVtbl->GetFactory(this,A)
#define ID2D1Brush_GetOpacity(this) (this)->lpVtbl->GetOpacity(this)
#define ID2D1Brush_GetTransform(this,A) (this)->lpVtbl->GetTransform(this,A)
#define ID2D1Brush_SetOpacity(this,A) (this)->lpVtbl->SetOpacity(this,A)
#define ID2D1Brush_SetTransform(this,A) (this)->lpVtbl->SetTransform(this,A)

#define INTERFACE ID2D1BitmapBrush
DECLARE_INTERFACE_(ID2D1BitmapBrush, ID2D1Brush)
{
  BEGIN_INTERFACE

  /* IUnknown methods */
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;

  /* ID2D1Resource methods */
  STDMETHOD_(void, GetFactory)(THIS_ ID2D1Factory **factory) PURE;

  /* ID2D1Brush methods */
  STDMETHOD_(void, SetOpacity)(THIS_ FLOAT opacity) PURE;
  STDMETHOD_(void, SetTransform)(THIS_ D2D1_MATRIX_3X2_F *transform) PURE;
  STDMETHOD_(FLOAT, GetOpacity)(THIS) PURE;
  STDMETHOD_(void, GetTransform)(THIS_ D2D1_MATRIX_3X2_F *transform) PURE;

  /* ID2D1BitmapBrush methods */
  STDMETHOD_(void, SetExtendModeX)(THIS_ D2D1_EXTEND_MODE extendModeX) PURE;
  STDMETHOD_(void, SetExtendModeY)(THIS_ D2D1_EXTEND_MODE extendModeY) PURE;
  STDMETHOD_(void, SetInterpolationMode)(THIS_ D2D1_BITMAP_INTERPOLATION_MODE interpolationMode) PURE;
  STDMETHOD_(void, SetBitmap)(THIS_ ID2D1Bitmap *bitmap) PURE;
  STDMETHOD_(D2D1_EXTEND_MODE, GetExtendModeX)(THIS) PURE;
  STDMETHOD_(D2D1_EXTEND_MODE, GetExtendModeY)(THIS) PURE;
  STDMETHOD_(D2D1_BITMAP_INTERPOLATION_MODE, GetInterpolationMode)(THIS) PURE;
  STDMETHOD_(void, GetBitmap)(THIS_ ID2D1Bitmap **bitmap) PURE;

  END_INTERFACE
};
#undef INTERFACE

#define ID2D1BitmapBrush_GetOpacity(this) (this)->lpVtbl->GetOpacity(this)
#define ID2D1BitmapBrush_GetTransform(this,A) (this)->lpVtbl->GetTransform(this,A)
#define ID2D1BitmapBrush_SetOpacity(this,A) (this)->lpVtbl->SetOpacity(this,A)
#define ID2D1BitmapBrush_SetTransform(this,A) (this)->lpVtbl->SetTransform(this,A)
#define ID2D1BitmapBrush_GetBitmap(this,A) (this)->lpVtbl->GetBitmap(this,A)
#define ID2D1BitmapBrush_GetExtendModeX(this) (this)->lpVtbl->GetExtendModeX(this)
#define ID2D1BitmapBrush_GetExtendModeY(this) (this)->lpVtbl->GetExtendModeY(this)
#define ID2D1BitmapBrush_GetInterpolationMode(this) (this)->lpVtbl->GetInterpolationMode(this)
#define ID2D1BitmapBrush_SetBitmap(this,A) (this)->lpVtbl->SetBitmap(this,A)
#define ID2D1BitmapBrush_SetExtendModeX(this,A) (this)->lpVtbl->SetExtendModeX(this,A)
#define ID2D1BitmapBrush_SetExtendModeY(this,A) (this)->lpVtbl->SetExtendModeY(this,A)
#define ID2D1BitmapBrush_SetInterpolationMode(this,A) (this)->lpVtbl->SetInterpolationMode(this,A)

#define INTERFACE ID2D1RenderTarget
DECLARE_INTERFACE_(ID2D1RenderTarget, ID2D1Resource)
{
  BEGIN_INTERFACE

  /* IUnknown methods */
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;

  /* ID2D1Resource methods */
  STDMETHOD_(void, GetFactory)(THIS_ ID2D1Factory **factory) PURE;

  /* ID2D1RenderTarget methods */
  STDMETHOD(CreateBitmap)(THIS_ D2D1_SIZE_U size, void *srcData, UINT32 pitch, D2D1_BITMAP_PROPERTIES *bitmapProperties, ID2D1Bitmap **bitmap) PURE;
  STDMETHOD(CreateBitmapFromWicBitmap)(THIS_ IWICBitmapSource *wicBitmapSource, ID2D1Bitmap **bitmap) PURE;
  STDMETHOD(CreateSharedBitmap)(THIS_ REFIID riid, void *data, D2D1_BITMAP_PROPERTIES *bitmapProperties, ID2D1Bitmap **bitmap) PURE;
  STDMETHOD(CreateBitmapBrush)(THIS_ ID2D1Bitmap *bitmap, ID2D1BitmapBrush **bitmapBrush) PURE;
  STDMETHOD(CreateSolidColorBrush)(THIS_ D2D1_COLOR_F *color, D2D1_BRUSH_PROPERTIES *brushProperties, ID2D1SolidColorBrush **solidColorBrush) PURE;
  STDMETHOD(CreateGradientStopCollection)(THIS_ D2D1_GRADIENT_STOP *gradientStops, UINT gradientStopsCount, ID2D1GradientStopCollection **gradientStopCollection) PURE;
  STDMETHOD(CreateLinearGradientBrush)(THIS_ D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES *linearGradientBrushProperties, D2D1_BRUSH_PROPERTIES *brushProperties, ID2D1GradientStopCollection *gradientStopCollection, ID2D1LinearGradientBrush **linearGradientBrush) PURE;
  STDMETHOD(CreateRadialGradientBrush)(THIS_ D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES *radialGradientBrushProperties, D2D1_BRUSH_PROPERTIES *brushProperties, ID2D1GradientStopCollection *gradientStopCollection, ID2D1RadialGradientBrush **radialGradientBrush) PURE;
  STDMETHOD(CreateCompatibleRenderTarget)(THIS_ ID2D1BitmapRenderTarget **bitmapRenderTarget) PURE;
  STDMETHOD(CreateLayer)(THIS_ ID2D1Layer **layer) PURE;
  STDMETHOD(CreateMesh)(THIS_ ID2D1Mesh **mesh) PURE;
  STDMETHOD_(void, DrawLine)(THIS_ D2D1_POINT_2F point0, D2D1_POINT_2F point1, ID2D1Brush *brush, FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle) PURE;
  STDMETHOD_(void, DrawRectangle)(THIS_ D2D1_RECT_F *rect, ID2D1Brush *brush, FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle) PURE;
  STDMETHOD_(void, FillRectangle)(THIS_ D2D1_RECT_F *rect, ID2D1Brush *brush) PURE;
  STDMETHOD_(void, DrawRoundedRectangle)(THIS_ D2D1_ROUNDED_RECT *roundedRect, ID2D1Brush *brush, FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle) PURE;
  STDMETHOD_(void, FillRoundedRectangle)(THIS_ D2D1_ROUNDED_RECT *roundedRect, ID2D1Brush *brush) PURE;
  STDMETHOD_(void, DrawEllipse)(THIS_ D2D1_ELLIPSE *ellipse, ID2D1Brush *brush, FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle) PURE;
  STDMETHOD_(void, FillEllipse)(THIS_ D2D1_ELLIPSE *ellipse, ID2D1Brush *brush) PURE;
  STDMETHOD_(void, DrawGeometry)(THIS_ ID2D1Geometry *geometry, ID2D1Brush *brush, FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle) PURE;
  STDMETHOD_(void, FillGeometry)(THIS_ ID2D1Geometry *geometry, ID2D1Brush *brush, ID2D1Brush *opacityBrush) PURE;
  STDMETHOD_(void, FillMesh)(THIS_ ID2D1Mesh *mesh, ID2D1Brush *brush) PURE;
  STDMETHOD_(void, FillOpacityMask)(THIS_ ID2D1Bitmap *opacityMask, ID2D1Brush *brush, D2D1_OPACITY_MASK_CONTENT content, D2D1_RECT_F *destinationRectangle, D2D1_RECT_F *sourceRectangle) PURE;
  STDMETHOD_(void, DrawBitmap)(THIS_ ID2D1Bitmap *bitmap, D2D1_RECT_F *destinationRectangle, FLOAT opacity, D2D1_BITMAP_INTERPOLATION_MODE interpolationMode , D2D1_RECT_F *sourceRectangle) PURE;
  STDMETHOD_(void, DrawText)(THIS_ WCHAR *string, UINT stringLength, IDWriteTextFormat *textFormat, D2D1_RECT_F *layoutRect, ID2D1Brush *defaultForegroundBrush, D2D1_DRAW_TEXT_OPTIONS options , DWRITE_MEASURING_MODE measuringMode) PURE;
  STDMETHOD_(void, DrawTextLayout)(THIS_ D2D1_POINT_2F origin, IDWriteTextLayout *textLayout, ID2D1Brush *defaultForegroundBrush, D2D1_DRAW_TEXT_OPTIONS options) PURE;
  STDMETHOD_(void, DrawGlyphRun)(THIS_ D2D1_POINT_2F baselineOrigin, DWRITE_GLYPH_RUN *glyphRun, ID2D1Brush *foregroundBrush, DWRITE_MEASURING_MODE measuringMode) PURE;
  STDMETHOD_(void, SetTransform)(THIS_ D2D1_MATRIX_3X2_F *transform) PURE;
  STDMETHOD_(void, GetTransform)(THIS_ D2D1_MATRIX_3X2_F *transform) PURE;
  STDMETHOD_(void, SetAntialiasMode)(THIS_ D2D1_ANTIALIAS_MODE antialiasMode) PURE;
  STDMETHOD_(D2D1_ANTIALIAS_MODE, GetAntialiasMode)(THIS) PURE;
  STDMETHOD_(void, SetTextAntialiasMode)(THIS_ D2D1_TEXT_ANTIALIAS_MODE textAntialiasMode) PURE;
  STDMETHOD_(D2D1_TEXT_ANTIALIAS_MODE, GetTextAntialiasMode)(THIS) PURE;
  STDMETHOD_(void, SetTextRenderingParams)(THIS_ IDWriteRenderingParams *textRenderingParams) PURE;
  STDMETHOD_(void, GetTextRenderingParams)(THIS_ IDWriteRenderingParams **textRenderingParams) PURE;
  STDMETHOD_(void, SetTags)(THIS_ D2D1_TAG tag1, D2D1_TAG tag2) PURE;
  STDMETHOD_(void, GetTags)(THIS_ D2D1_TAG *tag1, D2D1_TAG *tag2) PURE;
  STDMETHOD_(void, PushLayer)(THIS_ D2D1_LAYER_PARAMETERS *layerParameters, ID2D1Layer *layer) PURE;
  STDMETHOD_(void, PopLayer)(THIS) PURE;
  STDMETHOD(Flush)(THIS_ D2D1_TAG *tag1, D2D1_TAG *tag2) PURE;
  STDMETHOD_(void, SaveDrawingState)(THIS_ ID2D1DrawingStateBlock *drawingStateBlock) PURE;
  STDMETHOD_(void, RestoreDrawingState)(THIS_ ID2D1DrawingStateBlock *drawingStateBlock) PURE;
  STDMETHOD_(void, PushAxisAlignedClip)(THIS_ D2D1_RECT_F *clipRect, D2D1_ANTIALIAS_MODE antialiasMode) PURE;
  STDMETHOD_(void, PopAxisAlignedClip)(THIS) PURE;
  STDMETHOD_(void, Clear)(THIS_ D2D1_COLOR_F *clearColor) PURE;
  STDMETHOD_(void, BeginDraw)(THIS) PURE;
  STDMETHOD(EndDraw)(THIS_ D2D1_TAG *tag1, D2D1_TAG *tag2) PURE;
  STDMETHOD_(D2D1_PIXEL_FORMAT, GetPixelFormat)(THIS) PURE;
  STDMETHOD_(void, SetDpi)(THIS_ FLOAT dpiX, FLOAT dpiY) PURE;
  STDMETHOD_(void, GetDpi)(THIS_ FLOAT *dpiX, FLOAT *dpiY) PURE;
  STDMETHOD_(D2D1_SIZE_F, GetSize)(THIS) PURE;
  STDMETHOD_(D2D1_SIZE_U, GetPixelSize)(THIS) PURE;
  STDMETHOD_(UINT32, GetMaximumBitmapSize)(THIS) PURE;
  STDMETHOD_(BOOL, IsSupported)(THIS_ D2D1_RENDER_TARGET_PROPERTIES *renderTargetProperties) PURE;

  END_INTERFACE
};
#undef INTERFACE

#define ID2D1RenderTarget_QueryInterface(this,A,B) (this)->lpVtbl->QueryInterface(this,A,B)
#define ID2D1RenderTarget_AddRef(this) (this)->lpVtbl->AddRef(this)
#define ID2D1RenderTarget_Release(this) (this)->lpVtbl->Release(this)
#define ID2D1RenderTarget_GetFactory(this,A) (this)->lpVtbl->GetFactory(this,A)
#define ID2D1RenderTarget_BeginDraw(this) (this)->lpVtbl->BeginDraw(this)
#define ID2D1RenderTarget_Clear(this,A) (this)->lpVtbl->Clear(this,A)
#define ID2D1RenderTarget_CreateBitmap(this,A,B,C,D,E) (this)->lpVtbl->CreateBitmap(this,A,B,C,D,E)
#define ID2D1RenderTarget_CreateBitmapBrush(this,A,B) (this)->lpVtbl->CreateBitmapBrush(this,A,B)
#define ID2D1RenderTarget_CreateBitmapFromWicBitmap(this,A,B) (this)->lpVtbl->CreateBitmapFromWicBitmap(this,A,B)
#define ID2D1RenderTarget_CreateCompatibleRenderTarget(this,A) (this)->lpVtbl->CreateCompatibleRenderTarget(this,A)
#define ID2D1RenderTarget_CreateGradientStopCollection(this,A,B,C) (this)->lpVtbl->CreateGradientStopCollection(this,A,B,C)
#define ID2D1RenderTarget_CreateLayer(this,A) (this)->lpVtbl->CreateLayer(this,A)
#define ID2D1RenderTarget_CreateLinearGradientBrush(this,A,B,C,D) (this)->lpVtbl->CreateLinearGradientBrush(this,A,B,C,D)
#define ID2D1RenderTarget_CreateMesh(this,A) (this)->lpVtbl->CreateMesh(this,A)
#define ID2D1RenderTarget_CreateRadialGradientBrush(this,A,B,C,D) (this)->lpVtbl->CreateRadialGradientBrush(this,A,B,C,D)
#define ID2D1RenderTarget_CreateSharedBitmap(this,A,B,C,D) (this)->lpVtbl->CreateSharedBitmap(this,A,B,C,D)
#define ID2D1RenderTarget_CreateSolidColorBrush(this,A,B,C) (this)->lpVtbl->CreateSolidColorBrush(this,A,B,C)
#define ID2D1RenderTarget_DrawBitmap(this,A,B,C,D,E) (this)->lpVtbl->DrawBitmap(this,A,B,C,D,E)
#define ID2D1RenderTarget_DrawEllipse(this,A,B,C,D) (this)->lpVtbl->DrawEllipse(this,A,B,C,D)
#define ID2D1RenderTarget_DrawGeometry(this,A,B,C,D) (this)->lpVtbl->DrawGeometry(this,A,B,C,D)
#define ID2D1RenderTarget_DrawGlyphRun(this,A,B,C,D) (this)->lpVtbl->DrawGlyphRun(this,A,B,C,D)
#define ID2D1RenderTarget_DrawLine(this,A,B,C,D,E) (this)->lpVtbl->DrawLine(this,A,B,C,D,E)
#define ID2D1RenderTarget_DrawRectangle(this,A,B,C,D) (this)->lpVtbl->DrawRectangle(this,A,B,C,D)
#define ID2D1RenderTarget_DrawRoundedRectangle(this,A,B,C,D) (this)->lpVtbl->DrawRoundedRectangle(this,A,B,C,D)
#define ID2D1RenderTarget_DrawText(this,A,B,C,D,E,F,G) (this)->lpVtbl->DrawText(this,A,B,C,D,E,F,G)
#define ID2D1RenderTarget_DrawTextLayout(this,A,B,C,D) (this)->lpVtbl->DrawTextLayout(this,A,B,C,D)
#define ID2D1RenderTarget_EndDraw(this,A,B) (this)->lpVtbl->EndDraw(this,A,B)
#define ID2D1RenderTarget_FillEllipse(this,A,B) (this)->lpVtbl->FillEllipse(this,A,B)
#define ID2D1RenderTarget_FillGeometry(this,A,B,C) (this)->lpVtbl->FillGeometry(this,A,B,C)
#define ID2D1RenderTarget_FillMesh(this,A,B) (this)->lpVtbl->FillMesh(this,A,B)
#define ID2D1RenderTarget_FillOpacityMask(this,A,B,C,D,E) (this)->lpVtbl->FillOpacityMask(this,A,B,C,D,E)
#define ID2D1RenderTarget_FillRectangle(this,A,B) (this)->lpVtbl->FillRectangle(this,A,B)
#define ID2D1RenderTarget_FillRoundedRectangle(this,A,B) (this)->lpVtbl->FillRoundedRectangle(this,A,B)
#define ID2D1RenderTarget_Flush(this,A,B) (this)->lpVtbl->Flush(this,A,B)
#define ID2D1RenderTarget_GetAntialiasMode(this) (this)->lpVtbl->GetAntialiasMode(this)
#define ID2D1RenderTarget_GetDpi(this,A,B) (this)->lpVtbl->GetDpi(this,A,B)
#define ID2D1RenderTarget_GetMaximumBitmapSize(this) (this)->lpVtbl->GetMaximumBitmapSize(this)
#define ID2D1RenderTarget_GetPixelFormat(this) (this)->lpVtbl->GetPixelFormat(this)
#define ID2D1RenderTarget_GetPixelSize(this) (this)->lpVtbl->GetPixelSize(this)
#define ID2D1RenderTarget_GetSize(this) (this)->lpVtbl->GetSize(this)
#define ID2D1RenderTarget_GetTags(this,A,B) (this)->lpVtbl->GetTags(this,A,B)
#define ID2D1RenderTarget_GetTextAntialiasMode(this) (this)->lpVtbl->GetTextAntialiasMode(this)
#define ID2D1RenderTarget_GetTextRenderingParams(this,A) (this)->lpVtbl->GetTextRenderingParams(this,A)
#define ID2D1RenderTarget_GetTransform(this,A) (this)->lpVtbl->GetTransform(this,A)
#define ID2D1RenderTarget_IsSupported(this,A) (this)->lpVtbl->IsSupported(this,A)
#define ID2D1RenderTarget_PopAxisAlignedClip(this) (this)->lpVtbl->PopAxisAlignedClip(this)
#define ID2D1RenderTarget_PopLayer(this) (this)->lpVtbl->PopLayer(this)
#define ID2D1RenderTarget_PushAxisAlignedClip(this,A,B) (this)->lpVtbl->PushAxisAlignedClip(this,A,B)
#define ID2D1RenderTarget_PushLayer(this,A,B) (this)->lpVtbl->PushLayer(this,A,B)
#define ID2D1RenderTarget_RestoreDrawingState(this,A) (this)->lpVtbl->RestoreDrawingState(this,A)
#define ID2D1RenderTarget_SaveDrawingState(this,A) (this)->lpVtbl->SaveDrawingState(this,A)
#define ID2D1RenderTarget_SetAntialiasMode(this,A) (this)->lpVtbl->SetAntialiasMode(this,A)
#define ID2D1RenderTarget_SetDpi(this,A,B) (this)->lpVtbl->SetDpi(this,A,B)
#define ID2D1RenderTarget_SetTags(this,A,B) (this)->lpVtbl->SetTags(this,A,B)
#define ID2D1RenderTarget_SetTextAntialiasMode(this,A) (this)->lpVtbl->SetTextAntialiasMode(this,A)
#define ID2D1RenderTarget_SetTextRenderingParams(this,A) (this)->lpVtbl->SetTextRenderingParams(this,A)
#define ID2D1RenderTarget_SetTransform(this,A) (this)->lpVtbl->SetTransform(this,A)

#define INTERFACE ID2D1BitmapRenderTarget
DECLARE_INTERFACE_(ID2D1BitmapRenderTarget, ID2D1RenderTarget)
{
  BEGIN_INTERFACE

  /* IUnknown methods */
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;

  /* ID2D1Resource methods */
  STDMETHOD_(void, GetFactory)(THIS_ ID2D1Factory **factory) PURE;

  /* ID2D1RenderTarget methods */
  STDMETHOD(CreateBitmap)(THIS_ D2D1_SIZE_U size, void *srcData, UINT32 pitch, D2D1_BITMAP_PROPERTIES *bitmapProperties, ID2D1Bitmap **bitmap) PURE;
  STDMETHOD(CreateBitmapFromWicBitmap)(THIS_ IWICBitmapSource *wicBitmapSource, ID2D1Bitmap **bitmap) PURE;
  STDMETHOD(CreateSharedBitmap)(THIS_ REFIID riid, void *data, D2D1_BITMAP_PROPERTIES *bitmapProperties, ID2D1Bitmap **bitmap) PURE;
  STDMETHOD(CreateBitmapBrush)(THIS_ ID2D1Bitmap *bitmap, ID2D1BitmapBrush **bitmapBrush) PURE;
  STDMETHOD(CreateSolidColorBrush)(THIS_ D2D1_COLOR_F *color, D2D1_BRUSH_PROPERTIES *brushProperties, ID2D1SolidColorBrush **solidColorBrush) PURE;
  STDMETHOD(CreateGradientStopCollection)(THIS_ D2D1_GRADIENT_STOP *gradientStops, UINT gradientStopsCount, ID2D1GradientStopCollection **gradientStopCollection) PURE;
  STDMETHOD(CreateLinearGradientBrush)(THIS_ D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES *linearGradientBrushProperties, D2D1_BRUSH_PROPERTIES *brushProperties, ID2D1GradientStopCollection *gradientStopCollection, ID2D1LinearGradientBrush **linearGradientBrush) PURE;
  STDMETHOD(CreateRadialGradientBrush)(THIS_ D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES *radialGradientBrushProperties, D2D1_BRUSH_PROPERTIES *brushProperties, ID2D1GradientStopCollection *gradientStopCollection, ID2D1RadialGradientBrush **radialGradientBrush) PURE;
  STDMETHOD(CreateCompatibleRenderTarget)(THIS_ ID2D1BitmapRenderTarget **bitmapRenderTarget) PURE;
  STDMETHOD(CreateLayer)(THIS_ ID2D1Layer **layer) PURE;
  STDMETHOD(CreateMesh)(THIS_ ID2D1Mesh **mesh) PURE;
  STDMETHOD_(void, DrawLine)(THIS_ D2D1_POINT_2F point0, D2D1_POINT_2F point1, ID2D1Brush *brush, FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle) PURE;
  STDMETHOD_(void, DrawRectangle)(THIS_ D2D1_RECT_F *rect, ID2D1Brush *brush, FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle) PURE;
  STDMETHOD_(void, FillRectangle)(THIS_ D2D1_RECT_F *rect, ID2D1Brush *brush) PURE;
  STDMETHOD_(void, DrawRoundedRectangle)(THIS_ D2D1_ROUNDED_RECT *roundedRect, ID2D1Brush *brush, FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle) PURE;
  STDMETHOD_(void, FillRoundedRectangle)(THIS_ D2D1_ROUNDED_RECT *roundedRect, ID2D1Brush *brush) PURE;
  STDMETHOD_(void, DrawEllipse)(THIS_ D2D1_ELLIPSE *ellipse, ID2D1Brush *brush, FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle) PURE;
  STDMETHOD_(void, FillEllipse)(THIS_ D2D1_ELLIPSE *ellipse, ID2D1Brush *brush) PURE;
  STDMETHOD_(void, DrawGeometry)(THIS_ ID2D1Geometry *geometry, ID2D1Brush *brush, FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle) PURE;
  STDMETHOD_(void, FillGeometry)(THIS_ ID2D1Geometry *geometry, ID2D1Brush *brush, ID2D1Brush *opacityBrush) PURE;
  STDMETHOD_(void, FillMesh)(THIS_ ID2D1Mesh *mesh, ID2D1Brush *brush) PURE;
  STDMETHOD_(void, FillOpacityMask)(THIS_ ID2D1Bitmap *opacityMask, ID2D1Brush *brush, D2D1_OPACITY_MASK_CONTENT content, D2D1_RECT_F *destinationRectangle, D2D1_RECT_F *sourceRectangle) PURE;
  STDMETHOD_(void, DrawBitmap)(THIS_ ID2D1Bitmap *bitmap, D2D1_RECT_F *destinationRectangle, FLOAT opacity, D2D1_BITMAP_INTERPOLATION_MODE interpolationMode , D2D1_RECT_F *sourceRectangle) PURE;
  STDMETHOD_(void, DrawText)(THIS_ WCHAR *string, UINT stringLength, IDWriteTextFormat *textFormat, D2D1_RECT_F *layoutRect, ID2D1Brush *defaultForegroundBrush, D2D1_DRAW_TEXT_OPTIONS options , DWRITE_MEASURING_MODE measuringMode) PURE;
  STDMETHOD_(void, DrawTextLayout)(THIS_ D2D1_POINT_2F origin, IDWriteTextLayout *textLayout, ID2D1Brush *defaultForegroundBrush, D2D1_DRAW_TEXT_OPTIONS options) PURE;
  STDMETHOD_(void, DrawGlyphRun)(THIS_ D2D1_POINT_2F baselineOrigin, DWRITE_GLYPH_RUN *glyphRun, ID2D1Brush *foregroundBrush, DWRITE_MEASURING_MODE measuringMode) PURE;
  STDMETHOD_(void, SetTransform)(THIS_ D2D1_MATRIX_3X2_F *transform) PURE;
  STDMETHOD_(void, GetTransform)(THIS_ D2D1_MATRIX_3X2_F *transform) PURE;
  STDMETHOD_(void, SetAntialiasMode)(THIS_ D2D1_ANTIALIAS_MODE antialiasMode) PURE;
  STDMETHOD_(D2D1_ANTIALIAS_MODE, GetAntialiasMode)(THIS) PURE;
  STDMETHOD_(void, SetTextAntialiasMode)(THIS_ D2D1_TEXT_ANTIALIAS_MODE textAntialiasMode) PURE;
  STDMETHOD_(D2D1_TEXT_ANTIALIAS_MODE, GetTextAntialiasMode)(THIS) PURE;
  STDMETHOD_(void, SetTextRenderingParams)(THIS_ IDWriteRenderingParams *textRenderingParams) PURE;
  STDMETHOD_(void, GetTextRenderingParams)(THIS_ IDWriteRenderingParams **textRenderingParams) PURE;
  STDMETHOD_(void, SetTags)(THIS_ D2D1_TAG tag1, D2D1_TAG tag2) PURE;
  STDMETHOD_(void, GetTags)(THIS_ D2D1_TAG *tag1, D2D1_TAG *tag2) PURE;
  STDMETHOD_(void, PushLayer)(THIS_ D2D1_LAYER_PARAMETERS *layerParameters, ID2D1Layer *layer) PURE;
  STDMETHOD_(void, PopLayer)(THIS) PURE;
  STDMETHOD(Flush)(THIS_ D2D1_TAG *tag1, D2D1_TAG *tag2) PURE;
  STDMETHOD_(void, SaveDrawingState)(THIS_ ID2D1DrawingStateBlock *drawingStateBlock) PURE;
  STDMETHOD_(void, RestoreDrawingState)(THIS_ ID2D1DrawingStateBlock *drawingStateBlock) PURE;
  STDMETHOD_(void, PushAxisAlignedClip)(THIS_ D2D1_RECT_F *clipRect, D2D1_ANTIALIAS_MODE antialiasMode) PURE;
  STDMETHOD_(void, PopAxisAlignedClip)(THIS) PURE;
  STDMETHOD_(void, Clear)(THIS_ D2D1_COLOR_F *clearColor) PURE;
  STDMETHOD_(void, BeginDraw)(THIS) PURE;
  STDMETHOD(EndDraw)(THIS_ D2D1_TAG *tag1, D2D1_TAG *tag2) PURE;
  STDMETHOD_(D2D1_PIXEL_FORMAT, GetPixelFormat)(THIS) PURE;
  STDMETHOD_(void, SetDpi)(THIS_ FLOAT dpiX, FLOAT dpiY) PURE;
  STDMETHOD_(void, GetDpi)(THIS_ FLOAT *dpiX, FLOAT *dpiY) PURE;
  STDMETHOD_(D2D1_SIZE_F, GetSize)(THIS) PURE;
  STDMETHOD_(D2D1_SIZE_U, GetPixelSize)(THIS) PURE;
  STDMETHOD_(UINT32, GetMaximumBitmapSize)(THIS) PURE;
  STDMETHOD_(BOOL, IsSupported)(THIS_ D2D1_RENDER_TARGET_PROPERTIES *renderTargetProperties) PURE;

  /* ID2D1BitmapRenderTarget methods */
  STDMETHOD(GetBitmap)(THIS_ ID2D1Bitmap **bitmap) PURE;

  END_INTERFACE
};
#undef INTERFACE

#define ID2D1BitmapRenderTarget_BeginDraw(this) (this)->lpVtbl->BeginDraw(this)
#define ID2D1BitmapRenderTarget_Clear(this,A) (this)->lpVtbl->Clear(this,A)
#define ID2D1BitmapRenderTarget_CreateBitmap(this,A,B,C,D,E) (this)->lpVtbl->CreateBitmap(this,A,B,C,D,E)
#define ID2D1BitmapRenderTarget_CreateBitmapBrush(this,A,B) (this)->lpVtbl->CreateBitmapBrush(this,A,B)
#define ID2D1BitmapRenderTarget_CreateBitmapFromWicBitmap(this,A,B) (this)->lpVtbl->CreateBitmapFromWicBitmap(this,A,B)
#define ID2D1BitmapRenderTarget_CreateCompatibleRenderTarget(this,A) (this)->lpVtbl->CreateCompatibleRenderTarget(this,A)
#define ID2D1BitmapRenderTarget_CreateGradientStopCollection(this,A,B,C) (this)->lpVtbl->CreateGradientStopCollection(this,A,B,C)
#define ID2D1BitmapRenderTarget_CreateLayer(this,A) (this)->lpVtbl->CreateLayer(this,A)
#define ID2D1BitmapRenderTarget_CreateLinearGradientBrush(this,A,B,C,D) (this)->lpVtbl->CreateLinearGradientBrush(this,A,B,C,D)
#define ID2D1BitmapRenderTarget_CreateMesh(this,A) (this)->lpVtbl->CreateMesh(this,A)
#define ID2D1BitmapRenderTarget_CreateRadialGradientBrush(this,A,B,C,D) (this)->lpVtbl->CreateRadialGradientBrush(this,A,B,C,D)
#define ID2D1BitmapRenderTarget_CreateSharedBitmap(this,A,B,C,D) (this)->lpVtbl->CreateSharedBitmap(this,A,B,C,D)
#define ID2D1BitmapRenderTarget_CreateSolidColorBrush(this,A,B,C) (this)->lpVtbl->CreateSolidColorBrush(this,A,B,C)
#define ID2D1BitmapRenderTarget_DrawBitmap(this,A,B,C,D,E) (this)->lpVtbl->DrawBitmap(this,A,B,C,D,E)
#define ID2D1BitmapRenderTarget_DrawEllipse(this,A,B,C,D) (this)->lpVtbl->DrawEllipse(this,A,B,C,D)
#define ID2D1BitmapRenderTarget_DrawGeometry(this,A,B,C,D) (this)->lpVtbl->DrawGeometry(this,A,B,C,D)
#define ID2D1BitmapRenderTarget_DrawGlyphRun(this,A,B,C,D) (this)->lpVtbl->DrawGlyphRun(this,A,B,C,D)
#define ID2D1BitmapRenderTarget_DrawLine(this,A,B,C,D,E) (this)->lpVtbl->DrawLine(this,A,B,C,D,E)
#define ID2D1BitmapRenderTarget_DrawRectangle(this,A,B,C,D) (this)->lpVtbl->DrawRectangle(this,A,B,C,D)
#define ID2D1BitmapRenderTarget_DrawRoundedRectangle(this,A,B,C,D) (this)->lpVtbl->DrawRoundedRectangle(this,A,B,C,D)
#define ID2D1BitmapRenderTarget_DrawText(this,A,B,C,D,E,F,G) (this)->lpVtbl->DrawText(this,A,B,C,D,E,F,G)
#define ID2D1BitmapRenderTarget_DrawTextLayout(this,A,B,C,D) (this)->lpVtbl->DrawTextLayout(this,A,B,C,D)
#define ID2D1BitmapRenderTarget_EndDraw(this,A,B) (this)->lpVtbl->EndDraw(this,A,B)
#define ID2D1BitmapRenderTarget_FillEllipse(this,A,B) (this)->lpVtbl->FillEllipse(this,A,B)
#define ID2D1BitmapRenderTarget_FillGeometry(this,A,B,C) (this)->lpVtbl->FillGeometry(this,A,B,C)
#define ID2D1BitmapRenderTarget_FillMesh(this,A,B) (this)->lpVtbl->FillMesh(this,A,B)
#define ID2D1BitmapRenderTarget_FillOpacityMask(this,A,B,C,D,E) (this)->lpVtbl->FillOpacityMask(this,A,B,C,D,E)
#define ID2D1BitmapRenderTarget_FillRectangle(this,A,B) (this)->lpVtbl->FillRectangle(this,A,B)
#define ID2D1BitmapRenderTarget_FillRoundedRectangle(this,A,B) (this)->lpVtbl->FillRoundedRectangle(this,A,B)
#define ID2D1BitmapRenderTarget_Flush(this,A,B) (this)->lpVtbl->Flush(this,A,B)
#define ID2D1BitmapRenderTarget_GetAntialiasMode(this) (this)->lpVtbl->GetAntialiasMode(this)
#define ID2D1BitmapRenderTarget_GetDpi(this,A,B) (this)->lpVtbl->GetDpi(this,A,B)
#define ID2D1BitmapRenderTarget_GetMaximumBitmapSize(this) (this)->lpVtbl->GetMaximumBitmapSize(this)
#define ID2D1BitmapRenderTarget_GetPixelFormat(this) (this)->lpVtbl->GetPixelFormat(this)
#define ID2D1BitmapRenderTarget_GetPixelSize(this) (this)->lpVtbl->GetPixelSize(this)
#define ID2D1BitmapRenderTarget_GetSize(this) (this)->lpVtbl->GetSize(this)
#define ID2D1BitmapRenderTarget_GetTags(this,A,B) (this)->lpVtbl->GetTags(this,A,B)
#define ID2D1BitmapRenderTarget_GetTextAntialiasMode(this) (this)->lpVtbl->GetTextAntialiasMode(this)
#define ID2D1BitmapRenderTarget_GetTextRenderingParams(this,A) (this)->lpVtbl->GetTextRenderingParams(this,A)
#define ID2D1BitmapRenderTarget_GetTransform(this,A) (this)->lpVtbl->GetTransform(this,A)
#define ID2D1BitmapRenderTarget_IsSupported(this,A) (this)->lpVtbl->IsSupported(this,A)
#define ID2D1BitmapRenderTarget_PopAxisAlignedClip(this) (this)->lpVtbl->PopAxisAlignedClip(this)
#define ID2D1BitmapRenderTarget_PopLayer(this) (this)->lpVtbl->PopLayer(this)
#define ID2D1BitmapRenderTarget_PushAxisAlignedClip(this,A,B) (this)->lpVtbl->PushAxisAlignedClip(this,A,B)
#define ID2D1BitmapRenderTarget_PushLayer(this,A,B) (this)->lpVtbl->PushLayer(this,A,B)
#define ID2D1BitmapRenderTarget_RestoreDrawingState(this,A) (this)->lpVtbl->RestoreDrawingState(this,A)
#define ID2D1BitmapRenderTarget_SaveDrawingState(this,A) (this)->lpVtbl->SaveDrawingState(this,A)
#define ID2D1BitmapRenderTarget_SetAntialiasMode(this,A) (this)->lpVtbl->SetAntialiasMode(this,A)
#define ID2D1BitmapRenderTarget_SetDpi(this,A,B) (this)->lpVtbl->SetDpi(this,A,B)
#define ID2D1BitmapRenderTarget_SetTags(this,A,B) (this)->lpVtbl->SetTags(this,A,B)
#define ID2D1BitmapRenderTarget_SetTextAntialiasMode(this,A) (this)->lpVtbl->SetTextAntialiasMode(this,A)
#define ID2D1BitmapRenderTarget_SetTextRenderingParams(this,A) (this)->lpVtbl->SetTextRenderingParams(this,A)
#define ID2D1BitmapRenderTarget_SetTransform(this,A) (this)->lpVtbl->SetTransform(this,A)
#define ID2D1BitmapRenderTarget_GetBitmap(this,A) (this)->lpVtbl->GetBitmap(this,A)

#define INTERFACE ID2D1DCRenderTarget
DECLARE_INTERFACE_(ID2D1DCRenderTarget, ID2D1RenderTarget)
{
  BEGIN_INTERFACE

  /* IUnknown methods */
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;

  /* ID2D1Resource methods */
  STDMETHOD_(void, GetFactory)(THIS_ ID2D1Factory **factory) PURE;

  /* ID2D1RenderTarget methods */
  STDMETHOD(CreateBitmap)(THIS_ D2D1_SIZE_U size, void *srcData, UINT32 pitch, D2D1_BITMAP_PROPERTIES *bitmapProperties, ID2D1Bitmap **bitmap) PURE;
  STDMETHOD(CreateBitmapFromWicBitmap)(THIS_ IWICBitmapSource *wicBitmapSource, ID2D1Bitmap **bitmap) PURE;
  STDMETHOD(CreateSharedBitmap)(THIS_ REFIID riid, void *data, D2D1_BITMAP_PROPERTIES *bitmapProperties, ID2D1Bitmap **bitmap) PURE;
  STDMETHOD(CreateBitmapBrush)(THIS_ ID2D1Bitmap *bitmap, ID2D1BitmapBrush **bitmapBrush) PURE;
  STDMETHOD(CreateSolidColorBrush)(THIS_ D2D1_COLOR_F *color, D2D1_BRUSH_PROPERTIES *brushProperties, ID2D1SolidColorBrush **solidColorBrush) PURE;
  STDMETHOD(CreateGradientStopCollection)(THIS_ D2D1_GRADIENT_STOP *gradientStops, UINT gradientStopsCount, ID2D1GradientStopCollection **gradientStopCollection) PURE;
  STDMETHOD(CreateLinearGradientBrush)(THIS_ D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES *linearGradientBrushProperties, D2D1_BRUSH_PROPERTIES *brushProperties, ID2D1GradientStopCollection *gradientStopCollection, ID2D1LinearGradientBrush **linearGradientBrush) PURE;
  STDMETHOD(CreateRadialGradientBrush)(THIS_ D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES *radialGradientBrushProperties, D2D1_BRUSH_PROPERTIES *brushProperties, ID2D1GradientStopCollection *gradientStopCollection, ID2D1RadialGradientBrush **radialGradientBrush) PURE;
  STDMETHOD(CreateCompatibleRenderTarget)(THIS_ ID2D1BitmapRenderTarget **bitmapRenderTarget) PURE;
  STDMETHOD(CreateLayer)(THIS_ ID2D1Layer **layer) PURE;
  STDMETHOD(CreateMesh)(THIS_ ID2D1Mesh **mesh) PURE;
  STDMETHOD_(void, DrawLine)(THIS_ D2D1_POINT_2F point0, D2D1_POINT_2F point1, ID2D1Brush *brush, FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle) PURE;
  STDMETHOD_(void, DrawRectangle)(THIS_ D2D1_RECT_F *rect, ID2D1Brush *brush, FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle) PURE;
  STDMETHOD_(void, FillRectangle)(THIS_ D2D1_RECT_F *rect, ID2D1Brush *brush) PURE;
  STDMETHOD_(void, DrawRoundedRectangle)(THIS_ D2D1_ROUNDED_RECT *roundedRect, ID2D1Brush *brush, FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle) PURE;
  STDMETHOD_(void, FillRoundedRectangle)(THIS_ D2D1_ROUNDED_RECT *roundedRect, ID2D1Brush *brush) PURE;
  STDMETHOD_(void, DrawEllipse)(THIS_ D2D1_ELLIPSE *ellipse, ID2D1Brush *brush, FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle) PURE;
  STDMETHOD_(void, FillEllipse)(THIS_ D2D1_ELLIPSE *ellipse, ID2D1Brush *brush) PURE;
  STDMETHOD_(void, DrawGeometry)(THIS_ ID2D1Geometry *geometry, ID2D1Brush *brush, FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle) PURE;
  STDMETHOD_(void, FillGeometry)(THIS_ ID2D1Geometry *geometry, ID2D1Brush *brush, ID2D1Brush *opacityBrush) PURE;
  STDMETHOD_(void, FillMesh)(THIS_ ID2D1Mesh *mesh, ID2D1Brush *brush) PURE;
  STDMETHOD_(void, FillOpacityMask)(THIS_ ID2D1Bitmap *opacityMask, ID2D1Brush *brush, D2D1_OPACITY_MASK_CONTENT content, D2D1_RECT_F *destinationRectangle, D2D1_RECT_F *sourceRectangle) PURE;
  STDMETHOD_(void, DrawBitmap)(THIS_ ID2D1Bitmap *bitmap, D2D1_RECT_F *destinationRectangle, FLOAT opacity, D2D1_BITMAP_INTERPOLATION_MODE interpolationMode , D2D1_RECT_F *sourceRectangle) PURE;
  STDMETHOD_(void, DrawText)(THIS_ WCHAR *string, UINT stringLength, IDWriteTextFormat *textFormat, D2D1_RECT_F *layoutRect, ID2D1Brush *defaultForegroundBrush, D2D1_DRAW_TEXT_OPTIONS options , DWRITE_MEASURING_MODE measuringMode) PURE;
  STDMETHOD_(void, DrawTextLayout)(THIS_ D2D1_POINT_2F origin, IDWriteTextLayout *textLayout, ID2D1Brush *defaultForegroundBrush, D2D1_DRAW_TEXT_OPTIONS options) PURE;
  STDMETHOD_(void, DrawGlyphRun)(THIS_ D2D1_POINT_2F baselineOrigin, DWRITE_GLYPH_RUN *glyphRun, ID2D1Brush *foregroundBrush, DWRITE_MEASURING_MODE measuringMode) PURE;
  STDMETHOD_(void, SetTransform)(THIS_ D2D1_MATRIX_3X2_F *transform) PURE;
  STDMETHOD_(void, GetTransform)(THIS_ D2D1_MATRIX_3X2_F *transform) PURE;
  STDMETHOD_(void, SetAntialiasMode)(THIS_ D2D1_ANTIALIAS_MODE antialiasMode) PURE;
  STDMETHOD_(D2D1_ANTIALIAS_MODE, GetAntialiasMode)(THIS) PURE;
  STDMETHOD_(void, SetTextAntialiasMode)(THIS_ D2D1_TEXT_ANTIALIAS_MODE textAntialiasMode) PURE;
  STDMETHOD_(D2D1_TEXT_ANTIALIAS_MODE, GetTextAntialiasMode)(THIS) PURE;
  STDMETHOD_(void, SetTextRenderingParams)(THIS_ IDWriteRenderingParams *textRenderingParams) PURE;
  STDMETHOD_(void, GetTextRenderingParams)(THIS_ IDWriteRenderingParams **textRenderingParams) PURE;
  STDMETHOD_(void, SetTags)(THIS_ D2D1_TAG tag1, D2D1_TAG tag2) PURE;
  STDMETHOD_(void, GetTags)(THIS_ D2D1_TAG *tag1, D2D1_TAG *tag2) PURE;
  STDMETHOD_(void, PushLayer)(THIS_ D2D1_LAYER_PARAMETERS *layerParameters, ID2D1Layer *layer) PURE;
  STDMETHOD_(void, PopLayer)(THIS) PURE;
  STDMETHOD(Flush)(THIS_ D2D1_TAG *tag1, D2D1_TAG *tag2) PURE;
  STDMETHOD_(void, SaveDrawingState)(THIS_ ID2D1DrawingStateBlock *drawingStateBlock) PURE;
  STDMETHOD_(void, RestoreDrawingState)(THIS_ ID2D1DrawingStateBlock *drawingStateBlock) PURE;
  STDMETHOD_(void, PushAxisAlignedClip)(THIS_ D2D1_RECT_F *clipRect, D2D1_ANTIALIAS_MODE antialiasMode) PURE;
  STDMETHOD_(void, PopAxisAlignedClip)(THIS) PURE;
  STDMETHOD_(void, Clear)(THIS_ D2D1_COLOR_F *clearColor) PURE;
  STDMETHOD_(void, BeginDraw)(THIS) PURE;
  STDMETHOD(EndDraw)(THIS_ D2D1_TAG *tag1, D2D1_TAG *tag2) PURE;
  STDMETHOD_(D2D1_PIXEL_FORMAT, GetPixelFormat)(THIS) PURE;
  STDMETHOD_(void, SetDpi)(THIS_ FLOAT dpiX, FLOAT dpiY) PURE;
  STDMETHOD_(void, GetDpi)(THIS_ FLOAT *dpiX, FLOAT *dpiY) PURE;
  STDMETHOD_(D2D1_SIZE_F, GetSize)(THIS) PURE;
  STDMETHOD_(D2D1_SIZE_U, GetPixelSize)(THIS) PURE;
  STDMETHOD_(UINT32, GetMaximumBitmapSize)(THIS) PURE;
  STDMETHOD_(BOOL, IsSupported)(THIS_ D2D1_RENDER_TARGET_PROPERTIES *renderTargetProperties) PURE;

  /* ID2D1DCRenderTarget methods */
  STDMETHOD(BindDC)(THIS_ HDC hDC, RECT *pSubRect) PURE;

  END_INTERFACE
};
#undef INTERFACE

#define ID2D1DCRenderTarget_BeginDraw(this) (this)->lpVtbl->BeginDraw(this)
#define ID2D1DCRenderTarget_Clear(this,A) (this)->lpVtbl->Clear(this,A)
#define ID2D1DCRenderTarget_CreateBitmap(this,A,B,C,D,E) (this)->lpVtbl->CreateBitmap(this,A,B,C,D,E)
#define ID2D1DCRenderTarget_CreateBitmapBrush(this,A,B) (this)->lpVtbl->CreateBitmapBrush(this,A,B)
#define ID2D1DCRenderTarget_CreateBitmapFromWicBitmap(this,A,B) (this)->lpVtbl->CreateBitmapFromWicBitmap(this,A,B)
#define ID2D1DCRenderTarget_CreateCompatibleRenderTarget(this,A) (this)->lpVtbl->CreateCompatibleRenderTarget(this,A)
#define ID2D1DCRenderTarget_CreateGradientStopCollection(this,A,B,C) (this)->lpVtbl->CreateGradientStopCollection(this,A,B,C)
#define ID2D1DCRenderTarget_CreateLayer(this,A) (this)->lpVtbl->CreateLayer(this,A)
#define ID2D1DCRenderTarget_CreateLinearGradientBrush(this,A,B,C,D) (this)->lpVtbl->CreateLinearGradientBrush(this,A,B,C,D)
#define ID2D1DCRenderTarget_CreateMesh(this,A) (this)->lpVtbl->CreateMesh(this,A)
#define ID2D1DCRenderTarget_CreateRadialGradientBrush(this,A,B,C,D) (this)->lpVtbl->CreateRadialGradientBrush(this,A,B,C,D)
#define ID2D1DCRenderTarget_CreateSharedBitmap(this,A,B,C,D) (this)->lpVtbl->CreateSharedBitmap(this,A,B,C,D)
#define ID2D1DCRenderTarget_CreateSolidColorBrush(this,A,B,C) (this)->lpVtbl->CreateSolidColorBrush(this,A,B,C)
#define ID2D1DCRenderTarget_DrawBitmap(this,A,B,C,D,E) (this)->lpVtbl->DrawBitmap(this,A,B,C,D,E)
#define ID2D1DCRenderTarget_DrawEllipse(this,A,B,C,D) (this)->lpVtbl->DrawEllipse(this,A,B,C,D)
#define ID2D1DCRenderTarget_DrawGeometry(this,A,B,C,D) (this)->lpVtbl->DrawGeometry(this,A,B,C,D)
#define ID2D1DCRenderTarget_DrawGlyphRun(this,A,B,C,D) (this)->lpVtbl->DrawGlyphRun(this,A,B,C,D)
#define ID2D1DCRenderTarget_DrawLine(this,A,B,C,D,E) (this)->lpVtbl->DrawLine(this,A,B,C,D,E)
#define ID2D1DCRenderTarget_DrawRectangle(this,A,B,C,D) (this)->lpVtbl->DrawRectangle(this,A,B,C,D)
#define ID2D1DCRenderTarget_DrawRoundedRectangle(this,A,B,C,D) (this)->lpVtbl->DrawRoundedRectangle(this,A,B,C,D)
#define ID2D1DCRenderTarget_DrawText(this,A,B,C,D,E,F,G) (this)->lpVtbl->DrawText(this,A,B,C,D,E,F,G)
#define ID2D1DCRenderTarget_DrawTextLayout(this,A,B,C,D) (this)->lpVtbl->DrawTextLayout(this,A,B,C,D)
#define ID2D1DCRenderTarget_EndDraw(this,A,B) (this)->lpVtbl->EndDraw(this,A,B)
#define ID2D1DCRenderTarget_FillEllipse(this,A,B) (this)->lpVtbl->FillEllipse(this,A,B)
#define ID2D1DCRenderTarget_FillGeometry(this,A,B,C) (this)->lpVtbl->FillGeometry(this,A,B,C)
#define ID2D1DCRenderTarget_FillMesh(this,A,B) (this)->lpVtbl->FillMesh(this,A,B)
#define ID2D1DCRenderTarget_FillOpacityMask(this,A,B,C,D,E) (this)->lpVtbl->FillOpacityMask(this,A,B,C,D,E)
#define ID2D1DCRenderTarget_FillRectangle(this,A,B) (this)->lpVtbl->FillRectangle(this,A,B)
#define ID2D1DCRenderTarget_FillRoundedRectangle(this,A,B) (this)->lpVtbl->FillRoundedRectangle(this,A,B)
#define ID2D1DCRenderTarget_Flush(this,A,B) (this)->lpVtbl->Flush(this,A,B)
#define ID2D1DCRenderTarget_GetAntialiasMode(this) (this)->lpVtbl->GetAntialiasMode(this)
#define ID2D1DCRenderTarget_GetDpi(this,A,B) (this)->lpVtbl->GetDpi(this,A,B)
#define ID2D1DCRenderTarget_GetMaximumBitmapSize(this) (this)->lpVtbl->GetMaximumBitmapSize(this)
#define ID2D1DCRenderTarget_GetPixelFormat(this) (this)->lpVtbl->GetPixelFormat(this)
#define ID2D1DCRenderTarget_GetPixelSize(this) (this)->lpVtbl->GetPixelSize(this)
#define ID2D1DCRenderTarget_GetSize(this) (this)->lpVtbl->GetSize(this)
#define ID2D1DCRenderTarget_GetTags(this,A,B) (this)->lpVtbl->GetTags(this,A,B)
#define ID2D1DCRenderTarget_GetTextAntialiasMode(this) (this)->lpVtbl->GetTextAntialiasMode(this)
#define ID2D1DCRenderTarget_GetTextRenderingParams(this,A) (this)->lpVtbl->GetTextRenderingParams(this,A)
#define ID2D1DCRenderTarget_GetTransform(this,A) (this)->lpVtbl->GetTransform(this,A)
#define ID2D1DCRenderTarget_IsSupported(this,A) (this)->lpVtbl->IsSupported(this,A)
#define ID2D1DCRenderTarget_PopAxisAlignedClip(this) (this)->lpVtbl->PopAxisAlignedClip(this)
#define ID2D1DCRenderTarget_PopLayer(this) (this)->lpVtbl->PopLayer(this)
#define ID2D1DCRenderTarget_PushAxisAlignedClip(this,A,B) (this)->lpVtbl->PushAxisAlignedClip(this,A,B)
#define ID2D1DCRenderTarget_PushLayer(this,A,B) (this)->lpVtbl->PushLayer(this,A,B)
#define ID2D1DCRenderTarget_RestoreDrawingState(this,A) (this)->lpVtbl->RestoreDrawingState(this,A)
#define ID2D1DCRenderTarget_SaveDrawingState(this,A) (this)->lpVtbl->SaveDrawingState(this,A)
#define ID2D1DCRenderTarget_SetAntialiasMode(this,A) (this)->lpVtbl->SetAntialiasMode(this,A)
#define ID2D1DCRenderTarget_SetDpi(this,A,B) (this)->lpVtbl->SetDpi(this,A,B)
#define ID2D1DCRenderTarget_SetTags(this,A,B) (this)->lpVtbl->SetTags(this,A,B)
#define ID2D1DCRenderTarget_SetTextAntialiasMode(this,A) (this)->lpVtbl->SetTextAntialiasMode(this,A)
#define ID2D1DCRenderTarget_SetTextRenderingParams(this,A) (this)->lpVtbl->SetTextRenderingParams(this,A)
#define ID2D1DCRenderTarget_SetTransform(this,A) (this)->lpVtbl->SetTransform(this,A)
#define ID2D1DCRenderTarget_BindDC(this,A,B) (this)->lpVtbl->BindDC(this,A,B)

#define INTERFACE ID2D1DrawingStateBlock
DECLARE_INTERFACE_(ID2D1DrawingStateBlock, ID2D1Resource)
{
  BEGIN_INTERFACE

  /* IUnknown methods */
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;

  /* ID2D1Resource methods */
  STDMETHOD_(void, GetFactory)(THIS_ ID2D1Factory **factory) PURE;

  /* ID2D1DrawingStateBlock methods */
  STDMETHOD_(void, GetDescription)(THIS_ D2D1_DRAWING_STATE_DESCRIPTION *stateDescription) PURE;
  STDMETHOD_(void, SetDescription)(THIS_ D2D1_DRAWING_STATE_DESCRIPTION *stateDescription) PURE;
  STDMETHOD_(void, SetTextRenderingParams)(THIS_ IDWriteRenderingParams *textRenderingParams) PURE;
  STDMETHOD_(void, GetTextRenderingParams)(THIS_ IDWriteRenderingParams **textRenderingParams) PURE;

  END_INTERFACE
};
#undef INTERFACE

#define ID2D1DrawingStateBlock_QueryInterface(this,A,B) (this)->lpVtbl->QueryInterface(this,A,B)
#define ID2D1DrawingStateBlock_AddRef(this) (this)->lpVtbl->AddRef(this)
#define ID2D1DrawingStateBlock_Release(this) (this)->lpVtbl->Release(this)
#define ID2D1DrawingStateBlock_GetFactory(this,A) (this)->lpVtbl->GetFactory(this,A)
#define ID2D1DrawingStateBlock_GetDescription(this,A) (this)->lpVtbl->GetDescription(this,A)
#define ID2D1DrawingStateBlock_GetTextRenderingParams(this,A) (this)->lpVtbl->GetTextRenderingParams(this,A)
#define ID2D1DrawingStateBlock_SetDescription(this,A) (this)->lpVtbl->SetDescription(this,A)
#define ID2D1DrawingStateBlock_SetTextRenderingParams(this,A) (this)->lpVtbl->SetTextRenderingParams(this,A)

#define INTERFACE ID2D1Geometry
DECLARE_INTERFACE_(ID2D1Geometry, ID2D1Resource)
{
  BEGIN_INTERFACE

  /* IUnknown methods */
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;

  /* ID2D1Resource methods */
  STDMETHOD_(void, GetFactory)(THIS_ ID2D1Factory **factory) PURE;

  /* ID2D1Geometry methods */
  STDMETHOD(GetBounds)(THIS_ D2D1_MATRIX_3X2_F *worldTransform, D2D1_RECT_F *bounds) PURE;
  STDMETHOD(GetWidenedBounds)(THIS_ FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle, D2D1_MATRIX_3X2_F *worldTransform, D2D1_RECT_F *bounds) PURE;
  STDMETHOD(StrokeContainsPoint)(THIS_ D2D1_POINT_2F point, FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle, D2D1_MATRIX_3X2_F *worldTransform, BOOL *contains) PURE;
  STDMETHOD(FillContainsPoint)(THIS_ D2D1_POINT_2F point, D2D1_MATRIX_3X2_F *worldTransform, BOOL *contains) PURE;
  STDMETHOD(CompareWithGeometry)(THIS_ ID2D1Geometry *inputGeometry, D2D1_MATRIX_3X2_F *inputGeometryTransform, D2D1_GEOMETRY_RELATION *relation) PURE;
  STDMETHOD(Simplify)(THIS_ D2D1_GEOMETRY_SIMPLIFICATION_OPTION simplificationOption, D2D1_MATRIX_3X2_F *worldTransform, ID2D1SimplifiedGeometrySink *geometrySink) PURE;
  STDMETHOD(Tessellate)(THIS_ D2D1_MATRIX_3X2_F *worldTransform, ID2D1TessellationSink *tessellationSink) PURE;
  STDMETHOD(CombineWithGeometry)(THIS_ ID2D1Geometry *inputGeometry, D2D1_COMBINE_MODE combineMode, D2D1_MATRIX_3X2_F *inputGeometryTransform, ID2D1SimplifiedGeometrySink *geometrySink) PURE;
  STDMETHOD(Outline)(THIS_ D2D1_MATRIX_3X2_F *worldTransform, ID2D1SimplifiedGeometrySink *geometrySink) PURE;
  STDMETHOD(ComputeArea)(THIS_ D2D1_MATRIX_3X2_F *worldTransform, FLOAT *area) PURE;
  STDMETHOD(ComputeLength)(THIS_ D2D1_MATRIX_3X2_F *worldTransform, FLOAT *length) PURE;
  STDMETHOD(ComputePointAtLength)(THIS_ FLOAT length, D2D1_MATRIX_3X2_F *worldTransform, D2D1_POINT_2F *point, D2D1_POINT_2F *unitTangentVector) PURE;
  STDMETHOD(Widen)(THIS_ FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle, D2D1_MATRIX_3X2_F *worldTransform, ID2D1SimplifiedGeometrySink *geometrySink) PURE;

  END_INTERFACE
};
#undef INTERFACE

#define ID2D1Geometry_QueryInterface(this,A,B) (this)->lpVtbl->QueryInterface(this,A,B)
#define ID2D1Geometry_AddRef(this) (this)->lpVtbl->AddRef(this)
#define ID2D1Geometry_Release(this) (this)->lpVtbl->Release(this)
#define ID2D1Geometry_GetFactory(this,A) (this)->lpVtbl->GetFactory(this,A)
#define ID2D1Geometry_CombineWithGeometry(this,A,B,C,D) (this)->lpVtbl->CombineWithGeometry(this,A,B,C,D)
#define ID2D1Geometry_CompareWithGeometry(this,A,B,C) (this)->lpVtbl->CompareWithGeometry(this,A,B,C)
#define ID2D1Geometry_ComputeArea(this,A,B) (this)->lpVtbl->ComputeArea(this,A,B)
#define ID2D1Geometry_ComputeLength(this,A,B) (this)->lpVtbl->ComputeLength(this,A,B)
#define ID2D1Geometry_ComputePointAtLength(this,A,B,C,D) (this)->lpVtbl->ComputePointAtLength(this,A,B,C,D)
#define ID2D1Geometry_FillContainsPoint(this,A,B,C) (this)->lpVtbl->FillContainsPoint(this,A,B,C)
#define ID2D1Geometry_GetBounds(this,A,B) (this)->lpVtbl->GetBounds(this,A,B)
#define ID2D1Geometry_GetWidenedBounds(this,A,B,C,D) (this)->lpVtbl->GetWidenedBounds(this,A,B,C,D)
#define ID2D1Geometry_Outline(this,A,B) (this)->lpVtbl->Outline(this,A,B)
#define ID2D1Geometry_StrokeContainsPoint(this,A,B,C,D,E) (this)->lpVtbl->StrokeContainsPoint(this,A,B,C,D,E)
#define ID2D1Geometry_Simplify(this,A,B,C) (this)->lpVtbl->Simplify(this,A,B,C)
#define ID2D1Geometry_Tessellate(this,A,B) (this)->lpVtbl->Tessellate(this,A,B)
#define ID2D1Geometry_Widen(this,A,B,C,D) (this)->lpVtbl->Widen(this,A,B,C,D)

#define INTERFACE ID2D1EllipseGeometry
DECLARE_INTERFACE_(ID2D1EllipseGeometry, ID2D1Geometry)
{
  BEGIN_INTERFACE

  /* IUnknown methods */
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;

  /* ID2D1Resource methods */
  STDMETHOD_(void, GetFactory)(THIS_ ID2D1Factory **factory) PURE;

  /* ID2D1Geometry methods */
  STDMETHOD(GetBounds)(THIS_ D2D1_MATRIX_3X2_F *worldTransform, D2D1_RECT_F *bounds) PURE;
  STDMETHOD(GetWidenedBounds)(THIS_ FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle, D2D1_MATRIX_3X2_F *worldTransform, D2D1_RECT_F *bounds) PURE;
  STDMETHOD(StrokeContainsPoint)(THIS_ D2D1_POINT_2F point, FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle, D2D1_MATRIX_3X2_F *worldTransform, BOOL *contains) PURE;
  STDMETHOD(FillContainsPoint)(THIS_ D2D1_POINT_2F point, D2D1_MATRIX_3X2_F *worldTransform, BOOL *contains) PURE;
  STDMETHOD(CompareWithGeometry)(THIS_ ID2D1Geometry *inputGeometry, D2D1_MATRIX_3X2_F *inputGeometryTransform, D2D1_GEOMETRY_RELATION *relation) PURE;
  STDMETHOD(Simplify)(THIS_ D2D1_GEOMETRY_SIMPLIFICATION_OPTION simplificationOption, D2D1_MATRIX_3X2_F *worldTransform, ID2D1SimplifiedGeometrySink *geometrySink) PURE;
  STDMETHOD(Tessellate)(THIS_ D2D1_MATRIX_3X2_F *worldTransform, ID2D1TessellationSink *tessellationSink) PURE;
  STDMETHOD(CombineWithGeometry)(THIS_ ID2D1Geometry *inputGeometry, D2D1_COMBINE_MODE combineMode, D2D1_MATRIX_3X2_F *inputGeometryTransform, ID2D1SimplifiedGeometrySink *geometrySink) PURE;
  STDMETHOD(Outline)(THIS_ D2D1_MATRIX_3X2_F *worldTransform, ID2D1SimplifiedGeometrySink *geometrySink) PURE;
  STDMETHOD(ComputeArea)(THIS_ D2D1_MATRIX_3X2_F *worldTransform, FLOAT *area) PURE;
  STDMETHOD(ComputeLength)(THIS_ D2D1_MATRIX_3X2_F *worldTransform, FLOAT *length) PURE;
  STDMETHOD(ComputePointAtLength)(THIS_ FLOAT length, D2D1_MATRIX_3X2_F *worldTransform, D2D1_POINT_2F *point, D2D1_POINT_2F *unitTangentVector) PURE;
  STDMETHOD(Widen)(THIS_ FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle, D2D1_MATRIX_3X2_F *worldTransform, ID2D1SimplifiedGeometrySink *geometrySink) PURE;

  /* ID2D1EllipseGeometry methods */
  STDMETHOD_(void, GetEllipse)(THIS_ D2D1_ELLIPSE *ellipse) PURE;

  END_INTERFACE
};
#undef INTERFACE

#define ID2D1EllipseGeometry_CombineWithGeometry(this,A,B,C,D) (this)->lpVtbl->CombineWithGeometry(this,A,B,C,D)
#define ID2D1EllipseGeometry_CompareWithGeometry(this,A,B,C) (this)->lpVtbl->CompareWithGeometry(this,A,B,C)
#define ID2D1EllipseGeometry_ComputeArea(this,A,B) (this)->lpVtbl->ComputeArea(this,A,B)
#define ID2D1EllipseGeometry_ComputeLength(this,A,B) (this)->lpVtbl->ComputeLength(this,A,B)
#define ID2D1EllipseGeometry_ComputePointAtLength(this,A,B,C,D) (this)->lpVtbl->ComputePointAtLength(this,A,B,C,D)
#define ID2D1EllipseGeometry_FillContainsPoint(this,A,B,C) (this)->lpVtbl->FillContainsPoint(this,A,B,C)
#define ID2D1EllipseGeometry_GetBounds(this,A,B) (this)->lpVtbl->GetBounds(this,A,B)
#define ID2D1EllipseGeometry_GetWidenedBounds(this,A,B,C,D) (this)->lpVtbl->GetWidenedBounds(this,A,B,C,D)
#define ID2D1EllipseGeometry_Outline(this,A,B) (this)->lpVtbl->Outline(this,A,B)
#define ID2D1EllipseGeometry_StrokeContainsPoint(this,A,B,C,D,E) (this)->lpVtbl->StrokeContainsPoint(this,A,B,C,D,E)
#define ID2D1EllipseGeometry_Simplify(this,A,B,C) (this)->lpVtbl->Simplify(this,A,B,C)
#define ID2D1EllipseGeometry_Tessellate(this,A,B) (this)->lpVtbl->Tessellate(this,A,B)
#define ID2D1EllipseGeometry_Widen(this,A,B,C,D) (this)->lpVtbl->Widen(this,A,B,C,D)
#define ID2D1EllipseGeometry_GetEllipse(this,A) (this)->lpVtbl->GetEllipse(this,A)

#define INTERFACE ID2D1Factory
DECLARE_INTERFACE_(ID2D1Factory, IUnknown)
{
  BEGIN_INTERFACE

  /* IUnknown methods */
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;

  /* ID2D1Factory methods */
  STDMETHOD(ReloadSystemMetrics)(THIS) PURE;
  STDMETHOD_(void, GetDesktopDpi)(THIS_ FLOAT *dpiX, FLOAT *dpiY) PURE;
  STDMETHOD(CreateRectangleGeometry)(THIS_ D2D1_RECT_F *rectangle, ID2D1RectangleGeometry **rectangleGeometry) PURE;
  STDMETHOD(CreateRoundedRectangleGeometry)(THIS_ D2D1_ROUNDED_RECT *roundedRectangle, ID2D1RoundedRectangleGeometry **roundedRectangleGeometry) PURE;
  STDMETHOD(CreateEllipseGeometry)(THIS_ D2D1_ELLIPSE *ellipse, ID2D1EllipseGeometry **ellipseGeometry) PURE;
  STDMETHOD(CreateGeometryGroup)(THIS_ D2D1_FILL_MODE fillMode, ID2D1Geometry **geometries, UINT geometriesCount, ID2D1GeometryGroup **geometryGroup) PURE;
  STDMETHOD(CreateTransformedGeometry)(THIS_ ID2D1Geometry *sourceGeometry, D2D1_MATRIX_3X2_F *transform, ID2D1TransformedGeometry **transformedGeometry) PURE;
  STDMETHOD(CreatePathGeometry)(THIS_ ID2D1PathGeometry **pathGeometry) PURE;
  STDMETHOD(CreateStrokeStyle)(THIS_ D2D1_STROKE_STYLE_PROPERTIES *strokeStyleProperties, FLOAT *dashes, UINT dashesCount, ID2D1StrokeStyle **strokeStyle) PURE;
  STDMETHOD(CreateDrawingStateBlock)(THIS_ D2D1_DRAWING_STATE_DESCRIPTION *drawingStateDescription, IDWriteRenderingParams *textRenderingParams, ID2D1DrawingStateBlock **drawingStateBlock) PURE;
  STDMETHOD(CreateWicBitmapRenderTarget)(THIS_ IWICBitmap *target, D2D1_RENDER_TARGET_PROPERTIES *renderTargetProperties, ID2D1RenderTarget **renderTarget) PURE;
  STDMETHOD(CreateHwndRenderTarget)(THIS_ D2D1_RENDER_TARGET_PROPERTIES *renderTargetProperties, D2D1_HWND_RENDER_TARGET_PROPERTIES *hwndRenderTargetProperties, ID2D1HwndRenderTarget **hwndRenderTarget) PURE;
  STDMETHOD(CreateDxgiSurfaceRenderTarget)(THIS_ IDXGISurface *dxgiSurface, D2D1_RENDER_TARGET_PROPERTIES *renderTargetProperties, ID2D1RenderTarget **renderTarget) PURE;
  STDMETHOD(CreateDCRenderTarget)(THIS_ D2D1_RENDER_TARGET_PROPERTIES *renderTargetProperties, ID2D1DCRenderTarget **dcRenderTarget) PURE;

  END_INTERFACE
};
#undef INTERFACE

#define ID2D1Factory_QueryInterface(this,A,B) (this)->lpVtbl->QueryInterface(this,A,B)
#define ID2D1Factory_AddRef(this) (this)->lpVtbl->AddRef(this)
#define ID2D1Factory_Release(this) (this)->lpVtbl->Release(this)
#define ID2D1Factory_CreateDCRenderTarget(this,A,B) (this)->lpVtbl->CreateDCRenderTarget(this,A,B)
#define ID2D1Factory_CreateDrawingStateBlock(this,A,B,C) (this)->lpVtbl->CreateDrawingStateBlock(this,A,B,C)
#define ID2D1Factory_CreateDxgiSurfaceRenderTarget(this,A,B,C) (this)->lpVtbl->CreateDxgiSurfaceRenderTarget(this,A,B,C)
#define ID2D1Factory_CreateEllipseGeometry(this,A,B) (this)->lpVtbl->CreateEllipseGeometry(this,A,B)
#define ID2D1Factory_CreateGeometryGroup(this,A,B,C,D) (this)->lpVtbl->CreateGeometryGroup(this,A,B,C,D)
#define ID2D1Factory_CreateHwndRenderTarget(this,A,B,C) (this)->lpVtbl->CreateHwndRenderTarget(this,A,B,C)
#define ID2D1Factory_CreatePathGeometry(this,A) (this)->lpVtbl->CreatePathGeometry(this,A)
#define ID2D1Factory_CreateRectangleGeometry(this,A,B) (this)->lpVtbl->CreateRectangleGeometry(this,A,B)
#define ID2D1Factory_CreateRoundedRectangleGeometry(this,A,B) (this)->lpVtbl->CreateRoundedRectangleGeometry(this,A,B)
#define ID2D1Factory_CreateStrokeStyle(this,A,B,C,D) (this)->lpVtbl->CreateStrokeStyle(this,A,B,C,D)
#define ID2D1Factory_CreateTransformedGeometry(this,A,B,C) (this)->lpVtbl->CreateTransformedGeometry(this,A,B,C)
#define ID2D1Factory_CreateWicBitmapRenderTarget(this,A,B,C) (this)->lpVtbl->CreateWicBitmapRenderTarget(this,A,B,C)
#define ID2D1Factory_GetDesktopDpi(this,A,B) (this)->lpVtbl->GetDesktopDpi(this,A,B)
#define ID2D1Factory_ReloadSystemMetrics(this) (this)->lpVtbl->ReloadSystemMetrics(this)

#define INTERFACE ID2D1GdiInteropRenderTarget
DECLARE_INTERFACE_(ID2D1GdiInteropRenderTarget, IUnknown)
{
  BEGIN_INTERFACE

  /* IUnknown methods */
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;

  /* ID2D1GdiInteropRenderTarget methods */
  STDMETHOD(GetDC)(THIS_ D2D1_DC_INITIALIZE_MODE mode, HDC *hdc) PURE;
  STDMETHOD(ReleaseDC)(THIS_ RECT *update) PURE;

  END_INTERFACE
};
#undef INTERFACE

#define ID2D1GdiInteropRenderTarget_QueryInterface(this,A,B) (this)->lpVtbl->QueryInterface(this,A,B)
#define ID2D1GdiInteropRenderTarget_AddRef(this) (this)->lpVtbl->AddRef(this)
#define ID2D1GdiInteropRenderTarget_Release(this) (this)->lpVtbl->Release(this)
#define ID2D1GdiInteropRenderTarget_GetDC(this,A,B) (this)->lpVtbl->GetDC(this,A,B)
#define ID2D1GdiInteropRenderTarget_ReleaseDC(this,A) (this)->lpVtbl->ReleaseDC(this,A)

#define INTERFACE ID2D1GeometryGroup
DECLARE_INTERFACE_(ID2D1GeometryGroup, ID2D1Geometry)
{
  BEGIN_INTERFACE

  /* IUnknown methods */
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;

  /* ID2D1Resource methods */
  STDMETHOD_(void, GetFactory)(THIS_ ID2D1Factory **factory) PURE;

  /* ID2D1Geometry methods */
  STDMETHOD(GetBounds)(THIS_ D2D1_MATRIX_3X2_F *worldTransform, D2D1_RECT_F *bounds) PURE;
  STDMETHOD(GetWidenedBounds)(THIS_ FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle, D2D1_MATRIX_3X2_F *worldTransform, D2D1_RECT_F *bounds) PURE;
  STDMETHOD(StrokeContainsPoint)(THIS_ D2D1_POINT_2F point, FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle, D2D1_MATRIX_3X2_F *worldTransform, BOOL *contains) PURE;
  STDMETHOD(FillContainsPoint)(THIS_ D2D1_POINT_2F point, D2D1_MATRIX_3X2_F *worldTransform, BOOL *contains) PURE;
  STDMETHOD(CompareWithGeometry)(THIS_ ID2D1Geometry *inputGeometry, D2D1_MATRIX_3X2_F *inputGeometryTransform, D2D1_GEOMETRY_RELATION *relation) PURE;
  STDMETHOD(Simplify)(THIS_ D2D1_GEOMETRY_SIMPLIFICATION_OPTION simplificationOption, D2D1_MATRIX_3X2_F *worldTransform, ID2D1SimplifiedGeometrySink *geometrySink) PURE;
  STDMETHOD(Tessellate)(THIS_ D2D1_MATRIX_3X2_F *worldTransform, ID2D1TessellationSink *tessellationSink) PURE;
  STDMETHOD(CombineWithGeometry)(THIS_ ID2D1Geometry *inputGeometry, D2D1_COMBINE_MODE combineMode, D2D1_MATRIX_3X2_F *inputGeometryTransform, ID2D1SimplifiedGeometrySink *geometrySink) PURE;
  STDMETHOD(Outline)(THIS_ D2D1_MATRIX_3X2_F *worldTransform, ID2D1SimplifiedGeometrySink *geometrySink) PURE;
  STDMETHOD(ComputeArea)(THIS_ D2D1_MATRIX_3X2_F *worldTransform, FLOAT *area) PURE;
  STDMETHOD(ComputeLength)(THIS_ D2D1_MATRIX_3X2_F *worldTransform, FLOAT *length) PURE;
  STDMETHOD(ComputePointAtLength)(THIS_ FLOAT length, D2D1_MATRIX_3X2_F *worldTransform, D2D1_POINT_2F *point, D2D1_POINT_2F *unitTangentVector) PURE;
  STDMETHOD(Widen)(THIS_ FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle, D2D1_MATRIX_3X2_F *worldTransform, ID2D1SimplifiedGeometrySink *geometrySink) PURE;

  /* ID2D1GeometryGroup methods */
  STDMETHOD_(D2D1_FILL_MODE, GetFillMode)(THIS) PURE;
  STDMETHOD_(UINT32, GetSourceGeometryCount)(THIS) PURE;
  STDMETHOD_(void, GetSourceGeometries)(THIS_ ID2D1Geometry **geometries, UINT geometriesCount) PURE;

  END_INTERFACE
};
#undef INTERFACE

#define ID2D1GeometryGroup_CombineWithGeometry(this,A,B,C,D) (this)->lpVtbl->CombineWithGeometry(this,A,B,C,D)
#define ID2D1GeometryGroup_CompareWithGeometry(this,A,B,C) (this)->lpVtbl->CompareWithGeometry(this,A,B,C)
#define ID2D1GeometryGroup_ComputeArea(this,A,B) (this)->lpVtbl->ComputeArea(this,A,B)
#define ID2D1GeometryGroup_ComputeLength(this,A,B) (this)->lpVtbl->ComputeLength(this,A,B)
#define ID2D1GeometryGroup_ComputePointAtLength(this,A,B,C,D) (this)->lpVtbl->ComputePointAtLength(this,A,B,C,D)
#define ID2D1GeometryGroup_FillContainsPoint(this,A,B,C) (this)->lpVtbl->FillContainsPoint(this,A,B,C)
#define ID2D1GeometryGroup_GetBounds(this,A,B) (this)->lpVtbl->GetBounds(this,A,B)
#define ID2D1GeometryGroup_GetWidenedBounds(this,A,B,C,D) (this)->lpVtbl->GetWidenedBounds(this,A,B,C,D)
#define ID2D1GeometryGroup_Outline(this,A,B) (this)->lpVtbl->Outline(this,A,B)
#define ID2D1GeometryGroup_StrokeContainsPoint(this,A,B,C,D,E) (this)->lpVtbl->StrokeContainsPoint(this,A,B,C,D,E)
#define ID2D1GeometryGroup_Simplify(this,A,B,C) (this)->lpVtbl->Simplify(this,A,B,C)
#define ID2D1GeometryGroup_Tessellate(this,A,B) (this)->lpVtbl->Tessellate(this,A,B)
#define ID2D1GeometryGroup_Widen(this,A,B,C,D) (this)->lpVtbl->Widen(this,A,B,C,D)
#define ID2D1GeometryGroup_GetFillMode(this) (this)->lpVtbl->GetFillMode(this)
#define ID2D1GeometryGroup_GetSourceGeometries(this,A,B) (this)->lpVtbl->GetSourceGeometries(this,A,B)
#define ID2D1GeometryGroup_GetSourceGeometryCount(this) (this)->lpVtbl->GetSourceGeometryCount(this)

#define INTERFACE ID2D1SimplifiedGeometrySink
DECLARE_INTERFACE_(ID2D1SimplifiedGeometrySink, IUnknown)
{
  BEGIN_INTERFACE

  /* IUnknown methods */
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;

  /* ID2D1SimplifiedGeometrySink methods */
  STDMETHOD_(void, SetFillMode)(THIS_ D2D1_FILL_MODE fillMode) PURE;
  STDMETHOD_(void, SetSegmentFlags)(THIS_ D2D1_PATH_SEGMENT vertexFlags) PURE;
  STDMETHOD_(void, BeginFigure)(THIS_ D2D1_POINT_2F startPoint, D2D1_FIGURE_BEGIN figureBegin) PURE;
  STDMETHOD_(void, AddLines)(THIS_ D2D1_POINT_2F *points, UINT pointsCount) PURE;
  STDMETHOD_(void, AddBeziers)(THIS_ D2D1_BEZIER_SEGMENT *beziers, UINT beziersCount) PURE;
  STDMETHOD_(void, EndFigure)(THIS_ D2D1_FIGURE_END figureEnd) PURE;
  STDMETHOD(Close)(THIS) PURE;

  END_INTERFACE
};
#undef INTERFACE

#define ID2D1SimplifiedGeometrySink_QueryInterface(this,A,B) (this)->lpVtbl->QueryInterface(this,A,B)
#define ID2D1SimplifiedGeometrySink_AddRef(this) (this)->lpVtbl->AddRef(this)
#define ID2D1SimplifiedGeometrySink_Release(this) (this)->lpVtbl->Release(this)
#define ID2D1SimplifiedGeometrySink_AddBeziers(this,A,B) (this)->lpVtbl->AddBeziers(this,A,B)
#define ID2D1SimplifiedGeometrySink_AddLines(this,A,B) (this)->lpVtbl->AddLines(this,A,B)
#define ID2D1SimplifiedGeometrySink_BeginFigure(this,A,B) (this)->lpVtbl->BeginFigure(this,A,B)
#define ID2D1SimplifiedGeometrySink_Close(this) (this)->lpVtbl->Close(this)
#define ID2D1SimplifiedGeometrySink_EndFigure(this,A) (this)->lpVtbl->EndFigure(this,A)
#define ID2D1SimplifiedGeometrySink_SetFillMode(this,A) (this)->lpVtbl->SetFillMode(this,A)
#define ID2D1SimplifiedGeometrySink_SetSegmentFlags(this,A) (this)->lpVtbl->SetSegmentFlags(this,A)

#define INTERFACE ID2D1GeometrySink
DECLARE_INTERFACE_(ID2D1GeometrySink, ID2D1SimplifiedGeometrySink)
{
  BEGIN_INTERFACE

  /* IUnknown methods */
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;

  /* ID2D1SimplifiedGeometrySink methods */
  STDMETHOD_(void, SetFillMode)(THIS_ D2D1_FILL_MODE fillMode) PURE;
  STDMETHOD_(void, SetSegmentFlags)(THIS_ D2D1_PATH_SEGMENT vertexFlags) PURE;
  STDMETHOD_(void, BeginFigure)(THIS_ D2D1_POINT_2F startPoint, D2D1_FIGURE_BEGIN figureBegin) PURE;
  STDMETHOD_(void, AddLines)(THIS_ D2D1_POINT_2F *points, UINT pointsCount) PURE;
  STDMETHOD_(void, AddBeziers)(THIS_ D2D1_BEZIER_SEGMENT *beziers, UINT beziersCount) PURE;
  STDMETHOD_(void, EndFigure)(THIS_ D2D1_FIGURE_END figureEnd) PURE;
  STDMETHOD(Close)(THIS) PURE;

  /* ID2D1GeometrySink methods */
  STDMETHOD_(void, AddLine)(THIS_ D2D1_POINT_2F point) PURE;
  STDMETHOD_(void, AddBezier)(THIS_ D2D1_BEZIER_SEGMENT *bezier) PURE;
  STDMETHOD_(void, AddQuadraticBezier)(THIS_ D2D1_QUADRATIC_BEZIER_SEGMENT *bezier) PURE;
  STDMETHOD_(void, AddQuadraticBeziers)(THIS_ D2D1_QUADRATIC_BEZIER_SEGMENT *beziers, UINT bezierCount) PURE;
  STDMETHOD_(void, AddArc)(THIS_ D2D1_ARC_SEGMENT *arc) PURE;

  END_INTERFACE
};
#undef INTERFACE

#define ID2D1GeometrySink_QueryInterface(this,A,B) (this)->lpVtbl->QueryInterface(this,A,B)
#define ID2D1GeometrySink_AddRef(this) (this)->lpVtbl->AddRef(this)
#define ID2D1GeometrySink_Release(this) (this)->lpVtbl->Release(this)
#define ID2D1GeometrySink_AddBeziers(this,A,B) (this)->lpVtbl->AddBeziers(this,A,B)
#define ID2D1GeometrySink_AddLines(this,A,B) (this)->lpVtbl->AddLines(this,A,B)
#define ID2D1GeometrySink_BeginFigure(this,A,B) (this)->lpVtbl->BeginFigure(this,A,B)
#define ID2D1GeometrySink_Close(this) (this)->lpVtbl->Close(this)
#define ID2D1GeometrySink_EndFigure(this,A) (this)->lpVtbl->EndFigure(this,A)
#define ID2D1GeometrySink_SetFillMode(this,A) (this)->lpVtbl->SetFillMode(this,A)
#define ID2D1GeometrySink_SetSegmentFlags(this,A) (this)->lpVtbl->SetSegmentFlags(this,A)
#define ID2D1GeometrySink_AddArc(this,A) (this)->lpVtbl->AddArc(this,A)
#define ID2D1GeometrySink_AddBezier(this,A) (this)->lpVtbl->AddBezier(this,A)
#define ID2D1GeometrySink_AddLine(this,A) (this)->lpVtbl->AddLine(this,A)
#define ID2D1GeometrySink_AddQuadraticBezier(this,A) (this)->lpVtbl->AddQuadraticBezier(this,A)
#define ID2D1GeometrySink_AddQuadraticBeziers(this,A,B) (this)->lpVtbl->AddQuadraticBeziers(this,A,B)

#define INTERFACE ID2D1GradientStopCollection
DECLARE_INTERFACE_(ID2D1GradientStopCollection, ID2D1Resource)
{
  BEGIN_INTERFACE

  /* IUnknown methods */
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;

  /* ID2D1Resource methods */
  STDMETHOD_(void, GetFactory)(THIS_ ID2D1Factory **factory) PURE;

  /* ID2D1GradientStopCollection methods */
  STDMETHOD_(UINT32, GetGradientStopCount)(THIS) PURE;
  STDMETHOD_(void, GetGradientStops)(THIS_ D2D1_GRADIENT_STOP *gradientStops, UINT gradientStopsCount) PURE;
  STDMETHOD_(D2D1_GAMMA, GetColorInterpolationGamma)(THIS) PURE;
  STDMETHOD_(D2D1_EXTEND_MODE, GetExtendMode)(THIS) PURE;

  END_INTERFACE
};
#undef INTERFACE

#define ID2D1GradientStopCollection_QueryInterface(this,A,B) (this)->lpVtbl->QueryInterface(this,A,B)
#define ID2D1GradientStopCollection_AddRef(this) (this)->lpVtbl->AddRef(this)
#define ID2D1GradientStopCollection_Release(this) (this)->lpVtbl->Release(this)
#define ID2D1GradientStopCollection_GetFactory(this,A) (this)->lpVtbl->GetFactory(this,A)
#define ID2D1GradientStopCollection_GetColorInterpolationGamma(this) (this)->lpVtbl->GetColorInterpolationGamma(this)
#define ID2D1GradientStopCollection_GetExtendMode(this) (this)->lpVtbl->GetExtendMode(this)
#define ID2D1GradientStopCollection_GetGradientStopCount(this) (this)->lpVtbl->GetGradientStopCount(this)
#define ID2D1GradientStopCollection_GetGradientStops(this,A,B) (this)->lpVtbl->GetGradientStops(this,A,B)

#define INTERFACE ID2D1HwndRenderTarget
DECLARE_INTERFACE_(ID2D1HwndRenderTarget, ID2D1RenderTarget)
{
  BEGIN_INTERFACE

  /* IUnknown methods */
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;

  /* ID2D1Resource methods */
  STDMETHOD_(void, GetFactory)(THIS_ ID2D1Factory **factory) PURE;

  /* ID2D1RenderTarget methods */
  STDMETHOD(CreateBitmap)(THIS_ D2D1_SIZE_U size, void *srcData, UINT32 pitch, D2D1_BITMAP_PROPERTIES *bitmapProperties, ID2D1Bitmap **bitmap) PURE;
  STDMETHOD(CreateBitmapFromWicBitmap)(THIS_ IWICBitmapSource *wicBitmapSource, ID2D1Bitmap **bitmap) PURE;
  STDMETHOD(CreateSharedBitmap)(THIS_ REFIID riid, void *data, D2D1_BITMAP_PROPERTIES *bitmapProperties, ID2D1Bitmap **bitmap) PURE;
  STDMETHOD(CreateBitmapBrush)(THIS_ ID2D1Bitmap *bitmap, ID2D1BitmapBrush **bitmapBrush) PURE;
  STDMETHOD(CreateSolidColorBrush)(THIS_ D2D1_COLOR_F *color, D2D1_BRUSH_PROPERTIES *brushProperties, ID2D1SolidColorBrush **solidColorBrush) PURE;
  STDMETHOD(CreateGradientStopCollection)(THIS_ D2D1_GRADIENT_STOP *gradientStops, UINT gradientStopsCount, ID2D1GradientStopCollection **gradientStopCollection) PURE;
  STDMETHOD(CreateLinearGradientBrush)(THIS_ D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES *linearGradientBrushProperties, D2D1_BRUSH_PROPERTIES *brushProperties, ID2D1GradientStopCollection *gradientStopCollection, ID2D1LinearGradientBrush **linearGradientBrush) PURE;
  STDMETHOD(CreateRadialGradientBrush)(THIS_ D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES *radialGradientBrushProperties, D2D1_BRUSH_PROPERTIES *brushProperties, ID2D1GradientStopCollection *gradientStopCollection, ID2D1RadialGradientBrush **radialGradientBrush) PURE;
  STDMETHOD(CreateCompatibleRenderTarget)(THIS_ ID2D1BitmapRenderTarget **bitmapRenderTarget) PURE;
  STDMETHOD(CreateLayer)(THIS_ ID2D1Layer **layer) PURE;
  STDMETHOD(CreateMesh)(THIS_ ID2D1Mesh **mesh) PURE;
  STDMETHOD_(void, DrawLine)(THIS_ D2D1_POINT_2F point0, D2D1_POINT_2F point1, ID2D1Brush *brush, FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle) PURE;
  STDMETHOD_(void, DrawRectangle)(THIS_ D2D1_RECT_F *rect, ID2D1Brush *brush, FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle) PURE;
  STDMETHOD_(void, FillRectangle)(THIS_ D2D1_RECT_F *rect, ID2D1Brush *brush) PURE;
  STDMETHOD_(void, DrawRoundedRectangle)(THIS_ D2D1_ROUNDED_RECT *roundedRect, ID2D1Brush *brush, FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle) PURE;
  STDMETHOD_(void, FillRoundedRectangle)(THIS_ D2D1_ROUNDED_RECT *roundedRect, ID2D1Brush *brush) PURE;
  STDMETHOD_(void, DrawEllipse)(THIS_ D2D1_ELLIPSE *ellipse, ID2D1Brush *brush, FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle) PURE;
  STDMETHOD_(void, FillEllipse)(THIS_ D2D1_ELLIPSE *ellipse, ID2D1Brush *brush) PURE;
  STDMETHOD_(void, DrawGeometry)(THIS_ ID2D1Geometry *geometry, ID2D1Brush *brush, FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle) PURE;
  STDMETHOD_(void, FillGeometry)(THIS_ ID2D1Geometry *geometry, ID2D1Brush *brush, ID2D1Brush *opacityBrush) PURE;
  STDMETHOD_(void, FillMesh)(THIS_ ID2D1Mesh *mesh, ID2D1Brush *brush) PURE;
  STDMETHOD_(void, FillOpacityMask)(THIS_ ID2D1Bitmap *opacityMask, ID2D1Brush *brush, D2D1_OPACITY_MASK_CONTENT content, D2D1_RECT_F *destinationRectangle, D2D1_RECT_F *sourceRectangle) PURE;
  STDMETHOD_(void, DrawBitmap)(THIS_ ID2D1Bitmap *bitmap, D2D1_RECT_F *destinationRectangle, FLOAT opacity, D2D1_BITMAP_INTERPOLATION_MODE interpolationMode , D2D1_RECT_F *sourceRectangle) PURE;
  STDMETHOD_(void, DrawText)(THIS_ WCHAR *string, UINT stringLength, IDWriteTextFormat *textFormat, D2D1_RECT_F *layoutRect, ID2D1Brush *defaultForegroundBrush, D2D1_DRAW_TEXT_OPTIONS options , DWRITE_MEASURING_MODE measuringMode) PURE;
  STDMETHOD_(void, DrawTextLayout)(THIS_ D2D1_POINT_2F origin, IDWriteTextLayout *textLayout, ID2D1Brush *defaultForegroundBrush, D2D1_DRAW_TEXT_OPTIONS options) PURE;
  STDMETHOD_(void, DrawGlyphRun)(THIS_ D2D1_POINT_2F baselineOrigin, DWRITE_GLYPH_RUN *glyphRun, ID2D1Brush *foregroundBrush, DWRITE_MEASURING_MODE measuringMode) PURE;
  STDMETHOD_(void, SetTransform)(THIS_ D2D1_MATRIX_3X2_F *transform) PURE;
  STDMETHOD_(void, GetTransform)(THIS_ D2D1_MATRIX_3X2_F *transform) PURE;
  STDMETHOD_(void, SetAntialiasMode)(THIS_ D2D1_ANTIALIAS_MODE antialiasMode) PURE;
  STDMETHOD_(D2D1_ANTIALIAS_MODE, GetAntialiasMode)(THIS) PURE;
  STDMETHOD_(void, SetTextAntialiasMode)(THIS_ D2D1_TEXT_ANTIALIAS_MODE textAntialiasMode) PURE;
  STDMETHOD_(D2D1_TEXT_ANTIALIAS_MODE, GetTextAntialiasMode)(THIS) PURE;
  STDMETHOD_(void, SetTextRenderingParams)(THIS_ IDWriteRenderingParams *textRenderingParams) PURE;
  STDMETHOD_(void, GetTextRenderingParams)(THIS_ IDWriteRenderingParams **textRenderingParams) PURE;
  STDMETHOD_(void, SetTags)(THIS_ D2D1_TAG tag1, D2D1_TAG tag2) PURE;
  STDMETHOD_(void, GetTags)(THIS_ D2D1_TAG *tag1, D2D1_TAG *tag2) PURE;
  STDMETHOD_(void, PushLayer)(THIS_ D2D1_LAYER_PARAMETERS *layerParameters, ID2D1Layer *layer) PURE;
  STDMETHOD_(void, PopLayer)(THIS) PURE;
  STDMETHOD(Flush)(THIS_ D2D1_TAG *tag1, D2D1_TAG *tag2) PURE;
  STDMETHOD_(void, SaveDrawingState)(THIS_ ID2D1DrawingStateBlock *drawingStateBlock) PURE;
  STDMETHOD_(void, RestoreDrawingState)(THIS_ ID2D1DrawingStateBlock *drawingStateBlock) PURE;
  STDMETHOD_(void, PushAxisAlignedClip)(THIS_ D2D1_RECT_F *clipRect, D2D1_ANTIALIAS_MODE antialiasMode) PURE;
  STDMETHOD_(void, PopAxisAlignedClip)(THIS) PURE;
  STDMETHOD_(void, Clear)(THIS_ D2D1_COLOR_F *clearColor) PURE;
  STDMETHOD_(void, BeginDraw)(THIS) PURE;
  STDMETHOD(EndDraw)(THIS_ D2D1_TAG *tag1, D2D1_TAG *tag2) PURE;
  STDMETHOD_(D2D1_PIXEL_FORMAT, GetPixelFormat)(THIS) PURE;
  STDMETHOD_(void, SetDpi)(THIS_ FLOAT dpiX, FLOAT dpiY) PURE;
  STDMETHOD_(void, GetDpi)(THIS_ FLOAT *dpiX, FLOAT *dpiY) PURE;
  STDMETHOD_(D2D1_SIZE_F, GetSize)(THIS) PURE;
  STDMETHOD_(D2D1_SIZE_U, GetPixelSize)(THIS) PURE;
  STDMETHOD_(UINT32, GetMaximumBitmapSize)(THIS) PURE;
  STDMETHOD_(BOOL, IsSupported)(THIS_ D2D1_RENDER_TARGET_PROPERTIES *renderTargetProperties) PURE;

  /* ID2D1HwndRenderTarget methods */
  STDMETHOD_(D2D1_WINDOW_STATE, CheckWindowState)(THIS) PURE;
  STDMETHOD(Resize)(THIS_ D2D1_SIZE_U *pixelSize) PURE;
  STDMETHOD_(HWND, GetHwnd)(THIS) PURE;

  END_INTERFACE
};
#undef INTERFACE

#define ID2D1HwndRenderTarget_QueryInterface(this,A,B) (this)->lpVtbl->QueryInterface(this,A,B)
#define ID2D1HwndRenderTarget_AddRef(this) (this)->lpVtbl->AddRef(this)
#define ID2D1HwndRenderTarget_Release(this) (this)->lpVtbl->Release(this)
#define ID2D1HwndRenderTarget_BeginDraw(this) (this)->lpVtbl->BeginDraw(this)
#define ID2D1HwndRenderTarget_Clear(this,A) (this)->lpVtbl->Clear(this,A)
#define ID2D1HwndRenderTarget_CreateBitmap(this,A,B,C,D,E) (this)->lpVtbl->CreateBitmap(this,A,B,C,D,E)
#define ID2D1HwndRenderTarget_CreateBitmapBrush(this,A,B) (this)->lpVtbl->CreateBitmapBrush(this,A,B)
#define ID2D1HwndRenderTarget_CreateBitmapFromWicBitmap(this,A,B) (this)->lpVtbl->CreateBitmapFromWicBitmap(this,A,B)
#define ID2D1HwndRenderTarget_CreateCompatibleRenderTarget(this,A) (this)->lpVtbl->CreateCompatibleRenderTarget(this,A)
#define ID2D1HwndRenderTarget_CreateGradientStopCollection(this,A,B,C) (this)->lpVtbl->CreateGradientStopCollection(this,A,B,C)
#define ID2D1HwndRenderTarget_CreateLayer(this,A) (this)->lpVtbl->CreateLayer(this,A)
#define ID2D1HwndRenderTarget_CreateLinearGradientBrush(this,A,B,C,D) (this)->lpVtbl->CreateLinearGradientBrush(this,A,B,C,D)
#define ID2D1HwndRenderTarget_CreateMesh(this,A) (this)->lpVtbl->CreateMesh(this,A)
#define ID2D1HwndRenderTarget_CreateRadialGradientBrush(this,A,B,C,D) (this)->lpVtbl->CreateRadialGradientBrush(this,A,B,C,D)
#define ID2D1HwndRenderTarget_CreateSharedBitmap(this,A,B,C,D) (this)->lpVtbl->CreateSharedBitmap(this,A,B,C,D)
#define ID2D1HwndRenderTarget_CreateSolidColorBrush(this,A,B,C) (this)->lpVtbl->CreateSolidColorBrush(this,A,B,C)
#define ID2D1HwndRenderTarget_DrawBitmap(this,A,B,C,D,E) (this)->lpVtbl->DrawBitmap(this,A,B,C,D,E)
#define ID2D1HwndRenderTarget_DrawEllipse(this,A,B,C,D) (this)->lpVtbl->DrawEllipse(this,A,B,C,D)
#define ID2D1HwndRenderTarget_DrawGeometry(this,A,B,C,D) (this)->lpVtbl->DrawGeometry(this,A,B,C,D)
#define ID2D1HwndRenderTarget_DrawGlyphRun(this,A,B,C,D) (this)->lpVtbl->DrawGlyphRun(this,A,B,C,D)
#define ID2D1HwndRenderTarget_DrawLine(this,A,B,C,D,E) (this)->lpVtbl->DrawLine(this,A,B,C,D,E)
#define ID2D1HwndRenderTarget_DrawRectangle(this,A,B,C,D) (this)->lpVtbl->DrawRectangle(this,A,B,C,D)
#define ID2D1HwndRenderTarget_DrawRoundedRectangle(this,A,B,C,D) (this)->lpVtbl->DrawRoundedRectangle(this,A,B,C,D)
#define ID2D1HwndRenderTarget_DrawText(this,A,B,C,D,E,F,G) (this)->lpVtbl->DrawText(this,A,B,C,D,E,F,G)
#define ID2D1HwndRenderTarget_DrawTextLayout(this,A,B,C,D) (this)->lpVtbl->DrawTextLayout(this,A,B,C,D)
#define ID2D1HwndRenderTarget_EndDraw(this,A,B) (this)->lpVtbl->EndDraw(this,A,B)
#define ID2D1HwndRenderTarget_FillEllipse(this,A,B) (this)->lpVtbl->FillEllipse(this,A,B)
#define ID2D1HwndRenderTarget_FillGeometry(this,A,B,C) (this)->lpVtbl->FillGeometry(this,A,B,C)
#define ID2D1HwndRenderTarget_FillMesh(this,A,B) (this)->lpVtbl->FillMesh(this,A,B)
#define ID2D1HwndRenderTarget_FillOpacityMask(this,A,B,C,D,E) (this)->lpVtbl->FillOpacityMask(this,A,B,C,D,E)
#define ID2D1HwndRenderTarget_FillRectangle(this,A,B) (this)->lpVtbl->FillRectangle(this,A,B)
#define ID2D1HwndRenderTarget_FillRoundedRectangle(this,A,B) (this)->lpVtbl->FillRoundedRectangle(this,A,B)
#define ID2D1HwndRenderTarget_Flush(this,A,B) (this)->lpVtbl->Flush(this,A,B)
#define ID2D1HwndRenderTarget_GetAntialiasMode(this) (this)->lpVtbl->GetAntialiasMode(this)
#define ID2D1HwndRenderTarget_GetDpi(this,A,B) (this)->lpVtbl->GetDpi(this,A,B)
#define ID2D1HwndRenderTarget_GetMaximumBitmapSize(this) (this)->lpVtbl->GetMaximumBitmapSize(this)
#define ID2D1HwndRenderTarget_GetPixelFormat(this) (this)->lpVtbl->GetPixelFormat(this)
#define ID2D1HwndRenderTarget_GetPixelSize(this) (this)->lpVtbl->GetPixelSize(this)
#define ID2D1HwndRenderTarget_GetSize(this) (this)->lpVtbl->GetSize(this)
#define ID2D1HwndRenderTarget_GetTags(this,A,B) (this)->lpVtbl->GetTags(this,A,B)
#define ID2D1HwndRenderTarget_GetTextAntialiasMode(this) (this)->lpVtbl->GetTextAntialiasMode(this)
#define ID2D1HwndRenderTarget_GetTextRenderingParams(this,A) (this)->lpVtbl->GetTextRenderingParams(this,A)
#define ID2D1HwndRenderTarget_GetTransform(this,A) (this)->lpVtbl->GetTransform(this,A)
#define ID2D1HwndRenderTarget_IsSupported(this,A) (this)->lpVtbl->IsSupported(this,A)
#define ID2D1HwndRenderTarget_PopAxisAlignedClip(this) (this)->lpVtbl->PopAxisAlignedClip(this)
#define ID2D1HwndRenderTarget_PopLayer(this) (this)->lpVtbl->PopLayer(this)
#define ID2D1HwndRenderTarget_PushAxisAlignedClip(this,A,B) (this)->lpVtbl->PushAxisAlignedClip(this,A,B)
#define ID2D1HwndRenderTarget_PushLayer(this,A,B) (this)->lpVtbl->PushLayer(this,A,B)
#define ID2D1HwndRenderTarget_RestoreDrawingState(this,A) (this)->lpVtbl->RestoreDrawingState(this,A)
#define ID2D1HwndRenderTarget_SaveDrawingState(this,A) (this)->lpVtbl->SaveDrawingState(this,A)
#define ID2D1HwndRenderTarget_SetAntialiasMode(this,A) (this)->lpVtbl->SetAntialiasMode(this,A)
#define ID2D1HwndRenderTarget_SetDpi(this,A,B) (this)->lpVtbl->SetDpi(this,A,B)
#define ID2D1HwndRenderTarget_SetTags(this,A,B) (this)->lpVtbl->SetTags(this,A,B)
#define ID2D1HwndRenderTarget_SetTextAntialiasMode(this,A) (this)->lpVtbl->SetTextAntialiasMode(this,A)
#define ID2D1HwndRenderTarget_SetTextRenderingParams(this,A) (this)->lpVtbl->SetTextRenderingParams(this,A)
#define ID2D1HwndRenderTarget_SetTransform(this,A) (this)->lpVtbl->SetTransform(this,A)
#define ID2D1HwndRenderTarget_CheckWindowState(this) (this)->lpVtbl->CheckWindowState(this)
#define ID2D1HwndRenderTarget_GetHwnd(this) (this)->lpVtbl->GetHwnd(this)
#define ID2D1HwndRenderTarget_Resize(this,A) (this)->lpVtbl->Resize(this,A)

#define INTERFACE ID2D1Layer
DECLARE_INTERFACE_(ID2D1Layer, ID2D1Resource)
{
  BEGIN_INTERFACE

  /* IUnknown methods */
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;

  /* ID2D1Resource methods */
  STDMETHOD_(void, GetFactory)(THIS_ ID2D1Factory **factory) PURE;

  /* ID2D1Layer methods */
  STDMETHOD_(D2D1_SIZE_F, GetSize)(THIS) PURE;

  END_INTERFACE
};
#undef INTERFACE

#define ID2D1Layer_QueryInterface(this,A,B) (this)->lpVtbl->QueryInterface(this,A,B)
#define ID2D1Layer_AddRef(this) (this)->lpVtbl->AddRef(this)
#define ID2D1Layer_Release(this) (this)->lpVtbl->Release(this)
#define ID2D1Layer_GetFactory(this,A) (this)->lpVtbl->GetFactory(this,A)
#define ID2D1Layer_GetSize(this) (this)->lpVtbl->GetSize(this)

#define INTERFACE ID2D1LinearGradientBrush
DECLARE_INTERFACE_(ID2D1LinearGradientBrush, ID2D1Brush)
{
  BEGIN_INTERFACE

  /* IUnknown methods */
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;

  /* ID2D1Resource methods */
  STDMETHOD_(void, GetFactory)(THIS_ ID2D1Factory **factory) PURE;

  /* ID2D1Brush methods */
  STDMETHOD_(void, SetOpacity)(THIS_ FLOAT opacity) PURE;
  STDMETHOD_(void, SetTransform)(THIS_ D2D1_MATRIX_3X2_F *transform) PURE;
  STDMETHOD_(FLOAT, GetOpacity)(THIS) PURE;
  STDMETHOD_(void, GetTransform)(THIS_ D2D1_MATRIX_3X2_F *transform) PURE;

  /* ID2D1LinearGradientBrush methods */
  STDMETHOD_(void, SetStartPoint)(THIS_ D2D1_POINT_2F startPoint) PURE;
  STDMETHOD_(void, SetEndPoint)(THIS_ D2D1_POINT_2F endPoint) PURE;
  STDMETHOD_(D2D1_POINT_2F, GetStartPoint)(THIS) PURE;
  STDMETHOD_(D2D1_POINT_2F, GetEndPoint)(THIS) PURE;
  STDMETHOD_(void, GetGradientStopCollection)(THIS_ ID2D1GradientStopCollection **gradientStopCollection) PURE;

  END_INTERFACE
};
#undef INTERFACE

#define ID2D1LinearGradientBrush_GetOpacity(this) (this)->lpVtbl->GetOpacity(this)
#define ID2D1LinearGradientBrush_GetTransform(this,A) (this)->lpVtbl->GetTransform(this,A)
#define ID2D1LinearGradientBrush_SetOpacity(this,A) (this)->lpVtbl->SetOpacity(this,A)
#define ID2D1LinearGradientBrush_SetTransform(this,A) (this)->lpVtbl->SetTransform(this,A)
#define ID2D1LinearGradientBrush_GetEndPoint(this) (this)->lpVtbl->GetEndPoint(this)
#define ID2D1LinearGradientBrush_GetGradientStopCollection(this,A) (this)->lpVtbl->GetGradientStopCollection(this,A)
#define ID2D1LinearGradientBrush_GetStartPoint(this) (this)->lpVtbl->GetStartPoint(this)
#define ID2D1LinearGradientBrush_SetEndPoint(this,A) (this)->lpVtbl->SetEndPoint(this,A)
#define ID2D1LinearGradientBrush_SetStartPoint(this,A) (this)->lpVtbl->SetStartPoint(this,A)

#define INTERFACE ID2D1Mesh
DECLARE_INTERFACE_(ID2D1Mesh, ID2D1Resource)
{
  BEGIN_INTERFACE

  /* IUnknown methods */
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;

  /* ID2D1Resource methods */
  STDMETHOD_(void, GetFactory)(THIS_ ID2D1Factory **factory) PURE;

  /* ID2D1Mesh methods */
  STDMETHOD(Open)(THIS_ ID2D1TessellationSink **tessellationSink) PURE;

  END_INTERFACE
};
#undef INTERFACE

#define ID2D1Mesh_QueryInterface(this,A,B) (this)->lpVtbl->QueryInterface(this,A,B)
#define ID2D1Mesh_AddRef(this) (this)->lpVtbl->AddRef(this)
#define ID2D1Mesh_Release(this) (this)->lpVtbl->Release(this)
#define ID2D1Mesh_GetFactory(this,A) (this)->lpVtbl->GetFactory(this,A)
#define ID2D1Mesh_Open(this,A) (this)->lpVtbl->Open(this,A)

#define INTERFACE ID2D1PathGeometry
DECLARE_INTERFACE_(ID2D1PathGeometry, ID2D1Geometry)
{
  BEGIN_INTERFACE

  /* IUnknown methods */
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;

  /* ID2D1Resource methods */
  STDMETHOD_(void, GetFactory)(THIS_ ID2D1Factory **factory) PURE;

  /* ID2D1Geometry methods */
  STDMETHOD(GetBounds)(THIS_ D2D1_MATRIX_3X2_F *worldTransform, D2D1_RECT_F *bounds) PURE;
  STDMETHOD(GetWidenedBounds)(THIS_ FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle, D2D1_MATRIX_3X2_F *worldTransform, D2D1_RECT_F *bounds) PURE;
  STDMETHOD(StrokeContainsPoint)(THIS_ D2D1_POINT_2F point, FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle, D2D1_MATRIX_3X2_F *worldTransform, BOOL *contains) PURE;
  STDMETHOD(FillContainsPoint)(THIS_ D2D1_POINT_2F point, D2D1_MATRIX_3X2_F *worldTransform, BOOL *contains) PURE;
  STDMETHOD(CompareWithGeometry)(THIS_ ID2D1Geometry *inputGeometry, D2D1_MATRIX_3X2_F *inputGeometryTransform, D2D1_GEOMETRY_RELATION *relation) PURE;
  STDMETHOD(Simplify)(THIS_ D2D1_GEOMETRY_SIMPLIFICATION_OPTION simplificationOption, D2D1_MATRIX_3X2_F *worldTransform, ID2D1SimplifiedGeometrySink *geometrySink) PURE;
  STDMETHOD(Tessellate)(THIS_ D2D1_MATRIX_3X2_F *worldTransform, ID2D1TessellationSink *tessellationSink) PURE;
  STDMETHOD(CombineWithGeometry)(THIS_ ID2D1Geometry *inputGeometry, D2D1_COMBINE_MODE combineMode, D2D1_MATRIX_3X2_F *inputGeometryTransform, ID2D1SimplifiedGeometrySink *geometrySink) PURE;
  STDMETHOD(Outline)(THIS_ D2D1_MATRIX_3X2_F *worldTransform, ID2D1SimplifiedGeometrySink *geometrySink) PURE;
  STDMETHOD(ComputeArea)(THIS_ D2D1_MATRIX_3X2_F *worldTransform, FLOAT *area) PURE;
  STDMETHOD(ComputeLength)(THIS_ D2D1_MATRIX_3X2_F *worldTransform, FLOAT *length) PURE;
  STDMETHOD(ComputePointAtLength)(THIS_ FLOAT length, D2D1_MATRIX_3X2_F *worldTransform, D2D1_POINT_2F *point, D2D1_POINT_2F *unitTangentVector) PURE;
  STDMETHOD(Widen)(THIS_ FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle, D2D1_MATRIX_3X2_F *worldTransform, ID2D1SimplifiedGeometrySink *geometrySink) PURE;

  /* ID2D1PathGeometry methods */
  STDMETHOD(Open)(THIS_ ID2D1GeometrySink **geometrySink) PURE;
  STDMETHOD(Stream)(THIS_ ID2D1GeometrySink *geometrySink) PURE;
  STDMETHOD(GetSegmentCount)(THIS_ UINT32 *count) PURE;
  STDMETHOD(GetFigureCount)(THIS_ UINT32 *count) PURE;

  END_INTERFACE
};
#undef INTERFACE

#define ID2D1PathGeometry_CombineWithGeometry(this,A,B,C,D) (this)->lpVtbl->CombineWithGeometry(this,A,B,C,D)
#define ID2D1PathGeometry_CompareWithGeometry(this,A,B,C) (this)->lpVtbl->CompareWithGeometry(this,A,B,C)
#define ID2D1PathGeometry_ComputeArea(this,A,B) (this)->lpVtbl->ComputeArea(this,A,B)
#define ID2D1PathGeometry_ComputeLength(this,A,B) (this)->lpVtbl->ComputeLength(this,A,B)
#define ID2D1PathGeometry_ComputePointAtLength(this,A,B,C,D) (this)->lpVtbl->ComputePointAtLength(this,A,B,C,D)
#define ID2D1PathGeometry_FillContainsPoint(this,A,B,C) (this)->lpVtbl->FillContainsPoint(this,A,B,C)
#define ID2D1PathGeometry_GetBounds(this,A,B) (this)->lpVtbl->GetBounds(this,A,B)
#define ID2D1PathGeometry_GetWidenedBounds(this,A,B,C,D) (this)->lpVtbl->GetWidenedBounds(this,A,B,C,D)
#define ID2D1PathGeometry_Outline(this,A,B) (this)->lpVtbl->Outline(this,A,B)
#define ID2D1PathGeometry_StrokeContainsPoint(this,A,B,C,D,E) (this)->lpVtbl->StrokeContainsPoint(this,A,B,C,D,E)
#define ID2D1PathGeometry_Simplify(this,A,B,C) (this)->lpVtbl->Simplify(this,A,B,C)
#define ID2D1PathGeometry_Tessellate(this,A,B) (this)->lpVtbl->Tessellate(this,A,B)
#define ID2D1PathGeometry_Widen(this,A,B,C,D) (this)->lpVtbl->Widen(this,A,B,C,D)
#define ID2D1PathGeometry_GetFigureCount(this,A) (this)->lpVtbl->GetFigureCount(this,A)
#define ID2D1PathGeometry_GetSegmentCount(this,A) (this)->lpVtbl->GetSegmentCount(this,A)
#define ID2D1PathGeometry_Open(this,A) (this)->lpVtbl->Open(this,A)
#define ID2D1PathGeometry_Stream(this,A) (this)->lpVtbl->Stream(this,A)

#define INTERFACE ID2D1RadialGradientBrush
DECLARE_INTERFACE_(ID2D1RadialGradientBrush, ID2D1Brush)
{
  BEGIN_INTERFACE

  /* IUnknown methods */
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;

  /* ID2D1Resource methods */
  STDMETHOD_(void, GetFactory)(THIS_ ID2D1Factory **factory) PURE;

  /* ID2D1Brush methods */
  STDMETHOD_(void, SetOpacity)(THIS_ FLOAT opacity) PURE;
  STDMETHOD_(void, SetTransform)(THIS_ D2D1_MATRIX_3X2_F *transform) PURE;
  STDMETHOD_(FLOAT, GetOpacity)(THIS) PURE;
  STDMETHOD_(void, GetTransform)(THIS_ D2D1_MATRIX_3X2_F *transform) PURE;

  /* ID2D1RadialGradientBrush methods */
  STDMETHOD_(void, SetCenter)(THIS_ D2D1_POINT_2F center) PURE;
  STDMETHOD_(void, SetGradientOriginOffset)(THIS_ D2D1_POINT_2F gradientOriginOffset) PURE;
  STDMETHOD_(void, SetRadiusX)(THIS_ FLOAT radiusX) PURE;
  STDMETHOD_(void, SetRadiusY)(THIS_ FLOAT radiusY) PURE;
  STDMETHOD_(D2D1_POINT_2F, GetCenter)(THIS) PURE;
  STDMETHOD_(D2D1_POINT_2F, GetGradientOriginOffset)(THIS) PURE;
  STDMETHOD_(FLOAT, GetRadiusX)(THIS) PURE;
  STDMETHOD_(FLOAT, GetRadiusY)(THIS) PURE;
  STDMETHOD_(void, GetGradientStopCollection)(THIS_ ID2D1GradientStopCollection **gradientStopCollection) PURE;

  END_INTERFACE
};
#undef INTERFACE

#define ID2D1RadialGradientBrush_GetOpacity(this) (this)->lpVtbl->GetOpacity(this)
#define ID2D1RadialGradientBrush_GetTransform(this,A) (this)->lpVtbl->GetTransform(this,A)
#define ID2D1RadialGradientBrush_SetOpacity(this,A) (this)->lpVtbl->SetOpacity(this,A)
#define ID2D1RadialGradientBrush_SetTransform(this,A) (this)->lpVtbl->SetTransform(this,A)
#define ID2D1RadialGradientBrush_GetCenter(this) (this)->lpVtbl->GetCenter(this)
#define ID2D1RadialGradientBrush_GetGradientOriginOffset(this) (this)->lpVtbl->GetGradientOriginOffset(this)
#define ID2D1RadialGradientBrush_GetGradientStopCollection(this,A) (this)->lpVtbl->GetGradientStopCollection(this,A)
#define ID2D1RadialGradientBrush_GetRadiusX(this) (this)->lpVtbl->GetRadiusX(this)
#define ID2D1RadialGradientBrush_GetRadiusY(this) (this)->lpVtbl->GetRadiusY(this)
#define ID2D1RadialGradientBrush_SetCenter(this,A) (this)->lpVtbl->SetCenter(this,A)
#define ID2D1RadialGradientBrush_SetGradientOriginOffset(this,A) (this)->lpVtbl->SetGradientOriginOffset(this,A)
#define ID2D1RadialGradientBrush_SetRadiusX(this,A) (this)->lpVtbl->SetRadiusX(this,A)
#define ID2D1RadialGradientBrush_SetRadiusY(this,A) (this)->lpVtbl->SetRadiusY(this,A)

#define INTERFACE ID2D1RectangleGeometry
DECLARE_INTERFACE_(ID2D1RectangleGeometry, ID2D1Geometry)
{
  BEGIN_INTERFACE

  /* IUnknown methods */
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;

  /* ID2D1Resource methods */
  STDMETHOD_(void, GetFactory)(THIS_ ID2D1Factory **factory) PURE;

  /* ID2D1Geometry methods */
  STDMETHOD(GetBounds)(THIS_ D2D1_MATRIX_3X2_F *worldTransform, D2D1_RECT_F *bounds) PURE;
  STDMETHOD(GetWidenedBounds)(THIS_ FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle, D2D1_MATRIX_3X2_F *worldTransform, D2D1_RECT_F *bounds) PURE;
  STDMETHOD(StrokeContainsPoint)(THIS_ D2D1_POINT_2F point, FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle, D2D1_MATRIX_3X2_F *worldTransform, BOOL *contains) PURE;
  STDMETHOD(FillContainsPoint)(THIS_ D2D1_POINT_2F point, D2D1_MATRIX_3X2_F *worldTransform, BOOL *contains) PURE;
  STDMETHOD(CompareWithGeometry)(THIS_ ID2D1Geometry *inputGeometry, D2D1_MATRIX_3X2_F *inputGeometryTransform, D2D1_GEOMETRY_RELATION *relation) PURE;
  STDMETHOD(Simplify)(THIS_ D2D1_GEOMETRY_SIMPLIFICATION_OPTION simplificationOption, D2D1_MATRIX_3X2_F *worldTransform, ID2D1SimplifiedGeometrySink *geometrySink) PURE;
  STDMETHOD(Tessellate)(THIS_ D2D1_MATRIX_3X2_F *worldTransform, ID2D1TessellationSink *tessellationSink) PURE;
  STDMETHOD(CombineWithGeometry)(THIS_ ID2D1Geometry *inputGeometry, D2D1_COMBINE_MODE combineMode, D2D1_MATRIX_3X2_F *inputGeometryTransform, ID2D1SimplifiedGeometrySink *geometrySink) PURE;
  STDMETHOD(Outline)(THIS_ D2D1_MATRIX_3X2_F *worldTransform, ID2D1SimplifiedGeometrySink *geometrySink) PURE;
  STDMETHOD(ComputeArea)(THIS_ D2D1_MATRIX_3X2_F *worldTransform, FLOAT *area) PURE;
  STDMETHOD(ComputeLength)(THIS_ D2D1_MATRIX_3X2_F *worldTransform, FLOAT *length) PURE;
  STDMETHOD(ComputePointAtLength)(THIS_ FLOAT length, D2D1_MATRIX_3X2_F *worldTransform, D2D1_POINT_2F *point, D2D1_POINT_2F *unitTangentVector) PURE;
  STDMETHOD(Widen)(THIS_ FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle, D2D1_MATRIX_3X2_F *worldTransform, ID2D1SimplifiedGeometrySink *geometrySink) PURE;

  /* ID2D1RectangleGeometry methods */
  STDMETHOD_(void, GetRect)(THIS_ D2D1_RECT_F *rect) PURE;

  END_INTERFACE
};
#undef INTERFACE

#define ID2D1RectangleGeometry_CombineWithGeometry(this,A,B,C,D) (this)->lpVtbl->CombineWithGeometry(this,A,B,C,D)
#define ID2D1RectangleGeometry_CompareWithGeometry(this,A,B,C) (this)->lpVtbl->CompareWithGeometry(this,A,B,C)
#define ID2D1RectangleGeometry_ComputeArea(this,A,B) (this)->lpVtbl->ComputeArea(this,A,B)
#define ID2D1RectangleGeometry_ComputeLength(this,A,B) (this)->lpVtbl->ComputeLength(this,A,B)
#define ID2D1RectangleGeometry_ComputePointAtLength(this,A,B,C,D) (this)->lpVtbl->ComputePointAtLength(this,A,B,C,D)
#define ID2D1RectangleGeometry_FillContainsPoint(this,A,B,C) (this)->lpVtbl->FillContainsPoint(this,A,B,C)
#define ID2D1RectangleGeometry_GetBounds(this,A,B) (this)->lpVtbl->GetBounds(this,A,B)
#define ID2D1RectangleGeometry_GetWidenedBounds(this,A,B,C,D) (this)->lpVtbl->GetWidenedBounds(this,A,B,C,D)
#define ID2D1RectangleGeometry_Outline(this,A,B) (this)->lpVtbl->Outline(this,A,B)
#define ID2D1RectangleGeometry_StrokeContainsPoint(this,A,B,C,D,E) (this)->lpVtbl->StrokeContainsPoint(this,A,B,C,D,E)
#define ID2D1RectangleGeometry_Simplify(this,A,B,C) (this)->lpVtbl->Simplify(this,A,B,C)
#define ID2D1RectangleGeometry_Tessellate(this,A,B) (this)->lpVtbl->Tessellate(this,A,B)
#define ID2D1RectangleGeometry_Widen(this,A,B,C,D) (this)->lpVtbl->Widen(this,A,B,C,D)
#define ID2D1RectangleGeometry_GetRect(this,A) (this)->lpVtbl->GetRect(this,A)

#define INTERFACE ID2D1RoundedRectangleGeometry
DECLARE_INTERFACE_(ID2D1RoundedRectangleGeometry, ID2D1Geometry)
{
  BEGIN_INTERFACE

  /* IUnknown methods */
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;

  /* ID2D1Resource methods */
  STDMETHOD_(void, GetFactory)(THIS_ ID2D1Factory **factory) PURE;

  /* ID2D1Geometry methods */
  STDMETHOD(GetBounds)(THIS_ D2D1_MATRIX_3X2_F *worldTransform, D2D1_RECT_F *bounds) PURE;
  STDMETHOD(GetWidenedBounds)(THIS_ FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle, D2D1_MATRIX_3X2_F *worldTransform, D2D1_RECT_F *bounds) PURE;
  STDMETHOD(StrokeContainsPoint)(THIS_ D2D1_POINT_2F point, FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle, D2D1_MATRIX_3X2_F *worldTransform, BOOL *contains) PURE;
  STDMETHOD(FillContainsPoint)(THIS_ D2D1_POINT_2F point, D2D1_MATRIX_3X2_F *worldTransform, BOOL *contains) PURE;
  STDMETHOD(CompareWithGeometry)(THIS_ ID2D1Geometry *inputGeometry, D2D1_MATRIX_3X2_F *inputGeometryTransform, D2D1_GEOMETRY_RELATION *relation) PURE;
  STDMETHOD(Simplify)(THIS_ D2D1_GEOMETRY_SIMPLIFICATION_OPTION simplificationOption, D2D1_MATRIX_3X2_F *worldTransform, ID2D1SimplifiedGeometrySink *geometrySink) PURE;
  STDMETHOD(Tessellate)(THIS_ D2D1_MATRIX_3X2_F *worldTransform, ID2D1TessellationSink *tessellationSink) PURE;
  STDMETHOD(CombineWithGeometry)(THIS_ ID2D1Geometry *inputGeometry, D2D1_COMBINE_MODE combineMode, D2D1_MATRIX_3X2_F *inputGeometryTransform, ID2D1SimplifiedGeometrySink *geometrySink) PURE;
  STDMETHOD(Outline)(THIS_ D2D1_MATRIX_3X2_F *worldTransform, ID2D1SimplifiedGeometrySink *geometrySink) PURE;
  STDMETHOD(ComputeArea)(THIS_ D2D1_MATRIX_3X2_F *worldTransform, FLOAT *area) PURE;
  STDMETHOD(ComputeLength)(THIS_ D2D1_MATRIX_3X2_F *worldTransform, FLOAT *length) PURE;
  STDMETHOD(ComputePointAtLength)(THIS_ FLOAT length, D2D1_MATRIX_3X2_F *worldTransform, D2D1_POINT_2F *point, D2D1_POINT_2F *unitTangentVector) PURE;
  STDMETHOD(Widen)(THIS_ FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle, D2D1_MATRIX_3X2_F *worldTransform, ID2D1SimplifiedGeometrySink *geometrySink) PURE;

  /* ID2D1RoundedRectangleGeometry methods */
  STDMETHOD_(void, GetRoundedRect)(THIS_ D2D1_ROUNDED_RECT *roundedRect) PURE;

  END_INTERFACE
};
#undef INTERFACE

#define ID2D1RoundedRectangleGeometry_CombineWithGeometry(this,A,B,C,D) (this)->lpVtbl->CombineWithGeometry(this,A,B,C,D)
#define ID2D1RoundedRectangleGeometry_CompareWithGeometry(this,A,B,C) (this)->lpVtbl->CompareWithGeometry(this,A,B,C)
#define ID2D1RoundedRectangleGeometry_ComputeArea(this,A,B) (this)->lpVtbl->ComputeArea(this,A,B)
#define ID2D1RoundedRectangleGeometry_ComputeLength(this,A,B) (this)->lpVtbl->ComputeLength(this,A,B)
#define ID2D1RoundedRectangleGeometry_ComputePointAtLength(this,A,B,C,D) (this)->lpVtbl->ComputePointAtLength(this,A,B,C,D)
#define ID2D1RoundedRectangleGeometry_FillContainsPoint(this,A,B,C) (this)->lpVtbl->FillContainsPoint(this,A,B,C)
#define ID2D1RoundedRectangleGeometry_GetBounds(this,A,B) (this)->lpVtbl->GetBounds(this,A,B)
#define ID2D1RoundedRectangleGeometry_GetWidenedBounds(this,A,B,C,D) (this)->lpVtbl->GetWidenedBounds(this,A,B,C,D)
#define ID2D1RoundedRectangleGeometry_Outline(this,A,B) (this)->lpVtbl->Outline(this,A,B)
#define ID2D1RoundedRectangleGeometry_StrokeContainsPoint(this,A,B,C,D,E) (this)->lpVtbl->StrokeContainsPoint(this,A,B,C,D,E)
#define ID2D1RoundedRectangleGeometry_Simplify(this,A,B,C) (this)->lpVtbl->Simplify(this,A,B,C)
#define ID2D1RoundedRectangleGeometry_Tessellate(this,A,B) (this)->lpVtbl->Tessellate(this,A,B)
#define ID2D1RoundedRectangleGeometry_Widen(this,A,B,C,D) (this)->lpVtbl->Widen(this,A,B,C,D)
#define ID2D1RoundedRectangleGeometry_GetRoundedRect(this,A) (this)->lpVtbl->GetRoundedRect(this,A)

#define INTERFACE ID2D1SolidColorBrush
DECLARE_INTERFACE_(ID2D1SolidColorBrush, ID2D1Brush)
{
  BEGIN_INTERFACE

  /* IUnknown methods */
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;

  /* ID2D1Resource methods */
  STDMETHOD_(void, GetFactory)(THIS_ ID2D1Factory **factory) PURE;

  /* ID2D1Brush methods */
  STDMETHOD_(void, SetOpacity)(THIS_ FLOAT opacity) PURE;
  STDMETHOD_(void, SetTransform)(THIS_ D2D1_MATRIX_3X2_F *transform) PURE;
  STDMETHOD_(FLOAT, GetOpacity)(THIS) PURE;
  STDMETHOD_(void, GetTransform)(THIS_ D2D1_MATRIX_3X2_F *transform) PURE;

  /* ID2D1SolidColorBrush methods */
  STDMETHOD_(void, SetColor)(THIS_ D2D1_COLOR_F *color) PURE;
  STDMETHOD_(D2D1_COLOR_F, GetColor)(THIS) PURE;

  END_INTERFACE
};
#undef INTERFACE

#define ID2D1SolidColorBrush_GetOpacity(this) (this)->lpVtbl->GetOpacity(this)
#define ID2D1SolidColorBrush_GetTransform(this,A) (this)->lpVtbl->GetTransform(this,A)
#define ID2D1SolidColorBrush_SetOpacity(this,A) (this)->lpVtbl->SetOpacity(this,A)
#define ID2D1SolidColorBrush_SetTransform(this,A) (this)->lpVtbl->SetTransform(this,A)
#define ID2D1SolidColorBrush_GetColor(this) (this)->lpVtbl->GetColor(this)
#define ID2D1SolidColorBrush_SetColor(this,A) (this)->lpVtbl->SetColor(this,A)

#define INTERFACE ID2D1StrokeStyle
DECLARE_INTERFACE_(ID2D1StrokeStyle, ID2D1Resource)
{
  BEGIN_INTERFACE

  /* IUnknown methods */
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;

  /* ID2D1Resource methods */
  STDMETHOD_(void, GetFactory)(THIS_ ID2D1Factory **factory) PURE;

  /* ID2D1StrokeStyle methods */
  STDMETHOD_(D2D1_CAP_STYLE, GetStartCap)(THIS) PURE;
  STDMETHOD_(D2D1_CAP_STYLE, GetEndCap)(THIS) PURE;
  STDMETHOD_(D2D1_CAP_STYLE, GetDashCap)(THIS) PURE;
  STDMETHOD_(FLOAT, GetMiterLimit)(THIS) PURE;
  STDMETHOD_(D2D1_LINE_JOIN, GetLineJoin)(THIS) PURE;
  STDMETHOD_(FLOAT, GetDashOffset)(THIS) PURE;
  STDMETHOD_(D2D1_DASH_STYLE, GetDashStyle)(THIS) PURE;
  STDMETHOD_(UINT32, GetDashesCount)(THIS) PURE;
  STDMETHOD_(void, GetDashes)(THIS_ FLOAT *dashes, UINT dashesCount) PURE;

  END_INTERFACE
};
#undef INTERFACE

#define ID2D1StrokeStyle_QueryInterface(this,A,B) (this)->lpVtbl->QueryInterface(this,A,B)
#define ID2D1StrokeStyle_AddRef(this) (this)->lpVtbl->AddRef(this)
#define ID2D1StrokeStyle_Release(this) (this)->lpVtbl->Release(this)
#define ID2D1StrokeStyle_GetFactory(this,A) (this)->lpVtbl->GetFactory(this,A)
#define ID2D1StrokeStyle_GetDashCap(this) (this)->lpVtbl->GetDashCap(this)
#define ID2D1StrokeStyle_GetDashes(this,A,B) (this)->lpVtbl->GetDashes(this,A,B)
#define ID2D1StrokeStyle_GetDashesCount(this) (this)->lpVtbl->GetDashesCount(this)
#define ID2D1StrokeStyle_GetDashOffset(this) (this)->lpVtbl->GetDashOffset(this)
#define ID2D1StrokeStyle_GetDashStyle(this) (this)->lpVtbl->GetDashStyle(this)
#define ID2D1StrokeStyle_GetEndCap(this) (this)->lpVtbl->GetEndCap(this)
#define ID2D1StrokeStyle_GetLineJoin(this) (this)->lpVtbl->GetLineJoin(this)
#define ID2D1StrokeStyle_GetMiterLimit(this) (this)->lpVtbl->GetMiterLimit(this)
#define ID2D1StrokeStyle_GetStartCap(this) (this)->lpVtbl->GetStartCap(this)

#define INTERFACE ID2D1TessellationSink
DECLARE_INTERFACE_(ID2D1TessellationSink, IUnknown)
{
  BEGIN_INTERFACE

  /* IUnknown methods */
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;

  /* ID2D1TessellationSink methods */
  STDMETHOD_(void, AddTriangles)(THIS_ D2D1_TRIANGLE *triangles, UINT trianglesCount) PURE;
  STDMETHOD(Close)(THIS) PURE;

  END_INTERFACE
};
#undef INTERFACE

#define ID2D1TessellationSink_QueryInterface(this,A,B) (this)->lpVtbl->QueryInterface(this,A,B)
#define ID2D1TessellationSink_AddRef(this) (this)->lpVtbl->AddRef(this)
#define ID2D1TessellationSink_Release(this) (this)->lpVtbl->Release(this)
#define ID2D1TessellationSink_AddTriangles(this,A,B) (this)->lpVtbl->AddTriangles(this,A,B)
#define ID2D1TessellationSink_Close(this) (this)->lpVtbl->Close(this)

#define INTERFACE ID2D1TransformedGeometry
DECLARE_INTERFACE_(ID2D1TransformedGeometry, ID2D1Geometry)
{
  BEGIN_INTERFACE

  /* IUnknown methods */
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppvObject) PURE;
  STDMETHOD_(ULONG, AddRef)(THIS) PURE;
  STDMETHOD_(ULONG, Release)(THIS) PURE;

  /* ID2D1Resource methods */
  STDMETHOD_(void, GetFactory)(THIS_ ID2D1Factory **factory) PURE;

  /* ID2D1Geometry methods */
  STDMETHOD(GetBounds)(THIS_ D2D1_MATRIX_3X2_F *worldTransform, D2D1_RECT_F *bounds) PURE;
  STDMETHOD(GetWidenedBounds)(THIS_ FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle, D2D1_MATRIX_3X2_F *worldTransform, D2D1_RECT_F *bounds) PURE;
  STDMETHOD(StrokeContainsPoint)(THIS_ D2D1_POINT_2F point, FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle, D2D1_MATRIX_3X2_F *worldTransform, BOOL *contains) PURE;
  STDMETHOD(FillContainsPoint)(THIS_ D2D1_POINT_2F point, D2D1_MATRIX_3X2_F *worldTransform, BOOL *contains) PURE;
  STDMETHOD(CompareWithGeometry)(THIS_ ID2D1Geometry *inputGeometry, D2D1_MATRIX_3X2_F *inputGeometryTransform, D2D1_GEOMETRY_RELATION *relation) PURE;
  STDMETHOD(Simplify)(THIS_ D2D1_GEOMETRY_SIMPLIFICATION_OPTION simplificationOption, D2D1_MATRIX_3X2_F *worldTransform, ID2D1SimplifiedGeometrySink *geometrySink) PURE;
  STDMETHOD(Tessellate)(THIS_ D2D1_MATRIX_3X2_F *worldTransform, ID2D1TessellationSink *tessellationSink) PURE;
  STDMETHOD(CombineWithGeometry)(THIS_ ID2D1Geometry *inputGeometry, D2D1_COMBINE_MODE combineMode, D2D1_MATRIX_3X2_F *inputGeometryTransform, ID2D1SimplifiedGeometrySink *geometrySink) PURE;
  STDMETHOD(Outline)(THIS_ D2D1_MATRIX_3X2_F *worldTransform, ID2D1SimplifiedGeometrySink *geometrySink) PURE;
  STDMETHOD(ComputeArea)(THIS_ D2D1_MATRIX_3X2_F *worldTransform, FLOAT *area) PURE;
  STDMETHOD(ComputeLength)(THIS_ D2D1_MATRIX_3X2_F *worldTransform, FLOAT *length) PURE;
  STDMETHOD(ComputePointAtLength)(THIS_ FLOAT length, D2D1_MATRIX_3X2_F *worldTransform, D2D1_POINT_2F *point, D2D1_POINT_2F *unitTangentVector) PURE;
  STDMETHOD(Widen)(THIS_ FLOAT strokeWidth, ID2D1StrokeStyle *strokeStyle, D2D1_MATRIX_3X2_F *worldTransform, ID2D1SimplifiedGeometrySink *geometrySink) PURE;

  /* ID2D1TransformedGeometry methods */
  STDMETHOD_(void, GetSourceGeometry)(THIS_ ID2D1Geometry **sourceGeometry) PURE;
  STDMETHOD_(void, GetTransform)(THIS_ D2D1_MATRIX_3X2_F *transform) PURE;

  END_INTERFACE
};
#undef INTERFACE

#define ID2D1TransformedGeometry_CombineWithGeometry(this,A,B,C,D) (this)->lpVtbl->CombineWithGeometry(this,A,B,C,D)
#define ID2D1TransformedGeometry_CompareWithGeometry(this,A,B,C) (this)->lpVtbl->CompareWithGeometry(this,A,B,C)
#define ID2D1TransformedGeometry_ComputeArea(this,A,B) (this)->lpVtbl->ComputeArea(this,A,B)
#define ID2D1TransformedGeometry_ComputeLength(this,A,B) (this)->lpVtbl->ComputeLength(this,A,B)
#define ID2D1TransformedGeometry_ComputePointAtLength(this,A,B,C,D) (this)->lpVtbl->ComputePointAtLength(this,A,B,C,D)
#define ID2D1TransformedGeometry_FillContainsPoint(this,A,B,C) (this)->lpVtbl->FillContainsPoint(this,A,B,C)
#define ID2D1TransformedGeometry_GetBounds(this,A,B) (this)->lpVtbl->GetBounds(this,A,B)
#define ID2D1TransformedGeometry_GetWidenedBounds(this,A,B,C,D) (this)->lpVtbl->GetWidenedBounds(this,A,B,C,D)
#define ID2D1TransformedGeometry_Outline(this,A,B) (this)->lpVtbl->Outline(this,A,B)
#define ID2D1TransformedGeometry_StrokeContainsPoint(this,A,B,C,D,E) (this)->lpVtbl->StrokeContainsPoint(this,A,B,C,D,E)
#define ID2D1TransformedGeometry_Simplify(this,A,B,C) (this)->lpVtbl->Simplify(this,A,B,C)
#define ID2D1TransformedGeometry_Tessellate(this,A,B) (this)->lpVtbl->Tessellate(this,A,B)
#define ID2D1TransformedGeometry_Widen(this,A,B,C,D) (this)->lpVtbl->Widen(this,A,B,C,D)
#define ID2D1TransformedGeometry_GetSourceGeometry(this,A) (this)->lpVtbl->GetSourceGeometry(this,A)
#define ID2D1TransformedGeometry_GetTransform(this,A) (this)->lpVtbl->GetTransform(this,A)



static const IID IID_ID2D1Factory = {0x06152247,0x6f50,0x465a,{0x92,0x45,0x11,0x8b,0xfd,0x3b,0x60,0x07}};


#ifdef __cplusplus
extern "C" {
#endif

HRESULT WINAPI D2D1CreateFactory(
  D2D1_FACTORY_TYPE factoryType,
  REFIID riid,
  CONST D2D1_FACTORY_OPTIONS *pFactoryOptions,
  void **ppIFactory
);
  
WINBOOL WINAPI D2D1InvertMatrix(
  D2D1_MATRIX_3X2_F *matrix
);

WINBOOL WINAPI D2D1IsMatrixInvertible(
  const D2D1_MATRIX_3X2_F *matrix
);

void WINAPI D2D1MakeRotateMatrix(
  FLOAT angle,
  D2D1_POINT_2F center,
  D2D1_MATRIX_3X2_F *matrix
);

void WINAPI D2D1MakeSkewMatrix(
  FLOAT angleX,
  FLOAT angleY,
  D2D1_POINT_2F center,
  D2D1_MATRIX_3X2_F *matrix
);

#ifdef __cplusplus
}
#endif

#endif /* _D2D1_H */
