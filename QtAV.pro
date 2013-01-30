TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = libqtav examples tests

libqtav.file = src/libQtAV.pro
examples.depends += libqtav
tests.depends += libqtav

OTHER_FILES += README.md TODO.txt Changelog


