greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
contains(QT_CONFIG, opengl): QT += opengl
CONFIG -= app_bundle

TARGET = videographicsitem
TEMPLATE = app
PROJECTROOT = $$PWD/../..
include($$PROJECTROOT/src/libQtAV.pri)
include($$PROJECTROOT/widgets/libQtAVWidgets.pri)
preparePaths($$OUT_PWD/../../out)


SOURCES += main.cpp \
        videoplayer.cpp

HEADERS  += videoplayer.h
