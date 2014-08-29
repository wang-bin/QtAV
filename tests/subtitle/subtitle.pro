#-------------------------------------------------
#
# Project created by QtCreator 2014-08-05T17:12:03
#
#-------------------------------------------------

QT += opengl

TARGET = subtitle
CONFIG -= app_bundle

STATICLINK = 0
PROJECTROOT = $$PWD/../..
include($$PROJECTROOT/src/libQtAV.pri)
preparePaths($$OUT_PWD/../../out)

SOURCES += main.cpp
