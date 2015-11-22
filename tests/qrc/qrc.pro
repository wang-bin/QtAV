TEMPLATE = app
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG -= app_bundle

PROJECTROOT = $$PWD/../..
include($$PROJECTROOT/src/libQtAV.pri)
include($$PROJECTROOT/widgets/libQtAVWidgets.pri)
preparePaths($$OUT_PWD/../../out)
SOURCES += main.cpp

exists(test.mp4) {
  RESOURCES += media.qrc
} else {
  warning("put test.mp4 in this directory!")
}
