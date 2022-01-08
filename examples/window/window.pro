TEMPLATE = app
CONFIG -= app_bundle
QT = core gui
greaterThan(QT_MAJOR_VERSION, 5): QT += opengl
PROJECTROOT = $$PWD/../..
include($$PROJECTROOT/src/libQtAV.pri)
preparePaths($$OUT_PWD/../../out)

SOURCES += main.cpp
