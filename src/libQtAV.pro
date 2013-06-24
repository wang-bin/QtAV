TEMPLATE = lib
TARGET = QtAV

QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG *= qtav-buildlib

#var with '_' can not pass to pri?
STATICLINK = 0
PROJECTROOT = $$PWD/..
!include(libQtAV.pri): error("could not find libQtAV.pri")
preparePaths($$OUT_PWD/../out)


RESOURCES += ../i18n/QtAV.qrc

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
OTHER_FILES += $$RC_FILE

TRANSLATIONS = $${PROJECTROOT}/i18n/QtAV_zh_CN.ts

*msvc* {
#link FFmpeg and portaudio which are built by gcc need /SAFESEH:NO
    QMAKE_LFLAGS += /SAFESEH:NO
    INCLUDEPATH += compat/msvc
}
#UINT64_C: C99 math features, need -D__STDC_CONSTANT_MACROS in CXXFLAGS
DEFINES += __STDC_CONSTANT_MACROS

LIBS += -Lextra -lavcodec -lavformat -lavutil -lswscale
config_swresample {
    DEFINES += QTAV_HAVE_SWRESAMPLE=1
    LIBS += -lswresample
}
config_avresample {
    DEFINES += QTAV_HAVE_AVRESAMPLE=1
    LIBS += -lavresample
}

ipp-link {
    DEFINES += IPP_LINK
    ICCROOT = $$(IPPROOT)/../compiler
    INCLUDEPATH += $$(IPPROOT)/include
    LIBS *= -L$$(IPPROOT)/lib/intel64 -L$$(IPPROOT)/lib/ia32 -lippcc -lippcore -lippi \
            -L$$(IPPROOT)/../compiler/lib/ia32 -L$$(IPPROOT)/../compiler/lib/intel64 -lsvml -limf
    #omp for static link. _t is multi-thread static link
}

config_portaudio {
    SOURCES += AOPortAudio.cpp
    HEADERS += QtAV/AOPortAudio.h
    SDK_HEADERS += QtAV/AOPortAudio.h
    DEFINES *= HAVE_PORTAUDIO=1
    LIBS *= -lportaudio
    #win32: LIBS *= -lwinmm #-lksguid #-luuid
}
config_openal {
    SOURCES += AOOpenAL.cpp
    HEADERS += QtAV/AOOpenAL.h \
               QtAV/private/AOOpenAL_p.h
    SDK_HEADERS += QtAV/AOOpenAL.h
    DEFINES *= HAVE_OPENAL=1
    win32:LIBS *= -lOpenAL32
    else: LIBS *= -lopenal
}

config_gdiplus {
    DEFINES *= HAVE_GDIPLUS=1
    SOURCES += GDIRenderer.cpp
    HEADERS += QtAV/GDIRenderer.h
    SDK_HEADERS += QtAV/GDIRenderer.h
    LIBS += -lgdiplus
}

config_direct2d {
    DEFINES *= HAVE_DIRECT2D=1
#TODO: check whether support Direct2D, i.e. version at least XP
    !*msvc*: INCLUDEPATH += $$PROJECTROOT/contrib/d2d1headers
    SOURCES += Direct2DRenderer.cpp
    HEADERS += QtAV/Direct2DRenderer.h
    SDK_HEADERS += QtAV/Direct2DRenderer.h
    #LIBS += -lD2d1
}
config_xv {
    DEFINES *= HAVE_XV=1
    SOURCES += XVRenderer.cpp
    HEADERS += QtAV/XVRenderer.h
    SDK_HEADERS += QtAV/XVRenderer.h
    LIBS += -lXv
}
config_gl {
    QT *= opengl
    DEFINES *= HAVE_GL=1
    SOURCES += GLWidgetRenderer.cpp
    HEADERS += QtAV/GLWidgetRenderer.h
    SDK_HEADERS += QtAV/GLWidgetRenderer.h
}
SOURCES += \
    QtAV_Compat.cpp \
    QtAV_Global.cpp \
    AudioThread.cpp \
    AVThread.cpp \
    AudioDecoder.cpp \
    AudioOutput.cpp \
    AudioResampler.cpp \
    AudioResamplerFF.cpp \
    AVDecoder.cpp \
    AVDemuxer.cpp \
    AVDemuxThread.cpp \
    EventFilter.cpp \
    Filter.cpp \
    FilterContext.cpp \
    GraphicsItemRenderer.cpp \
    ImageConverter.cpp \
    ImageConverterFF.cpp \
    ImageConverterIPP.cpp \
    QPainterRenderer.cpp \
    OSD.cpp \
    OSDFilter.cpp \
    Packet.cpp \
    AVPlayer.cpp \
    VideoCapture.cpp \
    VideoRenderer.cpp \
    VideoRendererTypes.cpp \
    WidgetRenderer.cpp \
    AVOutput.cpp \
    AVClock.cpp \
    Statistics.cpp \
    VideoDecoder.cpp \
    VideoThread.cpp

SDK_HEADERS *= \
    QtAV/dptr.h \
    QtAV/QtAV_Global.h \
    QtAV/AudioResampler.h \
    QtAV/AudioDecoder.h \
    QtAV/AudioOutput.h \
    QtAV/AVDecoder.h \
    QtAV/AVDemuxer.h \
    QtAV/BlockingQueue.h \
    QtAV/Filter.h \
    QtAV/FilterContext.h \
    QtAV/GraphicsItemRenderer.h \
    QtAV/ImageConverter.h \
    QtAV/QPainterRenderer.h \
    QtAV/OSD.h \
    QtAV/OSDFilter.h \
    QtAV/Packet.h \
    QtAV/AVPlayer.h \
    QtAV/VideoCapture.h \
    QtAV/VideoRenderer.h \
    QtAV/VideoRendererTypes.h \
    QtAV/WidgetRenderer.h \
    QtAV/AVOutput.h \
    QtAV/AVClock.h \
    QtAV/VideoDecoder.h \
    QtAV/FactoryDefine.h \
    QtAV/ImageConverterTypes.h \
    QtAV/Statistics.h \
    QtAV/version.h


HEADERS *= \
    $$SDK_HEADERS \
    QtAV/prepost.h \
    QtAV/AVDemuxThread.h \
    QtAV/AVThread.h \
    QtAV/AudioThread.h \
    QtAV/VideoThread.h \
    QtAV/QtAV_Compat.h \
    QtAV/EventFilter.h \
    QtAV/singleton.h \
    QtAV/factory.h \
    QtAV/private/AudioOutput_p.h \
    QtAV/private/AudioResampler_p.h \
    QtAV/private/AVThread_p.h \
    QtAV/private/AVDecoder_p.h \
    QtAV/private/AVOutput_p.h \
    QtAV/private/Filter_p.h \
    QtAV/private/GraphicsItemRenderer_p.h \
    QtAV/private/ImageConverter_p.h \
    QtAV/private/VideoRenderer_p.h \
    QtAV/private/QPainterRenderer_p.h \
    QtAV/private/WidgetRenderer_p.h \
    QtAV/private/Direct2DRenderer_p.h \
    QtAV/private/GLWidgetRenderer_p.h \
    QtAV/private/GDIRenderer_p.h \
    QtAV/private/XVRenderer_p.h \
    QtAV/AudioResamplerTypes.h


SDK_INCLUDE_FOLDER = QtAV
include($$PROJECTROOT/deploy.pri)
