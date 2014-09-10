*maemo*: DEFINES += Q_OS_MAEMO
# Add more folders to ship with the application, here
folder_01.source = qml/QMLPlayer
folder_01.target = qml
DEPLOYMENTFOLDERS = folder_01

# Additional import path used to resolve QML modules in Creator's code model
QML_IMPORT_PATH =

# If your application uses the Qt Mobility libraries, uncomment the following
# lines and add the respective components to the MOBILITY variable.
# CONFIG += mobility
# MOBILITY +=

RESOURCES += \
    qmlplayer.qrc

# The .cpp file which was generated for your project. Feel free to hack it.
SOURCES += main.cpp

# Installation path
# target.path =

# Please do not modify the following two lines. Required for deployment.
include(qtquick2applicationviewer/qtquick2applicationviewer.pri)
qtcAddDeployment()

!*msvc*: QMAKE_LFLAGS += -u __link_hack
isEmpty(PROJECTROOT): PROJECTROOT = $$PWD/../..
STATICLINK = 0
include($${PROJECTROOT}/examples/common/libcommon.pri)
preparePaths($$OUT_PWD/../../out)
mac: RC_FILE = $$PROJECTROOT/src/QtAV.icns

RC_ICONS = $$PROJECTROOT/src/QtAV.ico
QMAKE_TARGET_COMPANY = "Shanghai University->S3 Graphics | wbsecg1@gmail.com"
QMAKE_TARGET_DESCRIPTION = "Multimedia playback framework based on Qt & FFmpeg. https://github.com/wang-bin/QtAV"
QMAKE_TARGET_COPYRIGHT = "Copyright (C) 2012-2014 WangBin, wbsecg1@gmail.com"
QMAKE_TARGET_PRODUCT = "QtAV QMLlayer"
