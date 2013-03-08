TEMPLATE = lib
TARGET = QtAV

QT += core gui opengl
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG *= qtav-buildlib
CONFIG *= portaudio
#CONFIG *= openal
win32 {
CONFIG *= gdi
#TODO: link or dynamic link?
*msvc*: CONFIG *= direct2d #gcc may not have heeaders and libs
}

#var with '_' can not pass to pri?
STATICLINK = 0
PROJECTROOT = $$PWD/..
!include(libQtAV.pri): error("could not find libQtAV.pri")
preparePaths($$OUT_PWD/../out)

win32 {
    RC_FILE = $${PROJECTROOT}/res/QtAV.rc
#no depends for rc file by default, even if rc includes a header
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
    DEFINES *= HAVE_PORTAUDIO=1
    LIBS *= -lportaudio
    #win32: LIBS *= -lwinmm #-lksguid #-luuid
}
config_openal {
    SOURCES += AOOpenAL.cpp
    HEADERS += QtAV/AOOpenAL.h \
               QtAV/private/AOOpenAL_p.h
    DEFINES *= HAVE_OPENAL=1
    win32:LIBS *= -lOpenAL32
    else: LIBS *= -lopenal
}

config_gdiplus {
    SOURCES += GDIRenderer.cpp
    HEADERS += QtAV/GDIRenderer.h
    LIBS += -lgdiplus
}

config_direct2d {
#TODO: check whether support Direct2D, i.e. version at least XP
    !*msvc*: INCLUDEPATH += $$PROJECTROOT/contrib/d2d1headers
    SOURCES += Direct2DRenderer.cpp
    HEADERS += QtAV/Direct2DRenderer.h
    #LIBS += -lD2d1
}

SOURCES += \
    QtAV_Compat.cpp \
    QtAV_Global.cpp \
    AudioThread.cpp \
    AVThread.cpp \
    AudioDecoder.cpp \
    AudioOutput.cpp \
    AVDecoder.cpp \
    AVDemuxer.cpp \
    AVDemuxThread.cpp \
    EventFilter.cpp \
    GLWidgetRenderer.cpp \
    GraphicsItemRenderer.cpp \
    ImageConverter.cpp \
    ImageConverterFF.cpp \
    ImageConverterIPP.cpp \
    ImageRenderer.cpp \
    Packet.cpp \
    AVPlayer.cpp \
    VideoCapture.cpp \
    VideoRenderer.cpp \
    WidgetRenderer.cpp \
    AVOutput.cpp \
    AVClock.cpp \
    VideoDecoder.cpp \
    VideoThread.cpp

HEADERS += \
    QtAV/prepost.h \
    QtAV/dptr.h \
    QtAV/QtAV_Global.h \
    QtAV/QtAV_Compat.h \
    QtAV/AVThread.h \
    QtAV/AudioThread.h \
    QtAV/EventFilter.h \
    QtAV/private/AudioOutput_p.h \
    QtAV/private/AVThread_p.h \
    QtAV/private/AVDecoder_p.h \
    QtAV/private/AVOutput_p.h \
    QtAV/private/GraphicsItemRenderer_p.h \
    QtAV/private/ImageConverter_p.h \
    QtAV/private/ImageRenderer_p.h \
    QtAV/private/VideoRenderer_p.h \
    QtAV/private/WidgetRenderer_p.h \
    QtAV/AudioDecoder.h \
    QtAV/AudioOutput.h \
    QtAV/AVDecoder.h \
    QtAV/AVDemuxer.h \
    QtAV/AVDemuxThread.h \
    QtAV/BlockingQueue.h \
    QtAV/GLWidgetRenderer.h \
    QtAV/GraphicsItemRenderer.h \
    QtAV/ImageConverter.h \
    QtAV/ImageRenderer.h \
    QtAV/Packet.h \
    QtAV/AVPlayer.h \
    QtAV/VideoCapture.h \
    QtAV/VideoRenderer.h \
    QtAV/WidgetRenderer.h \
    QtAV/AVOutput.h \
    QtAV/AVClock.h \
    QtAV/VideoDecoder.h \
    QtAV/VideoThread.h \
    QtAV/singleton.h \
    QtAV/factory.h \
    QtAV/FactoryDefine.h \
    QtAV/ImageConverterTypes.h \
    QtAV/version.h
