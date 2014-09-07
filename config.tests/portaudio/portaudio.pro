CONFIG -= qt
CONFIG += console
*msvc* {
#link FFmpeg and portaudio which are built by gcc need /SAFESEH:NO
    QMAKE_LFLAGS += /SAFESEH:NO
    INCLUDEPATH += ../../src/compat/msvc
}
SOURCES += main.cpp

LIBS += -lportaudio
include(../paths.pri)
