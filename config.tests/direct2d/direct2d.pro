CONFIG -= qt
CONFIG += console
!*msvc*:INCLUDEPATH += ../../contrib/d2d1headers
SOURCES += main.cpp

#dynamic load
#LIBS += -ld2d1
