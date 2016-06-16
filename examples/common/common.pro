#-------------------------------------------------
#
# Project created by QtCreator 2014-01-03T11:07:23
#
#-------------------------------------------------
# Qt4 need QDesktopServices
QT = core gui sql

TARGET = common
TEMPLATE = lib
DEFINES += BUILD_QOPT_LIB

CONFIG *= common-buildlib staticlib
staticlib: DEFINES += BUILD_COMMON_STATIC
#var with '_' can not pass to pri?
PROJECTROOT = $$PWD/../..
!include(libcommon.pri): error("could not find libcommon.pri")
preparePaths($$OUT_PWD/../../out)

INCLUDEPATH += $$PROJECTROOT/src
# android apk hack
android {
  QT += svg
  LIBS += -L$$qtLongName($$BUILD_DIR/lib)
  isEqual(QT_MAJOR_VERSION, 5):isEqual(QT_MINOR_VERSION, 4):lessThan(QT_PATCH_VERSION, 2) {
    LIBS += -lQt5AV
  } else {
    LIBS += -lQtAV #QML app does not link to libQtAV but we need it. why no QmlAV plugin if remove this?
  }
} else {
#include($$PROJECTROOT/libQtAV.pri)
}

RESOURCES += \
    theme/theme.qrc

#QMAKE_LFLAGS += -u _link_hack

#SystemParametersInfo
!winrt:*msvc*: LIBS += -lUser32

HEADERS = common.h \
    Config.h \
    qoptions.h \
    ScreenSaver.h \
    common_export.h

SOURCES = common.cpp \
    Config.cpp \
    qoptions.cpp

!macx: SOURCES += ScreenSaver.cpp
macx:!ios {
#SOURCE is ok
    OBJECTIVE_SOURCES += ScreenSaver.cpp
    LIBS += -framework CoreServices #-framework ScreenSaver
}
# don't install. was set in libcommon.pri
INSTALLS =
