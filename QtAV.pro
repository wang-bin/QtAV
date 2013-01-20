TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = libqtav gui tests

libqtav.file = src/libQtAV.pro
gui.file = gui/player.pro
gui.depends += libqtav

OTHER_FILES += README.md TODO.txt Changelog


