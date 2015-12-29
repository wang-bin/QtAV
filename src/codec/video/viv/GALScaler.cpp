// COPYRIGHT Deepin Inc.

#include "GALScaler.h"
#include "ImageConverter_p.h"
#include <galUtil.h>
#include "utils/Logger.h"

#define GC_ENSURE(x, ...) \
    do { \
        gceSTATUS ret = x; \
        if (ret < 0) { \
            qWarning("VIV error@%d. " #x "(%d): %s", __LINE__, ret, gcoOS_DebugStatus2Name(ret)); \
            return __VA_ARGS__; \
        } \
    } while(0);
#define GC_WARN(x) \
do { \
  gceSTATUS ret = x; \
  if(ret < 0) \
    qWarning("VIV error@%d. " #x "(%d): %s", __LINE__, ret, gcoOS_DebugStatus2Name(ret)); \
} while(0);

extern "C" {
void dma_copy_in_vmem(unsigned int dst, unsigned int src, int len);
void dma_copy_from_vmem(unsigned char* dst, unsigned int src, int len);
void dma_copy_to_vmem(unsigned int dst, unsigned char* src, int len);
}
namespace QtAV {
struct GALSurface {
    // read only properties
    gcoSURF surf;
    gceSURF_FORMAT format;
    gctUINT width, height;
    gctINT stride;
    gctUINT32 phyAddr[3];
    gctPOINTER lgcAddr[3];

    GALSurface() : surf(gcvNULL), format(gcvSURF_UNKNOWN), width(0), height(0), stride(0)
    {
        memset(phyAddr, 0, sizeof(phyAddr));
        memset(lgcAddr, 0, sizeof(lgcAddr));
    }
    ~GALSurface() { destroy();}
    bool reset(gcoSURF s) {
        if (surf == s)
            return true;
        if (surf) {
            destroy();
        }
        surf = s;
        if (surf) {
            GC_ENSURE(gcoSURF_GetAlignedSize(surf, gcvNULL, gcvNULL, &stride), false);
            GC_ENSURE(gcoSURF_GetSize(surf, &width, &height, gcvNULL), false);
            GC_ENSURE(gcoSURF_GetFormat(surf, gcvNULL, &format), false);
            qDebug("gcoSURF: %dx%d, stride: %d, fomrat:%d", width, height, stride, format);
        } else {
            format = gcvSURF_UNKNOWN;
            width = height = 0;
            stride = 0;
        }
        return true;
    }
    bool isNull() const {return surf == gcvNULL;}
    bool lock() {
        if (surf)
            GC_ENSURE(gcoSURF_Lock(surf, phyAddr, lgcAddr), false);
        return true;
    }
    bool unlock() {
        if (surf && lgcAddr[0] != gcvNULL) { //TODO: unlock for every plane?
            GC_ENSURE(gcoSURF_Unlock(surf, lgcAddr[0]), false);
            memset(lgcAddr, 0, sizeof(lgcAddr));
        }
        return true;
    }
private:
    void destroy() {
        unlock();
        if (surf != gcvNULL) {
            GC_ENSURE(gcoSURF_Destroy(surf));
            surf = gcvNULL;
        }
    }
};
class AutoLock {
    GALSurface *surf;
public:
    AutoLock(GALSurface *s) : surf(s) {
        if (surf)
            surf->lock();
    }
    ~AutoLock() {
        if (surf) {
            surf->unlock();
        }
    }
};

class GALScalerPrivate Q_DECL_FINAL: public ImageConverterPrivate
{
public:
    GALScalerPrivate() : ImageConverterPrivate()
      , gc_os(gcvNULL)
      , gc_hal(gcvNULL)
      , gc_2d(gcvNULL)
      , contiguous_size(0)
      , contiguous(gcvNULL)
      , contiguous_phys(gcvNULL)
      , host_in(true)
      , houst_out(false)
    {}
    ~GALScalerPrivate() {
        destroyGAL();
    }
    bool ensureGAL();
    bool destroyGAL();
    bool ensureSurface(GALSurface* surf, int w, int h, gceSURF_FORMAT fmt);
    gcoOS gc_os;
    gcoHAL gc_hal;
    gco2D gc_2d;
    gctSIZE_T contiguous_size;
    gctPOINTER contiguous;
    gctPHYS_ADDR contiguous_phys;

    GALSurface surf_in;
    GALSurface surf_out;

    bool host_in;
    bool host_out;
};

typedef struct {
    gceSURF_FORMAT gc;
    VideoFormat::PixelFormat pixfmt;
} gc_fmt_entry;
// TODO: more formats and confirm
static const gc_fmt_entry gc_fmts[] = {
    { gcvSURF_A8R8G8B8, VideoFormat::Format_RGB32},
    { gcvSURF_A8R8G8B8, VideoFormat::Format_ARGB32},
    { gcvSURF_A8R8G8B8, VideoFormat::Format_BGRA32},
    { gcvSURF_R5G6B5, VideoFormat::Format_RGB565 },
    { gcvSURF_I420, VideoFormat::Format_YUV420P }
};
static gceSURF_FORMAT pixelFormatToGAL(VideoFormat::PixelFormat pixfmt)
{
    for (const gc_fmt_entry* e = gc_fmts; e < gc_fmts + sizeof(gc_fmts)/sizeof(gc_fmts[0]); ++e)
        if (e->pixfmt == pixfmt)
            return e->gc;
    return gcvSURF_UNKNOWN;
}

GALScaler::GALScaler()
    : ImageConverter(*new GALScalerPrivate())
{}

bool GALScaler::check() const
{
    if (!ImageConverter::check())
        return false;
    DPTR_D(const GALScaler);
    return VideoFormat(VideoFormat::pixelFormatFromFFmpeg(d.fmt_out)).isRGB();
}

void GALScaler::setHostSource(bool value)
{
    DPTR_D(const GALScaler);
    d.host_in = value;
}

void GALScaler::setHostTarget(bool value)
{
    DPTR_D(const GALScaler);
    d.host_out = value;
}

bool GALScaler::convert(const quint8 * const src[], const int srcStride[])
{
    DPTR_D(GALScaler);
    const gceSURF_FORMAT srcFmt = pixelFormatToGAL(VideoFormat::pixelFormatFromFFmpeg(d.fmt_in));
    if (!d.ensureSurface(&d.surf_in, d.w_in, d.h_in, srcFmt)) {
        return false;
    }
    AutoLock lock(&d.surf_in);
    Q_UNUSED(lock);
    const VideoFormat fmt(d.fmt_in);
    // d.w_in*d.h_in, 1/4, 1/4
    // TODO: use gcoSURF api to get stride, height etc
    for (int i = 0; i < fmt.planeCount(); ++i) {
        // src[2] is 0x0!
        //qDebug("dma_copy_in_vmem %d: %p=>%p len:%d", i, src[i], d.surf_in.phyAddr[i], srcStride[i]*fmt.height(d.h_in, i));
        if (d.host_in)
            dma_copy_to_vmem(d.surf_in.phyAddr[i], src[i], srcStride[i]*fmt.height(d.h_in, i));
        else
            dma_copy_in_vmem(d.surf_in.phyAddr[i], (gctUINT32)(quintptr)src[i], srcStride[i]*fmt.height(d.h_in, i));
    }
    // TODO: setup d.gc_2d only if parameters changed
    gcsRECT dstRect = {0, 0, (gctINT32)d.surf_out.width, (gctINT32)d.surf_out.height};
    GC_ENSURE(gco2D_SetClipping(d.gc_2d, &dstRect), false);
    GC_ENSURE(gcoSURF_SetDither(d.surf_out.surf, gcvTRUE), false);
    gctUINT8 horKernel = 1, verKernel = 1;
    GC_ENSURE(gco2D_SetKernelSize(d.gc_2d, horKernel, verKernel), false); //TODO: check ok not < 0?
    GC_ENSURE(gco2D_EnableDither(d.gc_2d, gcvTRUE), false);
    gcsRECT srcRect = { 0, 0, d.w_in, d.h_in};
    GC_ENSURE(gcoSURF_FilterBlit(d.surf_in.surf, d.surf_out.surf, &srcRect, &dstRect, &dstRect), false)
    GC_ENSURE(gco2D_EnableDither(d.gc_2d, gcvFALSE), false)
    //GC_ENSURE(gco2D_Flush(d.gc_2d), false); //tiled flash the screen?
    GC_ENSURE(gcoHAL_Commit(d.gc_hal, gcvTRUE), false);
    return true;
}

bool GALScaler::convert(const quint8 *const src[], const int srcStride[], quint8 *const dst[], const int dstStride[])
{
    DPTR_D(GALScaler);
    AutoLock lock(&d.surf_out);
    Q_UNUSED(lock);
    // lock dstSurf, copy from dstSurf, unlock dstSurf
    if (!convert(src, srcStride))
        return false;
    static const bool no_copy = !!qgetenv("VPU_NO_COPY").toInt() || !qgetenv("VPU_COPY").toInt();
    static const bool partial_copy = qgetenv("VPU_PARTIAL_COPY").toInt();
    const VideoFormat fmt(d.fmt_out);
    for (int p = 0; p < fmt.planeCount(); ++p) {
        if (d.host_out || !no_copy || partial_copy) { //TODO: capture
            // dma copy. check img->stride
            if (d.pitchs.at(p) == dstStride[p]) {
                // qMin(scaler->outHeight(), img->height)
                dma_copy_from_vmem(dst[p], (unsigned int)(quintptr)d.bits.at(p), dstStride[p]*fmt.height(d.h_out, p));
            } else {
                qWarning("different stride @plane%d. vmem: %d, host: %d", p, d.pitchs.at(p), dstStride[p]);
                for (int i = fmt.height(d.h_out, p) - 1; i >= 0; --i)
                    dma_copy_from_vmem(dst[p] + i*dstStride[p], (unsigned int)(quintptr)d.bits.at(p) + i*d.pitchs.at(p), dstStride[p]);
            }
        }
        if (!d.host_out || no_copy || partial_copy) {
            qint32 *ptr = (qint32*)dst[p];
            *(ptr++) = 0x12345678;
            *(ptr++) = (qint32)(quintptr)d.bits.at(p);
            *(ptr++) = d.w_out;
            *(ptr++) = fmt.height(d.h_out, p);
            *(ptr++) = d.pitchs.at(p);
        }
    }
    return true;
}

bool GALScaler::prepareData() // only for output
{
    DPTR_D(GALScaler);
    if (!check())
        return false;
    const int nb_planes = qMax(av_pix_fmt_count_planes(d.fmt_out), 0);
    qDebug("prepare GAL resource.%dx%d=>%dx%d nb_planes: %d. gcfmt: %d", d.w_in, d.h_in, d.w_out, d.h_out, nb_planes, pixelFormatToGAL(VideoFormat::pixelFormatFromFFmpeg(d.fmt_out)));
    d.bits.resize(nb_planes);
    d.pitchs.resize(nb_planes);
    //d.surf_out.destroy(); // destroy out surface is enough?
    const VideoFormat fmt(d.fmt_out);
    if (!d.ensureSurface(&d.surf_out, d.w_out, d.h_out, pixelFormatToGAL(fmt.pixelFormat())))
        return false;
    if (d.surf_out.lock())
        d.surf_out.unlock();
    // if unlock here, we must lock again to when copying from gal
    qDebug("lock surf_out.surf: %p, phy:%p, lgc:%p", d.surf_out.surf, d.surf_out.phyAddr[0], d.surf_out.lgcAddr[0]);
    for (int i = 0; i < nb_planes; ++i) {
        d.bits[i] = (quint8*)(quintptr)d.surf_out.phyAddr[i];
        d.pitchs[i] = fmt.width(d.surf_out.stride, i);
    }
    qDebug() << "surf_out bits/pitch:" << d.bits << d.pitchs;
    return true;
}

bool GALScalerPrivate::ensureGAL()
{
    if (gc_hal && contiguous)
        return true;
    /* Construct the gcoOS object. */
    GC_ENSURE(gcoOS_Construct(gcvNULL, &gc_os), false);
    qDebug("gcoOS_Construct, gc_os:%p", gc_os);
    /* Construct the gcoHAL object. */
    GC_ENSURE(gcoHAL_Construct(gcvNULL, gc_os, &gc_hal), false);
    gceCHIPMODEL model;
    gctUINT32 revision, features, minor_features;
    GC_ENSURE(gcoHAL_QueryChipIdentity(gc_hal, &model, &revision, &features, &minor_features), false);
    qDebug("GAL model: %#x, revision: %u, features: %u:%u", model, revision, features, minor_features);
    // gcvFEATURE_YUV420_TILER for tiled map?
    if (!gcoHAL_IsFeatureAvailable(gc_hal, gcvFEATURE_YUV420_SCALER)) {
        qWarning("GAL: I420 scaler is not supported.");
        return false;
    }
    GC_ENSURE(gcoHAL_QueryVideoMemory(gc_hal, NULL, NULL, NULL, NULL, &contiguous_phys, &contiguous_size), false);
    if (contiguous_size <= 0) {
        qWarning("invalid contiguous_size: %lu", contiguous_size);
        return false;
    }
    /* Map the contiguous memory. */
    GC_ENSURE(gcoHAL_MapMemory(gc_hal, contiguous_phys, contiguous_size, &contiguous), false);
    GC_ENSURE(gcoHAL_Get2DEngine(gc_hal, &gc_2d), false);
    return true;
}

bool GALScalerPrivate::destroyGAL()
{
    // destroy source surface and hal/os/contiguous?
    surf_in.reset(gcvNULL);
    surf_out.reset(gcvNULL);
    if (contiguous != gcvNULL) {
        qDebug("unmap contiguous:%p, phy: %p, size: %lu", contiguous, contiguous_phys, contiguous_size);
        /* Unmap the contiguous memory. */
        GC_WARN(gcoHAL_UnmapMemory(gc_hal, contiguous_phys, contiguous_size, contiguous));
    }
    if (gc_hal != gcvNULL) {
        GC_WARN(gcoHAL_Commit(gc_hal, gcvTRUE));
    }
    if (gc_hal != gcvNULL) {
        GC_WARN(gcoHAL_Destroy(gc_hal));
        gc_hal = gcvNULL;
    }
    if (gc_os != gcvNULL) {
        GC_WARN(gcoOS_Destroy(gc_os));
        gc_os = gcvNULL;
    }
    return true;
}

bool GALScalerPrivate::ensureSurface(GALSurface *surf, int w, int h, gceSURF_FORMAT fmt)
{
    if (!ensureGAL())
        return false;
    if (!surf)
        return false;
    if (!surf->isNull() && surf->format == fmt && surf->width == (gctUINT)FFALIGN(w, 16) && surf->height == (gctUINT)FFALIGN(h, 16))
        return true;
    gcoSURF s = gcvNULL;
    GC_ENSURE(gcoSURF_Construct(gc_hal, w, h, 1, gcvSURF_BITMAP, fmt, gcvPOOL_DEFAULT, &s), false);
    return surf->reset(s);
}
} //namespace QtAV
