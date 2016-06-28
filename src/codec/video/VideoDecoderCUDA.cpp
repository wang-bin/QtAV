/******************************************************************************
    QtAV:  Multimedia framework based on Qt and FFmpeg
    Copyright (C) 2012-2016 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV (from 2014)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
******************************************************************************/

#include "QtAV/VideoDecoder.h"
#include "QtAV/Packet.h"
#include "QtAV/private/AVDecoder_p.h"
#include "QtAV/private/factory.h"
#include <QtCore/QQueue>
#if QTAV_HAVE(DLLAPI_CUDA)
#include "dllapi.h"
#endif //QTAV_HAVE(DLLAPI_CUDA)
#include "QtAV/private/AVCompat.h"
#include "utils/BlockingQueue.h"
/*
 * TODO: VC1, HEVC bsf
 */
#define COPY_ON_DECODE 1
/*
 * avc1, ccv1 => h264 + sps, pps, nal. use filter or lavcudiv
 * http://blog.csdn.net/gavinr/article/details/7183499
 * https://www.ffmpeg.org/ffmpeg-bitstream-filters.html
 * http://hi.baidu.com/freenaut/item/49ca125112587314aaf6d733
 * CUDA_ERROR_INVALID_VALUE "cuvidDecodePicture(p->dec, cuvidpic)"
 */

#include "cuda/helper_cuda.h"
#include "cuda/cuda_api.h"
#include "utils/Logger.h"
#include "SurfaceInteropCUDA.h"

//decode error if not floating context

namespace QtAV {

static const unsigned int kMaxDecodeSurfaces = 20;
class VideoDecoderCUDAPrivate;
class VideoDecoderCUDA : public VideoDecoder
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(VideoDecoderCUDA)
    Q_PROPERTY(CopyMode copyMode READ copyMode WRITE setCopyMode NOTIFY copyModeChanged)
    Q_PROPERTY(int surfaces READ surfaces WRITE setSurfaces)
    Q_PROPERTY(Flags flags READ flags WRITE setFlags)
    Q_PROPERTY(Deinterlace deinterlace READ deinterlace WRITE setDeinterlace)
    Q_FLAGS(Flags)
    Q_ENUMS(Flags)
    Q_ENUMS(Deinterlace)
    Q_ENUMS(CopyMode)
public:
    enum Flags {
        Default = cudaVideoCreate_Default,   // Default operation mode: use dedicated video engines
        CUDA = cudaVideoCreate_PreferCUDA,   // Use a CUDA-based decoder if faster than dedicated engines (requires a valid vidLock object for multi-threading)
        DXVA = cudaVideoCreate_PreferDXVA,   // Go through DXVA internally if possible (requires D3D9 interop)
        CUVID = cudaVideoCreate_PreferCUVID  // Use dedicated video engines directly
    };
    enum Deinterlace {
        Bob = cudaVideoDeinterlaceMode_Bob,           // Drop one field
        Weave = cudaVideoDeinterlaceMode_Weave,       // Weave both fields (no deinterlacing)
        Adaptive = cudaVideoDeinterlaceMode_Adaptive  // Adaptive deinterlacing
    };
    enum CopyMode {
        ZeroCopy, // default for linux
        DirectCopy, // use the same host address without additional copy to frame. If address does not change, it should be safe
        GenericCopy
    };

    VideoDecoderCUDA();
    ~VideoDecoderCUDA();
    VideoDecoderId id() const Q_DECL_OVERRIDE;
    QString description() const Q_DECL_OVERRIDE;
    void flush() Q_DECL_OVERRIDE;
    bool decode(const Packet &packet) Q_DECL_OVERRIDE;
    virtual VideoFrame frame() Q_DECL_OVERRIDE;

    // properties
    int surfaces() const;
    void setSurfaces(int n);
    Flags flags() const;
    void setFlags(Flags f);
    Deinterlace deinterlace() const;
    void setDeinterlace(Deinterlace di);
    CopyMode copyMode() const;
    void setCopyMode(CopyMode value);
Q_SIGNALS:
    void copyModeChanged(CopyMode value);
};
extern VideoDecoderId VideoDecoderId_CUDA;
FACTORY_REGISTER(VideoDecoder, CUDA, "CUDA")

static struct {
    AVCodecID ffCodec;
    cudaVideoCodec cudaCodec;
} const ff_cuda_codecs[] = {
    { QTAV_CODEC_ID(MPEG1VIDEO), cudaVideoCodec_MPEG1 },
    { QTAV_CODEC_ID(MPEG2VIDEO), cudaVideoCodec_MPEG2 },
    { QTAV_CODEC_ID(MPEG4),      cudaVideoCodec_MPEG4 },
    { QTAV_CODEC_ID(VC1),        cudaVideoCodec_VC1   },
    { QTAV_CODEC_ID(H264),       cudaVideoCodec_H264  },
    { QTAV_CODEC_ID(H264),       cudaVideoCodec_H264_SVC},
    { QTAV_CODEC_ID(H264),       cudaVideoCodec_H264_MVC},
    { QTAV_CODEC_ID(HEVC),       cudaVideoCodec_HEVC },
    { QTAV_CODEC_ID(VP8),        cudaVideoCodec_VP8 },
    { QTAV_CODEC_ID(VP9),        cudaVideoCodec_VP9 },
    { QTAV_CODEC_ID(NONE),       cudaVideoCodec_NumCodecs}
};

static cudaVideoCodec mapCodecFromFFmpeg(AVCodecID codec)
{
    for (int i = 0; ff_cuda_codecs[i].ffCodec != QTAV_CODEC_ID(NONE); ++i) {
        if (ff_cuda_codecs[i].ffCodec == codec) {
            return ff_cuda_codecs[i].cudaCodec;
        }
    }
    return cudaVideoCodec_NumCodecs;
}
static AVCodecID mapCodecToFFmpeg(cudaVideoCodec cudaCodec)
{
    for (int i = 0; ff_cuda_codecs[i].ffCodec != QTAV_CODEC_ID(NONE); ++i) {
        if (ff_cuda_codecs[i].cudaCodec == cudaCodec) {
            return ff_cuda_codecs[i].ffCodec;
        }
    }
    return QTAV_CODEC_ID(NONE);
}

class VideoDecoderCUDAPrivate Q_DECL_FINAL: public VideoDecoderPrivate
                , protected cuda_api
{
public:
    VideoDecoderCUDAPrivate():
        VideoDecoderPrivate()
      , can_load(true)
      , host_data(0)
      , host_data_size(0)
      , create_flags(cudaVideoCreate_Default)
      , deinterlace(cudaVideoDeinterlaceMode_Adaptive)
      , yuv_range(ColorRange_Limited)
      , nb_dec_surface(kMaxDecodeSurfaces)
      , copy_mode(VideoDecoderCUDA::DirectCopy)
    {
#ifndef Q_OS_WIN
        // CUDA+GL/D3D interop requires NV GPU is used for rendering. Windows can use CUDA with intel GPU, I don't know how to detect this, so 0-copy interop can fail in this case. But linux always requires nvidia GPU to use CUDA, so interop works.
        copy_mode = VideoDecoderCUDA::ZeroCopy;
#endif
#if QTAV_HAVE(DLLAPI_CUDA)
        can_load = dllapi::testLoad("nvcuvid");
#endif //QTAV_HAVE(DLLAPI_CUDA)
        available = false;
        bsf = 0;
        cuctx = 0;
        cudev = 0;
        dec = 0;
        vid_ctx_lock = 0;
        parser = 0;
        stream = 0;
        force_sequence_update = false;
        frame_queue.setCapacity(20);
        frame_queue.setThreshold(10);
        surface_in_use.resize(nb_dec_surface);
        surface_in_use.fill(false);
        if (!can_load)
            return;
        if (!isLoaded()) //cuda_api
            return;
        interop_res = cuda::InteropResourcePtr();
    }
    ~VideoDecoderCUDAPrivate() {
        if (bsf)
            av_bitstream_filter_close(bsf);
        if (!can_load)
            return;
        if (!isLoaded()) //cuda_api
            return;
        // if not reset here, CUDA_ERROR_CONTEXT_IS_DESTROYED in ~cuda::InteropResource()
        // QSharedPointer.reset() is in qt5
        interop_res = cuda::InteropResourcePtr(); // in interop object it's weak ptr. It's safe to reset here before cuda resouce is released.
        releaseCuda();
    }
    bool open() Q_DECL_OVERRIDE;
    bool initCuda();
    bool releaseCuda();
    bool createCUVIDDecoder(cudaVideoCodec cudaCodec, int cw, int ch);
    void createInterop() {
        if (copy_mode == VideoDecoderCUDA::ZeroCopy) {
#if QTAV_HAVE(CUDA_GL)
            if (!OpenGLHelper::isOpenGLES())
                interop_res = cuda::InteropResourcePtr(new cuda::GLInteropResource());
#endif //QTAV_HAVE(CUDA_GL)
#if QTAV_HAVE(CUDA_EGL)
            if (OpenGLHelper::isOpenGLES())
                interop_res = cuda::InteropResourcePtr(new cuda::EGLInteropResource());
#endif //QTAV_HAVE(CUDA_EGL)
        }
#ifndef QT_NO_OPENGL
        else if (copy_mode == VideoDecoderCUDA::DirectCopy) {
            interop_res = cuda::InteropResourcePtr(new cuda::HostInteropResource());
        }
#endif //QT_NO_OPENGL
        if (!interop_res)
            return;
        interop_res->setDevice(cudev);
        interop_res->setShareContext(cuctx); //it not share the context, interop res will create it's own context, context switch is slow
        interop_res->setDecoder(dec);
        interop_res->setLock(vid_ctx_lock);
    }

    bool createCUVIDParser();
    bool flushParser();
    bool processDecodedData(CUVIDPARSERDISPINFO *cuviddisp, VideoFrame* outFrame = 0);
    bool doParseVideoData(CUVIDSOURCEDATAPACKET* pPkt) {
        //CUDA_ENSURE(cuCtxPushCurrent(cuctx), false);
        AutoCtxLock lock(this, vid_ctx_lock);
        Q_UNUSED(lock);
        CUDA_ENSURE(cuvidParseVideoData(parser, pPkt), false);
        return true;
    }

    bool doDecodePicture(CUVIDPICPARAMS *cuvidpic) {
        AutoCtxLock lock(this, vid_ctx_lock);
        Q_UNUSED(lock);
        CUDA_ENSURE(cuvidDecodePicture(dec, cuvidpic), false);
        return true;
    }

    VideoFrame getNextFrame() {
#if COPY_ON_DECODE
        return frame_queue.take();
#else
        VideoFrame vf;
        CUVIDPARSERDISPINFO *cuviddisp = frame_queue.take();
        processDecodedData(cuviddisp, &vf);

        return vf;
#endif //COPY_ON_DECODE
    }
    static int CUDAAPI HandleVideoSequence(void *obj, CUVIDEOFORMAT *cuvidfmt) {
        VideoDecoderCUDAPrivate *p = reinterpret_cast<VideoDecoderCUDAPrivate*>(obj);
        CUVIDDECODECREATEINFO *dci = &p->dec_create_info;
        if ((cuvidfmt->codec != dci->CodecType)
            || (cuvidfmt->coded_width != dci->ulWidth)
            || (cuvidfmt->coded_height != dci->ulHeight)
            || (cuvidfmt->chroma_format != dci->ChromaFormat)
            || p->force_sequence_update) {
            qDebug("recreate cuvid parser");
            p->force_sequence_update = false;
            p->yuv_range = cuvidfmt->video_signal_description.video_full_range_flag ? ColorRange_Full : ColorRange_Limited;
            //coded_width or width?
            p->createCUVIDDecoder(cuvidfmt->codec, cuvidfmt->coded_width, cuvidfmt->coded_height);
            // how about parser.ulMaxNumDecodeSurfaces? recreate?
            AVCodecID codec = mapCodecToFFmpeg(cuvidfmt->codec);
            p->setBSF(codec);
            p->createInterop();
        }
        //TODO: lavfilter
        return 1;
    }
    static int CUDAAPI HandlePictureDecode(void *obj, CUVIDPICPARAMS *cuvidpic) {
        VideoDecoderCUDAPrivate *p = reinterpret_cast<VideoDecoderCUDAPrivate*>(obj);
        //qDebug("%s @%d tid=%p dec=%p idx=%d inUse=%d", __FUNCTION__, __LINE__, QThread::currentThread(), p->dec, cuvidpic->CurrPicIdx, p->surface_in_use[cuvidpic->CurrPicIdx]);
        p->doDecodePicture(cuvidpic);
        return 1;
    }
    static int CUDAAPI HandlePictureDisplay(void *obj, CUVIDPARSERDISPINFO *cuviddisp) {
        VideoDecoderCUDAPrivate *p = reinterpret_cast<VideoDecoderCUDAPrivate*>(obj);
        p->surface_in_use[cuviddisp->picture_index] = true;
        //qDebug("mark in use pic_index: %d", cuviddisp->picture_index);
#if COPY_ON_DECODE
        return p->processDecodedData(cuviddisp, 0);
#else
        p->frame_queue.put(cuviddisp);
        return 1;
#endif
    }
    void setBSF(AVCodecID codec);
    bool can_load; //if linked to cuvid, it's true. otherwise(use dllapi) equals to whether cuvid can be loaded
    uchar *host_data;
    int host_data_size;
    CUcontext cuctx;
    CUdevice cudev;

    cudaVideoCreateFlags create_flags;
    cudaVideoDeinterlaceMode deinterlace;
    CUvideodecoder dec;
    CUVIDDECODECREATEINFO dec_create_info;
    CUvideoctxlock vid_ctx_lock; //NULL
    CUVIDPICPARAMS pic_params;
    CUVIDEOFORMATEX extra_parser_info;
    CUvideoparser parser;
    CUstream stream;
    bool force_sequence_update;
    ColorRange yuv_range;
    /*
     * callbacks are in the same thread as cuvidParseVideoData. so video thread may be blocked
     * so create another thread?
     */
#if COPY_ON_DECODE
    BlockingQueue<VideoFrame> frame_queue;
#else
    BlockingQueue<CUVIDPARSERDISPINFO*> frame_queue;
#endif
    QVector<bool> surface_in_use;
    int nb_dec_surface;
    QString description;

    AVBitStreamFilterContext *bsf; //TODO: rename bsf_ctx

    VideoDecoderCUDA::CopyMode copy_mode;
    cuda::InteropResourcePtr interop_res; //may be still used in video frames when decoder is destroyed
};

VideoDecoderCUDA::VideoDecoderCUDA():
    VideoDecoder(*new VideoDecoderCUDAPrivate())
{
    // dynamic properties about static property details. used by UI
    // format: detail_property
    setProperty("detail_surfaces", tr("Decoding surfaces"));
    setProperty("detail_flags", tr("Decoder flags"));
    setProperty("detail_copyMode", QString("%1\n%2\n%3\n%4")
                .arg(tr("Performace: ZeroCopy > DirectCopy > GenericCopy"))
                .arg(tr("ZeroCopy: no copy back from GPU to System memory. Directly render the decoded data on GPU"))
                .arg(tr("DirectCopy: copy back to host memory but video frames and map to GL texture"))
                .arg(tr("GenericCopy: copy back to host memory and each video frame"))
                );
    Q_UNUSED(QObject::tr("ZeroCopy"));
    Q_UNUSED(QObject::tr("DirectCopy"));
    Q_UNUSED(QObject::tr("GenericCopy"));
    Q_UNUSED(QObject::tr("copyMode"));
}

VideoDecoderCUDA::~VideoDecoderCUDA()
{
}

VideoDecoderId VideoDecoderCUDA::id() const
{
    return VideoDecoderId_CUDA;
}

QString VideoDecoderCUDA::description() const
{
    DPTR_D(const VideoDecoderCUDA);
    if (!d.description.isEmpty())
        return d.description;
    return QStringLiteral("NVIDIA CUVID");
}

void VideoDecoderCUDA::flush()
{
    DPTR_D(VideoDecoderCUDA);
    d.frame_queue.clear();
    d.surface_in_use.fill(false);
}

bool VideoDecoderCUDA::decode(const Packet &packet)
{
    if (!isAvailable())
        return false;
    DPTR_D(VideoDecoderCUDA);
    if (!d.parser) {
        qWarning("CUVID parser not ready");
        return false;
    }
    if (packet.isEOF()) {
        if (!d.flushParser()) {
            qDebug("Error decode EOS"); // when?
            return false;
        }
        return !d.frame_queue.isEmpty();
    }
    uint8_t *outBuf = 0;
    int outBufSize = 0;
    int filtered = 0;
    if (d.bsf) {
        // h264_mp4toannexb_filter does not use last parameter 'keyFrame', so just set 0
        //return: 0: not changed, no outBuf allocated. >0: ok. <0: fail
        filtered = av_bitstream_filter_filter(d.bsf, d.codec_ctx, NULL, &outBuf, &outBufSize
                                                  , (const uint8_t*)packet.data.constData(), packet.data.size()
                                                  , 0);//d.is_keyframe);
        //qDebug("%s @%d filtered=%d outBuf=%p, outBufSize=%d", __FUNCTION__, __LINE__, filtered, outBuf, outBufSize);
        if (filtered < 0) {
            qDebug("failed to filter: %s", av_err2str(filtered));
        }
    } else {
        outBuf = (uint8_t*)packet.data.constData();
        outBufSize = packet.data.size();
    }

    CUVIDSOURCEDATAPACKET cuvid_pkt;
    memset(&cuvid_pkt, 0, sizeof(CUVIDSOURCEDATAPACKET));
    cuvid_pkt.payload = outBuf;// (unsigned char *)packet.data.constData();
    cuvid_pkt.payload_size = outBufSize; //packet.data.size();
    // TODO: other flags
    if (packet.pts >= 0.0) {
        cuvid_pkt.flags = CUVID_PKT_TIMESTAMP;
        cuvid_pkt.timestamp = packet.pts * 1000.0; // TODO: 10MHz?
    }
    //TODO: fill NALU header for h264? https://devtalk.nvidia.com/default/topic/515571/what-the-data-format-34-cuvidparsevideodata-34-can-accept-/
    d.doParseVideoData(&cuvid_pkt);
    if (filtered > 0) {
        av_freep(&outBuf);
    }
    // callbacks are in the same thread as this. so no queue is required?
    //qDebug("frame queue size on decode: %d", d.frame_queue.size());
    return !d.frame_queue.isEmpty();
    // video thread: if dec.hasFrame() keep pkt for the next loop and not decode, direct display the frame
}

VideoFrame VideoDecoderCUDA::frame()
{
    DPTR_D(VideoDecoderCUDA);
    if (d.frame_queue.isEmpty())
        return VideoFrame();
#if COPY_ON_DECODE
    return d.frame_queue.take();
#else
    return d.getNextFrame();
#endif
}

int VideoDecoderCUDA::surfaces() const
{
    return d_func().nb_dec_surface;
}

void VideoDecoderCUDA::setSurfaces(int n)
{
    if (n <= 0)
        n = kMaxDecodeSurfaces;
    DPTR_D(VideoDecoderCUDA);
    d.nb_dec_surface = n;
    d.surface_in_use.resize(n);
    d.surface_in_use.fill(false);
}

VideoDecoderCUDA::Flags VideoDecoderCUDA::flags() const
{
    return (Flags)d_func().create_flags;
}

void VideoDecoderCUDA::setFlags(Flags f)
{
    d_func().create_flags = (cudaVideoCreateFlags)f;
}

VideoDecoderCUDA::Deinterlace VideoDecoderCUDA::deinterlace() const
{
    return (Deinterlace)d_func().deinterlace;
}

void VideoDecoderCUDA::setDeinterlace(Deinterlace di)
{
    d_func().deinterlace = (cudaVideoDeinterlaceMode)di;
}

VideoDecoderCUDA::CopyMode VideoDecoderCUDA::copyMode() const
{
    return d_func().copy_mode;
}

void VideoDecoderCUDA::setCopyMode(CopyMode value)
{
    DPTR_D(VideoDecoderCUDA);
    if (d.copy_mode == value)
        return;
    d.copy_mode = value;
    Q_EMIT copyModeChanged(value);
}

bool VideoDecoderCUDAPrivate::open()
{
    //TODO: destroy decoder
    // d.available is true if cuda decoder is ready
    if (!can_load) {
        qWarning("VideoDecoderCUDAPrivate::open(): CUVID library not available");
        return false;
    }
    if (!isLoaded()) //cuda_api
        return false;
    if (!cuctx)
        initCuda();
    setBSF(codec_ctx->codec_id);
    // max decoder surfaces is computed in createCUVIDDecoder. createCUVIDParser use the value
    if (!createCUVIDDecoder(mapCodecFromFFmpeg(codec_ctx->codec_id), codec_ctx->coded_width, codec_ctx->coded_height))
        return false;
    if (!createCUVIDParser())
        return false;
    available = true;
    return true;
}

bool VideoDecoderCUDAPrivate::initCuda()
{
    CUDA_ENSURE(cuInit(0), false);
    cudev = GetMaxGflopsGraphicsDeviceId();

    int clockRate;
    cuDeviceGetAttribute(&clockRate, CU_DEVICE_ATTRIBUTE_CLOCK_RATE, cudev);
    int major, minor;
    CUDA_WARN(cuDeviceComputeCapability(&major, &minor, cudev));
    char devname[256];
    CUDA_WARN(cuDeviceGetName(devname, 256, cudev));
    description = QStringLiteral("CUDA device: %1 %2.%3 %4 MHz @%5").arg(QLatin1String((const char*)devname)).arg(major).arg(minor).arg(clockRate/1000).arg(cudev);

    // cuD3DCtxCreate > cuGLCtxCreate(deprecated) > cuCtxCreate (fallback if d3d and gl return status is failed)
    CUDA_ENSURE(cuCtxCreate(&cuctx, CU_CTX_SCHED_BLOCKING_SYNC, cudev), false); //CU_CTX_SCHED_AUTO: slower in my test
#if 0 //FIXME: why mingw crash?
    unsigned api_ver = 0;
    CUDA_ENSURE(cuCtxGetApiVersion(cuctx, &api_ver), false);
    qDebug("cuCtxGetApiVersion: %u", api_ver);
#endif
    CUDA_ENSURE(cuCtxPopCurrent(&cuctx), false);
    CUDA_ENSURE(cuvidCtxLockCreate(&vid_ctx_lock, cuctx), 0);
    {
        AutoCtxLock lock(this, vid_ctx_lock);
        Q_UNUSED(lock);
        //Flags- Parameters for stream creation (must be 0 (CU_STREAM_DEFAULT=0 in cuda5) in cuda 4.2, no CU_STREAM_NON_BLOCKING)
        CUDA_ENSURE(cuStreamCreate(&stream, CU_STREAM_DEFAULT), false);
        //require compute capability >= 1.1
        //flag: Reserved for future use, must be 0
        //cuStreamAddCallback(stream, CUstreamCallback, this, 0);
    }
    return true;
}

bool VideoDecoderCUDAPrivate::releaseCuda()
{
    available = false;
    if (cuctx)
        CUDA_WARN(cuCtxPushCurrent(cuctx)); //cuMemFreeHost need the context of cuMemAllocHost which was called in VideoThread, while releaseCuda() in dtor can be called in any thread
    if (!can_load)
        return true;
    if (dec) {
        CUDA_WARN(cuvidDestroyDecoder(dec));
        dec = 0;
    }
    if (parser) {
        CUDA_WARN(cuvidDestroyVideoParser(parser));
        parser = 0;
    }
    if (stream) {
        CUDA_WARN(cuStreamDestroy(stream));
        stream = 0;
    }
    if (host_data) {
        CUDA_WARN(cuMemFreeHost(host_data)); //CUDA_ERROR_INVALID_CONTEXT
        host_data = 0;
        host_data_size = 0;
    }
    if (vid_ctx_lock) {
        CUDA_WARN(cuvidCtxLockDestroy(vid_ctx_lock));
        vid_ctx_lock = 0;
    }
    if (cuctx) {
        CUDA_ENSURE(cuCtxDestroy(cuctx), false);
        cuctx = 0;
    }
    return true;
}

bool VideoDecoderCUDAPrivate::createCUVIDDecoder(cudaVideoCodec cudaCodec, int cw, int ch)
{
    if (cudaCodec == cudaVideoCodec_NumCodecs) {
        return false;
    }
    AutoCtxLock lock(this, vid_ctx_lock);
    Q_UNUSED(lock);
    if (dec) {
        CUDA_ENSURE(cuvidDestroyDecoder(dec), false);
    }
    memset(&dec_create_info, 0, sizeof(CUVIDDECODECREATEINFO));
    dec_create_info.ulWidth = cw; // Coded Sequence Width
    dec_create_info.ulHeight = ch;
    dec_create_info.ulNumDecodeSurfaces = nb_dec_surface; //same as ulMaxNumDecodeSurfaces
    dec_create_info.CodecType = cudaCodec;
    dec_create_info.ChromaFormat = cudaVideoChromaFormat_420;  // cudaVideoChromaFormat_XXX (only 4:2:0 is currently supported)
    //cudaVideoCreate_PreferCUVID is slow in example. DXVA may failed to create (CUDA_ERROR_NO_DEVICE)
    dec_create_info.ulCreationFlags = create_flags;
    // TODO: lav yv12
    dec_create_info.OutputFormat = cudaVideoSurfaceFormat_NV12; // NV12 (currently the only supported output format)
    dec_create_info.DeinterlaceMode = deinterlace;
    // No scaling
    dec_create_info.ulTargetWidth = cw;
    dec_create_info.ulTargetHeight = ch;
    //TODO: dec_create_info.display_area.
    dec_create_info.ulNumOutputSurfaces = 2;  // We won't simultaneously map more than 8 surfaces
    dec_create_info.vidLock = vid_ctx_lock;//vidCtxLock; //FIXME

    // Limit decode memory to 24MB (16M pixels at 4:2:0 = 24M bytes)
    // otherwise CUDA_ERROR_OUT_OF_MEMORY on cuMemcpyDtoH
    // if ulNumDecodeSurfaces < ulMaxNumDecodeSurfaces, CurrPicIdx may be > ulNumDecodeSurfaces
    /*
     * TODO: check video memory, e.g. runtime api extern __host__ cudaError_t CUDARTAPI cudaMemGetInfo(size_t *free, size_t *total);
     * 24MB is too small for 4k video, only n2 surfaces can be use so decoding will be too slow
     */
#if 0
    while (dec_create_info.ulNumDecodeSurfaces * codec_ctx->coded_width * codec_ctx->coded_height > 16*1024*1024) {
        dec_create_info.ulNumDecodeSurfaces--;
    }
#endif
    // create the decoder
    available = false;
    CUDA_ENSURE(cuvidCreateDecoder(&dec, &dec_create_info), false);
    available = true;
    return true;
}

bool VideoDecoderCUDAPrivate::createCUVIDParser()
{
    cudaVideoCodec cudaCodec = mapCodecFromFFmpeg(codec_ctx->codec_id);
    if (cudaCodec == cudaVideoCodec_NumCodecs) {
        QString es(QObject::tr("Codec %1 is not supported by CUDA").arg(QLatin1String(avcodec_get_name(codec_ctx->codec_id))));
        //emit error(AVError::CodecError, es);
        qWarning() << es;
        available = false;
        return false;
    }
    if (parser) {
        CUDA_WARN(cuvidDestroyVideoParser(parser));
        parser = 0;
    }
    //lavfilter check level C
    CUVIDPARSERPARAMS parser_params;
    memset(&parser_params, 0, sizeof(CUVIDPARSERPARAMS));
    parser_params.CodecType = cudaCodec;
    /*!
     * CUVIDPICPARAMS.CurrPicIdx <= kMaxDecodeSurfaces.
     * CUVIDPARSERDISPINFO.picture_index <= kMaxDecodeSurfaces
     * HandlePictureDecode must check whether CUVIDPICPARAMS.CurrPicIdx is in use
     * HandlePictureDisplay must mark CUVIDPARSERDISPINFO.picture_index is in use
     * If a frame is unmapped, mark the index not in use
     *
     */
    parser_params.ulMaxNumDecodeSurfaces = nb_dec_surface;
    //parser_params.ulMaxDisplayDelay = 4; //?
    parser_params.pUserData = this;
    // Parser callbacks
    // The parser will call these synchronously from within cuvidParseVideoData(), whenever a picture is ready to
    // be decoded and/or displayed.
    parser_params.pfnSequenceCallback = VideoDecoderCUDAPrivate::HandleVideoSequence;
    parser_params.pfnDecodePicture = VideoDecoderCUDAPrivate::HandlePictureDecode;
    parser_params.pfnDisplayPicture = VideoDecoderCUDAPrivate::HandlePictureDisplay;
    parser_params.ulErrorThreshold = 0;//!wait for key frame
    //parser_params.pExtVideoInfo

    qDebug("~~~~~~~~~~~~~~~~extradata: %p %d", codec_ctx->extradata, codec_ctx->extradata_size);
    memset(&extra_parser_info, 0, sizeof(CUVIDEOFORMATEX));
    // nalu
    extra_parser_info.format.seqhdr_data_length = 0;
    if (codec_ctx->codec_id != QTAV_CODEC_ID(H264) && codec_ctx->codec_id != QTAV_CODEC_ID(HEVC)) {
        if (codec_ctx->extradata_size > 0) {
            extra_parser_info.format.seqhdr_data_length = codec_ctx->extradata_size;
            memcpy(extra_parser_info.raw_seqhdr_data, codec_ctx->extradata, FFMIN(sizeof(extra_parser_info.raw_seqhdr_data), codec_ctx->extradata_size));
        }
    }
    parser_params.pExtVideoInfo = &extra_parser_info;

    CUDA_ENSURE(cuvidCreateVideoParser(&parser, &parser_params), false);
    CUVIDSOURCEDATAPACKET seq_pkt;
    seq_pkt.payload = extra_parser_info.raw_seqhdr_data;
    seq_pkt.payload_size = extra_parser_info.format.seqhdr_data_length;
    if (seq_pkt.payload_size > 0) {
        CUDA_ENSURE(cuvidParseVideoData(parser, &seq_pkt), false);
    }
    //lavfilter: cuStreamCreate
    force_sequence_update = true;
    //DecodeSequenceData()
    return true;
}

bool VideoDecoderCUDAPrivate::flushParser()
{
    CUVIDSOURCEDATAPACKET flush_packet;
    memset(&flush_packet, 0, sizeof(CUVIDSOURCEDATAPACKET));
    flush_packet.flags |= CUVID_PKT_ENDOFSTREAM;
    return doParseVideoData(&flush_packet);
}

bool VideoDecoderCUDAPrivate::processDecodedData(CUVIDPARSERDISPINFO *cuviddisp, VideoFrame* outFrame) {
    int num_fields = cuviddisp->progressive_frame ? 1 : 2+cuviddisp->repeat_first_field;
    for (int active_field = 0; active_field < num_fields; ++active_field) {
        CUVIDPROCPARAMS proc_params;
        memset(&proc_params, 0, sizeof(CUVIDPROCPARAMS));
        proc_params.progressive_frame = cuviddisp->progressive_frame; //check user config
        proc_params.second_field = active_field == 1; //check user config
        proc_params.top_field_first = cuviddisp->top_field_first;
        proc_params.unpaired_field = cuviddisp->progressive_frame == 1;

        //const uint cw = dec_create_info.ulWidth;//PAD_ALIGN(dec_create_info.ulWidth, 0x3F);
        const uint ch = dec_create_info.ulHeight;//PAD_ALIGN(dec_create_info.ulHeight, 0x0F); //?
        CUdeviceptr devptr;
        unsigned int pitch;
        {
        AutoCtxLock lock(this, vid_ctx_lock);
        Q_UNUSED(lock);
        //CUDA_ENSURE(cuCtxPushCurrent(cuctx), false);
        CUDA_ENSURE(cuvidMapVideoFrame(dec, cuviddisp->picture_index, &devptr, &pitch, &proc_params), false);
        CUVIDAutoUnmapper unmapper(this, dec, devptr);
        Q_UNUSED(unmapper);
        if (copy_mode != VideoDecoderCUDA::ZeroCopy) {
            const int size = pitch*ch*3/2;
            if (size > host_data_size && host_data) {
                cuMemFreeHost(host_data);
                host_data = 0;
                host_data_size = 0;
            }
            if (!host_data) {
                CUDA_ENSURE(cuMemAllocHost((void**)&host_data, size), false);
                host_data_size = size;
            }
            // copy to the memory not allocated by cuda is possible but much slower
            // TODO: cuMemcpy2D?
            CUDA_ENSURE(cuMemcpyDtoHAsync(host_data, devptr, size, stream), false);
            CUDA_WARN(cuStreamSynchronize(stream));
        }
        } // lock end
        //CUDA_ENSURE(cuCtxPopCurrent(&cuctx), false);
        //qDebug("cuCtxPopCurrent %p", cuctx);

        VideoFrame frame;
        if (copy_mode != VideoDecoderCUDA::GenericCopy && interop_res) {
            if (OpenGLHelper::isOpenGLES() && copy_mode == VideoDecoderCUDA::ZeroCopy) {
                proc_params.Reserved[0] = pitch; // TODO: pass pitch to setSurface()
                frame = VideoFrame(codec_ctx->width, codec_ctx->height, VideoFormat::Format_RGB32);
                frame.setBytesPerLine(codec_ctx->width * 4); //used by gl to compute texture size
            } else {
                frame = VideoFrame(codec_ctx->width, codec_ctx->height, VideoFormat::Format_NV12);
            }
            cuda::SurfaceInteropCUDA *interop = new cuda::SurfaceInteropCUDA(interop_res);
            interop->setSurface(cuviddisp->picture_index, proc_params, codec_ctx->width, codec_ctx->height, ch); //TODO: both surface size(for copy 2d) and frame size(for map host)
            frame.setMetaData(QStringLiteral("surface_interop"), QVariant::fromValue(VideoSurfaceInteropPtr(interop)));
        } else {
            uchar *planes[] = {
                host_data,
                host_data + pitch * ch
            };
            frame = VideoFrame(codec_ctx->width, codec_ctx->height, VideoFormat::Format_NV12);
            frame.setBits(planes);
        }
        int pitches[] = { (int)pitch, (int)pitch };
        if (!frame.format().isRGB()) {
            frame.setBytesPerLine(pitches);
            frame.setColorRange(yuv_range);
        }
        frame.setTimestamp((double)cuviddisp->timestamp/1000.0);
        if (codec_ctx && codec_ctx->sample_aspect_ratio.num > 1) //skip 1/1 because is the default value
            frame.setDisplayAspectRatio(frame.displayAspectRatio()*av_q2d(codec_ctx->sample_aspect_ratio));
        if (copy_mode == VideoDecoderCUDA::GenericCopy)
            frame = frame.clone();
        if (outFrame) {
            *outFrame = frame;
        }
#if COPY_ON_DECODE
        frame_queue.put(frame);
#endif
        //qDebug("frame queue size: %d", frame_queue.size());
        surface_in_use[cuviddisp->picture_index] = false; // FIXME: 0-copy still use the index
    }
    return true;
}

void VideoDecoderCUDAPrivate::setBSF(AVCodecID codec)
{
    if (codec == QTAV_CODEC_ID(H264)) {
        if (!bsf)
            bsf = av_bitstream_filter_init("h264_mp4toannexb");
        Q_ASSERT(bsf && "h264_mp4toannexb bsf not found");
    } else if (codec == QTAV_CODEC_ID(HEVC)) {
        if (!bsf)
            bsf = av_bitstream_filter_init("hevc_mp4toannexb");
        Q_ASSERT(bsf && "hevc_mp4toannexb bsf not found");
    } else {
        if (bsf) {
            av_bitstream_filter_close(bsf);
            bsf = 0;
        }
    }
}

} //namespace QtAV

#include "VideoDecoderCUDA.moc"
