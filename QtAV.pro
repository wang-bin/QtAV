include(root.pri)

TEMPLATE = subdirs
CONFIG -= ordered
SUBDIRS = libqtav tools
libqtav.file = src/libQtAV.pro
!no-widgets {
  SUBDIRS += libqtavwidgets
  libqtavwidgets.file = widgets/libQtAVWidgets.pro
  libqtavwidgets.depends = libqtav
  examples.depends += libqtavwidgets #TODO: enable widgets based examples
}
greaterThan(QT_MAJOR_VERSION, 4) {
  # qtHaveModule does not exist in Qt5.0
  isEqual(QT_MINOR_VERSION, 0)|qtHaveModule(quick) {
    SUBDIRS += libqmlav
    libqmlav.file = qml/libQmlAV.pro
    libqmlav.depends += libqtav
    examples.depends += libqmlav
  }
}
!no-examples {
  SUBDIRS += examples
  examples.depends += libqtav
}
!cross_compile:!no-tests {
  SUBDIRS += tests
  tests.depends += libqtav libqtavwidgets
}
OTHER_FILES += README.md TODO.txt Changelog
OTHER_FILES += templates/vo.h templates/vo.cpp templates/COPYRIGHT.h templates/mkclass.sh
OTHER_FILES += \
	templates/base.h templates/base.cpp templates/base_p.h \
	templates/derived.h templates/derived.cpp templates/derived_p.h \
	templates/final.h templates/final.cpp
#OTHER_FILES += config.test/mktest.sh


EssentialDepends = avutil avcodec avformat swscale
OptionalDepends = \
    swresample \
    avresample \
    avdevice
# QtOpenGL module. In Qt5 we can disable it and still have opengl support
!no-gl:!no-widgets {
  greaterThan(QT_MAJOR_VERSION, 4):qtHaveModule(opengl):!config_gl {
    GL=config_gl done_config_gl
    cache(CONFIG, add, GL)
  } else {
    OptionalDepends *= gl #ios require code sign
  }
}
!no-avfilter: OptionalDepends *= avfilter
## sse2 sse4_1 may be defined in Qt5 qmodule.pri but is not included. Qt4 defines sse and sse2
!no-sse4_1:!sse4_1: OptionalDepends *= sse4_1
# no-xxx can set in $$PWD/user.conf
!no-dsound: win32: OptionalDepends *= dsound
!no-openal: OptionalDepends *= openal
!no-pulseaudio: OptionalDepends *= pulseaudio
!no-portaudio: OptionalDepends *= portaudio
!no-direct2d:!no-widgets: OptionalDepends *= direct2d
!no-gdiplus:!no-widgets: OptionalDepends *= gdiplus
# why win32 is false?
!no-dxva: OptionalDepends *= dxva
!no-libass: OptionalDepends *= libass
unix {
    !no-xv:!no-widgets: OptionalDepends *= xv
    !no-vaapi: OptionalDepends *= vaapi
    !no-cedarv: OptionalDepends *= libcedarv
}

runConfigTests()
!config_avresample:!config_swresample {
  error("libavresample or libswresample is required. Setup your environment correctly then delete $$BUILD_DIR/.qmake.conf and run qmake again")
}


PACKAGE_VERSION = $$QTAV_VERSION
PACKAGE_NAME= QtAV

include(pack.pri)
#packageSet($$QTAV_VERSION, QtAV)
