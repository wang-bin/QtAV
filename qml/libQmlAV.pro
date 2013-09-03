TEMPLATE = lib
CONFIG += qt plugin
TARGET = QmlAV
QT += quick qml

CONFIG *= qmlav-buildlib

#var with '_' can not pass to pri?
STATICLINK = 0
PROJECTROOT = $$PWD/..
!include($$PROJECTROOT/src/libQtAV.pri): error("could not find libQtAV.pri")
!include(libQmlAV.pri): error("could not find libQmlAV.pri")
preparePaths($$OUT_PWD/../out)

DESTDIR = $$BUILD_DIR/bin/QtAV
RESOURCES += 
message($$BUILD_DIR)
plugin.files = $$PWD/qmldir
plugin.path = $$BUILD_DIR/bin/QtAV/ #TODO: Qt install dir
plugin.commands = -\$\(COPY_FILE\) $$shell_path($$plugin.files) $$shell_path($$plugin.path)
OTHER_FILES += $$plugin.files

QMAKE_POST_LINK += $$plugin.commands

win32 {
    RC_FILE = #$${PROJECTROOT}/res/QtAV.rc
#no depends for rc file by default, even if rc includes a header. Makefile target use '/' as default, so not works iwth win cmd
    rc.target = $$clean_path($$RC_FILE) #rc obj depends on clean path target
    rc.depends = $$PWD/QtAV/version.h
#why use multiple rule failed? i.e. add a rule without command
    isEmpty(QMAKE_SH) {
        rc.commands = @copy /B $$system_path($$RC_FILE)+,, #change file time
    } else {
        rc.commands = @touch $$RC_FILE #change file time
    }
    QMAKE_EXTRA_TARGETS += rc
}
OTHER_FILES += $$RC_FILE

*msvc* {
#link FFmpeg and portaudio which are built by gcc need /SAFESEH:NO
    QMAKE_LFLAGS += /SAFESEH:NO
    INCLUDEPATH += $$PROJECTROOT/srccompat/msvc
}
#UINT64_C: C99 math features, need -D__STDC_CONSTANT_MACROS in CXXFLAGS
DEFINES += __STDC_CONSTANT_MACROS

SOURCES += QQuickItemRenderer.cpp \
    plugin.cpp \
    QmlAVPlayer.cpp
HEADERS += QmlAV/private/QQuickItemRenderer_p.h \
    QmlAV/QmlAVPlayer.h \
    QmlAV/Export.h
SDK_HEADERS += QmlAV/QQuickItemRenderer.h

HEADERS *= \
    $$SDK_HEADERS

SDK_INCLUDE_FOLDER = QmlAV
include($$PROJECTROOT/deploy.pri)
