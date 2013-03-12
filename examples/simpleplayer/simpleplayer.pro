TEMPLATE = app
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = player
STATICLINK = 0
PROJECTROOT = $$PWD/../..
include($$PROJECTROOT/src/libQtAV.pri)
preparePaths($$OUT_PWD/../../out)

config_gdiplus {
    DEFINES *= HAVE_GDIPLUS=1
}
config_direct2d {
    DEFINES *= HAVE_DIRECT2D=1
}
SOURCES += main.cpp
HEADERS += 


