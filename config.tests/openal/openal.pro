CONFIG -= qt
CONFIG += console

SOURCES += main.cpp

win32: LIBS += -lOpenAL32
unix:!mac:!blackberry: LIBS += -lopenal
blackberry: LIBS += -lOpenAL
mac: LIBS += -framework OpenAL
mac: DEFINES += HEADER_OPENAL_PREFIX
include(../paths.pri)
