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


#ifndef __gc_hal_eglplatform_h_
#define __gc_hal_eglplatform_h_

/* Include VDK types. */
#include "gc_hal_types.h"
#include "gc_hal_base.h"
#include "gc_hal_eglplatform_type.h"
#ifdef __cplusplus
extern "C" {
#endif


#if defined(_WIN32) || defined(__VC32__) && !defined(__CYGWIN__) && !defined(__SCITECH_SNAP__)
/* Win32 and Windows CE platforms. */
#include <windows.h>
typedef HDC             HALNativeDisplayType;
typedef HWND            HALNativeWindowType;
typedef HBITMAP         HALNativePixmapType;

typedef struct __BITFIELDINFO{
    BITMAPINFO    bmi;
    RGBQUAD       bmiColors[2];
} BITFIELDINFO;

#elif defined(LINUX) && defined(EGL_API_DFB) && !defined(__APPLE__)
#include <directfb.h>
typedef struct _DFBDisplay * HALNativeDisplayType;
typedef IDirectFBWindow *  HALNativeWindowType;
typedef struct _DFBPixmap *  HALNativePixmapType;

#elif defined(LINUX) && defined(EGL_API_FB) && !defined(__APPLE__)

#if defined(EGL_API_WL)
/* Wayland platform. */
#include "wayland-server.h"
#include <wayland-egl.h>

#define WL_EGL_NUM_BACKBUFFERS 2

typedef struct _gcsWL_VIV_BUFFER
{
   struct wl_buffer wl_buffer;
   gcoSURF surface;
} gcsWL_VIV_BUFFER;

typedef struct _gcsWL_EGL_DISPLAY
{
   struct wl_display* wl_display;
   struct wl_viv* wl_viv;
} gcsWL_EGL_DISPLAY;

typedef struct _gcsWL_EGL_BUFFER_INFO
{
   gctINT32 width;
   gctINT32 height;
   gctINT32 stride;
   gceSURF_FORMAT format;
   gcuVIDMEM_NODE_PTR node;
   gcePOOL pool;
   gctUINT bytes;
   gcoSURF surface;
} gcsWL_EGL_BUFFER_INFO;

typedef struct _gcsWL_EGL_BUFFER
{
   struct wl_buffer* wl_buffer;
   gcsWL_EGL_BUFFER_INFO info;
} gcsWL_EGL_BUFFER;

typedef struct _gcsWL_EGL_WINDOW_INFO
{
   gctUINT width;
   gctUINT height;
   gceSURF_FORMAT format;
   gctUINT bpp;
} gcsWL_EGL_WINDOW_INFO;

struct wl_egl_window
{
   gcsWL_EGL_BUFFER backbuffers[WL_EGL_NUM_BACKBUFFERS];
   gcsWL_EGL_WINDOW_INFO info;
   gctUINT current;
   struct wl_surface* surface;
   struct wl_callback* pending;
};

typedef void*   HALNativeDisplayType;
typedef void*   HALNativeWindowType;
typedef void*   HALNativePixmapType;
#else
/* Linux platform for FBDEV. */
typedef struct _FBDisplay * HALNativeDisplayType;
typedef struct _FBWindow *  HALNativeWindowType;
typedef struct _FBPixmap *  HALNativePixmapType;
#endif
#elif defined(__ANDROID__) || defined(ANDROID)

struct egl_native_pixmap_t;

#if ANDROID_SDK_VERSION >= 9
    #include <android/native_window.h>

    typedef struct ANativeWindow*           HALNativeWindowType;
    typedef struct egl_native_pixmap_t*     HALNativePixmapType;
    typedef void*                           HALNativeDisplayType;
#else
    struct android_native_window_t;
    typedef struct android_native_window_t*    HALNativeWindowType;
    typedef struct egl_native_pixmap_t *        HALNativePixmapType;
    typedef void*                               HALNativeDisplayType;
#endif

#elif defined(LINUX) || defined(__APPLE__)
/* X11 platform. */
#include <X11/Xlib.h>
#include <X11/Xutil.h>

typedef Display *   HALNativeDisplayType;
typedef Window      HALNativeWindowType;

#ifdef CUSTOM_PIXMAP
typedef void *      HALNativePixmapType;
#else
typedef Pixmap      HALNativePixmapType;
#endif /* CUSTOM_PIXMAP */

/* Rename some badly named X defines. */
#ifdef Status
#   define XStatus      int
#   undef Status
#endif
#ifdef Always
#   define XAlways      2
#   undef Always
#endif
#ifdef CurrentTime
#   undef CurrentTime
#   define XCurrentTime 0
#endif

#elif defined(__QNXNTO__)
#include <screen/screen.h>

/* VOID */
typedef int              HALNativeDisplayType;
typedef screen_window_t  HALNativeWindowType;
typedef screen_pixmap_t  HALNativePixmapType;

#else

#error "Platform not recognized"

/* VOID */
typedef void *  HALNativeDisplayType;
typedef void *  HALNativeWindowType;
typedef void *  HALNativePixmapType;

#endif

/* define DUMMY according to the system */
#if defined(EGL_API_WL)
#   define WL_DUMMY (31415926)
#   define EGL_DUMMY WL_DUMMY
#elif defined(__ANDROID__) || defined(ANDROID)
#   define ANDROID_DUMMY (31415926)
#   define EGL_DUMMY ANDROID_DUMMY
#else
#   define EGL_DUMMY (31415926)
#endif

/*******************************************************************************
** Display. ********************************************************************
*/

gceSTATUS
gcoOS_GetDisplay(
    OUT HALNativeDisplayType * Display,
    IN gctPOINTER Context
    );

gceSTATUS
gcoOS_GetDisplayByIndex(
    IN gctINT DisplayIndex,
    OUT HALNativeDisplayType * Display,
    IN gctPOINTER Context
    );

gceSTATUS
gcoOS_GetDisplayInfo(
    IN HALNativeDisplayType Display,
    OUT gctINT * Width,
    OUT gctINT * Height,
    OUT gctSIZE_T * Physical,
    OUT gctINT * Stride,
    OUT gctINT * BitsPerPixel
    );



gceSTATUS
gcoOS_GetDisplayInfoEx(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window,
    IN gctUINT DisplayInfoSize,
    OUT halDISPLAY_INFO * DisplayInfo
    );

gceSTATUS
gcoOS_GetNextDisplayInfoExByIndex(
    IN gctINT Index,
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window,
    IN gctUINT DisplayInfoSize,
    OUT halDISPLAY_INFO * DisplayInfo
    );

gceSTATUS
gcoOS_GetDisplayVirtual(
    IN HALNativeDisplayType Display,
    OUT gctINT * Width,
    OUT gctINT * Height
    );

gceSTATUS
gcoOS_GetDisplayBackbuffer(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window,
    OUT gctPOINTER  *  context,
    OUT gcoSURF     *  surface,
    OUT gctUINT * Offset,
    OUT gctINT * X,
    OUT gctINT * Y
    );

gceSTATUS
gcoOS_SetDisplayVirtual(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window,
    IN gctUINT Offset,
    IN gctINT X,
    IN gctINT Y
    );

gceSTATUS
gcoOS_SetDisplayVirtualEx(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window,
    IN gctPOINTER Context,
    IN gcoSURF Surface,
    IN gctUINT Offset,
    IN gctINT X,
    IN gctINT Y
    );

gceSTATUS
gcoOS_SetSwapInterval(
    IN HALNativeDisplayType Display,
    IN gctINT Interval
);

gceSTATUS
gcoOS_GetSwapInterval(
    IN HALNativeDisplayType Display,
    IN gctINT_PTR Min,
    IN gctINT_PTR Max
);

gceSTATUS
gcoOS_DisplayBufferRegions(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window,
    IN gctINT NumRects,
    IN gctINT_PTR Rects
    );

gceSTATUS
gcoOS_DestroyDisplay(
    IN HALNativeDisplayType Display
    );

gceSTATUS
gcoOS_InitLocalDisplayInfo(
    IN HALNativeDisplayType Display,
    IN OUT gctPOINTER * localDisplay
    );

gceSTATUS
gcoOS_DeinitLocalDisplayInfo(
    IN HALNativeDisplayType Display,
    IN OUT gctPOINTER * localDisplay
    );

gceSTATUS
gcoOS_GetDisplayInfoEx2(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window,
    IN gctPOINTER  localDisplay,
    IN gctUINT DisplayInfoSize,
    OUT halDISPLAY_INFO * DisplayInfo
    );

gceSTATUS
gcoOS_GetDisplayBackbufferEx(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window,
    IN gctPOINTER  localDisplay,
    OUT gctPOINTER  *  context,
    OUT gcoSURF     *  surface,
    OUT gctUINT * Offset,
    OUT gctINT * X,
    OUT gctINT * Y
    );

gceSTATUS
gcoOS_IsValidDisplay(
    IN HALNativeDisplayType Display
    );

gceSTATUS
gcoOS_GetNativeVisualId(
    IN HALNativeDisplayType Display,
    OUT gctINT* nativeVisualId
    );

gctBOOL
gcoOS_SynchronousFlip(
    IN HALNativeDisplayType Display
    );

/*******************************************************************************
** Windows. ********************************************************************
*/

gceSTATUS
gcoOS_CreateWindow(
    IN HALNativeDisplayType Display,
    IN gctINT X,
    IN gctINT Y,
    IN gctINT Width,
    IN gctINT Height,
    OUT HALNativeWindowType * Window
    );

gceSTATUS
gcoOS_GetWindowInfo(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window,
    OUT gctINT * X,
    OUT gctINT * Y,
    OUT gctINT * Width,
    OUT gctINT * Height,
    OUT gctINT * BitsPerPixel,
    OUT gctUINT * Offset
    );

gceSTATUS
gcoOS_DestroyWindow(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window
    );

gceSTATUS
gcoOS_DrawImage(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window,
    IN gctINT Left,
    IN gctINT Top,
    IN gctINT Right,
    IN gctINT Bottom,
    IN gctINT Width,
    IN gctINT Height,
    IN gctINT BitsPerPixel,
    IN gctPOINTER Bits
    );

gceSTATUS
gcoOS_GetImage(
    IN HALNativeWindowType Window,
    IN gctINT Left,
    IN gctINT Top,
    IN gctINT Right,
    IN gctINT Bottom,
    OUT gctINT * BitsPerPixel,
    OUT gctPOINTER * Bits
    );

gceSTATUS
gcoOS_GetWindowInfoEx(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window,
    OUT gctINT * X,
    OUT gctINT * Y,
    OUT gctINT * Width,
    OUT gctINT * Height,
    OUT gctINT * BitsPerPixel,
    OUT gctUINT * Offset,
    OUT gceSURF_FORMAT * Format
    );

gceSTATUS
gcoOS_DrawImageEx(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window,
    IN gctINT Left,
    IN gctINT Top,
    IN gctINT Right,
    IN gctINT Bottom,
    IN gctINT Width,
    IN gctINT Height,
    IN gctINT BitsPerPixel,
    IN gctPOINTER Bits,
    IN gceSURF_FORMAT  Format
    );

/*******************************************************************************
** Pixmaps. ********************************************************************
*/

gceSTATUS
gcoOS_CreatePixmap(
    IN HALNativeDisplayType Display,
    IN gctINT Width,
    IN gctINT Height,
    IN gctINT BitsPerPixel,
    OUT HALNativePixmapType * Pixmap
    );

gceSTATUS
gcoOS_GetPixmapInfo(
    IN HALNativeDisplayType Display,
    IN HALNativePixmapType Pixmap,
    OUT gctINT * Width,
    OUT gctINT * Height,
    OUT gctINT * BitsPerPixel,
    OUT gctINT * Stride,
    OUT gctPOINTER * Bits
    );

gceSTATUS
gcoOS_DrawPixmap(
    IN HALNativeDisplayType Display,
    IN HALNativePixmapType Pixmap,
    IN gctINT Left,
    IN gctINT Top,
    IN gctINT Right,
    IN gctINT Bottom,
    IN gctINT Width,
    IN gctINT Height,
    IN gctINT BitsPerPixel,
    IN gctPOINTER Bits
    );

gceSTATUS
gcoOS_DestroyPixmap(
    IN HALNativeDisplayType Display,
    IN HALNativePixmapType Pixmap
    );

gceSTATUS
gcoOS_GetPixmapInfoEx(
    IN HALNativeDisplayType Display,
    IN HALNativePixmapType Pixmap,
    OUT gctINT * Width,
    OUT gctINT * Height,
    OUT gctINT * BitsPerPixel,
    OUT gctINT * Stride,
    OUT gctPOINTER * Bits,
    OUT gceSURF_FORMAT * Format
    );

gceSTATUS
gcoOS_CopyPixmapBits(
    IN HALNativeDisplayType Display,
    IN HALNativePixmapType Pixmap,
    IN gctUINT DstWidth,
    IN gctUINT DstHeight,
    IN gctINT DstStride,
    IN gceSURF_FORMAT DstFormat,
    OUT gctPOINTER DstBits
    );

/*******************************************************************************
** OS relative. ****************************************************************
*/
gceSTATUS
gcoOS_LoadEGLLibrary(
    OUT gctHANDLE * Handle
    );

gceSTATUS
gcoOS_FreeEGLLibrary(
    IN gctHANDLE Handle
    );

gceSTATUS
gcoOS_ShowWindow(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window
    );

gceSTATUS
gcoOS_HideWindow(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window
    );

gceSTATUS
gcoOS_SetWindowTitle(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window,
    IN gctCONST_STRING Title
    );

gceSTATUS
gcoOS_CapturePointer(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window
    );

gceSTATUS
gcoOS_GetEvent(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window,
    OUT halEvent * Event
    );

gceSTATUS
gcoOS_CreateClientBuffer(
    IN gctINT Width,
    IN gctINT Height,
    IN gctINT Format,
    IN gctINT Type,
    OUT gctPOINTER * ClientBuffer
    );

gceSTATUS
gcoOS_GetClientBufferInfo(
    IN gctPOINTER ClientBuffer,
    OUT gctINT * Width,
    OUT gctINT * Height,
    OUT gctINT * Stride,
    OUT gctPOINTER * Bits
    );

gceSTATUS
gcoOS_DestroyClientBuffer(
    IN gctPOINTER ClientBuffer
    );

gceSTATUS
gcoOS_DestroyContext(
    IN gctPOINTER Display,
    IN gctPOINTER Context
    );

gceSTATUS
gcoOS_CreateContext(
    IN gctPOINTER LocalDisplay,
    IN gctPOINTER Context
    );

gceSTATUS
gcoOS_MakeCurrent(
    IN gctPOINTER LocalDisplay,
    IN HALNativeWindowType DrawDrawable,
    IN HALNativeWindowType ReadDrawable,
    IN gctPOINTER Context,
    IN gcoSURF ResolveTarget
    );

gceSTATUS
gcoOS_CreateDrawable(
    IN gctPOINTER LocalDisplay,
    IN HALNativeWindowType Drawable
    );

gceSTATUS
gcoOS_DestroyDrawable(
    IN gctPOINTER LocalDisplay,
    IN HALNativeWindowType Drawable
    );
gceSTATUS
gcoOS_SwapBuffers(
    IN gctPOINTER LocalDisplay,
    IN HALNativeWindowType Drawable,
    IN gcoSURF RenderTarget,
    IN gcoSURF ResolveTarget,
    IN gctPOINTER ResolveBits,
    OUT gctUINT *Width,
    OUT gctUINT *Height
    );
#ifdef __cplusplus
}
#endif

#endif /* __gc_hal_eglplatform_h_ */
