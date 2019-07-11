TARGET = QMLPlayer
!static:VERSION = $$QTAV_VERSION
QT += av svg qml quick sql
android {
  QT += androidextras
}
*maemo*: DEFINES += Q_OS_MAEMO
# Add more folders to ship with the application, here
folder_01.source = qml/QMLPlayer
folder_01.target = qml
#will copy to target path
#DEPLOYMENTFOLDERS = folder_01

# Additional import path used to resolve QML modules in Creator's code model
QML_IMPORT_PATH =

# If your application uses the Qt Mobility libraries, uncomment the following
# lines and add the respective components to the MOBILITY variable.
# CONFIG += mobility
# MOBILITY +=

RESOURCES += qmlplayer.qrc
TRANSLATIONS = i18n/QMLPlayer_zh_CN.ts i18n/QMLPlayer.ts

# The .cpp file which was generated for your project. Feel free to hack it.
SOURCES += main.cpp
lupdate_only{
SOURCES = qml/QMLPlayer/*.qml qml/QMLPlayer/*.js
}
# Installation path
target.path = $$[QT_INSTALL_BINS]


desktopfile.files = $$PWD/../../qtc_packaging/debian_generic/QMLPlayer.desktop
desktopfile.path = /usr/share/applications

# Please do not modify the following two lines. Required for deployment.
include(qtquick2applicationviewer/qtquick2applicationviewer.pri)
qtcAddDeployment()

COMMON = $$PWD/../common
INCLUDEPATH *= $$COMMON $$COMMON/..
RESOURCES += $$COMMON/theme/theme.qrc
isEmpty(PROJECTROOT): PROJECTROOT = $$PWD/../..

fonts.files = fonts
fonts.path = fonts
QMAKE_BUNDLE_DATA += fonts
mac: RC_FILE = $$PROJECTROOT/src/QtAV.icns
QMAKE_INFO_PLIST = $$COMMON/Info.plist
ios: QMAKE_INFO_PLIST = ios/Info.plist
videos.files = videos
videos.path = /
#QMAKE_BUNDLE_DATA += videos
defineTest(genRC) {
    RC_ICONS = $$PROJECTROOT/src/QtAV.ico
    QMAKE_TARGET_COMPANY = "Shanghai University->S3 Graphics->Deepin | wbsecg1@gmail.com"
    QMAKE_TARGET_DESCRIPTION = "QtAV Multimedia playback framework. http://qtav.org"
    QMAKE_TARGET_COPYRIGHT = "Copyright (C) 2012-2019 WangBin, wbsecg1@gmail.com"
    QMAKE_TARGET_PRODUCT = "QtAV $$1"
    export(RC_ICONS)
    export(QMAKE_TARGET_COMPANY)
    export(QMAKE_TARGET_DESCRIPTION)
    export(QMAKE_TARGET_COPYRIGHT)
    export(QMAKE_TARGET_PRODUCT)
    return(true)
}
genRC($$TARGET)
#SystemParametersInfo
!winrt:*msvc*: LIBS += -lUser32
DEFINES += BUILD_QOPT_LIB
HEADERS *= \
    $$COMMON/common.h \
    $$COMMON/Config.h \
    $$COMMON/qoptions.h \
    $$COMMON/ScreenSaver.h \
    $$COMMON/common_export.h

SOURCES *= \
    $$COMMON/common.cpp \
    $$COMMON/Config.cpp \
    $$COMMON/qoptions.cpp

!macx: SOURCES += $$COMMON/ScreenSaver.cpp
macx:!ios {
#SOURCE is ok
    OBJECTIVE_SOURCES += $$COMMON/ScreenSaver.cpp
    LIBS += -framework CoreServices #-framework ScreenSaver
}
SOURCES *= main.cpp
android {
  DISTFILES += \
    android/src/org/qtav/qmlplayer/QMLPlayerActivity.java \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/AndroidManifest.xml \
    android/res/values/libs.xml \
    android/build.gradle \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew \
    android/gradlew.bat

  ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
#user can put fonts in android/assets/fonts
}
winrt|wince {
# vs project: qmake -tp vc "CONFIG+=windeployqt"
  QT *= opengl #qtav is build with opengl module
  CONFIG *= windeployqt
  WINDEPLOYQT_OPTIONS = -qmldir $$shell_quote($$system_path($$_PRO_FILE_PWD_/qml/QMLPlayer))
  DISTFILES *=  # will be listed in "Distribution Files" in vs but not deployed. not sure what's the usage
# If a file does not exist, it will not be added to vs project
# If not using vs, edit AppxManifest.map for Qt5.5
  depend_dll.files = \
    $$[QT_INSTALL_BINS]/QtAV$${QTAV_MAJOR_VERSION}.dll \
    $$[QT_INSTALL_BINS]/avcodec-*.dll \
    $$[QT_INSTALL_BINS]/avformat-*.dll \
    $$[QT_INSTALL_BINS]/avutil-*.dll \
    $$[QT_INSTALL_BINS]/avfilter-*.dll \
    $$[QT_INSTALL_BINS]/swresample-*.dll \
    $$[QT_INSTALL_BINS]/swscale-*.dll
  exists($$[QT_INSTALL_BINS]/avresample-*.dll): depend_dll.files += $$[QT_INSTALL_BINS]/avresample-*.dll
  exists($$[QT_INSTALL_BINS]/ass.dll): depend_dll.files += $$[QT_INSTALL_BINS]/ass.dll
  #depend_dll.path = $$OUT_PWD
  DEPLOYMENT = depend_dll fonts #vs2015update1 error about multiple qt5core.dll(in both build dir and qtbin dir), we can remove them in `Deployment Files`
# WINRT_MANIFEST file: "=>\"
  VCLibsSuffix =
  winphone {
    VCLibsSuffix = .Phone
    WINRT_MANIFEST = winrt/WinPhone8.Package.appxmanifest
  } else:*-msvc2015 {
    #isEqual(VCPROJ_ARCH, ARM): VCLibsSuffix = .Phone
    WINRT_MANIFEST = winrt/WinRT10.Package.appxmanifest
  } else {
    WINRT_MANIFEST = winrt/WinRT8.Package.appxmanifest
  }
  OTHER_FILES *= winrt/*
  WINRT_ASSETS_PATH = winrt/assets #for qmake generated manifest
}
