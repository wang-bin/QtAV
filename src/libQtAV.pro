TEMPLATE = lib

QT += core gui opengl 
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG *= qtav-buildlib
#var with '_' can not pass to pri?
STATICLINK = 0
PROJECTROOT = $$PWD/..
isEmpty(BUILD_DIR):BUILD_DIR=$$(BUILD_DIR)
isEmpty(BUILD_DIR):BUILD_DIR=$$[BUILD_DIR]
isEmpty(BUILD_DIR):BUILD_IN_SRC = yes
!isEmpty(BUILD_IN_SRC):BUILD_DIR=$$OUT_PWD/../out
include($${PROJECTROOT}/common.pri)
!include(libQtAV.pri): error(could not find libQtAV.pri)

#src
unix: SOURCES += 
else:win32: SOURCES += 

SOURCES += \
    AudioThread.cpp \
    AVThread.cpp \
    AudioDecoder.cpp \
    AudioOutput.cpp \
    AVDecoder.cpp \
    AVDemuxer.cpp \
    AVDemuxThread.cpp \
    GraphicsItemRenderer.cpp \
    ImageRenderer.cpp \
    AVInfo.cpp \
    QAVPacket.cpp \
    QAVPacketQueue.cpp \
    AVPlayer.cpp \
    VideoRenderer.cpp \
    WidgetRenderer.cpp \
    AVOutput.cpp

HEADERS += QtAV/QtAV_Global.h QtAV/qtav.h \
    QtAV/AVThread.h \
    QtAV/AudioThread.h \
    QtAV/private/AVThread_p.h \
    QtAV/AudioDecoder.h \
    QtAV/AudioOutput.h \
    QtAV/AVDecoder.h \
    QtAV/AVDemuxer.h \
    QtAV/AVDemuxThread.h \
    QtAV/GraphicsItemRenderer.h \
    QtAV/ImageRenderer.h \
    QtAV/AVInfo.h \
    QtAV/QAVPacket.h \
    QtAV/QAVPacketQueue.h \
    QtAV/AVPlayer.h \
    QtAV/VideoRenderer.h \
    QtAV/private/VideoRenderer_p.h \
    QtAV/WidgetRenderer.h \
    QtAV/AVOutput.h


