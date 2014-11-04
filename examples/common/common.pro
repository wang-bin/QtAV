#-------------------------------------------------
#
# Project created by QtCreator 2014-01-03T11:07:23
#
#-------------------------------------------------
# Qt4 need QDesktopServices
QT = core gui

TARGET = common
TEMPLATE = lib
DEFINES += BUILD_QOPT_LIB

CONFIG *= common-buildlib

#var with '_' can not pass to pri?
STATICLINK = 0
PROJECTROOT = $$PWD/../..
!include(libcommon.pri): error("could not find libcommon.pri")
preparePaths($$OUT_PWD/../../out)


# android apk hack
android {
  QT += svg
  LIBS += -L$$qtLongName($$BUILD_DIR/lib)
  greaterThan(QT_MAJOR_VERSION, 4):greaterThan(QT_MINOR_VERSION, 3) {
    LIBS += -lQt5AV
  } else {
    LIBS += -lQtAV #QML app does not link to libQtAV but we need it. why no QmlAV plugin if remove this?
  }
}

RESOURCES += \
    theme/theme.qrc

#QMAKE_LFLAGS += -u _link_hack

#SystemParametersInfo
*msvc*: LIBS += -lUser32

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

include($$PROJECTROOT/deploy.pri)

target.path = $$[QT_INSTALL_BINS]
INSTALLS += target

