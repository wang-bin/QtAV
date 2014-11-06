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

QML_FILES = $$PWD/Video.qml

qtav_qml.files = $$PWD/qmldir $$PWD/Video.qml $$PWD/plugins.qmltypes
!ios: plugin.files = $$DESTDIR/$$qtSharedLib($$NAME)
plugin.path = $$BUILD_DIR/bin/QtAV/
mkpath($$plugin.path)
#plugin.depends = #makefile target
#windows: copy /y file1+file2+... dir. need '+'
for(f, plugin.files) {
  plugin.commands += $$escape_expand(\\n\\t)$$quote(-\$\(COPY_FILE\) $$shell_path($$f) $$shell_path($$plugin.path))
}
#join values seperated by space. so quote is needed
#plugin.commands = $$join(plugin.commands,$$escape_expand(\\n\\t))
OTHER_FILES += $$qtav_qml.files
#just append as a string to $$QMAKE_POST_LINK
isEmpty(QMAKE_POST_LINK): QMAKE_POST_LINK = $$plugin.commands
else: QMAKE_POST_LINK = $${QMAKE_POST_LINK}$$escape_expand(\\n\\t)$$plugin.commands

#QMAKE_EXTRA_TARGETS = plugin

#POST_TARGETDEPS = plugin #vs, xcode does not support
#no write permision. do it in makefile
#mkpath($$[QT_INSTALL_QML]/QtAV)

#http://stackoverflow.com/questions/14260542/qmake-extra-compilers-processing-steps
#http://danny-pope.com/?p=86
#custom compiler: auto update if source is newer
extra_copy.output = $$shell_path($$plugin.path)${QMAKE_FILE_BASE}${QMAKE_FILE_EXT}
# QMAKE_COPY_FILE, QMAKE_MKDIR_CMD ?
extra_copy.commands = -\$\(COPY_FILE\) ${QMAKE_FILE_NAME} $$shell_path($$plugin.path)
#extra_copy.depends = $$EXTRA_COPY_FILES #.input is already the depends
extra_copy.input = EXTRA_COPY_FILES
extra_copy.CONFIG += no_link
extra_copy.variable_out = POST_TARGETDEPS
QMAKE_EXTRA_COMPILERS += extra_copy #
# CAN NOT put $$TARGET here otherwise may result in circular dependency.
# update EXTRA_COPY_FILES will result in target relink
EXTRA_COPY_FILES = $$qtav_qml.files

QMAKE_WRITE_DEFAULT_RC = 1
QMAKE_TARGET_COMPANY = "Shanghai University->S3 Graphics->Deepin | wbsecg1@gmail.com"
QMAKE_TARGET_DESCRIPTION = "Multimedia playback framework based on Qt & FFmpeg. http://www.qtav.org"
QMAKE_TARGET_COPYRIGHT = "Copyright (C) 2012-2014 WangBin, wbsecg1@gmail.com"
QMAKE_TARGET_PRODUCT = "QtAV player"

*msvc* {
#link FFmpeg and portaudio which are built by gcc need /SAFESEH:NO
    QMAKE_LFLAGS += /SAFESEH:NO
    INCLUDEPATH += $$PROJECTROOT/srccompat/msvc
}
#UINT64_C: C99 math features, need -D__STDC_CONSTANT_MACROS in CXXFLAGS
DEFINES += __STDC_CONSTANT_MACROS

SOURCES += \
    plugin.cpp \
    QQuickItemRenderer.cpp \
    SGVideoNode.cpp \
    QmlAVPlayer.cpp \
    QuickSubtitle.cpp \
    MediaMetaData.cpp \
    QuickSubtitleItem.cpp \
    QuickVideoPreview.cpp

HEADERS += QmlAV/private/QQuickItemRenderer_p.h \
    QmlAV/QuickSubtitle.h \
    QmlAV/QuickSubtitleItem.h \
    QmlAV/QuickVideoPreview.h

SDK_HEADERS += \
    QmlAV/Export.h \
    QmlAV/MediaMetaData.h \
    QmlAV/SGVideoNode.h \
    QmlAV/QQuickItemRenderer.h \
    QmlAV/QmlAVPlayer.h

HEADERS *= \
    $$SDK_HEADERS


unix:!android:!mac {
#debian
qml_module_files = qmldir Video.qml plugins.qmltypes libQmlAV.so
DEB_INSTALL_LIST = $$join(qml_module_files, \\n.$$[QT_INSTALL_QML]/QtAV/, .$$[QT_INSTALL_QML]/QtAV/)
deb_install_list.target = qml-module-qtav.install
deb_install_list.commands = echo \"$$join(DEB_INSTALL_LIST, \\n)\" >$$PROJECTROOT/debian/$${deb_install_list.target}
QMAKE_EXTRA_TARGETS += deb_install_list
target.depends += $${deb_install_list.target}
}

target.path = $$[QT_INSTALL_QML]/QtAV
qtav_qml.path = $$[QT_INSTALL_QML]/QtAV
INSTALLS *= target qtav_qml

MODULE = QmlAV
include($$PROJECTROOT/deploy.pri)
