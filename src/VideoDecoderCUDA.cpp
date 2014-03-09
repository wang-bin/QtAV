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
#include <QtCore/QQueue>

/*
 * TODO: update helper_cuda with 5.5
 * avc1, ccv1 => h264 + sps, pps, nal. use filter or lavcudiv
 * flush api
 * CUDA_ERROR_INVALID_VALUE "cuvidDecodePicture(p->dec, cuvidpic)"
 */

//#define CUDA_FORCE_API_VERSION 3010
#include "cuda/helper_cuda.h"

//decode error if not floating context

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
    AutoCtxLock(CUvideoctxlock lck) { m_lock=lck; cuvidCtxLock(m_lock, 0); }
    ~AutoCtxLock() { cuvidCtxUnlock(m_lock, 0); }
};

class VideoDecoderCUDAPrivate : public VideoDecoderPrivate
{
public:
    VideoDecoderCUDAPrivate():
        VideoDecoderPrivate()
      , host_data(0)
      , host_data_size(0)
    {
        available = false;
        cuctx = 0;
        cudev = 0;
        dec = 0;
        vid_ctx_lock = 0;
        parser = 0;
        force_sequence_update = false;
        frame_queue.setCapacity(20);
        frame_queue.setThreshold(10);
        initCuda();
    }
    ~VideoDecoderCUDAPrivate() {
        releaseCuda();
    }
    bool initCuda() {
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
            AutoCtxLock lock(vid_ctx_lock);
            Q_UNUSED(lock);
            checkCudaErrors(cuStreamCreate(&stream, CU_STREAM_NON_BLOCKING)); //CU_STREAM_DEFAULT
            //require compute capability >= 1.1
            //flag: Reserved for future use, must be 0
            //cuStreamAddCallback(stream, CUstreamCallback, this, 0);
        }
        return true;
    }
    bool releaseCuda() {
        cuvidDestroyDecoder(dec);
        if (stream) {
            cuStreamDestroy(stream);
            stream = 0;
        }
        cuvidCtxLockDestroy(vid_ctx_lock);
        if (cuctx) {
            checkCudaErrors(cuCtxDestroy(cuctx));
        }
        return true;
    }
    bool createCUVIDDecoder(cudaVideoCodec cudaCodec, int w, int h) {
        if (cudaCodec == -1) {
            return false;
        }
        AutoCtxLock lock(vid_ctx_lock);
        Q_UNUSED(lock);
        if (dec) {
            checkCudaErrors(cuvidDestroyDecoder(dec));
        }
        memset(&dec_create_info, 0, sizeof(CUVIDDECODECREATEINFO));
        dec_create_info.ulWidth = w;
        dec_create_info.ulHeight = h;
        dec_create_info.ulNumDecodeSurfaces = frame_queue.capacity(); //? it's 20 in example
        dec_create_info.CodecType = cudaCodec;
        dec_create_info.ChromaFormat = cudaVideoChromaFormat_420;  // cudaVideoChromaFormat_XXX (only 4:2:0 is currently supported)
        //cudaVideoCreate_PreferCUVID is slow in example. DXVA may failed to create (CUDA_ERROR_NO_DEVICE)
        // what's the difference between CUDA and CUVID?
        dec_create_info.ulCreationFlags = cudaVideoCreate_PreferCUVID; //cudaVideoCreate_Default, cudaVideoCreate_PreferCUDA, cudaVideoCreate_PreferCUVID, cudaVideoCreate_PreferDXVA
        // TODO: lav yv12
        dec_create_info.OutputFormat = cudaVideoSurfaceFormat_NV12; // NV12 (currently the only supported output format)
        dec_create_info.DeinterlaceMode = cudaVideoDeinterlaceMode_Adaptive;// Weave: No deinterlacing
        //cudaVideoDeinterlaceMode_Adaptive;
        // No scaling
        dec_create_info.ulTargetWidth = dec_create_info.ulWidth;
        dec_create_info.ulTargetHeight = dec_create_info.ulHeight;
        dec_create_info.ulNumOutputSurfaces = 2;  // We won't simultaneously map more than 8 surfaces
        dec_create_info.vidLock = vid_ctx_lock;//vidCtxLock; //FIXME

        // Limit decode memory to 24MB (16M pixels at 4:2:0 = 24M bytes)
        while (dec_create_info.ulNumDecodeSurfaces * codec_ctx->coded_width * codec_ctx->coded_height > 16*1024*1024) {
            dec_create_info.ulNumDecodeSurfaces--;
        }
        qDebug("ulNumDecodeSurfaces: %d", dec_create_info.ulNumDecodeSurfaces);

        // create the decoder
        available = false;
        checkCudaErrors(cuvidCreateDecoder(&dec, &dec_create_info));
        available = true;
        return true;
    }
    bool createCUVIDParser() {
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
        return true;
    }
    bool processDecodedData(CUVIDPARSERDISPINFO *cuviddisp) {
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
                qWarning("cuvidMapVideoFrame failed on index %d (%p, %s)", cuviddisp->picture_index, cuStatus, _cudaGetErrorEnum(cuStatus));
                cuvidUnmapVideoFrame(dec, devptr);
                cuvidCtxUnlock(vid_ctx_lock, 0);
                return false;
            }
    #define PAD_ALIGN(x,mask) ( (x + mask) & ~mask )
            uint w = dec_create_info.ulWidth;//PAD_ALIGN(dec_create_info.ulWidth, 0x3F);
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
                    qWarning("cuMemAllocHost failed (%p, %s)", cuStatus, _cudaGetErrorEnum(cuStatus));
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
                qWarning("cuMemcpyDtoH failed (%p, %s)", cuStatus, _cudaGetErrorEnum(cuStatus));
                cuvidUnmapVideoFrame(dec, devptr);
                cuvidCtxUnlock(vid_ctx_lock, 0);
                return false;
            }
            cuStatus = cuCtxSynchronize();
            if (cuStatus != CUDA_SUCCESS) {
                qWarning("cuMemcpyDtoH failed (%p, %s)", cuStatus, _cudaGetErrorEnum(cuStatus));
            }
            cuvidUnmapVideoFrame(dec, devptr);
            cuvidCtxUnlock(vid_ctx_lock, 0);

            uchar *planes[] = {
                host_data,
                host_data + pitch * h
            };
            int pitches[] = { pitch, pitch };
            VideoFrame frame(w, h, VideoFormat::Format_NV12);
            frame.setBits(planes);
            frame.setBytesPerLine(pitches);
            //TODO: is clone required? may crash on clone, I should review clone()
            //frame = frame.clone();
            frame_queue.put(frame);
            qDebug("frame queue size: %d", frame_queue.size());
        }
        return true;
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
        }
        //TODO: lavfilter
        return 1;
    }
    static int CUDAAPI HandlePictureDecode(void *obj, CUVIDPICPARAMS *cuvidpic) {
        VideoDecoderCUDAPrivate *p = reinterpret_cast<VideoDecoderCUDAPrivate*>(obj);
        qDebug("%s @%d tid=%p dec=%p", __FUNCTION__, __LINE__, QThread::currentThread(), p->dec);
        AutoCtxLock lock(p->vid_ctx_lock);
        Q_UNUSED(lock);
        checkCudaErrors(cuvidDecodePicture(p->dec, cuvidpic));
        return true;
    }
    static int CUDAAPI HandlePictureDisplay(void *obj, CUVIDPARSERDISPINFO *cuviddisp) {
        VideoDecoderCUDAPrivate *p = reinterpret_cast<VideoDecoderCUDAPrivate*>(obj);
        qDebug("%s @%d tid=%p dec=%p", __FUNCTION__, __LINE__, QThread::currentThread(), p->dec);
        return p->processDecodedData(cuviddisp);
    }

    uchar *host_data;
    int host_data_size;
    CUcontext cuctx;
    CUdevice cudev;

    cudaVideoCreateFlags create_flags;
    CUvideodecoder dec;
    CUVIDDECODECREATEINFO dec_create_info;
    CUvideoctxlock vid_ctx_lock; //NULL
    CUVIDPICPARAMS pic_params;
    CUvideoparser parser;
    CUstream stream;
    bool force_sequence_update;
    /*
     * callbacks are in the same thread as cuvidParseVideoData. so video thread may be blocked
     * so create another thread?
     */
    BlockingQueue<VideoFrame> frame_queue;
    QString description;
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
    if (!d.parser) {
        qWarning("CUVID parser not ready");
        return false;
    }
    CUVIDSOURCEDATAPACKET cuvid_pkt;
    memset(&cuvid_pkt, 0, sizeof(CUVIDSOURCEDATAPACKET));
    cuvid_pkt.payload = (unsigned char *)encoded.constData();
    cuvid_pkt.payload_size = encoded.size();
    cuvid_pkt.flags = CUVID_PKT_TIMESTAMP;
    cuvid_pkt.timestamp = 0;// ?
    //TODO: fill NALU header for h264? https://devtalk.nvidia.com/default/topic/515571/what-the-data-format-34-cuvidparsevideodata-34-can-accept-/
    {
        //cuvidCtxUnlock(d.vid_ctx_lock, 0); //TODO: why wrong context?
        CUresult cuStatus = cuvidParseVideoData(d.parser, &cuvid_pkt);
        if (cuStatus != CUDA_SUCCESS) {
            qWarning("cuMemcpyDtoH failed (%p, %s)", cuStatus, _cudaGetErrorEnum(cuStatus));
        }
    }
    // callbacks are in the same thread as this. so no queue is required?
    qDebug("frame queue size on decode: %d", d.frame_queue.size());
    return !d.frame_queue.isEmpty();

}

VideoFrame VideoDecoderCUDA::frame()
{
    return d_func().frame_queue.take();
}

} //namespace QtAV
