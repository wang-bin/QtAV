/******************************************************************************
    QtAV:  Media play library based on Qt and FFmpeg
    Copyright (C) 2013-2015 Wang Bin <wbsecg1@gmail.com>
    Miroslav Bendik <miroslav.bendik@gmail.com>

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
#include "QtAV/private/AVDecoder_p.h"
#include "QtAV/Packet.h"
#include "QtAV/private/AVCompat.h"
#include "QtAV/private/factory.h"
#include <libavcodec/avcodec.h>
#define SUPPORT_FFMPEG_DEMUX 1
extern "C" {
#include "coda/vpuapi/vpuapi.h"
#include "coda/vpuapi/regdefine.h"
#include "coda/include/mixer.h"
#include "coda/include/vpuio.h"
#include "coda/include/vpuhelper.h"
#include "coda/vpuapi/vpuapifunc.h"
}
#include "utils/Logger.h"

//avformat ctx: flag CODEC_FLAG_TRUNCATED


//#define ENC_SOURCE_FRAME_DISPLAY
#define ENC_RECON_FRAME_DISPLAY
#define VPU_ENC_TIMEOUT       5000
#define VPU_DEC_TIMEOUT       5000
#define VPU_WAIT_TIME_OUT	10		//should be less than normal decoding time to give a chance to fill stream. if this value happens some problem. we should fix VPU_WaitInterrupt function
#define PARALLEL_VPU_WAIT_TIME_OUT 1 	//the value of timeout is 1 means we want to keep a waiting time to give a chance of an interrupt of the next core.
//#define PARALLEL_VPU_WAIT_TIME_OUT 0 	//the value of timeout is 0 means we just check interrupt flag. do not wait any time to give a chance of an interrupt of the next core.
#if PARALLEL_VPU_WAIT_TIME_OUT > 0
#undef VPU_DEC_TIMEOUT
#define VPU_DEC_TIMEOUT       1000
#endif


#define MAX_CHUNK_HEADER_SIZE 1024
#define MAX_DYNAMIC_BUFCOUNT	3
#define NUM_FRAME_BUF			19
#define MAX_ROT_BUF_NUM			2
#define EXTRA_FRAME_BUFFER_NUM	1

#define ENC_SRC_BUF_NUM			2
#define STREAM_BUF_SIZE		 0x300000  // max bitstream size

//#define STREAM_FILL_SIZE    (512 * 16)  //  4 * 1024 | 512 | 512+256( wrap around test )
#define STREAM_FILL_SIZE    0x2000  //  4 * 1024 | 512 | 512+256( wrap around test )

#define STREAM_END_SIZE			0
#define STREAM_END_SET_FLAG		0
#define STREAM_END_CLEAR_FLAG	-1
#define STREAM_READ_SIZE    (512 * 16)

#define FORCE_SET_VSYNC_FLAG
//#define TEST_USER_FRAME_BUFFER

#ifdef TEST_USER_FRAME_BUFFER
#define TEST_MULTIPLE_CALL_REGISTER_FRAME_BUFFER
#endif


#define VPU_ENSURE(x, ...) \
    do { \
        RetCode ret = x; \
        if (ret != RETCODE_SUCCESS) { \
            qWarning("Coda VPU error@%d. " #x ": %#x", __LINE__, ret,); \
            return __VA_ARGS__; \
        } \
    } while(0)
#define VPU_WARN(a) \
do { \
  RetCode res = a; \
  if(res != RETCODE_SUCCESS) \
    qWarning("Coda VPU error@%d. " #x ": %#x", __LINE__, ret,); \
} while(0);

namespace QtAV {

class VideoDecoderVPUPrivate;
class VideoDecoderVPU : public VideoDecoder
{
    Q_OBJECT
    DPTR_DECLARE_PRIVATE(VideoDecoderVPU)
public:
    VideoDecoderVPU();
    VideoDecoderId id() const Q_DECL_OVERRIDE;
    QString description() const Q_DECL_OVERRIDE {
        return QStringLiteral("Coda VPU");
    }
    bool decode(const Packet& packet) Q_DECL_OVERRIDE;
    void flush() Q_DECL_OVERRIDE;
    VideoFrame frame() Q_DECL_OVERRIDE;
};
extern VideoDecoderId VideoDecoderId_VPU;
FACTORY_REGISTER(VideoDecoder, VPU, "VPU")

class VideoDecoderVPUPrivate Q_DECL_FINAL: public VideoDecoderPrivate
{
public:
    VideoDecoderVPUPrivate()
        : VideoDecoderPrivate()
    {}
    ~VideoDecoderVPUPrivate() {
    }
    bool open()  Q_DECL_OVERRIDE;
    bool flush();
    int insIdx, coreIdx;
    int map_type;
    vpu_buffer_t vbStream;
    DecHandle handle;
    DecOpenParam decOP;
    DecOutputInfo outputInfo;
};

VideoDecoderVPU::VideoDecoderVPU()
    : VideoDecoder(*new VideoDecoderVPUPrivate())
{
}

VideoDecoderId VideoDecoderVPU::id() const
{
    return VideoDecoderId_VPU;
}

bool VideoDecoderVPUPrivate::open()
{
    memset(&outputInfo, 0, sizeof(outputInfo));
    int seqHeaderSize = 0;
    BYTE *seqHeader = osal_malloc(codec_ctx->extradata_size + MAX_CHUNK_HEADER_SIZE);	// allocate more buffer to fill the vpu specific header.
    if (!seqHeader) {
        qWarning("fail to allocate the seqHeader buffer");
        goto ERR_DEC_INIT;
    }
    memset(seqHeader, 0x00, codec_ctx->extradata_size+MAX_CHUNK_HEADER_SIZE);

    BYTE *picHeader = osal_malloc(MAX_CHUNK_HEADER_SIZE);
    if (!picHeader) {
        qWarning("fail to allocate the picHeader buffer\n");
        goto ERR_DEC_INIT;
    }
    memset(picHeader, 0x00, MAX_CHUNK_HEADER_SIZE);

    RetCode ret = VPU_Init(coreIdx);
#ifndef BIT_CODE_FILE_PATH
#endif
    if (ret != RETCODE_SUCCESS &&
        ret != RETCODE_CALLED_BEFORE) {
        qWarning("VPU_Init failed Error code is 0x%x \n", ret );
        goto ERR_DEC_INIT;
    }

    CheckVersion(coreIdx);

    memset(&decOP, 0, sizeof(decOP));
    decOP.bitstreamFormat = fourCCToCodStd(codec_ctx->codec_tag);
    if (decOP.bitstreamFormat == -1)
        decOP.bitstreamFormat = codecIdToCodStd(codec_ctx->codec_id);

    if (decOP.bitstreamFormat == -1) {
        qWarning("can not support video format in VPU tag=%c%c%c%c, codec_id=0x%x \n", ctxVideo->codec_tag>>0, ctxVideo->codec_tag>>8, ctxVideo->codec_tag>>16, ctxVideo->codec_tag>>24, ctxVideo->codec_id );
        goto ERR_DEC_INIT;
    }

    memset(&vbStream, 0, sizeof(vbStream));
    vbStream.size = STREAM_BUF_SIZE; //STREAM_BUF_SIZE;
    vbStream.size = ((vbStream.size+1023)&~1023);
    if (vdi_allocate_dma_memory(coreIdx, &vbStream) < 0) {
        qWarning("fail to allocate bitstream buffer\n" );
        goto ERR_DEC_INIT;
    }

    decOP.bitstreamBuffer = vbStream.phys_addr;
    decOP.bitstreamBufferSize = vbStream.size;
    decOP.mp4DeblkEnable = 0;

    decOP.mp4Class = fourCCToMp4Class(ctxVideo->codec_tag);
    if (decOP.mp4Class == -1)
        decOP.mp4Class = codecIdToMp4Class(ctxVideo->codec_id);

    if(decOP.bitstreamFormat == STD_THO || decOP.bitstreamFormat == STD_VP3) {
    }

    decOP.tiled2LinearEnable = (map_type>>4)&0x1;
    int mapType = map_type & 0xf;
    if (mapType) {
        decOP.wtlEnable = decConfig.wtlEnable; //TODO:
        if (decOP.wtlEnable) {
            //decConfig.rotAngle;
            //decConfig.mirDir;
            //decConfig.useRot = 0;
            //decConfig.useDering = 0;
            decOP.mp4DeblkEnable = 0;
            decOP.tiled2LinearEnable = 0;
        }
    }

    decOP.cbcrInterleave = CBCR_INTERLEAVE;
    if (mapType == TILED_FRAME_MB_RASTER_MAP ||
        mapType == TILED_FIELD_MB_RASTER_MAP) {
            decOP.cbcrInterleave = 1;
    }
    decOP.bwbEnable = VPU_ENABLE_BWB;
    decOP.frameEndian  = VPU_FRAME_ENDIAN;
    decOP.streamEndian = VPU_STREAM_ENDIAN;
    decOP.bitstreamMode = decConfig.bitstreamMode;

    int ppuEnable = (decConfig.useRot || decConfig.useDering || decOP.tiled2LinearEnable); //TODO:

    memset(&handle, 0, sizeof(handle));
    VPU_ENSURE(VPU_decOPen(&handle, &decOP), false); //TODO: goto ERR_DEC_OPEN

    DRAMConfig dramCfg = {0};
    VPU_ENSURE(VPU_DecGiveCommand(handle, GET_DRAM_CONFIG, &dramCfg), false); //TODO: goto ERR_DEC_OPEN
    qDebug("Dec Start : Press enter key to show menu.");
    qDebug("          : Press space key to stop.");

    int seqInited = 0;
    int seqFilled = 0;
    int bsfillSize = 0;
    int reUseChunk = 0;
    display_queue = frame_queue_init(MAX_REG_FRAME);
    init_VSYNC_flag();

    return true;
}

bool VideoDecoderVPUPrivate::flush()
{
    VPU_DecUpdateBitstreamBuffer(handle, STREAM_END_SIZE);	//tell VPU to reach the end of stream. starting flush decoded output in VPU

}

bool VideoDecoderVPU::decode(const Packet &packet)
{
    DPTR_D(VideoDecoderVPU);
    if (packet.data.isEmpty())
        return true;
    QByteArray chunkData(packet.data);
    DecOpenParam &decOP = d.decOP;
    if (decOP.bitstreamMode == BS_MODE_PIC_END) {
        if (reUseChunk) {
            reUseChunk = 0;
            goto FLUSH_BUFFER;
        }
        VPU_DecSetRdPtr(handle, decOP.bitstreamBuffer, 1);
    }
    if (packet.isEOF()) {
        if (!d.flush()) {
            qDebug("Error decode EOS"); // when?
            return false;
        }
        return true;
    }

    if (!seqInited && !seqFilled) {
        seqHeaderSize = BuildSeqHeader(seqHeader, decOP.bitstreamFormat, ic->streams[idxVideo]);	// make sequence data as reference file header to support VPU decoder.
        if (headerSize < 0) // indicate the stream dose not support in VPU.
        {
            DEBUG(DEB_LEV_ERR, "BuildOmxSeqHeader the stream does not support in VPU \n");
            goto ERR_DEC;
        }
        // TODO: omx always WriteBsBufFromBufHelper
        switch(decOP.bitstreamFormat) {
        case STD_THO:
        case STD_VP3:
            break;
        default: {
                size = WriteBsBufFromBufHelper(d.coreIdx, d.handle, &d.vbStream, seqHeader, seqHeaderSize, decOP.streamEndian);
                if (size < 0) {
                    qWarning("WriteBsBufFromBufHelper failed Error code is 0x%x \n", size );
                    goto ERR_DEC_OPEN; ////
                }

                bsfillSize += size;
            }
            break;
        }
        seqFilled = 1;
    }


    // Build and Fill picture Header data which is dedicated for VPU
    picHeaderSize = BuildPicHeader(picHeader, decOP.bitstreamFormat, ic->streams[idxVideo], pkt);
    switch(decOP.bitstreamFormat) {
    case STD_THO:
    case STD_VP3:
        break;
    default:
        size = WriteBsBufFromBufHelper(coreIdx, handle, &vbStream, picHeader, picHeaderSize, decOP.streamEndian);
        if (size < 0) {
            VLOG(ERR, "WriteBsBufFromBufHelper failed Error code is 0x%x \n", size );
            goto ERR_DEC_OPEN;
        }
        bsfillSize += size;
        break;
    }
    // Fill VCL data
    switch(decOP.bitstreamFormat) {
    case STD_VP3:
    case STD_THO:
        break;
    default: {
        if (decOP.bitstreamFormat == STD_RV) {
            chunkData.remove(0, 1+(cSlice*8));
        }
        size = WriteBsBufFromBufHelper(d.coreIdx, d.handle, &d.vbStream, chunkData.constData(), chunkData.size(), decOP.streamEndian);
        if (size <0) {
            qWarning("WriteBsBufFromBufHelper failed Error code is 0x%x", size);
            goto ERR_DEC_OPEN;
        }
        bsfillSize += size;
    }
    break;
    }
    chunkIdx++;

    if (!seqInited) {
        ConfigSeqReport(coreIdx, handle, decOP.bitstreamFormat); // TODO: remove
        if (decOP.bitstreamMode == BS_MODE_PIC_END) {
            ret = VPU_DecGetInitialInfo(handle, &initialInfo);
            if (ret != RETCODE_SUCCESS) {
                if (ret == RETCODE_MEMORY_ACCESS_VIOLATION)
                    PrintMemoryAccessViolationReason(coreIdx, NULL);
                qWarning("VPU_DecGetInitialInfo failed Error code is %#x", ret);
                goto ERR_DEC_OPEN;
            }
            VPU_ClearInterrupt(coreIdx);
        } else {
            // d.int_reason
            if ((int_reason & (1<<INT_BIT_BIT_BUF_EMPTY)) != (1<<INT_BIT_BIT_BUF_EMPTY)) {
                VPU_ENSURE(VPU_DecIssueSeqInit(handle), false);
            } else {
                // After VPU generate the BIT_EMPTY interrupt. HOST should feed the bitstream up to 1024 in case of seq_init
                if (bsfillSize < VPU_GBU_SIZE*2)
                    return true;
            }
            int kbhitRet = 0;
            while((kbhitRet = osal_kbhit()) == 0) {
                int_reason = VPU_WaitInterrupt(d.coreIdx, VPU_DEC_TIMEOUT);

                if (int_reason)
                    VPU_ClearInterrupt(coreIdx);
                if(int_reason & (1<<INT_BIT_BIT_BUF_EMPTY))
                    break;

                CheckUserDataInterrupt(d.coreIdx, d.handle, 1, decOP.bitstreamFormat, int_reason);
                if (int_reason && (int_reason & (1<<INT_BIT_SEQ_INIT))) {
                    seqInited = 1;
                    break;
                }
            }
            if (int_reason & (1<<INT_BIT_BIT_BUF_EMPTY) || int_reason == -1) {
                bsfillSize = 0;
                return true; // go to take next chunk.
            }
            if (seqInited) {
                ret = VPU_DecCompleteSeqInit(handle, &initialInfo);
                if (ret != RETCODE_SUCCESS) {
                    if (ret == RETCODE_MEMORY_ACCESS_VIOLATION)
                        PrintMemoryAccessViolationReason(coreIdx, NULL);
                    if (initialInfo.seqInitErrReason & (1<<31)) // this case happened only ROLLBACK mode
                        VLOG(ERR, "Not enough header : Parser has to feed right size of a sequence header  \n");
                    VLOG(ERR, "VPU_DecCompleteSeqInit failed Error code is 0x%x \n", ret );
                    goto ERR_DEC_OPEN;
                }
            } else {
                VLOG(ERR, "VPU_DecGetInitialInfo failed Error code is 0x%x \n", ret);
                goto ERR_DEC_OPEN;
            }
        }
        SaveSeqReport(coreIdx, handle, &initialInfo, decOP.bitstreamFormat);
        if (decOP.bitstreamFormat == STD_VP8) {
            // For VP8 frame upsampling infomration
            static const int scale_factor_mul[4] = {1, 5, 5, 2};
            static const int scale_factor_div[4] = {1, 4, 3, 1};
            hScaleFactor = initialInfo.vp8ScaleInfo.hScaleFactor;
            vScaleFactor = initialInfo.vp8ScaleInfo.vScaleFactor;
            scaledWidth = initialInfo.picWidth * scale_factor_mul[hScaleFactor] / scale_factor_div[hScaleFactor];
            scaledHeight = initialInfo.picHeight * scale_factor_mul[vScaleFactor] / scale_factor_div[vScaleFactor];
            framebufWidth = FFALIGN(scaledWidth, 16);
            if (IsSupportInterlaceMode(decOP.bitstreamFormat, &initialInfo))
                framebufHeight = FFALIGN(scaledHeight, 32); // framebufheight must be aligned by 31 because of the number of MB height would be odd in each filed picture.
            else
                framebufHeight = FFALIGN(scaledHeight, 16);
            rotbufWidth = (decConfig.rotAngle == 90 || decConfig.rotAngle == 270) ?
                FFALIGN(scaledHeight, 16) : FFALIGN(scaledWidth, 16);
            rotbufHeight = (decConfig.rotAngle == 90 || decConfig.rotAngle == 270) ?
                FFALIGN(scaledWidth, 16) : FFALIGN(scaledHeight, 16);
        } else {
            framebufWidth = FFALIGN(initialInfo.picWidth, 16);
            if (IsSupportInterlaceMode(decOP.bitstreamFormat, &initialInfo))
                framebufHeight = FFALIGN(decConfig.maxHeight, 32); // framebufheight must be aligned by 31 because of the number of MB height would be odd in each filed picture.
            else
                framebufHeight = FFALIGN(decConfig.maxHeight, 16);
            rotbufWidth = (decConfig.rotAngle == 90 || decConfig.rotAngle == 270) ?
                FFALIGN(initialInfo.picHeight, 16) : FFALIGN(initialInfo.picWidth, 16);
            rotbufHeight = (decConfig.rotAngle == 90 || decConfig.rotAngle == 270) ?
                FFALIGN(initialInfo.picWidth, 16) : FFALIGN(initialInfo.picHeight, 16);
        }

        rotStride = rotbufWidth;
        framebufStride = framebufWidth;
        framebufFormat = FORMAT_420;
        framebufSize = VPU_GetFrameBufSize(framebufStride, framebufHeight, mapType, framebufFormat, &dramCfg);
/*
        sw_mixer_close((coreIdx*MAX_NUM_VPU_CORE)+instIdx);
        if (!ppuEnable)
            sw_mixer_open((coreIdx*MAX_NUM_VPU_CORE)+instIdx, framebufStride, framebufHeight);
        else
            sw_mixer_open((coreIdx*MAX_NUM_VPU_CORE)+instIdx, rotStride, rotbufHeight);
*/
        // the size of pYuv should be aligned 8 byte. because of C&M HPI bus system constraint.

        secAxiUse.useBitEnable  = USE_BIT_INTERNAL_BUF;
        secAxiUse.useIpEnable   = USE_IP_INTERNAL_BUF;
        secAxiUse.useDbkYEnable = USE_DBKY_INTERNAL_BUF;
        secAxiUse.useDbkCEnable = USE_DBKC_INTERNAL_BUF;
        secAxiUse.useBtpEnable  = USE_BTP_INTERNAL_BUF;
        secAxiUse.useOvlEnable  = USE_OVL_INTERNAL_BUF;

        VPU_DecGiveCommand(handle, SET_SEC_AXI, &secAxiUse);
        regFrameBufCount = initialInfo.minFrameBufferCount + EXTRA_FRAME_BUFFER_NUM;

#ifdef SUPPORT_DEC_RESOLUTION_CHANGE
        decBufInfo.maxDecMbX = framebufWidth/16;
        decBufInfo.maxDecMbY = FFALIGN(framebufHeight, 32)/16;
        decBufInfo.maxDecMbNum = decBufInfo.maxDecMbX*decBufInfo.maxDecMbY;
#endif
#if defined(SUPPORT_DEC_SLICE_BUFFER) || defined(SUPPORT_DEC_RESOLUTION_CHANGE)
        // Register frame buffers requested by the decoder.
        VPU_ENSURE(VPU_DecRegisterFrameBuffer(handle, NULL, regFrameBufCount, framebufStride, framebufHeight, mapType, &decBufInfo), false); // frame map type (can be changed before register frame buffer)
#else
        // Register frame buffers requested by the decoder.
        VPU_ENSURE(VPU_DecRegisterFrameBuffer(handle, NULL, regFrameBufCount, framebufStride, framebufHeight, mapType), false); // frame map type (can be changed before register frame buffer)
#endif
        //omx check PrintMemoryAccessViolationReason(pVpu->coreIdx, NULL);
        VPU_DecGiveCommand(handle, GET_TILEDMAP_CONFIG, &mapCfg);
        if (ppuEnable) {
            ppIdx = 0;
            fbAllocInfo.format = framebufFormat;
            fbAllocInfo.cbcrInterleave = decOP.cbcrInterleave;
            if (decOP.tiled2LinearEnable)
                fbAllocInfo.mapType = LINEAR_FRAME_MAP;
            else
                fbAllocInfo.mapType = mapType;

            fbAllocInfo.stride  = rotStride;
            fbAllocInfo.height  = rotbufHeight;
            fbAllocInfo.num     = MAX_ROT_BUF_NUM;
            fbAllocInfo.endian  = decOP.frameEndian;
            fbAllocInfo.type    = FB_TYPE_PPU;
            VPU_ENSURE(VPU_DecAllocateFrameBuffer(handle, fbAllocInfo, fbPPU), false);

            ppIdx = 0;

            if (decConfig.useRot) {
                VPU_DecGiveCommand(handle, SET_ROTATION_ANGLE, &(decConfig.rotAngle));
                VPU_DecGiveCommand(handle, SET_MIRROR_DIRECTION, &(decConfig.mirDir));
            }
            if (decConfig.useDering)
                VPU_DecGiveCommand(handle, ENABLE_DERING, 0);
            VPU_DecGiveCommand(handle, SET_ROTATOR_STRIDE, &rotStride);
        }
        seqInited = 1;
    }

FLUSH_BUFFER:
    if((int_reason & (1<<INT_BIT_BIT_BUF_EMPTY)) != (1<<INT_BIT_BIT_BUF_EMPTY)
            && (int_reason & (1<<INT_BIT_DEC_FIELD)) != (1<<INT_BIT_DEC_FIELD)) {
        if (ppuEnable) {
            VPU_DecGiveCommand(handle, SET_ROTATOR_OUTPUT, &fbPPU[ppIdx]);
            /*
            if (decConfig.useRot) {
                VPU_DecGiveCommand(handle, ENABLE_ROTATION, 0);
                VPU_DecGiveCommand(handle, ENABLE_MIRRORING, 0);
            }
            if (decConfig.useDering)
                VPU_DecGiveCommand(handle, ENABLE_DERING, 0);*/
        }

        ConfigDecReport(d.coreIdx, d.handle, decOP.bitstreamFormat);
        // Start decoding a frame.
        VPU_ENSURE(VPU_DecStartOneFrame(handle, &decParam), false);
    } else {
        if(int_reason & (1<<INT_BIT_DEC_FIELD)) {
            VPU_ClearInterrupt(d.coreIdx);
            int_reason = 0;
        }
        // After VPU generate the BIT_EMPTY interrupt. HOST should feed the bitstreams than 512 byte.
        if (decOP.bitstreamMode != BS_MODE_PIC_END) {
            if (bsfillSize < VPU_GBU_SIZE)
                return true; //continue
        }
    }


    while ((kbhitRet = osal_kbhit()) == 0) {
        int_reason = VPU_WaitInterrupt(d.coreIdx, VPU_DEC_TIMEOUT);
        if (int_reason == (Uint32)-1 ) {// timeout
            VPU_SWReset(d.coreIdx, SW_RESET_SAFETY, d.handle);
            break;
        }
        CheckUserDataInterrupt(d.coreIdx, d.handle, d.outputInfo.indexFrameDecoded, decOP.bitstreamFormat, int_reason);
        if (int_reason & (1<<INT_BIT_DEC_FIELD)) {
            if (decOP.bitstreamMode == BS_MODE_PIC_END) {
                PhysicalAddress rdPtr, wrPtr;
                int room;
                VPU_DecGetBitstreamBuffer(d.handle, &rdPtr, &wrPtr, &room);
                if (rdPtr-decOP.bitstreamBuffer < (PhysicalAddress)(chunkData.size()+picHeaderSize+seqHeaderSize-8))	// there is full frame data in chunk data.
                    VPU_DecSetRdPtr(d.handle, rdPtr, 0);		//set rdPtr to the position of next field data.
                else {
                    // do not clear interrupt until feeding next field picture.
                    break;
                }
            }
        }
        if (int_reason)
            VPU_ClearInterrupt(d.coreIdx);
        if (int_reason & (1<<INT_BIT_BIT_BUF_EMPTY)) {
            if (decOP.bitstreamMode == BS_MODE_PIC_END) {
                VLOG(ERR, "Invalid operation is occurred in pic_end mode \n");
                goto ERR_DEC_OPEN;
            }
            break;
        }
        if (int_reason & (1<<INT_BIT_PIC_RUN))
            break;
    }

    if ((int_reason & (1<<INT_BIT_BIT_BUF_EMPTY))
            || (int_reason & (1<<INT_BIT_DEC_FIELD))) {
        bsfillSize = 0;
        return true;//continue; // go to take next chunk.
    }
    ret = VPU_DecGetOutputInfo(d.handle, &d.outputInfo);
    if (ret != RETCODE_SUCCESS) {
        VLOG(ERR,  "VPU_DecGetOutputInfo failed Error code is 0x%x \n", ret);
        if (ret == RETCODE_MEMORY_ACCESS_VIOLATION)
            PrintMemoryAccessViolationReason(d.coreIdx, &d.outputInfo);
        goto ERR_DEC_OPEN;
    }
    if ((d.outputInfo.decodingSuccess & 0x01) == 0) {
        VLOG(ERR, "VPU_DecGetOutputInfo decode fail framdIdx %d \n", frameIdx);
        VLOG(TRACE, "#%d, indexFrameDisplay %d || picType %d || indexFrameDecoded %d\n",
            frameIdx, d.outputInfo.indexFrameDisplay, d.outputInfo.picType, d.outputInfo.indexFrameDecoded );
    }
    VLOG(TRACE, "#%d:%d, indexDisplay %d || picType %d || indexDecoded %d || rdPtr=0x%x || wrPtr=0x%x || chunkSize = %d, consume=%d\n",
        instIdx, frameIdx, d.outputInfo.indexFrameDisplay, d.outputInfo.picType, d.outputInfo.indexFrameDecoded, d.outputInfo.rdPtr, d.outputInfo.wrPtr, chunkData.size()+picHeaderSize, d.outputInfo.consumedByte);


    SaveDecReport(d.coreIdx, d.handle, &d.outputInfo, decOP.bitstreamFormat, ((initialInfo.picWidth+15)&~15)/16); ///TODO:
    if (d.outputInfo.chunkReuseRequired) // reuse previous chunk. that would be 1 once framebuffer is full.
        reUseChunk = 1;

    if (d.outputInfo.indexFrameDisplay == -1)
        decodefinish = 1;
    if (decodefinish) {
        if (!ppuEnable || decodeIdx == 0)
        return true;//break;
    }
    if (d.outputInfo.indexFrameDisplay == -3 ||
        d.outputInfo.indexFrameDisplay == -2 ) // BIT doesn't have picture to be displayed
    {
        if (check_VSYNC_flag()) {
            clear_VSYNC_flag();
            if (frame_queue_dequeue(display_queue, &dispDoneIdx) == 0)
                VPU_DecClrDispFlag(handle, dispDoneIdx);
        }
#if defined(CNM_FPGA_PLATFORM) && defined(FPGA_LX_330)
#else
        if (d.outputInfo.indexFrameDecoded == -1)	// VPU did not decode a picture because there is not enough frame buffer to continue decoding
        {
            // if you can't get VSYN interrupt on your sw layer. this point is reasonable line to set VSYN flag.
            // but you need fine tune EXTRA_FRAME_BUFFER_NUM value not decoder to write being display buffer.
            if (frame_queue_count(display_queue) > 0)
                set_VSYNC_flag();
        }
#endif
        return true;
    }
    if (ppuEnable) {
        if (decodeIdx == 0) // if PP has been enabled, the first picture is saved at next time.
        {
            // save rotated dec width, height to display next decoding time.
            if (d.outputInfo.indexFrameDisplay >= 0)
                frame_queue_enqueue(display_queue, outputInfo.indexFrameDisplay);
            rcPrevDisp = outputInfo.rcDisplay;
            decodeIdx++;
            continue;

        }
    }


    decodeIdx++;
    if (outputInfo.indexFrameDisplay >= 0) // omx: <0 goto no display
        frame_queue_enqueue(display_queue, outputInfo.indexFrameDisplay);

    if (!saveImage) {
        if (!ppuEnable) { // TODO: fill out buffer. used by VideoFrame and XImage (GPU scale)
            vdi_read_memory(coreIdx, d.outputInfo.dispFrame.bufY, (BYTE *)dst,  (stride*picHeight*3/2), endian);
            //OmxSaveYuvImageBurstFormat(d.coreIdx, &d.outputInfo.dispFrame, framebufStride, framebufHeight);
        } else {
            ppIdx = (ppIdx+1)%MAX_ROT_BUF_NUM;
            OmxSaveYuvImageBurstFormat(&outputInfo.dispFrame, rotStride, rotbufHeight);
        }
#ifdef FORCE_SET_VSYNC_FLAG
        set_VSYNC_flag();
#endif
    }

    if (check_VSYNC_flag()) {
        clear_VSYNC_flag();
        if (frame_queue_dequeue(display_queue, &dispDoneIdx) == 0) // omx: != -1
            VPU_DecClrDispFlag(d.handle, dispDoneIdx);
    }
    // save rotated dec width, height to display next decoding time.
    rcPrevDisp = d.outputInfo.rcDisplay;

    if (d.outputInfo.numOfErrMBs) {
        totalNumofErrMbs += outputInfo.numOfErrMBs;
        VLOG(ERR, "Num of Error Mbs : %d, in Frame : %d \n", outputInfo.numOfErrMBs, frameIdx);
    }

    frameIdx++;

    return true;
}

void VideoDecoderVPU::flush()
{
    VPU_DecFrameBufferFlush(handle);
}

VideoFrame VideoDecoderVPU::frame()
{
    DPTR_D(VideoDecoderVPU);
    return frame;
}

} // namespace QtAV
#include "VideoDecoderVPU.moc"
