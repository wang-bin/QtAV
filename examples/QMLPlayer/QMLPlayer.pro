!static:VERSION = $$QTAV_VERSION # vc: will create exp and lib, result in static build error
QT += sql
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

RESOURCES += \
    qmlplayer.qrc
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

#!*msvc*: QMAKE_LFLAGS += -u __link_hack
isEmpty(PROJECTROOT): PROJECTROOT = $$PWD/../..
STATICLINK=1
include($${PROJECTROOT}/examples/common/libcommon.pri)
preparePaths($$OUT_PWD/../../out)
mac: RC_FILE = $$PROJECTROOT/src/QtAV.icns
genRC($$TARGET)
ios {
  RCC_DIR = #in qt_preprocess.mk, rule name is relative path while dependency name is absolute path
  MOC_DIR =
  QMAKE_INFO_PLIST = ios/Info.plist
}
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

#ubuntu14.04 use qt5.2, remove incompatible code in qmlplayer
!qtAtLeast(5, 3):unix: system("sed -i '/\/\/IF_QT53/,/\/\/ENDIF_QT53/d' qml/QMLPlayer/main.qml")
