#-------------------------------------------------
#
# Project created by QtCreator 2014-01-03T11:07:23
#
#-------------------------------------------------

QT       -= gui

# android apk hack
android {
  QT += svg
  LIBS += -lQtAV #QML app does not link to libQtAV but we need it. why no QmlAV plugin if remove this?
}
TARGET = common
TEMPLATE = lib

CONFIG *= common-buildlib

#var with '_' can not pass to pri?
STATICLINK = 0
PROJECTROOT = $$PWD/../..
!include(libcommon.pri): error("could not find libcommon.pri")
preparePaths($$OUT_PWD/../../out)

RESOURCES += \
    theme/theme.qrc

#QMAKE_LFLAGS += -u _link_hack

#SystemParametersInfo
*msvc*: LIBS += -lUser32

HEADERS = common.h \
    ScreenSaver.h
SOURCES = common.cpp
!macx: SOURCES += ScreenSaver.cpp
macx:!ios {
#SOURCE is ok
    OBJECTIVE_SOURCES += ScreenSaver.cpp
    LIBS += -framework CoreServices #-framework ScreenSaver
}

include($$PROJECTROOT/deploy.pri)

target.path = $$[QT_INSTALL_BINS]
INSTALLS += target

