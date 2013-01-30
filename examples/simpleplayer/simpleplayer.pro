TEMPLATE = app
#QT += opengl

TARGET = player
STATICLINK = 0
PROJECTROOT = $$PWD/../..
include($$PROJECTROOT/src/libQtAV.pri)
preparePaths($$OUT_PWD/../../out)

SOURCES += main.cpp
HEADERS += 


