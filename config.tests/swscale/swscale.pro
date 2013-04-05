CONFIG -= qt
CONFIG += console
DEFINES += __STDC_CONSTANT_MACROS
*msvc* {
#link FFmpeg and portaudio which are built by gcc need /SAFESEH:NO
    QMAKE_LFLAGS += /SAFESEH:NO
    INCLUDEPATH += ../../src/compat/msvc
}
*mingw* {
    INCLUDEPATH +=C:\Development\ffmpeg_$$QT_ARCH\include
    LIBS += -LC:\Development\ffmpeg_$$QT_ARCH\lib
    QMAKE_RPATHDIR += C:\Development\ffmpeg_$$QT_ARCH\lib
}
unix {
    CONFIG += link_pkgconfig
    PKGCONFIG += libavcodec
}

SOURCES += main.cpp

LIBS += -lswscale
