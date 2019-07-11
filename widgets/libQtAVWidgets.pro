TEMPLATE = lib
MODULE_INCNAME = QtAVWidgets # for mac framework. also used in install_sdk.pro
TARGET = QtAVWidgets
QT += gui
config_gl: QT += opengl
greaterThan(QT_MAJOR_VERSION, 4) {
  # qtHaveModule does not exist in Qt5.0
  qtHaveModule(widgets) {
    QT *= widgets
  }
  qtHaveModule(opengl) {
    QT *= opengl
  }
}
CONFIG *= qtavwidgets-buildlib

INCLUDEPATH += $$[QT_INSTALL_HEADERS]
#release: DEFINES += QT_NO_DEBUG_OUTPUT
#var with '_' can not pass to pri?
PROJECTROOT = $$PWD/..
!include($$PROJECTROOT/src/libQtAV.pri): error("could not find libQtAV.pri")
!include(libQtAVWidgets.pri): error("could not find libQtAVWidgets.pri")
preparePaths($$OUT_PWD/../out)

!rc_file {
    RC_ICONS = $$PROJECTROOT/src/QtAV.ico
    QMAKE_TARGET_COMPANY = "wbsecg1@gmail.com"
    QMAKE_TARGET_DESCRIPTION = "QtAVWidgets module. QtAV Multimedia framework. http://qtav.org"
    QMAKE_TARGET_COPYRIGHT = "Copyright (C) 2012-2019 WangBin, wbsecg1@gmail.com"
    QMAKE_TARGET_PRODUCT = "QtAV Widgets"
} else:win32 {
    RC_FILE = QtAVWidgets.rc
#no depends for rc file by default, even if rc includes a header. Makefile target use '/' as default, so not works iwth win cmd
    rc.target = $$clean_path($$RC_FILE) #rc obj depends on clean path target
    rc.depends = QtAVWidgets/version.h
#why use multiple rule failed? i.e. add a rule without command
    isEmpty(QMAKE_SH) {
        rc.commands = @copy /B $$system_path($$RC_FILE)+,, #change file time
    } else {
        rc.commands = @touch $$RC_FILE #change file time
    }
    QMAKE_EXTRA_TARGETS += rc
}

OTHER_FILES += $$RC_FILE

win32 {
#dynamicgl: __impl__GetDC __impl_ReleaseDC
  !winrt:LIBS *= -luser32
}

SDK_HEADERS *= \
    QtAVWidgets/QtAVWidgets \
    QtAVWidgets/QtAVWidgets.h \
    QtAVWidgets/global.h \
    QtAVWidgets/version.h \
    QtAVWidgets/VideoPreviewWidget.h \
    QtAVWidgets/GraphicsItemRenderer.h \
    QtAVWidgets/WidgetRenderer.h


SOURCES *= \
    global.cpp \
    VideoPreviewWidget.cpp \
    GraphicsItemRenderer.cpp \
    WidgetRenderer.cpp

contains(QT_CONFIG, opengl):greaterThan(QT_MAJOR_VERSION, 4) {
  SDK_HEADERS *= QtAVWidgets/OpenGLWidgetRenderer.h
  SOURCES *= OpenGLWidgetRenderer.cpp
  isEqual(QT_MAJOR_VERSION, 5):lessThan(QT_MINOR_VERSION, 4) {
    SDK_HEADERS *= QtAVWidgets/QOpenGLWidget.h
    SOURCES *= QOpenGLWidget.cpp
  }
}

config_gl {
  DEFINES *= QTAV_HAVE_GL=1
  SOURCES += GLWidgetRenderer2.cpp
  SDK_HEADERS += QtAVWidgets/GLWidgetRenderer2.h
  !contains(QT_CONFIG, dynamicgl) { #dynamicgl does not support old gl1 functions which used in GLWidgetRenderer
#GLWidgetRenderer depends on internal functions of QtAV
    #DEFINES *= QTAV_HAVE_GL1
    #SOURCES += GLWidgetRenderer.cpp
    #SDK_HEADERS += QtAVWidgets/GLWidgetRenderer.h
  }
}
config_gdiplus {
  DEFINES *= QTAV_HAVE_GDIPLUS=1
  SOURCES += GDIRenderer.cpp
  LIBS *= -lgdiplus -lgdi32
}
config_direct2d {
  DEFINES *= QTAV_HAVE_DIRECT2D=1
  !*msvc*: INCLUDEPATH += $$PROJECTROOT/contrib/d2d1headers
  SOURCES += Direct2DRenderer.cpp
  #LIBS += -lD2d1
}
config_xv {
  DEFINES *= QTAV_HAVE_XV=1
  SOURCES += XVRenderer.cpp
  LIBS *= -lXv -lX11 -lXext
}
config_x11 {
  DEFINES *= QTAV_HAVE_X11=1
  SOURCES *= X11Renderer.cpp
  LIBS *= -lX11
}
# QtAV/private/* may be used by developers to extend QtAV features without changing QtAV library
# headers not in QtAV/ and it's subdirs are used only by QtAV internally
HEADERS *= \
    $$SDK_HEADERS \
    $$SDK_PRIVATE_HEADERS

# from mkspecs/features/qt_module.prf
# OS X and iOS frameworks
mac_framework { # from common.pri
   #QMAKE_FRAMEWORK_VERSION = 4.0
   CONFIG += lib_bundle sliced_bundle qt_framework
   CONFIG -= qt_install_headers #no need to install these as well
   !debug_and_release|!build_all|CONFIG(release, debug|release) {
        FRAMEWORK_HEADERS.version = Versions
        FRAMEWORK_HEADERS.files = $$SDK_HEADERS
        FRAMEWORK_HEADERS.path = Headers
# 5.4(beta) workaround for wrong include path
# TODO: why <QtCore/qglobal.h> can be found?
        qtAtLeast(5,3): FRAMEWORK_HEADERS.path = Headers/$$MODULE_INCNAME
        FRAMEWORK_PRIVATE_HEADERS.version = Versions
        FRAMEWORK_PRIVATE_HEADERS.files = $$SDK_PRIVATE_HEADERS
        FRAMEWORK_PRIVATE_HEADERS.path = Headers/$$VERSION/$$MODULE_INCNAME/private
        QMAKE_BUNDLE_DATA += FRAMEWORK_HEADERS FRAMEWORK_PRIVATE_HEADERS
   }
}

mac {
   CONFIG += explicitlib
   macx-g++ {
       QMAKE_CFLAGS += -fconstant-cfstrings
       QMAKE_CXXFLAGS += -fconstant-cfstrings
   }
}

unix:!android:!mac {
#debian
DEB_INSTALL_LIST = .$$[QT_INSTALL_LIBS]/libQt*AVWidgets.so.*
libqtavwidgets.target = libqtavwidgets.install
libqtavwidgets.commands = echo \"$$join(DEB_INSTALL_LIST, \\n)\" >$$PROJECTROOT/debian/$${libqtavwidgets.target}
QMAKE_EXTRA_TARGETS += libqtavwidgets
target.depends *= $${libqtavwidgets.target}

DEB_INSTALL_LIST = $$join(SDK_HEADERS, \\n.$$[QT_INSTALL_HEADERS]/, .$$[QT_INSTALL_HEADERS]/)
DEB_INSTALL_LIST += .$$[QT_INSTALL_LIBS]/libQt*AVWidgets.prl .$$[QT_INSTALL_LIBS]/libQt*AVWidgets.so
MKSPECS_DIR=$$[QT_HOST_DATA]/mkspecs
DEB_INSTALL_LIST += .$${MKSPECS_DIR}/features/avwidgets.prf .$${MKSPECS_DIR}/modules/qt_lib_avwidgets.pri
qtavwidgets_dev.target = qtav-dev.install #like qtmultimedia5-dev, contains widgets headers
qtavwidgets_dev.commands = echo \"$$join(DEB_INSTALL_LIST, \\n)\" >>$$PROJECTROOT/debian/$${qtavwidgets_dev.target}
QMAKE_EXTRA_TARGETS += qtavwidgets_dev
target.depends *= $${qtavwidgets_dev.target}

greaterThan(QT_MAJOR_VERSION, 4) {
  qtavwidgets_dev_links.target = qtav-dev.links #like qtmultimedia5-dev, contains widgets .so
  qtavwidgets_dev_links.commands = echo \"$$[QT_INSTALL_LIBS]/libQtAVWidgets.so $$[QT_INSTALL_LIBS]/libQt$${QT_MAJOR_VERSION}AVWidgets.so\" >>$$PROJECTROOT/debian/$${qtavwidgets_dev_links.target}
  QMAKE_EXTRA_TARGETS *= qtavwidgets_dev_links
  target.depends *= $${qtavwidgets_dev_links.target}
} #Qt>=5
} #debian

MODULE_INCNAME = QtAVWidgets
MODULE_VERSION = $$VERSION
#use Qt version. limited by qmake
# windows: Qt5AV.dll, not Qt1AV.dll
!mac_framework: MODULE_VERSION = $${QT_MAJOR_VERSION}.$${QT_MINOR_VERSION}.$${QT_PATCH_VERSION}
!contains(QMAKE_HOST.os, Windows):include($$PROJECTROOT/deploy.pri)
