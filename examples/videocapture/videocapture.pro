TEMPLATE = app
CONFIG -= app_bundle
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

PROJECTROOT = $$PWD/../..
include($$PROJECTROOT/src/libQtAV.pri)
include($$PROJECTROOT/widgets/libQtAVWidgets.pri)
preparePaths($$OUT_PWD/../../out)

SOURCES += main.cpp \
    playerwindow.cpp

HEADERS += \
    playerwindow.h
