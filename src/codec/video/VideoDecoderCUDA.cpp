/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2012-2014 Wang Bin <wbsecg1@gmail.com>

*   This file is part of QtAV

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
#include "QtAV/private/VideoDecoder_p.h"
#include "QtAV/private/prepost.h"
#include <QtCore/QQueue>
#if QTAV_HAVE(DLLAPI_CUDA)
#include "dllapi.h"
#endif //QTAV_HAVE(DLLAPI_CUDA)
#include "QtAV/private/AVCompat.h"
#include "utils/BlockingQueue.h"

#define COPY_ON_DECODE 1
#define FILTER_ANNEXB_CUVID 0
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

//decode error if not floating context

namespace QtAV {

static const unsigned int kMaxDecodeSurfaces = 20;
class VideoDecoderCUDAPrivate;
class VideoDecoderCUDA : public VideoDecoder
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(VideoDecoderCUDA)
    Q_PROPERTY(int surfaces READ surfaces WRITE setSurfaces)
    Q_PROPERTY(Flags flags READ flags WRITE setFlags)
    Q_PROPERTY(Deinterlace deinterlace READ deinterlace WRITE setDeinterlace)
    Q_FLAGS(Flags)
    Q_ENUMS(Flags)
    Q_ENUMS(Deinterlace)
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

    VideoDecoderCUDA();
    virtual ~VideoDecoderCUDA();
    virtual VideoDecoderId id() const;
    virtual QString description() const;
    virtual void flush();
    virtual bool prepare();
    QTAV_DEPRECATED bool decode(const QByteArray &encoded) Q_DECL_FINAL;
    bool decode(const Packet &packet) Q_DECL_FINAL;
    virtual VideoFrame frame();

    // properties
    int surfaces() const;
    void setSurfaces(int n);
    Flags flags() const;
    void setFlags(Flags f);
    Deinterlace deinterlace() const;
    void setDeinterlace(Deinterlace di);
};


extern VideoDecoderId VideoDecoderId_CUDA;
FACTORY_REGISTER_ID_AUTO(VideoDecoder, CUDA, "CUDA")

void RegisterVideoDecoderCUDA_Man()
{
    FACTORY_REGISTER_ID_MAN(VideoDecoder, CUDA, "CUDA")
}


static cudaVideoCodec mapCodecFromFFmpeg(AVCodecID codec)
{
    static struct {
        AVCodecID ffcodec;
        cudaVideoCodec cudaCodec;
    } ff_cuda_codecs[] = {
        // AV_CODEC_ID_H265 is a macro defined as AV_CODEC_ID_HEVC. so we can avoid libavcodec version check. (from ffmpeg 2.1)
#if defined(AV_CODEC_ID_H265) && (CUDA_VERSION >= 6050) //TODO: check avcodec
        { QTAV_CODEC_ID(HEVC),       cudaVideoCodec_HEVC },
#endif //
        { QTAV_CODEC_ID(MPEG1VIDEO), cudaVideoCodec_MPEG1 },
        { QTAV_CODEC_ID(MPEG2VIDEO), cudaVideoCodec_MPEG2 },
        { QTAV_CODEC_ID(VC1),        cudaVideoCodec_VC1   },
        { QTAV_CODEC_ID(H264),       cudaVideoCodec_H264  },
        { QTAV_CODEC_ID(MPEG4),      cudaVideoCodec_MPEG4 },
        { (AVCodecID)-1, (cudaVideoCodec)-1}
    };
    for (int i = 0; ff_cuda_codecs[i].cudaCodec != -1; ++i) {
        if (ff_cuda_codecs[i].ffcodec == codec) {
            return ff_cuda_codecs[i].cudaCodec;
        }
    }
    return (cudaVideoCodec)-1;
}

#if NV_CONFIG(DLLAPI_CUDA) || defined(CUDA_LINK)
class AutoCtxLock
{
private:
    CUvideoctxlock m_lock;
public:
    AutoCtxLock(cuda_api*, CUvideoctxlock lck) { m_lock=lck; cuvidCtxLock(m_lock, 0); }
    ~AutoCtxLock() { cuvidCtxUnlock(m_lock, 0); }
};
#endif //NV_CONFIG(DLLAPI_CUDA) || defined(CUDA_LINK)
class VideoDecoderCUDAPrivate : public VideoDecoderPrivate
                , public cuda_api
{
public:
    VideoDecoderCUDAPrivate():
        VideoDecoderPrivate()
      , can_load(true)
      , flushing(false)
      , host_data(0)
      , host_data_size(0)
      , create_flags(cudaVideoCreate_Default)
      , deinterlace(cudaVideoDeinterlaceMode_Adaptive)
      , nb_dec_surface(kMaxDecodeSurfaces)
    {
#if QTAV_HAVE(DLLAPI_CUDA)
        can_load = dllapi::testLoad("nvcuvid");
#endif //QTAV_HAVE(DLLAPI_CUDA)
        available = false;
        bitstream_filter_ctx = 0;
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
    }
    ~VideoDecoderCUDAPrivate() {
        if (bitstream_filter_ctx)
            av_bitstream_filter_close(bitstream_filter_ctx);
        if (!can_load)
            return;
        if (!isLoaded()) //cuda_api
            return;
        releaseCuda();
    }
    bool initCuda();
    bool releaseCuda();
    bool createCUVIDDecoder(cudaVideoCodec cudaCodec, int w, int h);
    bool createCUVIDParser();
    bool flushParser();
    bool processDecodedData(CUVIDPARSERDISPINFO *cuviddisp, VideoFrame* outFrame = 0);
    bool doParseVideoData(CUVIDSOURCEDATAPACKET* pPkt) {
        AutoCtxLock lock(this, vid_ctx_lock);
        Q_UNUSED(lock);
        CUresult cuStatus = cuvidParseVideoData(parser, pPkt);
        if (cuStatus != CUDA_SUCCESS) {
            qWarning("cuvidParseVideoData failed (%#x, %s)", cuStatus, _cudaGetErrorEnum(cuStatus));
            return false;
        }
        return true;
    }

    bool doDecodePicture(CUVIDPICPARAMS *cuvidpic) {
        AutoCtxLock lock(this, vid_ctx_lock);
        Q_UNUSED(lock);
        checkCudaErrors(cuvidDecodePicture(dec, cuvidpic));
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
            //coded_width or width?
            p->createCUVIDDecoder(cuvidfmt->codec, cuvidfmt->coded_width, cuvidfmt->coded_height);
            // how about parser.ulMaxNumDecodeSurfaces? recreate?
        }
        //TODO: lavfilter
        return 1;
    }
    static int CUDAAPI HandlePictureDecode(void *obj, CUVIDPICPARAMS *cuvidpic) {
        VideoDecoderCUDAPrivate *p = reinterpret_cast<VideoDecoderCUDAPrivate*>(obj);
        if (p->flushing)
            return 0;
        //qDebug("%s @%d tid=%p dec=%p idx=%d inUse=%d", __FUNCTION__, __LINE__, QThread::currentThread(), p->dec, cuvidpic->CurrPicIdx, p->surface_in_use[cuvidpic->CurrPicIdx]);
        p->doDecodePicture(cuvidpic);
        return 1;
    }
    static int CUDAAPI HandlePictureDisplay(void *obj, CUVIDPARSERDISPINFO *cuviddisp) {
        VideoDecoderCUDAPrivate *p = reinterpret_cast<VideoDecoderCUDAPrivate*>(obj);
        if (p->flushing)
            return 0;
        p->surface_in_use[cuviddisp->picture_index] = true;
        //qDebug("mark in use pic_index: %d", cuviddisp->picture_index);
        //qDebug("%s @%d tid=%p dec=%p", __FUNCTION__, __LINE__, QThread::currentThread(), p->dec);
#if COPY_ON_DECODE
        return p->processDecodedData(cuviddisp, 0);
#else
        p->frame_queue.put(cuviddisp);
        return 1;
#endif
    }

    bool can_load; //if linked to cuvid, it's true. otherwise(use dllapi) equals to whether cuvid can be loaded
    bool flushing;
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

    AVBitStreamFilterContext *bitstream_filter_ctx;
};

VideoDecoderCUDA::VideoDecoderCUDA():
    VideoDecoder(*new VideoDecoderCUDAPrivate())
{
    // dynamic properties about static property details. used by UI
    // format: detail_property
    setProperty("detail_surfaces", tr("Decoding surfaces."));
    setProperty("detail_flags", tr("Decoder flags"));
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
    return "NVIDIA CUVID";
}

void VideoDecoderCUDA::flush()
{
    DPTR_D(VideoDecoderCUDA);
    d.frame_queue.clear();
}

bool VideoDecoderCUDA::prepare()
{
    //TODO: destroy decoder
    DPTR_D(VideoDecoderCUDA);
    if (!d.codec_ctx) {
        qWarning("AVCodecContext not ready");
        return false;
    }
    // d.available is true if cuda decoder is ready
    if (!d.can_load) {
        qWarning("VideoDecoderCUDA::prepare(): CUVID library not available");
        return false;
    }
    if (!d.isLoaded()) //cuda_api
        return false;
    if (!d.cuctx)
        d.initCuda();
    d.bitstream_filter_ctx = av_bitstream_filter_init("h264_mp4toannexb");
    Q_ASSERT_X(d.bitstream_filter_ctx, "av_bitstream_filter_init", "Unknown bitstream filter");
    // max decoder surfaces is computed in createCUVIDDecoder. createCUVIDParser use the value
    return d.createCUVIDDecoder(mapCodecFromFFmpeg(d.codec_ctx->codec_id), d.codec_ctx->coded_width, d.codec_ctx->coded_height)
            && d.createCUVIDParser();
}

bool VideoDecoderCUDA::decode(const QByteArray &encoded)
{
    if (!isAvailable())
        return false;
    DPTR_D(VideoDecoderCUDA);
    if (!d.parser) {
        qWarning("CUVID parser not ready");
        return false;
    }
    uint8_t *outBuf = 0;
    int outBufSize = 0;
    // h264_mp4toannexb_filter does not use last parameter 'keyFrame', so just set 0
    //return: 0: not changed, no outBuf allocated. >0: ok. <0: fail
    int filtered = av_bitstream_filter_filter(d.bitstream_filter_ctx, d.codec_ctx, NULL, &outBuf, &outBufSize
                                              , (const uint8_t*)encoded.constData(), encoded.size()
                                              , 0);//d.is_keyframe);
    //qDebug("%s @%d filtered=%d outBuf=%p, outBufSize=%d", __FUNCTION__, __LINE__, filtered, outBuf, outBufSize);
    if (filtered < 0) {
        qDebug("failed to filter: %s", av_err2str(filtered));
    }
    unsigned char *payload = outBuf;
    unsigned long payload_size = outBufSize;
#if 0 // see ffmpeg.c. FF_INPUT_BUFFER_PADDING_SIZE for alignment issue
    QByteArray data_with_pad;
    if (filtered > 0) {
        data_with_pad.resize(outBufSize + FF_INPUT_BUFFER_PADDING_SIZE);
        data_with_pad.fill(0);
        memcpy(data_with_pad.data(), outBuf, outBufSize);
        payload = (unsigned char*)data_with_pad.constData();
        payload_size = data_with_pad.size();
    }
#endif
    CUVIDSOURCEDATAPACKET cuvid_pkt;
    memset(&cuvid_pkt, 0, sizeof(CUVIDSOURCEDATAPACKET));
    cuvid_pkt.payload = payload;// (unsigned char *)encoded.constData();
    cuvid_pkt.payload_size = payload_size; //encoded.size();
    cuvid_pkt.flags = CUVID_PKT_TIMESTAMP;
    cuvid_pkt.timestamp = 0;// ?
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

bool VideoDecoderCUDA::decode(const Packet &packet)
{
    if (!isAvailable())
        return false;
    DPTR_D(VideoDecoderCUDA);
    if (!d.parser) {
        qWarning("CUVID parser not ready");
        return false;
    }
    uint8_t *outBuf = 0;
    int outBufSize = 0;
    // h264_mp4toannexb_filter does not use last parameter 'keyFrame', so just set 0
    //return: 0: not changed, no outBuf allocated. >0: ok. <0: fail
    int filtered = av_bitstream_filter_filter(d.bitstream_filter_ctx, d.codec_ctx, NULL, &outBuf, &outBufSize
                                              , (const uint8_t*)packet.data.constData(), packet.data.size()
                                              , 0);//d.is_keyframe);
    //qDebug("%s @%d filtered=%d outBuf=%p, outBufSize=%d", __FUNCTION__, __LINE__, filtered, outBuf, outBufSize);
    if (filtered < 0) {
        qDebug("failed to filter: %s", av_err2str(filtered));
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
#if COPY_ON_DECODE
    return d.frame_queue.take();
#else
    return d.getNextFrame();
#endif
}

bool VideoDecoderCUDAPrivate::initCuda()
{
    CUresult result = cuInit(0);
    if (result != CUDA_SUCCESS) {
        available = false;
        qWarning("cuInit(0) faile (%d)", result);
        return false;
    }
    cudev = GetMaxGflopsGraphicsDeviceId();

    int clockRate;
    cuDeviceGetAttribute(&clockRate, CU_DEVICE_ATTRIBUTE_CLOCK_RATE, cudev);
    int major, minor;
    cuDeviceComputeCapability(&major, &minor, cudev);
    char devname[256];
    cuDeviceGetName(devname, 256, cudev);
    description = QString("CUDA device: %1 %2.%3 %4 MHz").arg(devname).arg(major).arg(minor).arg(clockRate/1000);

    //TODO: cuD3DCtxCreate > cuGLCtxCreate > cuCtxCreate
    checkCudaErrors(cuCtxCreate(&cuctx, CU_CTX_SCHED_BLOCKING_SYNC, cudev)); //CU_CTX_SCHED_AUTO?
    CUcontext cuCurrent = NULL;
    result = cuCtxPopCurrent(&cuCurrent);
    if (result != CUDA_SUCCESS) {
        qWarning("cuCtxPopCurrent: %d\n", result);
        return false;
    }
    checkCudaErrors(cuvidCtxLockCreate(&vid_ctx_lock, cuctx));
    {
        AutoCtxLock lock(this, vid_ctx_lock);
        Q_UNUSED(lock);
        //Flags- Parameters for stream creation (must be 0 (CU_STREAM_DEFAULT=0 in cuda5) in cuda 4.2, no CU_STREAM_NON_BLOCKING)
        checkCudaErrors(cuStreamCreate(&stream, 0));//CU_STREAM_NON_BLOCKING)); //CU_STREAM_DEFAULT
        //require compute capability >= 1.1
        //flag: Reserved for future use, must be 0
        //cuStreamAddCallback(stream, CUstreamCallback, this, 0);
    }
    return true;
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

bool VideoDecoderCUDAPrivate::releaseCuda()
{
    available = false;
    if (!can_load)
        return true;
    if (dec) {
        cuvidDestroyDecoder(dec);
        dec = 0;
    }
    if (parser) {
        cuvidDestroyVideoParser(parser);
        parser = 0;
    }
    if (stream) {
        cuStreamDestroy(stream);
        stream = 0;
    }
    if (host_data) {
        cuMemFreeHost(host_data);
        host_data = 0;
        host_data_size = 0;
    }
    if (vid_ctx_lock) {
        cuvidCtxLockDestroy(vid_ctx_lock);
        vid_ctx_lock = 0;
    }
    if (cuctx) {
        checkCudaErrors(cuCtxDestroy(cuctx));
    }
    // TODO: dllapi unload
    return true;
}

bool VideoDecoderCUDAPrivate::createCUVIDDecoder(cudaVideoCodec cudaCodec, int w, int h)
{
    if (cudaCodec == -1) {
        return false;
    }
    AutoCtxLock lock(this, vid_ctx_lock);
    Q_UNUSED(lock);
    if (dec) {
        checkCudaErrors(cuvidDestroyDecoder(dec));
    }
    memset(&dec_create_info, 0, sizeof(CUVIDDECODECREATEINFO));
    dec_create_info.ulWidth = w;
    dec_create_info.ulHeight = h;
    dec_create_info.ulNumDecodeSurfaces = nb_dec_surface; //same as ulMaxNumDecodeSurfaces
    dec_create_info.CodecType = cudaCodec;
    dec_create_info.ChromaFormat = cudaVideoChromaFormat_420;  // cudaVideoChromaFormat_XXX (only 4:2:0 is currently supported)
    //cudaVideoCreate_PreferCUVID is slow in example. DXVA may failed to create (CUDA_ERROR_NO_DEVICE)
    dec_create_info.ulCreationFlags = create_flags;
    // TODO: lav yv12
    dec_create_info.OutputFormat = cudaVideoSurfaceFormat_NV12; // NV12 (currently the only supported output format)
    dec_create_info.DeinterlaceMode = deinterlace;
    // No scaling
    dec_create_info.ulTargetWidth = dec_create_info.ulWidth;
    dec_create_info.ulTargetHeight = dec_create_info.ulHeight;
    dec_create_info.ulNumOutputSurfaces = 2;  // We won't simultaneously map more than 8 surfaces
    dec_create_info.vidLock = vid_ctx_lock;//vidCtxLock; //FIXME

    // Limit decode memory to 24MB (16M pixels at 4:2:0 = 24M bytes)
    // otherwise CUDA_ERROR_OUT_OF_MEMORY on cuMemcpyDtoH
    // if ulNumDecodeSurfaces < ulMaxNumDecodeSurfaces, CurrPicIdx may be > ulNumDecodeSurfaces
    /*
     * TODO: check video memory, e.g. runtime apu extern __host__ cudaError_t CUDARTAPI cudaMemGetInfo(size_t *free, size_t *total);
     * 24MB is too small for 4k video, only n2 surfaces can be use so decoding will be too slow
     */
#if 0
    while (dec_create_info.ulNumDecodeSurfaces * codec_ctx->coded_width * codec_ctx->coded_height > 16*1024*1024) {
        dec_create_info.ulNumDecodeSurfaces--;
    }
#endif
    // create the decoder
    available = false;
    checkCudaErrors(cuvidCreateDecoder(&dec, &dec_create_info));
    available = true;
    return true;
}

bool VideoDecoderCUDAPrivate::createCUVIDParser()
{
    cudaVideoCodec cudaCodec = mapCodecFromFFmpeg(codec_ctx->codec_id);
    if (cudaCodec == -1) {
        QString es(QObject::tr("Codec %1 is not supported by CUDA").arg(avcodec_get_name(codec_ctx->codec_id)));
        //emit error(AVError::CodecError, es);
        qWarning() << es;
        available = false;
        return false;
    }
    if (parser) {
        cuvidDestroyVideoParser(parser);
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
    parser_params.pfnSequenceCallback = VideoDecoderCUDAPrivate::HandleVideoSequence;
    parser_params.pfnDecodePicture = VideoDecoderCUDAPrivate::HandlePictureDecode;
    parser_params.pfnDisplayPicture = VideoDecoderCUDAPrivate::HandlePictureDisplay;
    parser_params.ulErrorThreshold = 0;//!wait for key frame
    //parser_params.pExtVideoInfo

    qDebug("~~~~~~~~~~~~~~~~extradata: %p %d", codec_ctx->extradata, codec_ctx->extradata_size);
    /*!
     * NOTE: DO NOT call h264_extradata_to_annexb here if use av_bitstream_filter_filter
     * because av_bitstream_filter_filter will call h264_extradata_to_annexb and marks H264BSFContext has parsed
     * h264_extradata_to_annexb will not mark it
     * LAVFilter's
     */
#if FILTER_ANNEXB_CUVID
    memset(&extra_parser_info, 0, sizeof(CUVIDEOFORMATEX));
    // nalu
    // TODO: check mpeg, avc1, ccv1? (lavf)
    if (codec_ctx->extradata && codec_ctx->extradata_size >= 6) {
        int ret = h264_extradata_to_annexb(codec_ctx, FF_INPUT_BUFFER_PADDING_SIZE);
        if (ret >= 0) {
            qDebug("%s @%d ret %d, size %d", __FUNCTION__, __LINE__, ret, codec_ctx->extradata_size);
            memcpy(extra_parser_info.raw_seqhdr_data, codec_ctx->extradata, codec_ctx->extradata_size);
            extra_parser_info.format.seqhdr_data_length = codec_ctx->extradata_size;
        }
    }
    parser_params.pExtVideoInfo = &extra_parser_info;
#endif
    checkCudaErrors(cuvidCreateVideoParser(&parser, &parser_params));
    //lavfilter: cuStreamCreate
    force_sequence_update = true;
    //DecodeSequenceData()
    return true;
}

bool VideoDecoderCUDAPrivate::flushParser()
{
    flushing = true;
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

        CUdeviceptr devptr;
        unsigned int pitch;
        cuvidCtxLock(vid_ctx_lock, 0);
        CUresult cuStatus = cuvidMapVideoFrame(dec, cuviddisp->picture_index, &devptr, &pitch, &proc_params);
        if (cuStatus != CUDA_SUCCESS) {
            qWarning("cuvidMapVideoFrame failed on index %d (%#x, %s)", cuviddisp->picture_index, cuStatus, _cudaGetErrorEnum(cuStatus));
            cuvidUnmapVideoFrame(dec, devptr);
            cuvidCtxUnlock(vid_ctx_lock, 0);
            return false;
        }
#define PAD_ALIGN(x,mask) ( (x + mask) & ~mask )
        //uint w = dec_create_info.ulWidth;//PAD_ALIGN(dec_create_info.ulWidth, 0x3F);
        uint h = dec_create_info.ulHeight;//PAD_ALIGN(dec_create_info.ulHeight, 0x0F); //?
#undef PAD_ALIGN
        int size = pitch*h*3/2;
        if (size > host_data_size && host_data) {
            cuMemFreeHost(host_data);
            host_data = 0;
            host_data_size = 0;
        }
        if (!host_data) {
            cuStatus = cuMemAllocHost((void**)&host_data, size);
            if (cuStatus != CUDA_SUCCESS) {
                qWarning("cuMemAllocHost failed (%#x, %s)", cuStatus, _cudaGetErrorEnum(cuStatus));
                cuvidUnmapVideoFrame(dec, devptr);
                cuvidCtxUnlock(vid_ctx_lock, 0);
                return false;
            }
            host_data_size = size;
        }
        if (!host_data) {
            qWarning("No valid staging memory!");
            cuvidUnmapVideoFrame(dec, devptr);
            cuvidCtxUnlock(vid_ctx_lock, 0);
            return false;
        }
        cuStatus = cuMemcpyDtoHAsync(host_data, devptr, size, stream);
        if (cuStatus != CUDA_SUCCESS) {
            qWarning("cuMemcpyDtoHAsync failed (%#x, %s)", cuStatus, _cudaGetErrorEnum(cuStatus));
            cuvidUnmapVideoFrame(dec, devptr);
            cuvidCtxUnlock(vid_ctx_lock, 0);
            return false;
        }
        cuStatus = cuCtxSynchronize();
        if (cuStatus != CUDA_SUCCESS) {
            qWarning("cuCtxSynchronize failed (%#x, %s)", cuStatus, _cudaGetErrorEnum(cuStatus));
        }
        cuvidUnmapVideoFrame(dec, devptr);
        cuvidCtxUnlock(vid_ctx_lock, 0);
        //qDebug("mark not in use pic_index: %d", cuviddisp->picture_index);
        surface_in_use[cuviddisp->picture_index] = false;

        uchar *planes[] = {
            host_data,
            host_data + pitch * h
        };
        int pitches[] = { (int)pitch, (int)pitch };
        VideoFrame frame(codec_ctx->width, codec_ctx->height, VideoFormat::Format_NV12);
        frame.setBits(planes);
        frame.setBytesPerLine(pitches);
        frame.setTimestamp((double)cuviddisp->timestamp/1000.0);
        //TODO: is clone required? may crash on clone, I should review clone()
        //frame = frame.clone();
        if (outFrame) {
            *outFrame = frame.clone();
        }
#if COPY_ON_DECODE
        frame_queue.put(frame.clone());
#endif
        //qDebug("frame queue size: %d", frame_queue.size());
    }
    return true;
}

} //namespace QtAV

#include "VideoDecoderCUDA.moc"
