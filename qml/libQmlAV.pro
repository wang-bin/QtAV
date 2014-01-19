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

#DESTDIR = $$BUILD_DIR/bin/QtAV
RESOURCES += 
message($$BUILD_DIR)
# add a make command
defineReplace(mcmd) {
    return($$escape_expand(\\n\\t)$$1)
}
QML_FILES = $$PWD/Video.qml
plugin.files = $$PWD/qmldir $$PWD/Video.qml $$DESTDIR/$$qtSharedLib($$NAME)
plugin.path = $$BUILD_DIR/bin/qml/QtAV/ #TODO: Qt install dir
#plugin.depends = #makefile target
#windows: copy /y file1+file2+... dir. need '+'
for(f, plugin.files) {
  plugin.commands += $$mcmd(-\$\(COPY_FILE\) $$shell_path($$f) $$shell_path($$plugin.path))
}
OTHER_FILES += $$PWD/qmldir $$PWD/Video.qml
QMAKE_POST_LINK += $$plugin.commands #just append as a string to $$QMAKE_POST_LINK
QMAKE_EXTRA_TARGETS = plugin
#POST_TARGETDEPS = plugin #vs, xcode does not support
mkpath($$plugin.path)

#http://stackoverflow.com/questions/14260542/qmake-extra-compilers-processing-steps
#http://danny-pope.com/?p=86
#custom compiler: auto update if source is newer
#TODO: QMAKE_FIlE_EXT does not work
plugin_maker.output = ${QMAKE_FILE_BASE}${QMAKE_FIlE_EXT}
#plugin_maker.targetdir = $$plugin.path #not exist
plugin_maker.commands = -\$\(COPY_FILE\) ${QMAKE_FILE_NAME} $$shell_path($$plugin.path)${QMAKE_FILE_OUT}
plugin_maker.depends = $$PLUGIN_FILES
plugin_maker.input = PLUGIN_FILES
plugin_maker.CONFIG += no_link
plugin_maker.variable_out = POST_TARGETDEPS
#QMAKE_EXTRA_COMPILERS += plugin_maker

PLUGIN_FILES = $$PWD/qmldir $$PWD/Video.qml $$DESTDIR/$$qtSharedLib($$NAME)

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

HEADERS += QmlAV/private/QQuickItemRenderer_p.h
SDK_HEADERS += \
    QmlAV/Export.h \
    QmlAV/QQuickItemRenderer.h \
    QmlAV/QmlAVPlayer.h

HEADERS *= \
    $$SDK_HEADERS

SDK_INCLUDE_FOLDER = QmlAV
include($$PROJECTROOT/deploy.pri)
