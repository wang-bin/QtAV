TEMPLATE = subdirs
CONFIG -= ordered
SUBDIRS = libqtav examples tests

greaterThan(QT_MAJOR_VERSION, 4) {
  qtHaveModule(quick) {
    SUBDIRS += libqmlav
    libqmlav.file = qml/libQmlAV.pro
    libqmlav.depends += libqtav
    examples.depends += libqmlav
  }
}
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

PACKAGE_VERSION = 1.2.4
PACKAGE_NAME= QtAV

include(pack.pri)
#packageSet(1.2.4, QtAV)
