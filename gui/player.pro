TEMPLATE = app
QT += opengl

TARGET = player
STATICLINK = 0
PROJECTROOT = $$PWD/..
include($$PROJECTROOT/src/libQtAV.pri)
preparePaths($$OUT_PWD/../out)

#win32:LIBS += -lUser32

SOURCES += main.cpp
HEADERS += 


