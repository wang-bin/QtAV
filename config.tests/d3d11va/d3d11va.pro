CONFIG -= qt
CONFIG += console
CONFIG += c++11
SOURCES += main.cpp
include(../paths.pri)
LIBS += -lavcodec
