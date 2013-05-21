TEMPLATE = app
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = player
STATICLINK = 0
PROJECTROOT = $$PWD/../..
include($$PROJECTROOT/src/libQtAV.pri)
preparePaths($$OUT_PWD/../../out)

SOURCES += main.cpp \
    MainWindow.cpp \
    Button.cpp \
    Slider.cpp

HEADERS += \
    MainWindow.h \
    Button.h \
    Slider.h


include($$PROJECTROOT/deploy.pri)

