TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = libqtav examples tests

libqtav.file = src/libQtAV.pro
examples.depends += libqtav
tests.depends += libqtav

OTHER_FILES += README.md TODO.txt Changelog
OTHER_FILES += templates/vo.h templates/vo.cpp templates/COPYRIGHT.h templates/mkclass.sh
OTHER_FILES += \
	templates/base.h templates/base.cpp templates/base_p.h \
	templates/derived.h templates/derived.cpp templates/derived_p.h \
	templates/final.h templates/final.cpp
#OTHER_FILES += config.test/mktest.sh


EssentialDepends = avutil avcodec avformat swscale
OptionalDepends = portaudio direct2d gdiplus gl \
    swresample avresample
unix {
    OptionalDepends += xv
}

include(root.pri)

PACKAGE_VERSION = 1.2.3
PACKAGE_NAME= QtAV

include(pack.pri)
#packageSet(1.2.0, QtAV)
