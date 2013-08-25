TEMPLATE = app
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TRANSLATIONS = res/player_zh_CN.ts

STATICLINK = 0
PROJECTROOT = $$PWD/../..
include($$PROJECTROOT/src/libQtAV.pri)
preparePaths($$OUT_PWD/../../out)

SOURCES += main.cpp \
    MainWindow.cpp \
    Button.cpp \
    ClickableMenu.cpp \
    ScreenSaver.cpp \
    StatisticsView.cpp \
    Slider.cpp

HEADERS += \
    MainWindow.h \
    Button.h \
    ClickableMenu.h \
    ScreenSaver.h \
    StatisticsView.h \
    Slider.h

tv.files = res/tv.ini
BIN_INSTALLS += tv
include($$PROJECTROOT/deploy.pri)

RESOURCES += \
    res/player.qrc \
    theme.qrc
