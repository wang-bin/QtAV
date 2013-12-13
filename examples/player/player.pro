TEMPLATE = app
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TRANSLATIONS = res/player_zh_CN.ts

STATICLINK = 0
PROJECTROOT = $$PWD/../..
include($$PROJECTROOT/src/libQtAV.pri)
preparePaths($$OUT_PWD/../../out)

#SystemParametersInfo
*msvc*: LIBS += -lUser32
INCLUDEPATH += $$PWD

SOURCES += main.cpp \
    MainWindow.cpp \
    Button.cpp \
    ClickableMenu.cpp \
    ScreenSaver.cpp \
    StatisticsView.cpp \
    Slider.cpp \
    TVView.cpp \
    config/Config.cpp \
    config/VideoEQConfigPage.cpp \
    config/DecoderConfigPage.cpp

HEADERS += \
    MainWindow.h \
    Button.h \
    ClickableMenu.h \
    ScreenSaver.h \
    StatisticsView.h \
    Slider.h \
    TVView.h \
    config/Config.h \
    config/VideoEQConfigPage.h \
    config/DecoderConfigPage.h

tv.files = res/tv.ini
BIN_INSTALLS += tv
include($$PROJECTROOT/deploy.pri)

RESOURCES += \
    res/player.qrc \
    theme.qrc
