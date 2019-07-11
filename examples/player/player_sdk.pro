TARGET = Player
QT += sql svg
########## template for QtAV app project BEGIN ################
greaterThan(QT_MAJOR_VERSION, 4) {
  QT += avwidgets
} else {
  CONFIG += avwidgets
}
#rpath for osx qt4
mac {
  RPATHDIR *= @loader_path/../Frameworks @executable_path/../Frameworks
  QMAKE_LFLAGS_SONAME = -Wl,-install_name,@rpath/
  isEmpty(QMAKE_LFLAGS_RPATH): QMAKE_LFLAGS_RPATH=-Wl,-rpath,
  for(R,RPATHDIR) {
    QMAKE_LFLAGS *= \'$${QMAKE_LFLAGS_RPATH}$$R\'
  }
}
########## template for QtAV app project END ################
################# Add your own code below ###################
COMMON = $$PWD/../common
INCLUDEPATH *= $$COMMON $$COMMON/..
RESOURCES += $$COMMON/theme/theme.qrc
isEmpty(PROJECTROOT): PROJECTROOT = $$PWD/../..
mac: RC_FILE = $$PROJECTROOT/src/QtAV.icns
QMAKE_INFO_PLIST = $$COMMON/Info.plist
ios: QMAKE_INFO_PLIST = ios/Info.plist
videos.files = video
videos.path = /
QMAKE_BUNDLE_DATA += videos
defineTest(genRC) {
    RC_ICONS = $$PROJECTROOT/src/QtAV.ico
    QMAKE_TARGET_COMPANY = "Shanghai University->S3 Graphics->Deepin | wbsecg1@gmail.com"
    QMAKE_TARGET_DESCRIPTION = "QtAV Multimedia framework. http://qtav.org"
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
HEADERS = \
    $$COMMON/common.h \
    $$COMMON/Config.h \
    $$COMMON/qoptions.h \
    $$COMMON/ScreenSaver.h \
    $$COMMON/common_export.h

SOURCES = \
    $$COMMON/common.cpp \
    $$COMMON/Config.cpp \
    $$COMMON/qoptions.cpp

!macx: SOURCES += $$COMMON/ScreenSaver.cpp
macx:!ios {
#SOURCE is ok
    OBJECTIVE_SOURCES += $$COMMON/ScreenSaver.cpp
    LIBS += -framework CoreServices #-framework ScreenSaver
}
include(src.pri)
