TEMPLATE = lib

QT += core gui opengl
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG *= qtav-buildlib
#var with '_' can not pass to pri?
STATICLINK = 0
PROJECTROOT = $$PWD/..
!include(libQtAV.pri): error("could not find libQtAV.pri")
preparePaths($$OUT_PWD/../out)

win32:RC_FILE = $${PROJECTROOT}/res/QtAV.rc
OTHER_FILES += $$RC_FILE
#src
unix: SOURCES += 
else:win32: SOURCES += 

portaudio {
SOURCES += AOPortAudio.cpp
HEADERS += QtAV/AOPortAudio.h \
           QtAV/private/AOPortAudio_p.h
}
openal {
SOURCES += AOOpenAL.cpp
HEADERS += QtAV/AOOpenAL.h \
           QtAV/private/AOOpenAL_p.h
}

SOURCES += \
    QtAV_Compat.cpp \
    AudioThread.cpp \
    AVThread.cpp \
    AudioDecoder.cpp \
    AudioOutput.cpp \
    AVDecoder.cpp \
    AVDemuxer.cpp \
    AVDemuxThread.cpp \
    EventFilter.cpp \
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
    QtAV/GraphicsItemRenderer.h \
    QtAV/ImageConverter.h \
    QtAV/ImageConverterFF.h \
    QtAV/ImageConverterIPP.h \
    QtAV/ImageRenderer.h \
    QtAV/Packet.h \
    QtAV/AVPlayer.h \
    QtAV/VideoCapture.h \
    QtAV/VideoRenderer.h \
    QtAV/WidgetRenderer.h \
    QtAV/AVOutput.h \
    QtAV/AVClock.h \
    QtAV/VideoDecoder.h \
    QtAV/VideoThread.h
