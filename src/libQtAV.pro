TEMPLATE = lib
TARGET = QtAV
QT += core gui
greaterThan(QT_MAJOR_VERSION, 4) {
  QT += widgets
}
CONFIG *= qtav-buildlib
INCLUDEPATH += $$[QT_INSTALL_HEADERS]

#var with '_' can not pass to pri?
STATICLINK = 0
PROJECTROOT = $$PWD/..
!include(libQtAV.pri): error("could not find libQtAV.pri")
preparePaths($$OUT_PWD/../out)

RESOURCES += ../i18n/QtAV.qrc \
    shaders/shaders.qrc

win32 {
    RC_FILE = $${PROJECTROOT}/res/QtAV.rc
#no depends for rc file by default, even if rc includes a header. Makefile target use '/' as default, so not works iwth win cmd
    rc.target = $$clean_path($$RC_FILE) #rc obj depends on clean path target
    rc.depends = $$PWD/QtAV/version.h
#why use multiple rule failed? i.e. add a rule without command
    isEmpty(QMAKE_SH) {
        rc.commands = @copy /B $$system_path($$RC_FILE)+,, #change file time
    } else {
        rc.commands = @touch $$RC_FILE #change file time
    }
    QMAKE_EXTRA_TARGETS += rc
}
# copy runtime libs to qt sdk
!ios: copy_sdk_libs = $$DESTDIR/$$qtSharedLib($$NAME)
#plugin.depends = #makefile target
#windows: copy /y file1+file2+... dir. need '+'
for(f, copy_sdk_libs) {
  win32: copy_sdk_libs_cmd += $$quote(-\$\(COPY_FILE\) \"$$shell_path($$f)\" \"$$shell_path($$[QT_INSTALL_BINS])\")
  else: copy_sdk_libs_cmd += $$quote(-\$\(COPY_FILE\) \"$$shell_path($$f)\" \"$$shell_path($$[QT_INSTALL_LIBS])\")
}
#join values seperated by space. so quote is needed
copy_sdk_libs_cmd = $$join(copy_sdk_libs_cmd,$$escape_expand(\\n\\t))
#just append as a string to $$QMAKE_POST_LINK
isEmpty(QMAKE_POST_LINK): QMAKE_POST_LINK = $$copy_sdk_libs_cmd
else: QMAKE_POST_LINK = $${QMAKE_POST_LINK}$$escape_expand(\\n\\t)$$copy_sdk_libs_cmd

OTHER_FILES += $$RC_FILE
TRANSLATIONS = $${PROJECTROOT}/i18n/QtAV_zh_CN.ts

## sse2 sse4_1 may be defined in Qt5 qmodule.pri but is not included. Qt4 defines sse and sse2
sse4_1|config_sse4_1|contains(TARGET_ARCH_SUB, sse4.1) {
  CONFIG += sse2 #only sse4.1 is checked. sse2 now can be disabled if sse4.1 is disabled
  DEFINES += QTAV_HAVE_SSE4_1=1
## TODO: use SSE4_1_SOURCES
# all x64 processers supports sse2. unknown option for vc
  *msvc* {
    !isEqual(QT_ARCH, x86_64)|!x86_64: QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_SSE4_1
  } else {
    QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_SSE4_1 #gcc -msse4.1
  }
}
sse2|config_sse2|contains(TARGET_ARCH_SUB, sse2) {
  DEFINES += QTAV_HAVE_SSE2=1
# all x64 processers supports sse2. unknown option for vc
  *msvc* {
    !isEqual(QT_ARCH, x86_64)|!x86_64: QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_SSE2
  } else {
    QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_SSE2 #gcc -msse2
  }
}

*msvc* {
#link FFmpeg and portaudio which are built by gcc need /SAFESEH:NO
    QMAKE_LFLAGS += /SAFESEH:NO
    INCLUDEPATH += compat/msvc
}
#UINT64_C: C99 math features, need -D__STDC_CONSTANT_MACROS in CXXFLAGS
DEFINES += __STDC_CONSTANT_MACROS
android: CONFIG += config_opensl
android: LIBS += -lavcodec-55 -lavformat-55 -lavutil-52 -lswscale-2
else: LIBS += -lavcodec -lavformat -lavutil -lswscale
config_avfilter {
    DEFINES += QTAV_HAVE_AVFILTER=1
    SOURCES += LibAVFilter.cpp
    HEADERS += QtAV/LibAVFilter.h
    android: LIBS += -lavfilter-4
    else: LIBS += -lavfilter
}
config_swresample {
    DEFINES += QTAV_HAVE_SWRESAMPLE=1
    SOURCES += AudioResamplerFF.cpp
    android: LIBS += -lswresample-0
    else: LIBS += -lswresample
}
config_avresample {
    DEFINES += QTAV_HAVE_AVRESAMPLE=1
    SOURCES += AudioResamplerLibav.cpp
    android: LIBS += -lavresample-1
    else: LIBS += -lavresample
}
config_ipp {
    DEFINES += QTAV_HAVE_IPP=1
    ICCROOT = $$(IPPROOT)/../compiler
    INCLUDEPATH += $$(IPPROOT)/include
    SOURCES += ImageConverterIPP.cpp
    message("QMAKE_TARGET.arch" $$QMAKE_TARGET.arch)
    *64|contains(QMAKE_TARGET.arch, x86_64)|contains(TARGET_ARCH, x86_64) {
        IPPARCH=intel64
    } else {
        IPPARCH=ia32
    }
    LIBS *= -L$$(IPPROOT)/lib/$$IPPARCH -lippcc -lippcore -lippi \
            -L$$(IPPROOT)/../compiler/lib/$$IPPARCH -lsvml -limf
    #omp for static link. _t is multi-thread static link
}
config_portaudio {
    SOURCES += AudioOutputPortAudio.cpp
    DEFINES *= QTAV_HAVE_PORTAUDIO=1
    LIBS *= -lportaudio
    #win32: LIBS *= -lwinmm #-lksguid #-luuid
}
config_openal {
    SOURCES += AudioOutputOpenAL.cpp
    DEFINES *= QTAV_HAVE_OPENAL=1
    win32: LIBS += -lOpenAL32
    unix:!mac:!blackberry: LIBS += -lopenal
    blackberry: LIBS += -lOpenAL
    mac: LIBS += -framework OpenAL
    mac: DEFINES += HEADER_OPENAL_PREFIX
}
config_opensl {
    SOURCES += AudioOutputOpenSL.cpp
    DEFINES *= QTAV_HAVE_OPENSL=1
    LIBS += -lOpenSLES
}
config_gdiplus {
    DEFINES *= QTAV_HAVE_GDIPLUS=1
    SOURCES += GDIRenderer.cpp
    HEADERS += QtAV/private/GDIRenderer_p.h
    SDK_HEADERS += QtAV/GDIRenderer.h
    LIBS += -lgdiplus -lGdi32
}
config_direct2d {
    DEFINES *= QTAV_HAVE_DIRECT2D=1
    !*msvc*: INCLUDEPATH += $$PROJECTROOT/contrib/d2d1headers
    SOURCES += Direct2DRenderer.cpp
    HEADERS += QtAV/private/Direct2DRenderer_p.h
    SDK_HEADERS += QtAV/Direct2DRenderer.h
    #LIBS += -lD2d1
}
config_xv {
    DEFINES *= QTAV_HAVE_XV=1
    SOURCES += XVRenderer.cpp
    HEADERS += QtAV/private/XVRenderer_p.h
    SDK_HEADERS += QtAV/XVRenderer.h
    LIBS += -lXv
}
config_gl {
    QT *= opengl
    DEFINES *= QTAV_HAVE_GL=1
    SOURCES += GLWidgetRenderer.cpp
    HEADERS += QtAV/private/GLWidgetRenderer_p.h
    SDK_HEADERS += QtAV/GLWidgetRenderer.h
    OTHER_FILES += shaders/yuv_rgb.f.glsl shaders/rgb.f.glsl
}
CONFIG += config_cuda #config_dllapi config_dllapi_cuda
#CONFIG += config_cuda_link
config_cuda {
    DEFINES += QTAV_HAVE_CUDA=1
    HEADERS += cuda/dllapi/nv_inc.h cuda/helper_cuda.h
    SOURCES += VideoDecoderCUDA.cpp
    INCLUDEPATH += $$PWD/cuda cuda/dllapi
    config_dllapi:config_dllapi_cuda {
        DEFINES += QTAV_HAVE_DLLAPI_CUDA=1
        INCLUDEPATH += ../depends/dllapi/src
include(../depends/dllapi/src/libdllapi.pri)
        SOURCES += cuda/dllapi/cuda.cpp cuda/dllapi/nvcuvid.cpp cuda/dllapi/cuviddec.cpp
    } else:config_cuda_link {
        DEFINES += CUDA_LINK
        INCLUDEPATH += $$(CUDA_PATH)/include
        LIBS += -L$$(CUDA_PATH)/lib
        isEqual(TARGET_ARCH, x86): LIBS += -L$$(CUDA_PATH)/lib/Win32
        else: LIBS += -L$$(CUDA_PATH)/lib/x64
        LIBS += -lnvcuvid -lcuda
    }
    SOURCES += cuda/cuda_api.cpp
    HEADERS += cuda/cuda_api.h
}
config_dxva {
    DEFINES *= QTAV_HAVE_DXVA=1
    SOURCES += VideoDecoderDXVA.cpp
    LIBS += -lOle32
}
config_vaapi* {
    DEFINES *= QTAV_HAVE_VAAPI=1
    SOURCES += VideoDecoderVAAPI.cpp
    LIBS += -lva #TODO: dynamic load using dllapi
    config_vaapi-x11 {
        DEFINES *= QTAV_HAVE_VAAPI_X11=1
        LIBS += -lva-x11
    }
    #config_vaapi-drm {
    #   DEFINES *= QTAV_HAVE_VAAPI_DRM=1
    #   LIBS += -lva-drm
    #}
    config_vaapi-glx {
        DEFINES *= QTAV_HAVE_VAAPI_GLX=1
        LIBS *= -lva-glx
    }
}
config_libcedarv {
    DEFINES *= QTAV_HAVE_CEDARV=1
    SOURCES += VideoDecoderCedarv.cpp
    LIBS += -lvecore -lcedarv
}
#macx:!ios: CONFIG += config_vda
config_vda {
    DEFINES *= QTAV_HAVE_VDA=1
    SOURCES += VideoDecoderVDA.cpp
    LIBS += -framework VideoDecodeAcceleration -framework CoreVideo
}
SOURCES += \
    QtAV_Compat.cpp \
    QtAV_Global.cpp \
    utils/GPUMemCopy.cpp \
    AudioThread.cpp \
    AVThread.cpp \
    AudioDecoder.cpp \
    AudioFormat.cpp \
    AudioFrame.cpp \
    AudioOutput.cpp \
    AudioOutputTypes.cpp \
    AudioResampler.cpp \
    AudioResamplerTypes.cpp \
    AVDecoder.cpp \
    AVDemuxer.cpp \
    AVDemuxThread.cpp \
    ColorTransform.cpp \
    Frame.cpp \
    Filter.cpp \
    FilterContext.cpp \
    FilterManager.cpp \
    GraphicsItemRenderer.cpp \
    ImageConverter.cpp \
    ImageConverterFF.cpp \
    QPainterRenderer.cpp \
    OSD.cpp \
    OSDFilter.cpp \
    Packet.cpp \
    AVError.cpp \
    AVPlayer.cpp \
    VideoCapture.cpp \
    VideoFormat.cpp \
    VideoFrame.cpp \
    VideoRenderer.cpp \
    VideoRendererTypes.cpp \
    VideoOutput.cpp \
    VideoOutputEventFilter.cpp \
    WidgetRenderer.cpp \
    AVOutput.cpp \
    OutputSet.cpp \
    AVClock.cpp \
    Statistics.cpp \
    VideoDecoder.cpp \
    VideoDecoderTypes.cpp \
    VideoDecoderFFmpeg.cpp \
    VideoDecoderFFmpegHW.cpp \
    VideoThread.cpp \
    QAVIOContext.cpp \
    CommonTypes.cpp

SDK_HEADERS *= \
    QtAV/QtAV.h \
    QtAV/dptr.h \
    QtAV/QtAV_Global.h \
    QtAV/AudioResampler.h \
    QtAV/AudioResamplerTypes.h \
    QtAV/AudioDecoder.h \
    QtAV/AudioFormat.h \
    QtAV/AudioFrame.h \
    QtAV/AudioOutput.h \
    QtAV/AudioOutputTypes.h \
    QtAV/AVDecoder.h \
    QtAV/AVDemuxer.h \
    QtAV/BlockingQueue.h \
    QtAV/CommonTypes.h \
    QtAV/Filter.h \
    QtAV/FilterContext.h \
    QtAV/Frame.h \
    QtAV/GraphicsItemRenderer.h \
    QtAV/ImageConverter.h \
    QtAV/ImageConverterTypes.h \
    QtAV/QPainterRenderer.h \
    QtAV/OSD.h \
    QtAV/OSDFilter.h \
    QtAV/Packet.h \
    QtAV/AVError.h \
    QtAV/AVPlayer.h \
    QtAV/VideoCapture.h \
    QtAV/VideoRenderer.h \
    QtAV/VideoRendererTypes.h \
    QtAV/VideoOutput.h \
    QtAV/WidgetRenderer.h \
    QtAV/AVOutput.h \
    QtAV/AVClock.h \
    QtAV/VideoDecoder.h \
    QtAV/VideoDecoderTypes.h \
    QtAV/VideoFormat.h \
    QtAV/VideoFrame.h \
    QtAV/FactoryDefine.h \
    QtAV/Statistics.h \
    QtAV/SurfaceInterop.h \
    QtAV/version.h


HEADERS *= \
    $$SDK_HEADERS \
    utils/GPUMemCopy.h \
    QtAV/prepost.h \
    QtAV/AVDemuxThread.h \
    QtAV/AVThread.h \
    QtAV/AudioThread.h \
    QtAV/VideoThread.h \
    QtAV/ColorTransform.h \
    QtAV/VideoOutputEventFilter.h \
    QtAV/OutputSet.h \
    QtAV/QtAV_Compat.h \
    QtAV/QAVIOContext.h \
    QtAV/singleton.h \
    QtAV/factory.h \
    QtAV/FilterManager.h \
    QtAV/private/AudioOutput_p.h \
    QtAV/private/AudioResampler_p.h \
    QtAV/private/AVThread_p.h \
    QtAV/private/AVDecoder_p.h \
    QtAV/private/AVOutput_p.h \
    QtAV/private/Filter_p.h \
    QtAV/private/Frame_p.h \
    QtAV/private/GraphicsItemRenderer_p.h \
    QtAV/private/ImageConverter_p.h \
    QtAV/private/VideoDecoder_p.h \
    QtAV/private/VideoDecoderFFmpegHW_p.h \
    QtAV/VideoDecoderFFmpegHW.h \
    QtAV/private/VideoRenderer_p.h \
    QtAV/private/QPainterRenderer_p.h \
    QtAV/private/WidgetRenderer_p.h

SDK_INCLUDE_FOLDER = QtAV
include($$PROJECTROOT/deploy.pri)
