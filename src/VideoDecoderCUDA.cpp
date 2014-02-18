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

#include <QtAV/VideoDecoder.h>
#include "private/VideoDecoder_p.h"
#include <QtAV/QtAV_Compat.h>
#include <QtAV/BlockingQueue.h>
#include "prepost.h"

#include "cuda/helper_cuda.h"

#define USE_FLOATING_CONTEXTS 1

static inline cudaVideoCodec mapCodecFromFFmpeg(AVCodecID codec)
{
    static struct {
        AVCodecID ffcodec;
        cudaVideoCodec cudaCodec;
    } ff_cuda_codecs[] = {
        { CODEC_ID_MPEG1VIDEO, cudaVideoCodec_MPEG1 },
        { CODEC_ID_MPEG2VIDEO, cudaVideoCodec_MPEG2 },
        { CODEC_ID_VC1,        cudaVideoCodec_VC1   },
        { CODEC_ID_H264,       cudaVideoCodec_H264  },
        { CODEC_ID_MPEG4,      cudaVideoCodec_MPEG4 },
        { (AVCodecID)-1, (cudaVideoCodec)-1}
    };
    for (int i = 0; ff_cuda_codecs[i].cudaCodec != -1; ++i) {
        if (ff_cuda_codecs[i].ffcodec == codec) {
            return ff_cuda_codecs[i].cudaCodec;
        }
    }
    return (cudaVideoCodec)-1;
}

namespace QtAV {

class VideoDecoderCUDAPrivate;
class VideoDecoderCUDA : public VideoDecoder
{
    DPTR_DECLARE_PRIVATE(VideoDecoderCUDA)
public:
    VideoDecoderCUDA();
    virtual ~VideoDecoderCUDA();
    virtual bool prepare();
    virtual bool decode(const QByteArray &encoded);
    virtual VideoFrame frame();
};


extern VideoDecoderId VideoDecoderId_CUDA;
FACTORY_REGISTER_ID_AUTO(VideoDecoder, CUDA, "CUDA")

void RegisterVideoDecoderCUDA_Man()
{
    FACTORY_REGISTER_ID_MAN(VideoDecoder, CUDA, "CUDA")
}

class AutoCtxLock
{
private:
    CUvideoctxlock m_lock;
public:
#if USE_FLOATING_CONTEXTS
    AutoCtxLock(CUvideoctxlock lck) { m_lock=lck; cuvidCtxLock(m_lock, 0); }
    ~AutoCtxLock() { cuvidCtxUnlock(m_lock, 0); }
#else
    AutoCtxLock(CUvideoctxlock lck) { m_lock=lck; }
#endif
};

static const unsigned int cnMaximumSize = 20; // MAX_FRM_CNT;
#define MAX_FRAME_COUNT 2
class VideoDecoderCUDAPrivate : public VideoDecoderPrivate
{
public:
    VideoDecoderCUDAPrivate():
        VideoDecoderPrivate()
    {
        qDebug("%s @%d tid=%p", __FUNCTION__, __LINE__, QThread::currentThread());
        available = false;
        cuctx = 0;
        cudev = 0;
        dec = 0;
        vid_ctx_lock = 0;
        parser = 0;
        force_sequence_update = false;
        CUresult result = cuInit(0);
        if (result != CUDA_SUCCESS) {
            available = false;
            qWarning("cuInit(0) faile (%d)", result);
            return;
        }
        cudev = GetMaxGflopsGraphicsDeviceId();
        qDebug("%s @%d dev=%d", __FUNCTION__, __LINE__, cudev);

        initCuda();
        frame_queue.setCapacity(20);
        frame_queue.setThreshold(8);
    }
    ~VideoDecoderCUDAPrivate() {
        releaseCuda();
    }

    int GetMaxGflopsGraphicsDeviceId() {
        CUdevice current_device = 0, max_perf_device = 0;
        int device_count     = 0, sm_per_multiproc = 0;
        int max_compute_perf = 0, best_SM_arch     = 0;
        int major = 0, minor = 0, multiProcessorCount, clockRate;
        int bTCC = 0, version;
        char deviceName[256];

        cuDeviceGetCount(&device_count);
        if (device_count <= 0)
            return -1;

        cuDriverGetVersion(&version);
        // Find the best major SM Architecture GPU device that are graphics devices
        while (current_device < device_count) {
            cuDeviceGetName(deviceName, 256, current_device);
            cuDeviceComputeCapability(&major, &minor, current_device);
            if (version >= 3020) {
                cuDeviceGetAttribute(&bTCC, CU_DEVICE_ATTRIBUTE_TCC_DRIVER, current_device);
            } else {
                // Assume a Tesla GPU is running in TCC if we are running CUDA 3.1
                if (deviceName[0] == 'T')
                    bTCC = 1;
            }
            if (!bTCC) {
                if (major > 0 && major < 9999) {
                    best_SM_arch = std::max(best_SM_arch, major);
                }
            }
            current_device++;
        }
        // Find the best CUDA capable GPU device
        current_device = 0;
        while (current_device < device_count) {
            cuDeviceGetAttribute(&multiProcessorCount, CU_DEVICE_ATTRIBUTE_MULTIPROCESSOR_COUNT, current_device);
            cuDeviceGetAttribute(&clockRate, CU_DEVICE_ATTRIBUTE_CLOCK_RATE, current_device);
            cuDeviceComputeCapability(&major, &minor, current_device);
            if (version >= 3020) {
                cuDeviceGetAttribute(&bTCC, CU_DEVICE_ATTRIBUTE_TCC_DRIVER, current_device);
            } else {
                // Assume a Tesla GPU is running in TCC if we are running CUDA 3.1
                if (deviceName[0] == 'T')
                    bTCC = 1;
            }
            if (major == 9999 && minor == 9999) {
                sm_per_multiproc = 1;
            } else {
                sm_per_multiproc = _ConvertSMVer2Cores(major, minor);
            }
            // If this is a Tesla based GPU and SM 2.0, and TCC is disabled, this is a contendor
            if (!bTCC) {// Is this GPU running the TCC driver?  If so we pass on this
                qDebug("%s @%d", __FUNCTION__, __LINE__);
                int compute_perf = multiProcessorCount * sm_per_multiproc * clockRate;
                qDebug("%s @%d compute_perf=%d max_compute_perf=%d", __FUNCTION__, __LINE__, compute_perf, max_compute_perf);
                if (compute_perf > max_compute_perf) {
                    qDebug("%s @%d", __FUNCTION__, __LINE__);
                    // If we find GPU with SM major > 2, search only these
                    if (best_SM_arch > 2) {
                        qDebug("%s @%d best_SM_arch=%d", __FUNCTION__, __LINE__, best_SM_arch);
                        // If our device = dest_SM_arch, then we pick this one
                        if (major == best_SM_arch) {
                            qDebug("%s @%d", __FUNCTION__, __LINE__);
                            max_compute_perf = compute_perf;
                            max_perf_device = current_device;
                        }
                    } else {
                        qDebug("%s @%d", __FUNCTION__, __LINE__);
                        max_compute_perf = compute_perf;
                        max_perf_device = current_device;
                    }
                }
                cuDeviceGetName(deviceName, 256, current_device);
                qDebug("CUDA Device: %s, Compute: %d.%d, CUDA Cores: %d, Clock: %d MHz", deviceName, major, minor, multiProcessorCount * sm_per_multiproc, clockRate / 1000);
            }
            ++current_device;
        }
        return max_perf_device;
    }

    bool initCuda() {
        qDebug("%s @%d dev=%#x", __FUNCTION__, __LINE__, cudev);
        checkCudaErrors(cuCtxCreate(&cuctx, CU_CTX_SCHED_AUTO, cudev)); //CU_CTX_SCHED_AUTO?
        int major, minor;
        cuDeviceComputeCapability(&major, &minor, cudev);
        qDebug("%s @%d %d %d", __FUNCTION__, __LINE__, major, minor);
#if USE_FLOATING_CONTEXTS
        CUcontext cuCurrent = NULL;
        CUresult result = cuCtxPopCurrent(&cuCurrent);
        if (result != CUDA_SUCCESS) {
            qWarning("cuCtxPopCurrent: %d\n", result);
            return false;
        }
        checkCudaErrors(cuvidCtxLockCreate(&vid_ctx_lock, cuctx));
#endif //USE_FLOATING_CONTEXTS
        return true;
    }
    bool releaseCuda() {
        cuvidDestroyDecoder(dec);
        cuvidCtxLockDestroy(vid_ctx_lock);
        if (cuctx) {
            checkCudaErrors(cuCtxDestroy(cuctx));
        }
        return true;
    }
    bool createCUVIDDecoder(cudaVideoCodec cudaCodec, int w, int h) {
        qDebug("CUDA codec: %d.  %dx%d", cudaCodec, w, h);
        if (cudaCodec == -1) {
            return false;
        }
        AutoCtxLock lock(vid_ctx_lock);
        if (dec) {
            qDebug("%s @%d", __FUNCTION__, __LINE__);
            checkCudaErrors(cuvidDestroyDecoder(dec));
        }
        memset(&dec_create_info, 0, sizeof(CUVIDDECODECREATEINFO));
        dec_create_info.ulWidth = w;
        dec_create_info.ulHeight = h;
        dec_create_info.ulNumDecodeSurfaces = cnMaximumSize; //?
        dec_create_info.CodecType = cudaCodec;
        dec_create_info.ChromaFormat = cudaVideoChromaFormat_420;  // cudaVideoChromaFormat_XXX (only 4:2:0 is currently supported)
        dec_create_info.ulCreationFlags = cudaVideoCreate_PreferCUDA; //cudaVideoCreate_Default, cudaVideoCreate_PreferCUDA, cudaVideoCreate_PreferCUVID, cudaVideoCreate_PreferDXVA
        dec_create_info.OutputFormat = cudaVideoSurfaceFormat_NV12; // NV12 (currently the only supported output format)
        dec_create_info.DeinterlaceMode = cudaVideoDeinterlaceMode_Adaptive;// Weave: No deinterlacing
        //cudaVideoDeinterlaceMode_Adaptive;
        // No scaling
        dec_create_info.ulTargetWidth = dec_create_info.ulWidth;
        dec_create_info.ulTargetHeight = dec_create_info.ulHeight;
        dec_create_info.ulNumOutputSurfaces = MAX_FRAME_COUNT;  // We won't simultaneously map more than 8 surfaces
        dec_create_info.vidLock = vid_ctx_lock;//vidCtxLock; //FIXME

        // Limit decode memory to 24MB (16M pixels at 4:2:0 = 24M bytes)
        while (dec_create_info.ulNumDecodeSurfaces * codec_ctx->coded_width * codec_ctx->coded_height > 16*1024*1024) {
            dec_create_info.ulNumDecodeSurfaces--;
        }
        qDebug("ulNumDecodeSurfaces: %d", dec_create_info.ulNumDecodeSurfaces);

        // create the decoder
        CUresult oResult = cuvidCreateDecoder(&dec, &dec_create_info);
        assert(oResult == CUDA_SUCCESS);
        available = CUDA_SUCCESS == oResult;
        checkCudaErrors(oResult);
        if (!available) {
            qDebug("%s @%d", __FUNCTION__, __LINE__);
            return false;
        }
        return true;
    }
    bool createCUVIDParser() {
        qDebug("%s @%d", __FUNCTION__, __LINE__);
        cudaVideoCodec cudaCodec = mapCodecFromFFmpeg(codec_ctx->codec_id);
        if (cudaCodec == -1) {
            qWarning("CUVID does not support the codec");
            available = false;
            return false;
        }
        //lavfilter check level C
        CUVIDPARSERPARAMS parser_params;
        memset(&parser_params, 0, sizeof(CUVIDPARSERPARAMS));
        parser_params.CodecType = cudaCodec;
        parser_params.ulMaxNumDecodeSurfaces = 20; //?
        //parser_params.ulMaxDisplayDelay = 4; //?
        parser_params.pUserData = this;
        parser_params.pfnSequenceCallback = VideoDecoderCUDAPrivate::HandleVideoSequence;
        parser_params.pfnDecodePicture = VideoDecoderCUDAPrivate::HandlePictureDecode;
        parser_params.pfnDisplayPicture = VideoDecoderCUDAPrivate::HandlePictureDisplay;
        parser_params.ulErrorThreshold = 0;//!wait for key frame
        //parser_params.pExtVideoInfo

        checkCudaErrors(cuvidCreateVideoParser(&parser, &parser_params));
        //lavfilter: cuStreamCreate
        force_sequence_update = true;
        //DecodeSequenceData()
        qDebug("%s @%d tid=%p", __FUNCTION__, __LINE__, QThread::currentThread());
        return true;
    }
    bool processDecodedData(CUVIDPARSERDISPINFO *cuviddisp) {
        qDebug("%s @%d index=%d progressive=%d, repeat_first_field=%p tid=%p"
               , __FUNCTION__, __LINE__, cuviddisp->picture_index
               , cuviddisp->progressive_frame
               , cuviddisp->repeat_first_field
               , QThread::currentThread());
        // TODO: lock context
        CUdeviceptr devptr;
        unsigned int pitch;
        memset(&proc_params, 0, sizeof(CUVIDPROCPARAMS));
        proc_params.progressive_frame = cuviddisp->progressive_frame; //?
        proc_params.second_field = 0; //?
        proc_params.top_field_first = cuviddisp->top_field_first;
        proc_params.unpaired_field = cuviddisp->progressive_frame == 1;

        checkCudaErrors(cuvidMapVideoFrame(dec, cuviddisp->picture_index, &devptr, &pitch, &proc_params));
#define PAD_ALIGN(x,mask) ( (x + mask) & ~mask )
        uint w  = PAD_ALIGN(dec_create_info.ulWidth , 0x3F);
        uint h = PAD_ALIGN(dec_create_info.ulHeight, 0x0F); //?
#undef PAD_ALIGN
        //check size
        uchar *frame_data = 0;
        checkCudaErrors(cuMemAllocHost((void**)&frame_data, pitch*h*3/2));
        checkCudaErrors(cuMemcpyDtoH(frame_data, devptr, pitch*h*3/2));
        uchar *planes[] = {
            frame_data,
            frame_data + pitch * h
        };
        int pitches[] = { pitch, pitch };
        VideoFrame frame(w, h, VideoFormat::Format_NV12);
        frame.setBits(planes);
        frame.setBytesPerLine(pitches);
        frame = frame.clone();
        frame_queue.put(frame);
        return true;
    }

    // cuvid parser callbacks
    static int CUDAAPI HandleVideoSequence(void *obj, CUVIDEOFORMAT *cuvidfmt) {
        qDebug("%s @%d tid=%p", __FUNCTION__, __LINE__, QThread::currentThread());
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
        }
        //TODO: lavfilter
        return 1;
    }
    static int CUDAAPI HandlePictureDecode(void *obj, CUVIDPICPARAMS *cuvidpic) {
        VideoDecoderCUDAPrivate *p = reinterpret_cast<VideoDecoderCUDAPrivate*>(obj);
        qDebug("%s @%d tid=%p dec=%p", __FUNCTION__, __LINE__, QThread::currentThread(), p->dec);
        //TODO: lavfilter lock ctx
        checkCudaErrors(cuvidDecodePicture(p->dec, cuvidpic));
        return true;
    }
    static int CUDAAPI HandlePictureDisplay(void *obj, CUVIDPARSERDISPINFO *cuviddisp) {
        qDebug("%s @%d tid=%p", __FUNCTION__, __LINE__, QThread::currentThread());
        VideoDecoderCUDAPrivate *p = reinterpret_cast<VideoDecoderCUDAPrivate*>(obj);
        return p->processDecodedData(cuviddisp);
    }

    CUcontext cuctx;
    CUdevice cudev;

    cudaVideoCreateFlags create_flags;
    CUvideodecoder dec;
    CUVIDDECODECREATEINFO dec_create_info;
    CUvideoctxlock vid_ctx_lock; //NULL
    CUVIDPICPARAMS pic_params;
    CUVIDPROCPARAMS proc_params;

    CUvideoparser parser;
    bool force_sequence_update;
    BlockingQueue<VideoFrame> frame_queue;
};

VideoDecoderCUDA::VideoDecoderCUDA():
    VideoDecoder(*new VideoDecoderCUDAPrivate())
{
}

VideoDecoderCUDA::~VideoDecoderCUDA()
{
}

bool VideoDecoderCUDA::prepare()
{
    //TODO: destroy decoder
    DPTR_D(VideoDecoderCUDA);
    if (!d.codec_ctx) {
        qWarning("AVCodecContext not ready");
        return false;
    }
    return d.createCUVIDParser() && d.createCUVIDDecoder(mapCodecFromFFmpeg(d.codec_ctx->codec_id), d.codec_ctx->coded_width, d.codec_ctx->coded_height);
}

bool VideoDecoderCUDA::decode(const QByteArray &encoded)
{
    if (!isAvailable())
        return false;
    DPTR_D(VideoDecoderCUDA);
#if 0
    // fill parameter: https://devtalk.nvidia.com/default/topic/524154/?comment=3713188
    memset(&d.pic_params, 0, sizeof(CUVIDPICPARAMS));
    d.pic_params.nBitstreamDataLen = encoded.size();
    d.pic_params.pBitstreamData = (const unsigned char*)(encoded.data());
    //d.pic_params.intra_pic_flag = 1; //key frame
    //d.pic_params.PicWidthInMbs = (d.codec_ctx->width + 15)/16;
    //d.pic_params.FrameHeightInMbs = (d.codec_ctx->height + 15)/16;
    //d.pic_params.ref_pic_flag = 1;
    checkCudaErrors(cuvidDecodePicture(d.dec, &d.pic_params));


    CUdeviceptr dev;
    unsigned int pitch;
    //cuvidMapVideoFrame64
    memset(&d.proc_params, 0, sizeof(CUVIDPROCPARAMS));
    checkCudaErrors(cuvidMapVideoFrame(d.dec, d.pic_params.CurrPicIdx, &dev, &pitch, &d.proc_params));

#define PAD_ALIGN(x,mask) ( (x + mask) & ~mask )
    //uint w  = PAD_ALIGN(d.dec_create_info.ulWidth , 0x3F);
    uint h = PAD_ALIGN(d.dec_create_info.ulHeight, 0x0F); //?
#undef PAD_ALIGN
    void *dst = d.decoded.data();
    checkCudaErrors(cuMemAllocHost(&dst, pitch*h*3/2));
    checkCudaErrors(cuMemcpyDtoH(d.decoded.data(), dev, pitch*h*3/2));
#else
    if (!d.parser) {
        qWarning("CUVID parser not ready");
        return false;
    }
    CUVIDSOURCEDATAPACKET cuvid_pkt;
    memset(&cuvid_pkt, 0, sizeof(CUVIDSOURCEDATAPACKET));
    cuvid_pkt.payload = (unsigned char *)encoded.data();
    cuvid_pkt.payload_size = encoded.size();
    cuvid_pkt.flags = CUVID_PKT_TIMESTAMP;
    cuvid_pkt.timestamp = 0;// ?
    qDebug("%s @%d tid=%p datalen=%d", __FUNCTION__, __LINE__, QThread::currentThread(), encoded.size());
    //TODO: fill NALU header for h264? https://devtalk.nvidia.com/default/topic/515571/what-the-data-format-34-cuvidparsevideodata-34-can-accept-/
    cuvidParseVideoData(d.parser, &cuvid_pkt);
    // callbacks are in the same thread as this. so no queue is required?
#endif //0
    return !d.frame_queue.isEmpty();

}

VideoFrame VideoDecoderCUDA::frame()
{
    return d_func().frame_queue.take();
}

} //namespace QtAV
