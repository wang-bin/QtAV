#-------------------------------------------------
#
# Project created by QtCreator 2014-01-28T10:24:19
#
#-------------------------------------------------

TEMPLATE = app
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG -= app_bundle

PROJECTROOT = $$PWD/../..
include($$PROJECTROOT/src/libQtAV.pri)
include($$PROJECTROOT/widgets/libQtAVWidgets.pri)
preparePaths($$OUT_PWD/../../out)

SOURCES += main.cpp
