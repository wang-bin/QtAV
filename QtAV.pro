TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = libqtav examples tests

libqtav.file = src/libQtAV.pro
examples.depends += libqtav
tests.depends += libqtav

OTHER_FILES += README.md TODO.txt Changelog


lessThan(QT_MAJOR_VERSION, 5):include(common.pri)
build_dir = "BUILD_DIR=$$OUT_PWD"
write_file($$PWD/.build.cache, build_dir)

