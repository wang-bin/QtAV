#-------------------------------------------------
#
# Project created by QtCreator 2013-01-20T13:10:48
#
#-------------------------------------------------

QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = sharedoutput
TEMPLATE = app
CONFIG -= app_bundle

PROJECTROOT = $$PWD/../..
include($$PROJECTROOT/src/libQtAV.pri)
include($$PROJECTROOT/widgets/libQtAVWidgets.pri)
preparePaths($$OUT_PWD/../../out)

SOURCES += main.cpp \
        widget.cpp

HEADERS  += widget.h
