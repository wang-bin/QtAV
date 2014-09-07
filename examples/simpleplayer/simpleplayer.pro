TEMPLATE = app
CONFIG -= app_bundle
QT += opengl
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

STATICLINK = 0
PROJECTROOT = $$PWD/../..
include($$PROJECTROOT/src/libQtAV.pri)
preparePaths($$OUT_PWD/../../out)

SOURCES += main.cpp
