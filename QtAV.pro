TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = libqtav test

libqtav.file = src/libQtAV.pro
test.file = test/tst_QtAV.pro
test.depends += libqtav

OTHER_FILES += README.md TODO.txt


