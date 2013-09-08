TEMPLATE = app
TARGET = qmlvideofx

isEmpty(PROJECTROOT): PROJECTROOT = $$PWD/../..
include($${PROJECTROOT}/common.pri)
preparePaths($$OUT_PWD/../../out)

QT += quick #multimedia

SOURCES += filereader.cpp main.cpp
HEADERS += filereader.h trace.h

RESOURCES += qmlvideofx.qrc


maemo6: {
    DEFINES += SMALL_SCREEN_LAYOUT
    DEFINES += SMALL_SCREEN_PHYSICAL
}

target.path = $$[QT_INSTALL_EXAMPLES]/multimedia/video/qmlvideofx
INSTALLS += target
