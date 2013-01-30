TEMPLATE = app

TARGET = videowall
STATICLINK = 0
PROJECTROOT = $$PWD/../..
include($$PROJECTROOT/src/libQtAV.pri)
preparePaths($$OUT_PWD/../../out)

SOURCES += main.cpp VideoWall.cpp
HEADERS += VideoWall.h



