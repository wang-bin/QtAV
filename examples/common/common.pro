#-------------------------------------------------
#
# Project created by QtCreator 2014-01-03T11:07:23
#
#-------------------------------------------------

QT       -= gui

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

HEADERS = common.h
SOURCES = common.cpp
