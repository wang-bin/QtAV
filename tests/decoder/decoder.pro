QT       += core
QT       -= gui

CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app
TARGET = decoder

STATICLINK = 0
PROJECTROOT = $$PWD/../..
include($$PROJECTROOT/src/libQtAV.pri)
preparePaths($$OUT_PWD/../../out)

SOURCES += main.cpp
