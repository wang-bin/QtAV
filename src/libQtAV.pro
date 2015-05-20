TEMPLATE = lib
MODULE_INCNAME = QtAV # for mac framework. also used in install_sdk.pro
TARGET = QtAV
QT += core gui
#CONFIG *= ltcg
greaterThan(QT_MAJOR_VERSION, 4) {
  CONFIG *= config_opengl
  greaterThan(QT_MINOR_VERSION, 3) {
    CONFIG *= config_openglwindow
  }
} else {
config_gl: QT += opengl
}
CONFIG *= qtav-buildlib
staticlib: DEFINES += BUILD_QTAV_STATIC
INCLUDEPATH += $$[QT_INSTALL_HEADERS]
icon.files = $$PWD/$${TARGET}.svg
icon.path = /usr/share/icons/hicolor/64x64/apps
INSTALLS += icon

#mac: simd.prf will load qt_build_config and the result is soname will prefixed with QT_INSTALL_LIBS and link flag will append soname after QMAKE_LFLAGS_SONAME
config_libcedarv: CONFIG *= neon config_simd #need by qt4 addSimdCompiler(). neon or config_neon is required because tests/arch can not detect neon
## sse2 sse4_1 may be defined in Qt5 qmodule.pri but is not included. Qt4 defines sse and sse2
sse4_1|config_sse4_1|contains(TARGET_ARCH_SUB, sse4.1): CONFIG *= sse4_1 config_simd
sse2|config_sse2|contains(TARGET_ARCH_SUB, sse2): CONFIG *= sse2 config_simd

#release: DEFINES += QT_NO_DEBUG_OUTPUT
#var with '_' can not pass to pri?
PROJECTROOT = $$PWD/..
!include(libQtAV.pri): error("could not find libQtAV.pri")
preparePaths($$OUT_PWD/../out)
exists($$PROJECTROOT/contrib/libchardet/libchardet.pri) {
  include($$PROJECTROOT/contrib/libchardet/libchardet.pri)
  DEFINES += QTAV_HAVE_CHARDET=1 BUILD_CHARDET_STATIC
} else {
  warning("contrib/libchardet is missing. run 'git submodule update --init' first")
}
exists($$PROJECTROOT/contrib/capi/capi.pri) {
  include($$PROJECTROOT/contrib/capi/capi.pri)
  CONFIG *= capi
  DEFINES += QTAV_HAVE_CAPI=1 BUILD_CAPI_STATIC
} else {
  warning("contrib/capi is missing. run 'git submodule update --init' first")
}

RESOURCES += QtAV.qrc \
    shaders/shaders.qrc

!rc_file {
    RC_ICONS = QtAV.ico
    QMAKE_TARGET_COMPANY = "Shanghai University->S3 Graphics->Deepin | wbsecg1@gmail.com"
    QMAKE_TARGET_DESCRIPTION = "QtAV Multimedia playback framework. http://www.qtav.org"
    QMAKE_TARGET_COPYRIGHT = "Copyright (C) 2012-2015 WangBin, wbsecg1@gmail.com"
    QMAKE_TARGET_PRODUCT = "QtAV"
} else:win32 {
    RC_FILE = QtAV.rc
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

OTHER_FILES += $$RC_FILE QtAV.svg
TRANSLATIONS = i18n/QtAV_zh_CN.ts

sse4_1 {
  CONFIG += sse2 #only sse4.1 is checked. sse2 now can be disabled if sse4.1 is disabled
  DEFINES += QTAV_HAVE_SSE4_1=1
  !config_simd: CONFIG *= simd
  SSE4_1_SOURCES += utils/CopyFrame_SSE4.cpp
}
sse2 {
  DEFINES += QTAV_HAVE_SSE2=1
  !config_simd: CONFIG *= simd
  SSE2_SOURCES += utils/CopyFrame_SSE2.cpp
}

*msvc* {
#link FFmpeg and portaudio which are built by gcc need /SAFESEH:NO
    #QMAKE_LFLAGS += /SAFESEH:NO
    INCLUDEPATH += compat/msvc
}
#UINT64_C: C99 math features, need -D__STDC_CONSTANT_MACROS in CXXFLAGS
DEFINES += __STDC_CONSTANT_MACROS
android: CONFIG += config_opensl

config_swresample {
    DEFINES += QTAV_HAVE_SWRESAMPLE=1
    SOURCES += AudioResamplerFF.cpp
    LIBS += -lswresample
}
config_avresample {
    DEFINES += QTAV_HAVE_AVRESAMPLE=1
    SOURCES += AudioResamplerLibav.cpp
    LIBS += -lavresample
}
config_avdevice { #may depends on avfilter
    DEFINES += QTAV_HAVE_AVDEVICE=1
    LIBS *= -lavdevice
    static_ffmpeg {
      win32 {
        LIBS *= -lgdi32 -loleaut32
      } else:mac:!ios { # static ffmpeg
        LIBS += -framework Foundation -framework QTKit -framework CoreMedia -framework QuartzCore -framework CoreGraphics \
                -framework AVFoundation
      # assume avdevice targets to the same version as Qt and always >= 10.6
        !isEqual(QMAKE_MACOSX_DEPLOYMENT_TARGET, 10.6): LIBS += -framework AVFoundation
      }
    }
}
config_avfilter {
    DEFINES += QTAV_HAVE_AVFILTER=1
    LIBS += -lavfilter
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
config_dsound {
    SOURCES += output/audio/AudioOutputDSound.cpp
    DEFINES *= QTAV_HAVE_DSOUND=1
}
config_portaudio {
    SOURCES += output/audio/AudioOutputPortAudio.cpp
    DEFINES *= QTAV_HAVE_PORTAUDIO=1
    LIBS *= -lportaudio
    #win32: LIBS *= -lwinmm #-lksguid #-luuid
}
config_openal {
    SOURCES += output/audio/AudioOutputOpenAL.cpp
    DEFINES *= QTAV_HAVE_OPENAL=1
    win32: LIBS += -lOpenAL32
    unix:!mac:!blackberry: LIBS += -lopenal
    blackberry: LIBS += -lOpenAL
    mac: LIBS += -framework OpenAL
    mac: DEFINES += HEADER_OPENAL_PREFIX
    static_openal {
      DEFINES += AL_LIBTYPE_STATIC
      *linux*:!android: LIBS += -lasound
      win32: LIBS += -lwinmm
    }
}
config_opensl {
    SOURCES += output/audio/AudioOutputOpenSL.cpp
    DEFINES *= QTAV_HAVE_OPENSL=1
    LIBS += -lOpenSLES
}
config_pulseaudio {
    SOURCES += output/audio/AudioOutputPulse.cpp
    DEFINES *= QTAV_HAVE_PULSEAUDIO=1
    LIBS += -lpulse
}
CONFIG += config_cuda #config_dllapi config_dllapi_cuda
#CONFIG += config_cuda_link
config_cuda {
    DEFINES += QTAV_HAVE_CUDA=1
    HEADERS += cuda/dllapi/nv_inc.h cuda/helper_cuda.h
    SOURCES += codec/video/VideoDecoderCUDA.cpp
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
    SOURCES += codec/video/VideoDecoderDXVA.cpp
    LIBS += -lole32
}
config_vaapi* {
    DEFINES *= QTAV_HAVE_VAAPI=1
    SOURCES += codec/video/VideoDecoderVAAPI.cpp  vaapi/vaapi_helper.cpp vaapi/SurfaceInteropVAAPI.cpp
    HEADERS += vaapi/vaapi_helper.h  vaapi/SurfaceInteropVAAPI.h
    LIBS += -lva #dynamic load va-glx va-x11 using dllapi
}
config_libcedarv {
    DEFINES *= QTAV_HAVE_CEDARV=1
    QMAKE_CXXFLAGS *= -march=armv7-a
    SOURCES += codec/video/VideoDecoderCedarv.cpp
    !config_simd: CONFIG *= simd #addSimdCompiler xxx_ASM
    CONFIG += no_clang_integrated_as #see qtbase/src/gui/painting/painting.pri. add -fno-integrated-as from simd.prf
    NEON_ASM += codec/video/tiled_yuv.S #from libvdpau-sunxi
    LIBS += -lvecore -lcedarv
    OTHER_FILES += $$NEON_ASM
}
macx:!ios: CONFIG += config_vda
config_vda {
    DEFINES *= QTAV_HAVE_VDA=1
    SOURCES += codec/video/VideoDecoderVDA.cpp
    LIBS += -framework VideoDecodeAcceleration -framework CoreVideo -framework CoreFoundation \
            -framework IOSurface
}

config_gl|config_opengl {
  OTHER_FILES += shaders/planar.f.glsl shaders/rgb.f.glsl
  SDK_HEADERS *= \
    QtAV/OpenGLRendererBase.h \
    QtAV/OpenGLVideo.h \
    QtAV/VideoShader.h
  SDK_PRIVATE_HEADERS = \
    QtAV/private/OpenGLRendererBase_p.h
  HEADERS *= \
    utils/OpenGLHelper.h \
    ShaderManager.h
  SOURCES *= \
    output/video/OpenGLRendererBase.cpp \
    OpenGLVideo.cpp \
    VideoShader.cpp \
    ShaderManager.cpp \
    utils/OpenGLHelper.cpp
}
config_openglwindow {
  SDK_HEADERS *= QtAV/OpenGLWindowRenderer.h
  SOURCES *= output/video/OpenGLWindowRenderer.cpp
}
config_libass {
#link against libass instead of dynamic load
  !capi {
    LIBS += -lass
    DEFINES += CAPI_LINK_ASS
  }
  HEADERS *= subtitle/ass_api.h
  SOURCES *= subtitle/ass_api.cpp
  SOURCES *= subtitle/SubtitleProcessorLibASS.cpp
}

# mac is -FQTDIR we need -LQTDIR
LIBS *= -L$$[QT_INSTALL_LIBS] -lavcodec -lavformat -lswscale -lavutil
win32 {
#dynamicgl: __impl__GetDC __impl_ReleaseDC __impl_GetDesktopWindow
    LIBS += -luser32
}
# compat with old system
# use old libva.so to link against
glibc_compat: *linux*: LIBS += -lrt  # do not use clock_gettime in libc, GLIBC_2.17 is not available on old system
static_ffmpeg {
# libs needed by mac static ffmpeg. corefoundation: vda, avdevice
  mac: LIBS += -liconv -lbz2 -lz -framework CoreFoundation
  win32: LIBS *= -lws2_32 -lstrmiids -lvfw32 -luuid
  !mac:*g++* {
    LIBS += -lz
    QMAKE_LFLAGS += -Wl,-Bsymbolic #link to static lib, see http://ffmpeg.org/platform.html
  }
}
SOURCES += \
    AVCompat.cpp \
    QtAV_Global.cpp \
    subtitle/CharsetDetector.cpp \
    subtitle/PlainText.cpp \
    subtitle/PlayerSubtitle.cpp \
    subtitle/Subtitle.cpp \
    subtitle/SubtitleProcessor.cpp \
    subtitle/SubtitleProcessorFFmpeg.cpp \
    utils/GPUMemCopy.cpp \
    utils/Logger.cpp \
    AudioThread.cpp \
    utils/internal.cpp \
    AVThread.cpp \
    AudioFormat.cpp \
    AudioFrame.cpp \
    AudioResampler.cpp \
    AudioResamplerTypes.cpp \
    codec/AVDecoder.cpp \
    codec/audio/AudioDecoder.cpp \
    codec/audio/AudioDecoderFFmpeg.cpp \
    codec/AVEncoder.cpp \
    AVMuxer.cpp \
    AVDemuxer.cpp \
    AVDemuxThread.cpp \
    ColorTransform.cpp \
    Frame.cpp \
    filter/Filter.cpp \
    filter/FilterContext.cpp \
    filter/FilterManager.cpp \
    filter/LibAVFilter.cpp \
    filter/SubtitleFilter.cpp \
    ImageConverter.cpp \
    ImageConverterFF.cpp \
    Packet.cpp \
    PacketBuffer.cpp \
    AVError.cpp \
    AVPlayer.cpp \
    AVPlayerPrivate.cpp \
    AVClock.cpp \
    VideoCapture.cpp \
    VideoFormat.cpp \
    VideoFrame.cpp \
    io/MediaIO.cpp \
    io/QIODeviceIO.cpp \
    output/audio/AudioOutput.cpp \
    output/audio/AudioOutputBackend.cpp \
    output/video/VideoRenderer.cpp \
    output/video/VideoRendererTypes.cpp \
    output/video/VideoOutput.cpp \
    output/video/QPainterRenderer.cpp \
    output/AVOutput.cpp \
    output/OutputSet.cpp \
    Statistics.cpp \
    codec/video/VideoDecoder.cpp \
    codec/video/VideoDecoderTypes.cpp \
    codec/video/VideoDecoderFFmpegBase.cpp \
    codec/video/VideoDecoderFFmpeg.cpp \
    codec/video/VideoDecoderFFmpegHW.cpp \
    codec/video/VideoEncoder.cpp \
    codec/video/VideoEncoderFFmpeg.cpp \
    VideoThread.cpp \
    VideoFrameExtractor.cpp \
    CommonTypes.cpp

SDK_HEADERS *= \
    QtAV/QtAV \
    QtAV/QtAV.h \
    QtAV/dptr.h \
    QtAV/QtAV_Global.h \
    QtAV/AudioResampler.h \
    QtAV/AudioResamplerTypes.h \
    QtAV/AudioDecoder.h \
    QtAV/AudioFormat.h \
    QtAV/AudioFrame.h \
    QtAV/AudioOutput.h \
    QtAV/AVDecoder.h \
    QtAV/AVEncoder.h \
    QtAV/AVDemuxer.h \
    QtAV/AVMuxer.h \
    QtAV/CommonTypes.h \
    QtAV/Filter.h \
    QtAV/FilterContext.h \
    QtAV/LibAVFilter.h \
    QtAV/Frame.h \
    QtAV/QPainterRenderer.h \
    QtAV/Packet.h \
    QtAV/AVError.h \
    QtAV/AVPlayer.h \
    QtAV/VideoCapture.h \
    QtAV/VideoRenderer.h \
    QtAV/VideoRendererTypes.h \
    QtAV/VideoOutput.h \
    QtAV/MediaIO.h \
    QtAV/AVOutput.h \
    QtAV/AVClock.h \
    QtAV/VideoDecoder.h \
    QtAV/VideoDecoderTypes.h \
    QtAV/VideoEncoder.h \
    QtAV/VideoFormat.h \
    QtAV/VideoFrame.h \
    QtAV/VideoFrameExtractor.h \
    QtAV/FactoryDefine.h \
    QtAV/Statistics.h \
    QtAV/Subtitle.h \
    QtAV/SubtitleFilter.h \
    QtAV/SurfaceInterop.h \
    QtAV/version.h

SDK_PRIVATE_HEADERS *= \
    QtAV/private/factory.h \
    QtAV/private/mkid.h \
    QtAV/private/prepost.h \
    QtAV/private/singleton.h \
    QtAV/private/PlayerSubtitle.h \
    QtAV/private/SubtitleProcessor.h \
    QtAV/private/AVCompat.h \
    QtAV/private/AudioOutputBackend.h \
    QtAV/private/AudioResampler_p.h \
    QtAV/private/AVDecoder_p.h \
    QtAV/private/AVEncoder_p.h \
    QtAV/private/MediaIO_p.h \
    QtAV/private/AVOutput_p.h \
    QtAV/private/Filter_p.h \
    QtAV/private/Frame_p.h \
    QtAV/private/VideoShader_p.h \
    QtAV/private/VideoRenderer_p.h \
    QtAV/private/QPainterRenderer_p.h

# QtAV/private/* may be used by developers to extend QtAV features without changing QtAV library
# headers not in QtAV/ and it's subdirs are used only by QtAV internally
HEADERS *= \
    $$SDK_HEADERS \
    $$SDK_PRIVATE_HEADERS \
    AVPlayerPrivate.h \
    AVDemuxThread.h \
    AVThread.h \
    AVThread_p.h \
    AudioThread.h \
    PacketBuffer.h \
    VideoThread.h \
    ImageConverter.h \
    ImageConverter_p.h \
    codec/video/VideoDecoderFFmpegBase.h \
    codec/video/VideoDecoderFFmpegHW.h \
    codec/video/VideoDecoderFFmpegHW_p.h \
    filter/FilterManager.h \
    subtitle/CharsetDetector.h \
    subtitle/PlainText.h \
    utils/BlockingQueue.h \
    utils/GPUMemCopy.h \
    utils/Logger.h \
    utils/SharedPtr.h \
    utils/ring.h \
    utils/internal.h \
    output/OutputSet.h \
    QtAV/ColorTransform.h

# from mkspecs/features/qt_module.prf
# OS X and iOS frameworks
mac_framework { # from common.pri
   #QMAKE_FRAMEWORK_VERSION = 4.0
   CONFIG += lib_bundle sliced_bundle qt_framework
   CONFIG -= qt_install_headers #no need to install these as well
   !debug_and_release|!build_all|CONFIG(release, debug|release) {
        FRAMEWORK_HEADERS.version = Versions
        FRAMEWORK_HEADERS.files = $$SDK_HEADERS
        FRAMEWORK_HEADERS.path = Headers
# 5.4(beta) workaround for wrong include path
# TODO: why <QtCore/qglobal.h> can be found?
        greaterThan(QT_MAJOR_VERSION, 4):greaterThan(QT_MINOR_VERSION, 3): FRAMEWORK_HEADERS.path = Headers/$$MODULE_INCNAME
        FRAMEWORK_PRIVATE_HEADERS.version = Versions
        FRAMEWORK_PRIVATE_HEADERS.files = $$SDK_PRIVATE_HEADERS
        FRAMEWORK_PRIVATE_HEADERS.path = Headers/$$VERSION/$$MODULE_INCNAME/private
        QMAKE_BUNDLE_DATA += FRAMEWORK_HEADERS FRAMEWORK_PRIVATE_HEADERS
   }
}

mac {
   CONFIG += explicitlib
   macx-g++ {
       QMAKE_CFLAGS += -fconstant-cfstrings
       QMAKE_CXXFLAGS += -fconstant-cfstrings
   }
}

unix:!android:!mac {
#debian
DEB_INSTALL_LIST = .$$[QT_INSTALL_LIBS]/libQt*AV.so.*
libqtav.target = libqtav.install
libqtav.commands = echo \"$$join(DEB_INSTALL_LIST, \\n)\" >$$PROJECTROOT/debian/$${libqtav.target}
QMAKE_EXTRA_TARGETS += libqtav
target.depends *= $${libqtav.target}

DEB_INSTALL_LIST = $$join(SDK_HEADERS, \\n.$$[QT_INSTALL_HEADERS]/, .$$[QT_INSTALL_HEADERS]/)
DEB_INSTALL_LIST += .$$[QT_INSTALL_LIBS]/libQt*AV.prl .$$[QT_INSTALL_LIBS]/libQt*AV.so
DEB_INSTALL_LIST += .$$[QT_INSTALL_BINS]/../mkspecs/features/av.prf .$$[QT_INSTALL_BINS]/../mkspecs/modules/qt_lib_av.pri
qtav_dev.target = qtav-dev.install
qtav_dev.commands = echo \"$$join(DEB_INSTALL_LIST, \\n)\" >$$PROJECTROOT/debian/$${qtav_dev.target}
QMAKE_EXTRA_TARGETS += qtav_dev
target.depends *= $${qtav_dev.target}

DEB_INSTALL_LIST = $$join(SDK_PRIVATE_HEADERS, \\n.$$[QT_INSTALL_HEADERS]/QtAV/*/, .$$[QT_INSTALL_HEADERS]/QtAV/*/)
DEB_INSTALL_LIST += .$$[QT_INSTALL_BINS]/../mkspecs/modules/qt_lib_av_private.pri
qtav_private_dev.target = qtav-private-dev.install
qtav_private_dev.commands = echo \"$$join(DEB_INSTALL_LIST, \\n)\" >$$PROJECTROOT/debian/$${qtav_private_dev.target}
QMAKE_EXTRA_TARGETS += qtav_private_dev
target.depends *= $${qtav_private_dev.target}

greaterThan(QT_MAJOR_VERSION, 4):lessThan(QT_MINOR_VERSION, 4) {
  qtav_dev_links.target = qtav-dev.links
  qtav_dev_links.commands = echo \"$$[QT_INSTALL_LIBS]/libQtAV.so $$[QT_INSTALL_LIBS]/libQt$${QT_MAJOR_VERSION}AV.so\" >$$PROJECTROOT/debian/$${qtav_dev_links.target}
  QMAKE_EXTRA_TARGETS *= qtav_dev_links
  target.depends *= $${qtav_dev_links.target}
} #Qt<5.4
} #debian

MODULE_INCNAME = QtAV
MODULE_VERSION = $$VERSION
#use Qt version. limited by qmake
# windows: Qt5AV.dll, not Qt1AV.dll
!mac_framework: MODULE_VERSION = $${QT_MAJOR_VERSION}.$${QT_MINOR_VERSION}.$${QT_PATCH_VERSION}
include($$PROJECTROOT/deploy.pri)
