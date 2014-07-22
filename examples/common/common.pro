#-------------------------------------------------
#
# Project created by QtCreator 2014-01-03T11:07:23
#
#-------------------------------------------------

QT       -= gui

# android apk hack
android {
  QT += svg
  LIBS += -lQtAV #QML app does not link to libQtAV but we need it. why no QmlAV plugin if remove this?
}
TARGET = common
TEMPLATE = lib

CONFIG *= common-buildlib

#var with '_' can not pass to pri?
STATICLINK = 0
PROJECTROOT = $$PWD/../..
!include(libcommon.pri): error("could not find libcommon.pri")
preparePaths($$OUT_PWD/../../out)

!ios: copy_sdk_libs = $$DESTDIR/$$qtSharedLib($$NAME)
#plugin.depends = #makefile target
#windows: copy /y file1+file2+... dir. need '+'
for(f, copy_sdk_libs) {
  win32: copy_sdk_libs_cmd += $$quote(-\$\(COPY_FILE\) \"$$system_path($$f)\" \"$$system_path($$[QT_INSTALL_BINS])\")
  else: copy_sdk_libs_cmd += $$quote(-\$\(COPY_FILE\) \"$$system_path($$f)\" \"$$system_path($$[QT_INSTALL_LIBS])\")
}
#join values seperated by space. so quote is needed
copy_sdk_libs_cmd = $$join(copy_sdk_libs_cmd,$$escape_expand(\\n\\t))
#just append as a string to $$QMAKE_POST_LINK
isEmpty(QMAKE_POST_LINK): QMAKE_POST_LINK = $$copy_sdk_libs_cmd
else: QMAKE_POST_LINK = $${QMAKE_POST_LINK}$$escape_expand(\\n\\t)$$copy_sdk_libs_cmd

RESOURCES += \
    theme/theme.qrc

#QMAKE_LFLAGS += -u _link_hack

#SystemParametersInfo
*msvc*: LIBS += -lUser32

HEADERS = common.h \
    ScreenSaver.h
SOURCES = common.cpp
!macx: SOURCES += ScreenSaver.cpp
macx:!ios {
+#SOURCE is ok
    OBJECTIVE_SOURCES += ScreenSaver.cpp
    LIBS += -framework CoreServices #-framework ScreenSaver
}
