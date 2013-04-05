CONFIG -= qt
CONFIG += console
*msvc* {
#link FFmpeg and portaudio which are built by gcc need /SAFESEH:NO
    QMAKE_LFLAGS += /SAFESEH:NO
    INCLUDEPATH += ../../src/compat/msvc
}
*mingw*{
         INCLUDEPATH +=C:\Development\portaudio_$$QT_ARCH\include
         LIBS += -LC:\Development\portaudio_$$QT_ARCH\lib
         LIBS *= -lwinmm -lksguid -luuid# -lws2_32 #portaudio
}
SOURCES += main.cpp

LIBS += -lportaudio
