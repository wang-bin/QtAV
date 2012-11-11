TEMPLATE = app
QT += opengl

TARGET = tst_qtav
STATICLINK = 0
PROJECTROOT = $$PWD/..
isEmpty(BUILD_DIR):BUILD_DIR=$$(BUILD_DIR)
isEmpty(BUILD_DIR):BUILD_DIR=$$[BUILD_DIR]
isEmpty(BUILD_DIR):BUILD_IN_SRC = yes

!isEmpty(BUILD_IN_SRC):BUILD_DIR=$$OUT_PWD/../out
include($${PROJECTROOT}/common.pri)
include($$PROJECTROOT/src/libQtAV.pri)

#win32:LIBS += -lUser32

SOURCES += main.cpp
HEADERS += 


