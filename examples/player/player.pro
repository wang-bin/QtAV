TEMPLATE = app
QT += opengl
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TRANSLATIONS = res/player_zh_CN.ts

STATICLINK = 0
PROJECTROOT = $$PWD/../..
include($$PROJECTROOT/src/libQtAV.pri)
include($$PWD/../common/libcommon.pri)
preparePaths($$OUT_PWD/../../out)

INCLUDEPATH += $$PWD

SOURCES += main.cpp \
    MainWindow.cpp \
    Button.cpp \
    ClickableMenu.cpp \
    StatisticsView.cpp \
    Slider.cpp \
    TVView.cpp \
    EventFilter.cpp \
    config/Config.cpp \
    config/ConfigDialog.cpp \
    config/ConfigPageBase.cpp \
    config/CaptureConfigPage.cpp \
    config/VideoEQConfigPage.cpp \
    config/DecoderConfigPage.cpp \
    playlist/PlayListModel.cpp \
    playlist/PlayListItem.cpp \
    playlist/PlayListDelegate.cpp \
    playlist/PlayList.cpp \
    config/PropertyEditor.cpp \
    config/AVFormatConfigPage.cpp \
    config/AVFilterConfigPage.cpp

HEADERS += \
    MainWindow.h \
    Button.h \
    ClickableMenu.h \
    StatisticsView.h \
    Slider.h \
    TVView.h \
    EventFilter.h \
    config/Config.h \
    config/ConfigDialog.h \
    config/ConfigPageBase.h \
    config/CaptureConfigPage.h \
    config/VideoEQConfigPage.h \
    config/DecoderConfigPage.h \
    playlist/PlayListModel.h \
    playlist/PlayListItem.h \
    playlist/PlayListDelegate.h \
    playlist/PlayList.h \
    config/PropertyEditor.h \
    config/AVFormatConfigPage.h \
    config/AVFilterConfigPage.h

tv.files = res/tv.ini
BIN_INSTALLS += tv
include($$PROJECTROOT/deploy.pri)

RESOURCES += \
    res/player.qrc \
    theme.qrc

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
