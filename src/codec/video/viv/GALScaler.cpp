// COPYRIGHT Deepin Inc.

#include "GALScaler.h"
#include "ImageConverter_p.h"
#include <galUtil.h>

#define GC_ENSURE(x, ...) \
    do { \
        gceSTATUS ret = x; \
        if (ret < 0) { \
            qWarning("VIV error@%d. " #x "(%d): %s", __LINE__, ret, gckOS_DebugStatus2Name(ret)); \
            return __VA_ARGS__; \
        } \
    } while(0);
#define GC_WARN(x) \
do { \
  gceSTATUS ret = x; \
  if(ret < 0) \
    qWarning("VIV error@%d. " #x "(%d): %s", __LINE__, ret, gckOS_DebugStatus2Name(ret)); \
} while(0);

extern "C" {
void dma_copy_in_vmem(unsigned int dst, unsigned int src, int len);
void dma_copy_from_vmem(unsigned char* dst, unsigned int src, int len);
}
namespace QtAV {

typedef struct Test2D {
//      GalTest     base;
    GalRuntime  runtime;

    // destination surface
    gcoSURF                 dstSurf;
    gceSURF_FORMAT          dstFormat;
    gctUINT                 dstWidth;
    gctUINT                 dstHeight;
    gctINT                  dstStride;
    gctUINT32               dstPhyAddr;
    gctPOINTER              dstLgcAddr;

    //source surface
    gcoSURF                 srcSurf;
    gceSURF_FORMAT          srcFormat;
    gctUINT                 srcWidth;
    gctUINT                 srcHeight;
    gctINT                  srcStride;
    gctUINT32               srcPhyAddr;//TODO: not set
    gctPOINTER              srcLgcAddr;//TODO: not set
} Test2D;

class GALScalerPrivate Q_DECL_FINAL: public ImageConverterPrivate
{
public:
    GALScalerPrivate() : ImageConverterPrivate()
      , gc_os(gcvNULL)
      , gc_hal(gcvNULL)
      , gc_2d(gcvNULL)
    {}
    ~GALScalerPrivate() {
        close();
    }
    bool open(int w, int h, gceSURF_FORMAT fmt); // must reset test2d if false
    bool close();
    bool createSourceSurface(int w, int h, gceSURF_FORMAT fmt);
    gcoOS gc_os;
    gcoHAL gc_hal;
    gco2D gc_2d;
    gctSIZE_T contiguous_size;
    gctPOINTER contiguous;
    gctPHYS_ADDR contiguous_physical;

    Test2D test2D;
};

typedef struct {
    gceSURF_FORMAT gc;
    VideoFormat::PixelFormat pixfmt;
} gc_fmt_entry;
// TODO: more formats and confirm
static const gc_fmt_entry gc_fmts[] = {
    { gcvSURF_A8R8G8B8, VideoFormat::Format_ARGB32},
    { gcvSURF_R5G6B5, VideoFormat::Format_RGB565 }
};
gceSURF_FORMAT pixelFormatToGC(VideoFormat::PixelFormat pixfmt)
{
    for (const gc_fmt_entry* e = gc_fmts; e < gc_fmts + sizeof(gc_fmts)/sizeof(gc_fmts[0]); ++e)
        if (e->pixfmt == pixfmt)
            return e->gc;
    return gcvSURF_UNKNOWN;
}

VideoFormat::PixelFormat pixelFormatFromGC(gceSURF_FORMAT gc)
{
    for (const gc_fmt_entry* e = gc_fmts; e < gc_fmts + sizeof(gc_fmts)/sizeof(gc_fmts[0]); ++e)
        if (e->gc == gc)
            return e->pixfmt;
    return VideoFormat::Format_Invalid;
}

bool GALScaler::check() const
{
    if (!ImageConverter::check())
        return false;
    DPTR_D(const GALScaler);
    return VideoFormat(VideoFormat::pixelFormatFromFFmpeg(d.fmt_out)).isRGB();
}

bool GALScaler::convert(const quint8 * const src[], const int srcStride[])
{
    DPTR_D(GALScaler);
    const gceSURF_FORMAT srcFmt = pixelFormatToGC(VideoFormat::pixelFormatFromFFmpeg(d.fmt_in));
    if (!d.test2D.srcSurf
            || d.test2D.srcWidth != d.w_in || d.test2D.srcHeight != d.h_in
            || d.test2D.srcFormat != srcFmt) {
        if (d.test2D.srcSurf != gcvNULL) {
            if (d.test2D.srcLgcAddr)
                GC_WARN(gcoSURF_Unlock(d.test2D.srcSurf, d.test2D.srcLgcAddr));
            GC_WARN(gcoSURF_Destroy(d.test2D.srcSurf));
            d.test2D.srcLgcAddr = 0;
            d.test2D.srcSurf = gcvNULL;
        }
        if (!d.createSourceSurface(d.w_in, d.h_in, srcFmt)) {
            return false;
        }
    }
    gctUINT32 address[3];  //解码后的yuv文件的物理地址
    gctPOINTER memory[3]; 	//解码后的yuv文件的逻辑地址
    GC_WARN(gcoSURF_Lock(d.test2D.srcSurf, address, memory));

    const VideoFormat fmt(d.fmt_in);
    // d.w_in*d.h_in, 1/4, 1/4
    for (int i = 0; i < fmt.planeCount(); ++i) {
        dma_copy_in_vmem(address[i], (gctUINT32)(quintptr)src[i], srcStride[i]*fmt.height(d.h_in, i));
    }
    // TODO: setup gco2D only if parameters changed
    gco2D egn2D = d.test2D.runtime.engine2d;
    // set clippint rect
    gcsRECT dstRect = {0, 0, d.test2D.dstWidth, d.test2D.dstHeight};
    GC_ENSURE(gco2D_SetClipping(egn2D, &dstRect), false);
    GC_ENSURE(gcoSURF_SetDither(d.test2D.dstSurf, gcvTRUE), false);
    // set kernel size
    gctUINT8 horKernel = 1, verKernel = 1;
    GC_ENSURE(gco2D_SetKernelSize(egn2D, horKernel, verKernel), false); //TODO: check ok not < 0?
    GC_ENSURE(gco2D_EnableDither(egn2D, gcvTRUE), false);
    gcsRECT srcRect;
    srcRect.left = 0;
    srcRect.top = 0;
    srcRect.right = d.w_in;
    srcRect.bottom = d.h_in;
    GC_ENSURE(gcoSURF_FilterBlit(d.test2D.srcSurf, d.test2D.dstSurf, &srcRect, &dstRect, &dstRect), false)
    GC_ENSURE(gco2D_EnableDither(egn2D, gcvFALSE), false)
    GC_ENSURE(gco2D_Flush(egn2D), false);
    GC_ENSURE(gcoHAL_Commit(d.test2D.runtime.hal, gcvTRUE), false);

    return true;
}

bool GALScaler::convert(const quint8 *const src[], const int srcStride[],     quint8 *const dst[], const int dstStride[])
{
    return false;
}

bool GALScaler::prepareData()
{
    DPTR_D(GALScaler);
    if (!check())
        return false;
    const int nb_planes = qMax(av_pix_fmt_count_planes(d.fmt_out), 0);
    d.bits.resize(nb_planes);
    d.pitchs.resize(nb_planes);

    d.close();
    if (!d.open(d.w_out, d.h_out, pixelFormatToGC(VideoFormat::pixelFormatFromFFmpeg(d.fmt_out))))
        return false;
    d.bits[0] = (quint8*)d.test2D.dstPhyAddr;
    d.pitchs[0] = d.test2D.dstStride;
    return true;
}

bool GALScalerPrivate::open(int w, int h, gceSURF_FORMAT fmt)
{
    /* Construct the gcoOS object. */
    GC_ENSURE(gcoOS_Construct(gcvNULL, &gc_os), false);
    /* Construct the gcoHAL object. */
    GC_ENSURE(gcoHAL_Construct(gcvNULL, gc_os, &gc_hal), false);
    GC_ENSURE(gcoHAL_QueryVideoMemory(gc_hal,
                                      NULL, NULL,
                                      NULL, NULL,
                                      &contiguous_physical, &contiguous_size)
              , false);
    /* Map the contiguous memory. */
    if (contiguous_size > 0) {
        GC_ENSURE(gcoHAL_MapMemory(gc_hal, contiguous_physical, contiguous_size, &contiguous), false);
    }
    GC_ENSURE(gcoHAL_Get2DEngine(gc_hal, &gc_2d), false);
    GC_ENSURE(gcoSURF_Construct(gc_hal, w, h,
                    1, //TODO: depth. why 1?
                    gcvSURF_BITMAP, fmt, gcvPOOL_DEFAULT, &test2D.dstSurf)
              , false);
    GalRuntime runtime;
    memset(&runtime, 0, sizeof(runtime));
    runtime.os              = gc_os;
    runtime.hal             = gc_hal;
    runtime.engine2d        = gc_2d;
    runtime.target          = test2D.dstSurf;
    runtime.width           = w;
    runtime.height          = h;
    runtime.format          = fmt;
    runtime.pe20            = gcoHAL_IsFeatureAvailable(gc_hal, gcvFEATURE_2DPE20);
    runtime.fullDFB         = gcoHAL_IsFeatureAvailable(gc_hal, gcvFEATURE_FULL_DIRECTFB);

    memset(&test2D, 0, sizeof(test2D));
    test2D.runtime = runtime;
    test2D.dstSurf = runtime.target;
    test2D.dstFormat = runtime.format;
    test2D.srcFormat = gcvSURF_UNKNOWN;
    GC_ENSURE(gcoSURF_GetAlignedSize(test2D.dstSurf, &test2D.dstWidth, &test2D.dstHeight, &test2D.dstStride), false);
    // TODO: when to unlock? is it safe to lock twice?
    GC_ENSURE(gcoSURF_Lock(test2D.dstSurf, &test2D.dstPhyAddr, &test2D.dstLgcAddr), false);
    // TODO: gcvFEATURE_YUV420_TILER for tiled map?
    if (!gcoHAL_IsFeatureAvailable(test2D.runtime.hal, gcvFEATURE_YUV420_SCALER)) {
        qWarning("VIV: YUV420 scaler is not supported.");
        return false;
    }
    return true;
}

bool GALScalerPrivate::close()
{
    // destroy source surface
    if (test2D.srcSurf != gcvNULL) {
        if (test2D.srcLgcAddr)
            GC_WARN(gcoSURF_Unlock(test2D.srcSurf, test2D.srcLgcAddr));
        GC_WARN(gcoSURF_Destroy(test2D.srcSurf));
        test2D.srcLgcAddr = 0;
        test2D.srcSurf = gcvNULL;
    }
    if ((test2D.dstSurf != gcvNULL) && (test2D.dstLgcAddr != gcvNULL)) {
        GC_WARN(gcoSURF_Unlock(test2D.dstSurf, test2D.dstLgcAddr));
        test2D.dstLgcAddr = gcvNULL;
    }
    // destroy after gcoHAL_Commit?
    if (test2D.dstSurf != gcvNULL) {
        GC_WARN(gcoSURF_Destroy(test2D.dstSurf));
        test2D.dstSurf = gcvNULL;
    }
    if (gc_hal != gcvNULL)
        GC_WARN(gcoHAL_Commit(gc_hal, gcvTRUE));
    if (contiguous != gcvNULL) {
        /* Unmap the contiguous memory. */
        GC_WARN(gcoHAL_UnmapMemory(gc_hal, contiguous_physical, contiguous_size, contiguous));
    }
    if (gc_hal != gcvNULL) {
        GC_WARN(gcoHAL_Commit(gc_hal, gcvTRUE));
        GC_WARN(gcoHAL_Destroy(gc_hal));
        gc_hal = NULL;
    }
    if (gc_os != gcvNULL) {
        GC_WARN(gcoOS_Destroy(gc_os));
        gc_os = NULL;
    }
    return true;
}

bool GALScalerPrivate::createSourceSurface(int w, int h, gceSURF_FORMAT fmt)
{
    gcoSURF srcsurf = NULL;
    gceSTATUS status = gcoSURF_Construct(test2D.runtime.hal, w, h, 1, gcvSURF_BITMAP, fmt, gcvPOOL_DEFAULT, &srcsurf);
    if (status != gcvSTATUS_OK) {
        qWarning("surface construct error!");
        gcoSURF_Destroy(srcsurf);
        srcsurf = gcvNULL;
        return false;
    }
    gctUINT alignedWidth, alignedHeight;
    gctINT alignedStride;
    // TODO: what's the alignment?
    gcmVERIFY_OK(gcoSURF_GetAlignedSize(srcsurf, &alignedWidth, &alignedHeight, &alignedStride));
    if (w != alignedWidth || h != alignedHeight) { // TODO: what if ignore?
        printf("gcoSURF width and height is not aligned !\n");
        gcoSURF_Destroy(srcsurf);
        srcsurf = gcvNULL;
        return false;
    }
    test2D.srcSurf = srcsurf;
    GC_ENSURE(gcoSURF_GetAlignedSize(test2D.srcSurf, gcvNULL, gcvNULL, &test2D.srcStride), false);
    //? aligned ==  no-aligned already?
    GC_ENSURE(gcoSURF_GetSize(test2D.srcSurf, &test2D.srcWidth, &test2D.srcHeight, gcvNULL), false);
    GC_ENSURE(gcoSURF_GetFormat(test2D.srcSurf, gcvNULL, &test2D.srcFormat), false);
    return true;
}

} //namespace QtAV
