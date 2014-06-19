CONFIG -= qt
CONFIG += console
DEFINES += __STDC_CONSTANT_MACROS
*msvc* {
#link FFmpeg and portaudio which are built by gcc need /SAFESEH:NO
    QMAKE_LFLAGS += /SAFESEH:NO
    INCLUDEPATH += ../../src/compat/msvc
}
SOURCES += main.cpp

LIBS += -lavcodec
include(../paths.pri)
