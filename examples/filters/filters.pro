TEMPLATE = app
CONFIG -= app_bundle
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = filters
PROJECTROOT = $$PWD/../..
include($$PROJECTROOT/src/libQtAV.pri)
include($$PROJECTROOT/widgets/libQtAVWidgets.pri)
preparePaths($$OUT_PWD/../../out)

SOURCES += \
        main.cpp \
        SimpleFilter.cpp

HEADERS += \
        SimpleFilter.h

RESOURCES += \
    res.qrc



