TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = libqtav examples tests

libqtav.file = src/libQtAV.pro
examples.depends += libqtav
tests.depends += libqtav

OTHER_FILES += README.md TODO.txt Changelog

#use the following lines when building as a sub-project, write cache to this project src dir.
#if build this project alone and do not have sub-project depends on this lib, those lines are not necessary
lessThan(QT_MAJOR_VERSION, 5):include(common.pri)
build_dir = "BUILD_DIR=$$OUT_PWD"
write_file($$PWD/.build.cache, build_dir)

